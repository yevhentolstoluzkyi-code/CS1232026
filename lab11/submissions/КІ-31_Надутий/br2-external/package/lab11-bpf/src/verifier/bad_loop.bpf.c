// SPDX-License-Identifier: GPL-2.0
/*
 * bad_loop.bpf.c — верифікатор має відмовити: цикл може не закінчитися.
 * Умова виходу залежить від випадкового числа, тож верифікатор не може
 * довести, що програма завершиться. Нескінченний цикл у модулі
 * (lab11_oops mode=2) ядро просто виконує.
 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_openat")
int bad_loop(void *ctx)
{
	__u32 tries = 0;

	while (bpf_get_prandom_u32() != 0)	/* «колись же випаде нуль» */
		tries++;
	return tries & 1;
}

char LICENSE[] SEC("license") = "GPL";
