#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xe00b4984, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x5afe1292, __VMLINUX_SYMBOL_STR(single_release) },
	{ 0xe887354a, __VMLINUX_SYMBOL_STR(seq_read) },
	{ 0x7c4c2659, __VMLINUX_SYMBOL_STR(seq_lseek) },
	{ 0xc8255aca, __VMLINUX_SYMBOL_STR(unregister_qdisc) },
	{ 0x945e4c08, __VMLINUX_SYMBOL_STR(remove_proc_entry) },
	{ 0x54ca76cf, __VMLINUX_SYMBOL_STR(register_qdisc) },
	{ 0x99c316ce, __VMLINUX_SYMBOL_STR(proc_create_data) },
	{ 0xbc435770, __VMLINUX_SYMBOL_STR(dump_stack) },
	{ 0x8bf826c, __VMLINUX_SYMBOL_STR(_raw_spin_unlock_bh) },
	{ 0x318cadb1, __VMLINUX_SYMBOL_STR(reciprocal_value) },
	{ 0xa4eb4eff, __VMLINUX_SYMBOL_STR(_raw_spin_lock_bh) },
	{ 0x85670f1d, __VMLINUX_SYMBOL_STR(rtnl_is_locked) },
	{ 0xf82eacf7, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0xe2cb65c5, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x4f391d0e, __VMLINUX_SYMBOL_STR(nla_parse) },
	{ 0x5895dbc, __VMLINUX_SYMBOL_STR(nla_append) },
	{ 0x3ad5d0b8, __VMLINUX_SYMBOL_STR(skb_trim) },
	{ 0x2ddad155, __VMLINUX_SYMBOL_STR(nla_put) },
	{ 0xa735db59, __VMLINUX_SYMBOL_STR(prandom_u32) },
	{ 0x4cdb3178, __VMLINUX_SYMBOL_STR(ns_to_timeval) },
	{ 0xfd485ec8, __VMLINUX_SYMBOL_STR(seq_printf) },
	{ 0x990395ac, __VMLINUX_SYMBOL_STR(single_open) },
	{ 0xc87c1f84, __VMLINUX_SYMBOL_STR(ktime_get) },
	{ 0x50eedeb8, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xf6ebc03b, __VMLINUX_SYMBOL_STR(net_ratelimit) },
	{ 0xf70f5974, __VMLINUX_SYMBOL_STR(kfree_skb) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xb4390f9a, __VMLINUX_SYMBOL_STR(mcount) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "34647FF34800D46415B501E");
