/**************************************************
 * Emergency Aware Active queue management module  *
 * Project CSC573
 * TEAM : Mitul Panchal
 *	     Gowtham
 *	     Apoorv
 * Description: This module implements the queue APIs
 *		to provide new queuing discipline ered in linux tc
 * This code is based on
 * net/sched/sch_gred.c	Generic Random Early Detection queue.
 *
 *
 *              This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU General Public License
 *              as published by the Free Software Foundation; either version
 *              2 of the License, or (at your option) any later version.
 *
 * Authors:    J Hadi Salim (hadi@nortelnetworks.com) 1998,1999
 *
 *             991129: -  Bug fix with grio mode
 *		       - a better sing. AvgQ mode with Grio(WRED)
 *		       - A finer grained VQ dequeue based on sugestion
 *		         from Ren Liu
 *		       - More error checks
 *    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <net/pkt_sched.h>
#include <net/red.h>
#include <net/ip.h>
#include "ered.h"

#define MAX_COLORS  4    /* Red Yellow Blue */

#define ERED_DEF_COLOR 1 /* By default red color */
static int emergencyOn;
/* ered data structures 	  */
/* Can be moved to interface file */

struct ered_color_queue; /* color queue structure */
struct ered_sched;	 /* interface queue struct*/
struct emer_stats;	 /* statistics structure  */

struct ered_color_queue {
	u32		limit;		/* max color queue length	*/
	u32		colorId;		/* color id 			*/
	u32		colorBytes;	/* number of bytes of this color*/
	u32		colorPkts;	/* packets of this color 	*/
	u32		colorOccupancy;	/* current queue occupancy  	*/
	u8		prio;		/* the precedence of this color */
	/* may not be required */
	struct red_parms parms;
	struct red_vars  vars;
	struct red_stats stats;
};

struct ered_sched {
	struct ered_color_queue *colorq[MAX_COLORS]; /* red yellow blue queue */
	unsigned long	flags;
	u32		red_flags;
	u32 		colors;
	u32 		def; /* should be 0 */
	struct red_vars wred_set;
	u8	 	isEmergency[MAX_COLORS]; /* Indicates emeregency packet */

};

struct emer_stats {
	u32 total;	/* total number of packets in queue */
	u32 etotal;	/* total number of emergency packets in queue */
	u32 min;
	u32 emin;
	u32 pdrop;
	u32 pdontdrop;
	u32 epdontdrop;
	u32 emark;
	u32 epdrop;
	u32 fdrop;
	u32 efdrop;
	u32 colorOccupancy;
	u32 qavg;
	u32 qmin;
};

/* Instantiate emergency database */

static struct emer_stats  	ered_stats[MAX_COLORS]; /* Global statistics data */

/*****************************/
/* APIs for queue management */
/*****************************/

/* Function to obtain colorId from skbuff tc_index */
static inline u16 tc_to_colorId(struct sk_buff *sk)
{
	return (sk->tc_index & (MAX_COLORS-1));
}

/****************************
 * Enqueue function
 ***************************/

