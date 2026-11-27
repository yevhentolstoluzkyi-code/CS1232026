// SPDX-License-Identifier: GPL-2.0
/*
 * bad_null.bpf.c — верифікатор має відмовити: результат
 * bpf_map_lookup_elem() може бути NULL, а його розіменовують без перевірки.
 * Та сама помилка в модулі (lab11_oops mode=1) закінчується Oops.
 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 64);
	__type(key, __u32);
	__type(value, __u64);
} counts SEC(".maps");

SEC("tracepoint/syscalls/sys_enter_openat")
int bad_null(void *ctx)
{
	__u32 pid = bpf_get_current_pid_tgid() >> 32;
	__u64 *cnt = bpf_map_lookup_elem(&counts, &pid);

	*cnt += 1;		/* а якщо такого pid у мапі ще немає? */
	return 0;
}

char LICENSE[] SEC("license") = "GPL";
