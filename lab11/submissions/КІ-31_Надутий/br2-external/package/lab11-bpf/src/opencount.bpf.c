// SPDX-License-Identifier: GPL-2.0
/*
 * opencount.bpf.c — та сама задача, що й у модулі lab11_opencount, але
 * програмою eBPF: рахувати виклики openat() за іменем процесу і, якщо
 * попросили, надсилати в простір користувача ім'я кожного відкритого
 * файла процесу target.
 *
 * Компілюється clang у байт-код BPF (clang -target bpf), завантажується
 * програмою opencount через libbpf, перевіряється верифікатором ядра.
 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

#include "opencount.h"

/*
 * Контекст точки трасування syscalls/sys_enter_openat. Розкладку взято з
 *   /sys/kernel/tracing/events/syscalls/sys_enter_openat/format
 * Кожен аргумент системного виклику займає 8 байтів, хоч би який мав тип.
 */
struct openat_ctx {
	__u64 common;		/* common_type, common_flags, ..., common_pid */
	__s32 syscall_nr;	/* offset 8 */
	__u32 pad;
	__u64 dfd;		/* offset 16 */
	__u64 filename;		/* offset 24: const char __user * */
	__u64 flags;		/* offset 32 */
	__u64 mode;		/* offset 40 */
};

/* Мапа «ім'я процесу → кількість openat()». Її читає завантажувач. */
struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 1024);
	__type(key, struct comm_key);
	__type(value, __u64);
} counts SEC(".maps");

/* Налаштування: один елемент, ключ 0. Записує завантажувач. */
struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 1);
	__type(key, __u32);
	__type(value, struct config);
} config SEC(".maps");

/* Кільцевий буфер подій для простору користувача. */
struct {
	__uint(type, BPF_MAP_TYPE_RINGBUF);
	__uint(max_entries, 64 * 1024);
} events SEC(".maps");

static __always_inline int comm_eq(const char *a, const char *b)
{
	for (int i = 0; i < COMM_LEN; i++) {
		if (a[i] != b[i])
			return 0;
		if (a[i] == '\0')
			return 1;
	}
	return 1;
}

SEC("tracepoint/syscalls/sys_enter_openat")
int on_openat(struct openat_ctx *ctx)
{
	struct comm_key key = {};
	struct config *cfg;
	struct event *e;
	__u64 one = 1, *cnt;
	__u32 zero = 0;

	bpf_get_current_comm(key.comm, sizeof(key.comm));

	/*
	 * TODO 5. Лічильник у мапі counts для ключа key.
	 * bpf_map_lookup_elem повертає вказівник на значення або NULL.
	 * Є значення — збільште його атомарно (__sync_fetch_and_add): ту саму
	 * програму одночасно виконують кілька процесорів. Немає — створіть
	 * запис зі значенням 1 (bpf_map_update_elem з прапорцем BPF_NOEXIST).
	 * Подумайте, що буде, якщо два процесори одночасно не знайшли ключ,
	 * і обробіть цей випадок.
	 */

	/*
	 * TODO 6. Подія для простору користувача.
	 * Візьміть налаштування з мапи config (ключ zero). Якщо їх немає,
	 * verbose вимкнено або key.comm не дорівнює cfg->target (comm_eq) —
	 * поверніть 0. Інакше зарезервуйте в кільцевому буфері events місце
	 * під struct event (bpf_ringbuf_reserve; NULL — буфер повний, подію
	 * пропускаємо), заповніть pid (старші 32 біти
	 * bpf_get_current_pid_tgid()), comm і path і відправте
	 * (bpf_ringbuf_submit).
	 * Ім'я файла лежить у пам'яті процесу за адресою ctx->filename:
	 * розіменувати її напряму верифікатор не дасть — див.
	 * verifier/bad_userptr.bpf.c і функцію-помічника для рядків
	 * користувача в man 7 bpf-helpers.
	 */
	(void)cnt; (void)one; (void)cfg; (void)e; (void)zero;
	return 0;
}

char LICENSE[] SEC("license") = "GPL";