static int ered_enqueue(struct sk_buff *skb, struct Qdisc *sch)
{
	struct ered_color_queue *q = NULL;
	struct ered_sched *ered = qdisc_priv(sch);
	unsigned long qavg = 0;
	u16 color_id = tc_to_colorId(skb);
	int i;
	if (color_id >= ered->colors || (q = ered->colorq[color_id]) == NULL) {
		color_id = ered->def;

		q = ered->colorq[color_id];
		if (!q) {
			/* No queue configured for this color!! */
			/*if (skb_queue_len(&sch->q) < qdisc_dev(sch)->tx_queue_len)
				return qdisc_enqueue_tail(skb, sch);
			else*/
			goto drop; /* Drop the packet */
		}
		skb->tc_index = (skb->tc_index & ~(MAX_COLORS-1)) | color_id;
	}

	/* sum up all the qaves of prios < ours to get the new qave */
	qavg=0;
	for (i = 0; i < color_id; i++)
        {
		if (ered->colorq[i] && !red_is_idling(&ered->colorq[i]->vars))
			qavg += ered->colorq[i]->vars.qavg;
	}

	/* update colorq stats */
	q->colorPkts++;
	q->colorBytes += qdisc_pkt_len(skb);

	/* update total packets in queue */
	ered_stats[color_id].total++;

	/* Mark emergency packet if it is */
	ered->isEmergency[color_id] = isEmergencyPkt(skb);

	/* Update emergency packet stat */
	if(ered->isEmergency[color_id])
		ered_stats[color_id].etotal++;

	/* Calculate MAMT queue average */
	q->vars.qavg = red_calc_qavg(&q->parms, &q->vars, q->colorOccupancy);
	ered_stats[color_id].qavg= (qavg + q->vars.qavg)>>q->parms.Wlog;
	if(ered_stats[color_id].qavg < 5000)
		ered_stats[color_id].qmin++;
	if (red_is_idling(&q->vars))
		red_end_of_idle_period(&q->vars);

	switch (ered_action(&q->parms, &q->vars, qavg + q->vars.qavg, ered->isEmergency[color_id]))
 	{
	   case RED_DONT_MARK:
		if(ered->isEmergency[color_id] == EMER_PKT)
			ered_stats[color_id].emin++;
		else
			ered_stats[color_id].min++;
		break;
	   case RED_PROB_DONT_MARK:
		if(ered->isEmergency[color_id] == EMER_PKT)
			ered_stats[color_id].epdontdrop++;
		else
			ered_stats[color_id].pdontdrop++;
		break;
	   case RED_PROB_MARK:
		sch->qstats.overlimits++;
		if(ered->isEmergency[color_id] == EMER_PKT)
		{
			ered_stats[color_id].emark++;
			if(emergencyOn==1)
			   break; /* dont drop */
			ered_stats[color_id].epdrop++;
		}
		q->stats.pdrop++;
		ered_stats[color_id].pdrop++;
		goto drop;
		break;

	   case RED_HARD_MARK:
		sch->qstats.overlimits++;
		q->stats.forced_drop++;
		ered_stats[color_id].fdrop++;
		if (ered->isEmergency[color_id]==EMER_PKT)
		{
			/* get the actual remaining queue length */
			//actual_remq = sumation of all colorOccupancy

			ered_stats[color_id].efdrop++;
		}
		goto drop;
		break;
	}

	if (q->colorOccupancy + qdisc_pkt_len(skb) <= q->limit)
	{
		q->colorOccupancy += qdisc_pkt_len(skb);
		ered_stats[color_id].colorOccupancy = q->colorOccupancy;
		return qdisc_enqueue_tail(skb, sch);
	}
	q->stats.forced_drop++;
	ered_stats[color_id].fdrop++;
	if(ered->isEmergency[color_id]==EMER_PKT)
		ered_stats[color_id].efdrop++;
drop:
	return qdisc_drop(skb, sch);
}

static struct sk_buff *ered_dequeue(struct Qdisc *sch)
{
	struct sk_buff *skb;
	struct ered_sched *ered = qdisc_priv(sch);

	skb = qdisc_dequeue_head(sch);

	if (skb)
        {
		struct ered_color_queue *q;
		u16 color_id = tc_to_colorId(skb);

		if (color_id >= ered->colors || (q = ered->colorq[color_id]) == NULL)
		{
			net_warn_ratelimited("ERED: Unable to relocate colorQ 0x%x after dequeue, screwing up colorOccupancy\n", tc_to_colorId(skb));
		}
		else
		{
			q->colorOccupancy -= qdisc_pkt_len(skb);

			if (!q->colorOccupancy)
				red_start_of_idle_period(&q->vars);
		}
		return skb;
	}
	return NULL;
}

static unsigned int ered_drop(struct Qdisc *sch)
{
	struct sk_buff *skb;
	struct ered_sched *ered = qdisc_priv(sch);

	skb = qdisc_dequeue_tail(sch);
	if (skb)
  	{
		unsigned int len = qdisc_pkt_len(skb);
		struct ered_color_queue *q;
		u16 color_id = tc_to_colorId(skb);

		if (color_id >= ered->colors || (q = ered->colorq[color_id]) == NULL)
		{
			net_warn_ratelimited("ERED: Unable to relocate colorQ 0x%x while dropping, screwing up colorOccupancy\n",
					     tc_to_colorId(skb));
		}
		else
		{
			q->colorOccupancy -= len;
			q->stats.other++;

			if (!q->colorOccupancy)
				red_start_of_idle_period(&q->vars);
		}
		qdisc_drop(skb, sch);
		return len;
	}
	return 0;
}

