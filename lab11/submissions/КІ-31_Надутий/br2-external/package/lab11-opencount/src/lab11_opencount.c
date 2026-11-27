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
 * TODO 2. Зробіть ці дві змінні параметрами модуля:
 *   target  — рядок (ім'я процесу), задається лише під час завантаження,
 *             у /sys/module/lab11_opencount/parameters/ лише для читання;
 *             параметр-рядок у власний буфер — module_param_string();
 *   verbose — bool, який root може змінювати під час роботи модуля
 *             (права 0644).
 * Кожен параметр опишіть MODULE_PARM_DESC: опис покаже modinfo.
 */
static char target[TASK_COMM_LEN];
static bool verbose;

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
	/*
	 * TODO 3. Пробник.
	 *  1. Нас цікавить лише openat(): порівняйте id з __NR_openat.
	 *  2. Збільште total (atomic64_inc: пробник працює на всіх процесорах
	 *     одночасно).
	 *  3. Якщо target порожній — усе. Інакше візьміть ім'я поточного
	 *     процесу (get_task_comm(comm, current)) і порівняйте з target
	 *     (strncmp, TASK_COMM_LEN). Не збіглося — усе.
	 *  4. Збільште matched і запишіть номер у pr_debug:
	 *       "%s[%d] openat #%lld\n" (ім'я, task_pid_nr(current), номер).
	 *  5. Якщо verbose (READ_ONCE(verbose)) — дістаньте ім'я файла:
	 *     syscall_get_arguments(current, regs, args) дає аргументи виклику,
	 *     ім'я файла — args[1], і це адреса в пам'яті ПРОЦЕСУ. Скопіюйте
	 *     рядок у path через strncpy_from_user (не більше PATH_LEN байтів;
	 *     пам'ятайте про помилку й про рядок без нуля в кінці) і запишіть
	 *     у журнал pr_info_ratelimited:
	 *       "%s[%d] openat %s\n" (ім'я, pid, path).
	 */
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
	for_each_kernel_tracepoint(find_sys_enter, &tp_sys_enter);
	if (!tp_sys_enter) {
		pr_err("no sys_enter tracepoint: is CONFIG_FTRACE_SYSCALLS set?\n");
		return -ENOENT;
	}

	proc_entry = proc_create_single("lab11_opencount", 0444, NULL, opencount_show);
	if (!proc_entry)
		return -ENOMEM;

	/*
	 * TODO 4a. Зареєструйте probe_sys_enter на tp_sys_enter
	 * (tracepoint_probe_register, data = NULL). Якщо реєстрація не
	 * вдалася — запишіть помилку в журнал, приберіть уже створений
	 * /proc/lab11_opencount і поверніть код помилки: модуль, що не
	 * завантажився, не має лишати по собі нічого.
	 */

	pr_info("watching openat(), target=%s verbose=%d\n",
		target[0] ? target : "-", verbose);
	return 0;
}

static void __exit opencount_exit(void)
{
	/*
	 * TODO 4b. Зніміть пробник (tracepoint_probe_unregister) і, перш
	 * ніж ядро звільнить код модуля, дочекайтеся, поки пробник, що саме
	 * виконується на іншому процесорі, завершиться
	 * (tracepoint_synchronize_unregister). Потім приберіть /proc-файл.
	 */
	pr_info("unloaded: total=%lld matched=%lld\n",
		(long long)atomic64_read(&total), (long long)atomic64_read(&matched));
}

module_init(opencount_init);
module_exit(opencount_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LAB11 student");
MODULE_DESCRIPTION("LAB11: count openat() calls via the sys_enter tracepoint");
