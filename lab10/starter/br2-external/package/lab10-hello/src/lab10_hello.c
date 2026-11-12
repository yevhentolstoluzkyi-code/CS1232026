// SPDX-License-Identifier: GPL-2.0-only
/*
 * lab10_hello — найменший модуль ядра для ЛР 10.
 *
 * Код модуля тут навмисно простий: детально модулі розбиратимемо в ЛР 11.
 * Завдання ЛР 10 — щоб Buildroot зібрав модуль проти ядра цього самого
 * образу, поклав його в /lib/modules і щоб система завантажила його сама.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/utsname.h>

static char *greeting = "world";
module_param(greeting, charp, 0444);
MODULE_PARM_DESC(greeting, "Who to greet; see /sys/module/lab10_hello/parameters/greeting");

static unsigned long loaded_at;

static int __init lab10_hello_init(void)
{
	loaded_at = jiffies;
	pr_info("hello, %s (kernel %s, HZ=%d)\n",
		greeting, init_utsname()->release, HZ);
	return 0;
}

static void __exit lab10_hello_exit(void)
{
	pr_info("bye, %s after %u ms\n",
		greeting, jiffies_to_msecs(jiffies - loaded_at));
}

module_init(lab10_hello_init);
module_exit(lab10_hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LAB10 student");
MODULE_DESCRIPTION("LAB10: out-of-tree module packaged by Buildroot");