static inline void ered_destroy_colorq(struct ered_color_queue *q)
{
	kfree(q);
}

static void ered_reset(struct Qdisc *sch)
{
	int i;
	struct ered_sched *ered = qdisc_priv(sch);

	qdisc_reset_queue(sch);

	for (i = 0; i < ered->colors; i++)
	{
		struct ered_color_queue *q = ered->colorq[i];
		if (!q)
			continue;
		red_restart(&q->vars);
		q->colorOccupancy= 0;
	}
}

static inline int ered_change_para(struct Qdisc *sch, struct nlattr *dps)
{
	struct ered_sched *ered = qdisc_priv(sch);
	struct tc_gred_sopt *sopt;
	int i;

	if (dps == NULL)
		return -EINVAL;

	sopt = nla_data(dps);

	if (sopt->DPs > MAX_COLORS || sopt->DPs == 0 || sopt->def_DP >= sopt->DPs)
		return -EINVAL;

	sch_tree_lock(sch);
	ered->colors = sopt->DPs;
	ered->def = sopt->def_DP;
	ered->red_flags = sopt->flags;

	sch_tree_unlock(sch);

	for (i = ered->colors; i < MAX_COLORS; i++) {
		if (ered->colorq[i])
		{
			pr_warning("ERED: Warning: Destroying "
				   "shadowed VQ 0x%x\n", i);
			ered_destroy_colorq(ered->colorq[i]);
			ered->colorq[i] = NULL;
		}
	}
	return 0;
}

static inline int ered_change_colorq(struct Qdisc *sch, int color_id,
				 struct tc_gred_qopt *ctl, int prio,
				 u8 *stab, u32 max_P,
				 struct ered_color_queue **prealloc)
{
	struct ered_sched *ered = qdisc_priv(sch);
	struct ered_color_queue *q = ered->colorq[color_id];

	if (!q) {
		ered->colorq[color_id] = q = *prealloc;
		*prealloc = NULL;
		if (!q)
			return -ENOMEM;
	}

	q->colorId = color_id;
	q->prio = prio;
	q->limit = ctl->limit;

	if (q->colorOccupancy == 0)
		red_end_of_idle_period(&q->vars);

	red_set_parms(&q->parms,
		      ctl->qth_min, ctl->qth_max, ctl->Wlog, ctl->Plog,
		      ctl->Scell_log, stab, max_P);
	//printk("qth_max=%d\n", q->parms.qth_max);
	red_set_vars(&q->vars);
	return 0;
}

static const struct nla_policy ered_policy[TCA_GRED_MAX + 1] = {
	[TCA_GRED_PARMS]	= { .len = sizeof(struct tc_gred_qopt) },
	[TCA_GRED_STAB]		= { .len = 256 },
	[TCA_GRED_DPS]		= { .len = sizeof(struct tc_gred_sopt) },
	[TCA_GRED_MAX_P]	= { .type = NLA_U32 },
};

static int ered_change(struct Qdisc *sch, struct nlattr *opt)
{
	struct ered_sched *table = qdisc_priv(sch);
	struct tc_gred_qopt *ctl;
	struct nlattr *tb[TCA_GRED_MAX + 1];
	int err, prio = ERED_DEF_COLOR;
	u8 *stab;
	u32 max_P;
	struct ered_color_queue *colorq_alloc;

	if (opt == NULL)
		return -EINVAL;

	err = nla_parse_nested(tb, TCA_GRED_MAX, opt, ered_policy);
	if (err < 0)
		return err;

	if (tb[TCA_GRED_PARMS] == NULL && tb[TCA_GRED_STAB] == NULL)
		return ered_change_para(sch, opt);

	if (tb[TCA_GRED_PARMS] == NULL ||
	    tb[TCA_GRED_STAB] == NULL)
		return -EINVAL;

	max_P = tb[TCA_GRED_MAX_P] ? nla_get_u32(tb[TCA_GRED_MAX_P]) : 0;

	err = -EINVAL;
	ctl = nla_data(tb[TCA_GRED_PARMS]);
	stab = nla_data(tb[TCA_GRED_STAB]);

	if (ctl->DP >= table->colors)
		goto errout;

	if (ctl->prio == 0)
	{
		int def_prio = ERED_DEF_COLOR;

		if (table->colorq[table->def])
			def_prio = table->colorq[table->def]->prio;
		printk(KERN_DEBUG "GRED: DP %u does not have a prio "
			       "setting default to %d\n", ctl->DP, def_prio);

		prio = def_prio;
	}
	else
		prio = ctl->prio;

	colorq_alloc = kzalloc(sizeof(*colorq_alloc), GFP_KERNEL);
	sch_tree_lock(sch);

	err = ered_change_colorq(sch, ctl->DP, ctl, prio, stab, max_P, &colorq_alloc);
	if (err < 0)
		goto errout_locked;
	err = 0;

errout_locked:
	sch_tree_unlock(sch);
	kfree(colorq_alloc);
errout:
	return err;
}

