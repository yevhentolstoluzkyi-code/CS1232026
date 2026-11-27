// SPDX-License-Identifier: GPL-2.0
/*
 * bad_userptr.bpf.c — верифікатор має відмовити: filename — адреса в
 * пам'яті процесу, а не ядра. Читати її можна лише функцією-помічником
 * bpf_probe_read_user_str(), як в opencount.bpf.c.
 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

struct openat_ctx {
	__u64 common;
	__s32 syscall_nr;
	__u32 pad;
	__u64 dfd;
	__u64 filename;
	__u64 flags;
	__u64 mode;
};

SEC("tracepoint/syscalls/sys_enter_openat")
int bad_userptr(struct openat_ctx *ctx)
{
	const char *name = (const char *)ctx->filename;

	return name[0] == '/';	/* абсолютний шлях? */
}

char LICENSE[] SEC("license") = "GPL";
