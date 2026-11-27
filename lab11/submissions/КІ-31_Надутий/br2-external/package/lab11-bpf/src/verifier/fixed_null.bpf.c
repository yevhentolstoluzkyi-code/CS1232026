// SPDX-License-Identifier: GPL-2.0
/*
 * fixed_null.bpf.c — виправлений варіант: результат
 * bpf_map_lookup_elem() може бути NULL, тому перед доступом є перевірка.
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
int fixed_null(void *ctx)
{
	__u32 pid = bpf_get_current_pid_tgid() >> 32;
	__u64 *cnt = bpf_map_lookup_elem(&counts, &pid);

	if (cnt) __sync_fetch_and_add(cnt, 1); else { __u64 one = 1; bpf_map_update_elem(&counts, &pid, &one, BPF_NOEXIST); }
	return 0;
}

char LICENSE[] SEC("license") = "GPL";