static int ered_init(struct Qdisc *sch, struct nlattr *opt)
{
	struct nlattr *tb[TCA_GRED_MAX + 1];
	int err;

	if (opt == NULL)
		return -EINVAL;

	err = nla_parse_nested(tb, TCA_GRED_MAX, opt, ered_policy);
	if (err < 0)
		return err;

	if (tb[TCA_GRED_PARMS] || tb[TCA_GRED_STAB])
		return -EINVAL;
	return ered_change_para(sch, tb[TCA_GRED_DPS]);
}

static int ered_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct ered_sched *table = qdisc_priv(sch);
	struct nlattr *parms, *opts = NULL;
	int i;
	u32 max_p[MAX_COLORS];
	struct tc_gred_sopt sopt = {
		.DPs	= table->colors,
		.def_DP	= table->def,
		.grio	= 1,
		.flags	= table->red_flags,
	};

	opts = nla_nest_start(skb, TCA_OPTIONS);
	if (opts == NULL)
		goto nla_put_failure;
	if (nla_put(skb, TCA_GRED_DPS, sizeof(sopt), &sopt))
		goto nla_put_failure;

	for (i = 0; i < MAX_COLORS; i++) {
		struct ered_color_queue *q = table->colorq[i];

		max_p[i] = q ? q->parms.max_P : 0;
	}
	if (nla_put(skb, TCA_GRED_MAX_P, sizeof(max_p), max_p))
		goto nla_put_failure;

	parms = nla_nest_start(skb, TCA_GRED_PARMS);
	if (parms == NULL)
		goto nla_put_failure;

	for (i = 0; i < MAX_COLORS; i++) {
		struct ered_color_queue *q = table->colorq[i];
		struct tc_gred_qopt opt;
		unsigned long qavg;

		memset(&opt, 0, sizeof(opt));

		if (!q) {
			opt.DP = MAX_COLORS + i;
			goto append_opt;
		}

		opt.limit	= q->limit;
		opt.DP		= q->colorId;
		opt.backlog	= q->colorOccupancy;
		opt.prio	= q->prio;
		opt.qth_min	= q->parms.qth_min >> q->parms.Wlog;
		opt.qth_max	= q->parms.qth_max >> q->parms.Wlog;
		opt.Wlog	= q->parms.Wlog;
		opt.Plog	= q->parms.Plog;
		opt.Scell_log	= q->parms.Scell_log;
		opt.other	= q->stats.other;
		opt.early	= q->stats.prob_drop;
		opt.forced	= q->stats.forced_drop;
		opt.pdrop	= q->stats.pdrop;
		opt.packets	= q->colorPkts;
		opt.bytesin	= q->colorBytes;

		printk("color:%d,limit=%d, min=%d, max=%d\n", q->colorId,q->limit, q->parms.qth_min, q->parms.qth_max);
		qavg = red_calc_qavg(&q->parms, &q->vars,
				     q->vars.qavg >> q->parms.Wlog);
		opt.qave = qavg >> q->parms.Wlog;
append_opt:
		if (nla_append(skb, sizeof(opt), &opt) < 0)
			goto nla_put_failure;
	}

	nla_nest_end(skb, parms);

	return nla_nest_end(skb, opts);

nla_put_failure:
	nla_nest_cancel(skb, opts);
	return -EMSGSIZE;
}

