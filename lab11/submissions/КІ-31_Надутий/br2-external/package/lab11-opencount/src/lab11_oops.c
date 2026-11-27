// SPDX-License-Identifier: GPL-2.0-only
/*
 * lab11_oops — навмисно зламаний модуль для ЛР 11. НЕ завантажуйте його
 * на своїй машині: лише в QEMU, запущеному з -snapshot.
 *
 *   insmod lab11_oops.ko mode=1   — розіменування NULL під час завантаження:
 *                                   Oops, процес insmod убито, ядро «tainted»;
 *   insmod lab11_oops.ko mode=2   — нескінченний цикл у init: insmod не
 *                                   завершиться і не вбивається навіть kill -9.
 *
 * Ті самі дві помилки в програмі eBPF верифікатор не пропустить
 * (verifier/bad_null.bpf.c і verifier/bad_loop.bpf.c).
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/processor.h>

static int mode;
module_param(mode, int, 0444);
MODULE_PARM_DESC(mode, "0: load cleanly, 1: NULL dereference, 2: endless loop");

struct counter {
	long value;
};

/* noinline і volatile — щоб компілятор не викинув помилку як «неможливу». */
static noinline struct counter *find_counter(const char *name)
{
	return NULL;
}

static int __init lab11_oops_init(void)
{
	struct counter *c;
	volatile int spin = 1;

	switch (mode) {
	case 1:
		pr_info("mode=1: about to dereference a NULL pointer\n");
		c = find_counter("openat");
		c->value++;		/* ядро тут зупинить insmod з Oops */
		return 0;
	case 2:
		pr_info("mode=2: spinning forever, try kill -9\n");
		while (spin)
			cpu_relax();
		return 0;
	default:
		pr_info("loaded, nothing broken\n");
		return 0;
	}
}

static void __exit lab11_oops_exit(void)
{
	pr_info("unloaded\n");
}

module_init(lab11_oops_init);
module_exit(lab11_oops_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LAB11");
MODULE_DESCRIPTION("LAB11: what a bug in a kernel module looks like");
