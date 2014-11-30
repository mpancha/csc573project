#ifndef __ERED_H
#define __ERED_H

#include <net/inet_sock.h>
#define EMER_PKT 1


int e_ip_options_compile(struct ip_options *opt, struct sk_buff *skb);
static inline int isEmergencyPkt(struct sk_buff *skb)
{
//	struct ip_options *opt;
	const struct iphdr *iph;

	iph = ip_hdr(skb);
/*
	opt = &(IPCB(skb)->opt);
	opt->optlen = iph->ihl*4 - sizeof(struct iphdr);
*/
	if(iph->ihl > 5)
	{
		//printk("%x, %d\n",(unsigned int)iph,iph->ihl);
		return EMER_PKT;
	}
	else
	{
		//printk("NonEmer%x, %d\n",(unsigned int)iph,iph->ihl);
		return 0;
	}


//	return e_ip_options_compile(opt, skb);
}

int e_ip_options_compile(struct ip_options *opt, struct sk_buff *skb)
{
	int isEmergency=0;
	unsigned char *pp_ptr = NULL;
	unsigned char *optptr;
	unsigned char *iph;
	int optlen, l;

	if (skb != NULL) {
		optptr = (unsigned char *)&(ip_hdr(skb)[1]);
	} else
		optptr = opt->__data;
	iph = optptr - sizeof(struct iphdr);

	for (l = opt->optlen; l > 0; ) {
		switch (*optptr) {
		      case IPOPT_END:
			for (optptr++, l--; l>0; optptr++, l--) {
				if (*optptr != IPOPT_END) {
					*optptr = IPOPT_END;
					opt->is_changed = 1;
				}
			}
			goto eol;
		      case IPOPT_NOOP:
			l--;
			optptr++;
			continue;
		}
		if (unlikely(l < 2)) {
			pp_ptr = optptr;
			goto error;
		}
		optlen = optptr[1];
		switch (*optptr) {
			  case 150:
					isEmergency =1;
					printk("E");
					break;
		      default:
					break;
		}
		l -= optlen;
		optptr += optlen;
	}

eol:
error:
	return isEmergency;
}


#if 1
#define RED_PROB_DONT_MARK 12
/* This function is adapted form red_action function of red.h */
static inline int ered_action(const struct red_parms *p,
			     struct red_vars *v,
			     unsigned long qavg, u32 isEmergency)
{
	switch (red_cmp_thresh(p, qavg)) {
		case RED_BELOW_MIN_THRESH:
			v->qcount = -1;
			return RED_DONT_MARK;

		case RED_BETWEEN_TRESH:
			if (++v->qcount) {
				if (red_mark_probability(p, v, qavg)) {
					v->qcount = 0;
					v->qR = red_random(p);
					/*if(isEmergency == EMER_PKT)
					{
						return RED_DONT_MARK;
					}*/
					return RED_PROB_MARK;
				}
			} else
				v->qR = red_random(p);

			return RED_PROB_DONT_MARK;

		case RED_ABOVE_MAX_TRESH:
			v->qcount = -1;
			return RED_HARD_MARK;
	}

	BUG();
	return RED_DONT_MARK;
}
#endif

#endif