static void ered_destroy(struct Qdisc *sch)
{
	struct ered_sched *table = qdisc_priv(sch);
	int i;

	for (i = 0; i < table->colors; i++) {
		if (table->colorq[i])
			ered_destroy_colorq(table->colorq[i]);
	}
}

static struct Qdisc_ops ered_qdisc_ops __read_mostly = {
	.id		=	"ered",
	.priv_size	=	sizeof(struct ered_sched),
	.enqueue	=	ered_enqueue,
	.dequeue	=	ered_dequeue,
	.peek		=	qdisc_peek_head,
	.drop		=	ered_drop,
	.init		=	ered_init,
	.reset		=	ered_reset,
	.destroy	=	ered_destroy,
	.change		=	ered_change,
	.dump		=	ered_dump,
	.owner		=	THIS_MODULE,
};

static int showEmerStats(struct seq_file *m, void *v)
{
	int i;

    	seq_printf(m, "Emergency Red statistics\n");
	for(i=0; i<MAX_COLORS; i++)
	{
		if(i==1)
		   continue;
		seq_printf(m, "color:%d, total = %d, min = %d, pdontdrop=%d, pdrop = %d, fdrop = %d qlen=%d qavg=%d qmin=%d|", i, ered_stats[i].total, ered_stats[i].min, ered_stats[i].pdontdrop, ered_stats[i].pdrop, ered_stats[i].fdrop, ered_stats[i].colorOccupancy, ered_stats[i].qavg, ered_stats[i].qmin);
		seq_printf(m, "etotal = %d, emin = %d, emark = %d,epdontdrop=%d,epdrop=%d,efdrop = %d\n", ered_stats[i].etotal, ered_stats[i].emin, ered_stats[i].emark, ered_stats[i].epdontdrop,ered_stats[i].epdrop,ered_stats[i].efdrop);
	}
    	return 0;
}
static int emergencyStatOpen(struct inode *inode, struct file *file)
{
    return single_open(file, showEmerStats, NULL);
}

static const struct file_operations procOptions =
{
    .owner   = THIS_MODULE,
    .open    = emergencyStatOpen,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int setEmer(struct seq_file *n, void *v)
{
	int i;
	if(emergencyOn==0)
	   emergencyOn=1;
	else
	   emergencyOn=0;

	for(i=0; i<MAX_COLORS; i++)
	{
		ered_stats[i].total=0;	/* total number of packets in queue */
		ered_stats[i].etotal=0;	/* total number of emergency packets in queue */
		ered_stats[i].min=0;	/* total number of emergency packets in queue */
		ered_stats[i].emin=0;	/* total number of emergency packets in queue */
		ered_stats[i].pdrop=0;	/* total number of emergency packets in queue */
		ered_stats[i].pdontdrop=0;	/* total number of emergency packets in queue */
		ered_stats[i].epdontdrop=0;	/* total number of emergency packets in queue */
		ered_stats[i].emark=0;	/* total number of emergency packets in queue */
		ered_stats[i].epdrop=0;	/* total number of emergency packets in queue */
		ered_stats[i].fdrop=0;	/* total number of emergency packets in queue */
		ered_stats[i].efdrop=0;	/* total number of emergency packets in queue */
		ered_stats[i].colorOccupancy=0;
		ered_stats[i].qavg=0;
		ered_stats[i].qmin=0;
 	}
	seq_printf(n,"Emergency Flag:%d\n", emergencyOn);
	return 0;
}
static int toggleEmer(struct inode *inode, struct file *file)
{
    return single_open(file, setEmer , NULL);
}
static const struct file_operations emerOptions =
{
    .owner   = THIS_MODULE,
    .open    = toggleEmer,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int __init ered_module_init(void)
{
	proc_create("estat",0,NULL,&procOptions);
	proc_create("emergencyOnOff",0,NULL,&emerOptions);
	printk("ered: Module insert success! Now qdisc ered is available\n");
	printk("ered:estat created in proc\n");
	return register_qdisc(&ered_qdisc_ops);
}

static void __exit ered_module_exit(void)
{
	remove_proc_entry("estat",NULL);
	remove_proc_entry("emergencyOnOff",NULL);
	printk("ered:estat removed from proc\n");
	unregister_qdisc(&ered_qdisc_ops);
	printk("ered: Module remove success!\n");
}

module_init(ered_module_init)
module_exit(ered_module_exit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CSC573");
