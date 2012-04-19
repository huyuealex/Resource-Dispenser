#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
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
	{ 0x5edf33a3, "module_layout" },
	{ 0xfd386fa9, "kmem_cache_destroy" },
	{ 0x1da7e7dd, "kmalloc_caches" },
	{ 0xb85f3bbe, "pv_lock_ops" },
	{ 0x64e90ef0, "del_timer" },
	{ 0x20000329, "simple_strtoul" },
	{ 0xc0a3d105, "find_next_bit" },
	{ 0x9b62129b, "ida_get_new" },
	{ 0x4aabc7c4, "__tracepoint_kmalloc" },
	{ 0x933740ca, "cancel_work_sync" },
	{ 0xa59dc114, "elv_rb_latter_request" },
	{ 0xbf7f76bc, "task_nice" },
	{ 0x3168f5d, "init_timer_key" },
	{ 0x1251d30f, "call_rcu" },
	{ 0x3c2c5af5, "sprintf" },
	{ 0x7d11c268, "jiffies" },
	{ 0xcbe3938, "__blk_run_queue" },
	{ 0xfbe27a1c, "rb_first" },
	{ 0xb1c77ec7, "elv_rb_del" },
	{ 0xfe7c4287, "nr_cpu_ids" },
	{ 0x83636ee3, "wait_for_completion" },
	{ 0xa8b17b7, "elv_rb_find" },
	{ 0x9021b097, "del_timer_sync" },
	{ 0x7c229851, "kmem_cache_alloc_notrace" },
	{ 0x88941a06, "_raw_spin_unlock_irqrestore" },
	{ 0x7384036e, "current_task" },
	{ 0x37befc70, "jiffies_to_msecs" },
	{ 0xb72397d5, "printk" },
	{ 0x1fe624a, "elv_unregister" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0xc0580937, "rb_erase" },
	{ 0x98d1a25, "elv_dispatch_sort" },
	{ 0xb4390f9a, "mcount" },
	{ 0x3b1c6237, "kmem_cache_free" },
	{ 0x16305289, "warn_slowpath_null" },
	{ 0x615a3dcb, "mod_timer" },
	{ 0x4a971ec7, "radix_tree_delete" },
	{ 0xe15cd5fc, "put_io_context" },
	{ 0x27feddc6, "elv_register" },
	{ 0x51e49f77, "get_io_context" },
	{ 0xe0c5533a, "kmem_cache_alloc" },
	{ 0x8ff4079b, "pv_irq_ops" },
	{ 0xc754118, "__trace_note_message" },
	{ 0x8f48679a, "rb_prev" },
	{ 0x579fbcd2, "cpu_possible_mask" },
	{ 0x3bd1b1f6, "msecs_to_jiffies" },
	{ 0xf333a2fb, "_raw_spin_lock_irq" },
	{ 0x6443d74d, "_raw_spin_lock" },
	{ 0x7ecb001b, "__per_cpu_offset" },
	{ 0x587c70d8, "_raw_spin_lock_irqsave" },
	{ 0xa6dcc773, "rb_insert_color" },
	{ 0x3c119cd9, "kmem_cache_create" },
	{ 0x98c02180, "kblockd_schedule_work" },
	{ 0x8d66a3a, "warn_slowpath_fmt" },
	{ 0xe3fdd734, "ida_remove" },
	{ 0x37a0cba, "kfree" },
	{ 0xf5142158, "ida_pre_get" },
	{ 0x9754ec10, "radix_tree_preload" },
	{ 0x424fe461, "elv_rq_merge_ok" },
	{ 0x7628f3c7, "this_cpu_off" },
	{ 0x74c134b9, "__sw_hweight32" },
	{ 0x472d2a9a, "radix_tree_lookup" },
	{ 0x9f3ff7d5, "elv_rb_former_request" },
	{ 0xbdf5c25c, "rb_next" },
	{ 0x5e09ca75, "complete" },
	{ 0x6d6cbadc, "rb_last" },
	{ 0xd5688a7a, "radix_tree_insert" },
	{ 0x52872b97, "ida_destroy" },
	{ 0xd31e9de4, "elv_rb_add" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "72FB2DE7EE16C466DAEFE05");
