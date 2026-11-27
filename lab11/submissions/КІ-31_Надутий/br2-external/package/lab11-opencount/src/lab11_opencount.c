// SPDX-License-Identifier: GPL-2.0-only
/*
 * lab11_opencount — лічильник системного виклику openat() у модулі ядра.
 *
 * Модуль чіпляється до точки трасування sys_enter, через яку проходить
 * кожен системний виклик, і рахує:
 *   total   — усі виклики openat() у системі;
 *   matched — виклики процесів з іменем (comm), рівним параметру target.
 * Якщо verbose=1, кожен виклик процесу target ще й записується в журнал
 * ядра разом з іменем файла. Лічильники видно у /proc/lab11_opencount.
 * Журнал: pr_info/pr_err з префіксом «lab11_opencount: » (pr_fmt),
 * pr_info_ratelimited — щоб частий виклик не залив журнал, pr_debug —
 * вмикається динамічно, без перезавантаження модуля.
 *
 * Ту саму задачу в ЛР 11 розв'язує програма eBPF opencount.bpf.c.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/atomic.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/tracepoint.h>
#include <linux/uaccess.h>
#include <asm/syscall.h>
#include <asm/unistd.h>

#define PATH_LEN 128

/*
 * TODO 2: параметри модуля.
 * target задається лише при завантаженні;
 * verbose можна змінювати від root під час роботи.
 */
static char target[TASK_COMM_LEN];
module_param_string(target, target, sizeof(target), 0444);
MODULE_PARM_DESC(target, "Process comm to count (read-only)");

static bool verbose;
module_param(verbose, bool, 0644);
MODULE_PARM_DESC(verbose, "Log matching openat paths when enabled");

static atomic64_t total = ATOMIC64_INIT(0);
static atomic64_t matched = ATOMIC64_INIT(0);

static struct tracepoint *tp_sys_enter;
static struct proc_dir_entry *proc_entry;

/*
 * Пробник точки трасування sys_enter: викликається на вході в КОЖЕН
 * системний виклик кожного процесу. Перші два аргументи задає сама точка
 * трасування (TP_PROTO у include/trace/events/syscalls.h), data — те, що
 * ми передали під час реєстрації.
 *
 * Починаючи з ядра 6.13 точки трасування системних викликів «faultable»:
 * пробник працює з увімкненою витіснюваністю і може торкатися пам'яті
 * користувача (strncpy_from_user може заснути на сторінковому збої).
 */
static void probe_sys_enter(void *data, struct pt_regs *regs, long id)
{
        char comm[TASK_COMM_LEN];
        char path[PATH_LEN];
        unsigned long args[6];
        long copied;
        long long number;

        /* TODO 3.1: рахуємо лише openat(). */
        if (id != __NR_openat)
                return;

        atomic64_inc(&total);

        /* Без заданого target збіги не підраховуємо. */
        if (!target[0])
                return;

        get_task_comm(comm, current);
        if (strncmp(comm, target, TASK_COMM_LEN) != 0)
                return;

        /* TODO 3.2: атомарний лічильник збігів. */
        number = atomic64_inc_return(&matched);

        pr_debug("%s[%d] openat #%lld\n",
                 comm, task_pid_nr(current), number);

        /* TODO 3.3: шлях виводимо лише у verbose-режимі. */
        if (!READ_ONCE(verbose))
                return;

        syscall_get_arguments(current, regs, args);

        copied = strncpy_from_user(
                path, (const char __user *)args[1], PATH_LEN
        );

        if (copied < 0) {
                strscpy(path, "<fault>", sizeof(path));
        } else if (copied >= PATH_LEN) {
                path[PATH_LEN - 1] = '\0';
        }

        pr_info_ratelimited("%s[%d] openat %s\n",
                            comm, task_pid_nr(current), path);
}

static int opencount_show(struct seq_file *m, void *v)
{
	seq_printf(m, "target:  %s\n", target[0] ? target : "-");
	seq_printf(m, "verbose: %d\n", READ_ONCE(verbose));
	seq_printf(m, "total:   %lld\n", (long long)atomic64_read(&total));
	seq_printf(m, "matched: %lld\n", (long long)atomic64_read(&matched));
	return 0;
}

/* Точка трасування sys_enter не експортована модулям за ім'ям, тож
 * шукаємо її серед усіх точок ядра. */
static void find_sys_enter(struct tracepoint *tp, void *priv)
{
	if (strcmp(tp->name, "sys_enter") == 0)
		*(struct tracepoint **)priv = tp;
}

static int __init opencount_init(void)
{
        int ret;

        for_each_kernel_tracepoint(find_sys_enter, &tp_sys_enter);
        if (!tp_sys_enter) {
                pr_err("no sys_enter tracepoint: is CONFIG_FTRACE_SYSCALLS set?\n");
                return -ENOENT;
        }

        proc_entry = proc_create_single("lab11_opencount", 0444, NULL,
                                        opencount_show);
        if (!proc_entry)
                return -ENOMEM;

        ret = tracepoint_probe_register(tp_sys_enter, probe_sys_enter, NULL);
        if (ret) {
                pr_err("failed to register sys_enter probe: %d\n", ret);
                proc_remove(proc_entry);
                return ret;
        }

        pr_info("watching openat(), target=%s verbose=%d\n",
                target[0] ? target : "-", verbose);
        return 0;
}

static void __exit opencount_exit(void)
{
        tracepoint_probe_unregister(tp_sys_enter, probe_sys_enter, NULL);
        tracepoint_synchronize_unregister();
        proc_remove(proc_entry);

        pr_info("unloaded: total=%lld matched=%lld\n",
                (long long)atomic64_read(&total),
                (long long)atomic64_read(&matched));
}

module_init(opencount_init);
module_exit(opencount_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LAB11 student");
MODULE_DESCRIPTION("LAB11: count openat() calls via the sys_enter tracepoint");
