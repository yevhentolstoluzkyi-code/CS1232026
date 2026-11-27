/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Спільні для ядра й простору користувача структури ЛР 11: їх бачать і
 * програма eBPF (opencount.bpf.c), і завантажувач (opencount.c), тож
 * розкладка в пам'яті має збігатися байт у байт.
 */
#ifndef LAB11_OPENCOUNT_H
#define LAB11_OPENCOUNT_H

#define COMM_LEN 16		/* TASK_COMM_LEN у ядрі */
#define PATH_LEN 128

/* Ключ мапи counts: ім'я процесу (comm), доповнене нулями. */
struct comm_key {
	char comm[COMM_LEN];
};

/* Єдиний елемент мапи config: її заповнює завантажувач. */
struct config {
	char target[COMM_LEN];	/* за чиїми openat() стежити; "" — ні за чиїми */
	unsigned int verbose;	/* 1 — надсилати подію про кожен виклик target */
};

/* Подія в кільцевому буфері events. */
struct event {
	unsigned int pid;
	char comm[COMM_LEN];
	char path[PATH_LEN];
};

#endif
