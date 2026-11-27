// SPDX-License-Identifier: GPL-2.0
/*
 * opencount — завантажувач програми eBPF opencount.bpf.o.
 *
 *   opencount [-o OBJ] [-t COMM] [-v] [-d SEC] [-n N]
 *   opencount -c -o OBJ
 *
 *   -o OBJ   об'єктний файл BPF (типово /usr/lib/lab11/opencount.bpf.o)
 *   -t COMM  ім'я процесу, за яким стежити (як target у модулі)
 *   -v       друкувати кожен openat() процесу COMM (кільцевий буфер)
 *   -d SEC   працювати SEC секунд (типово — до Ctrl-C)
 *   -n N     скільки рядків таблиці друкувати наприкінці (типово 10)
 *   -c       лише завантажити (перевірка верифікатором) і вийти
 *
 * Порядок роботи: відкрити об'єкт → завантажити (тут працює верифікатор)
 * → записати налаштування в мапу config → під'єднати програму до точки
 * трасування → читати кільцевий буфер → наприкінці надрукувати мапу counts.
 */
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include "opencount.h"

#define MAX_ROWS 1024

static volatile sig_atomic_t stop;

static void on_signal(int sig)
{
	(void)sig;
	stop = 1;
}

/* libbpf друкує сюди свої повідомлення, зокрема журнал верифікатора. */
static int print_libbpf(enum libbpf_print_level level, const char *fmt, va_list ap)
{
	if (level == LIBBPF_DEBUG)
		return 0;
	return vfprintf(stderr, fmt, ap);
}

static int on_event(void *ctx, void *data, size_t size)
{
	const struct event *e = data;

	(void)ctx;
	if (size < sizeof(*e))
		return 0;
	printf("%-16s %7u  %s\n", e->comm, e->pid, e->path);
	return 0;
}

struct row {
	char comm[COMM_LEN];
	unsigned long long count;
};

static int by_count_desc(const void *a, const void *b)
{
	const struct row *x = a, *y = b;

	if (x->count != y->count)
		return x->count < y->count ? 1 : -1;
	return strncmp(x->comm, y->comm, COMM_LEN);
}

/*
 * Надрукувати N найбільших записів мапи counts і підсумок:
 *   COMM             OPENAT
 *   ...
 *   total: T in K processes
 * Повертає 0 або -1, якщо мапу прочитати не вдалося.
 */
static int print_top(int map_fd, int top)
{
	static struct row rows[MAX_ROWS];
	unsigned long long sum = 0;
	int n = 0;

	/*
	 * TODO 7. Обійдіть мапу map_fd і заповніть rows[0..n) та sum.
	 * bpf_map_get_next_key(fd, NULL, &k) дає перший ключ,
	 * bpf_map_get_next_key(fd, &prev, &k) — наступний за prev;
	 * коли ключі скінчилися, функція повертає помилку і errno == ENOENT
	 * (будь-яке інше errno — справжня помилка: поверніть -1).
	 * Значення ключа — bpf_map_lookup_elem(fd, &k, &value), тип __u64.
	 * Не більше MAX_ROWS рядків.
	 */
	(void)map_fd;

	qsort(rows, n, sizeof(rows[0]), by_count_desc);
	printf("%-16s %10s\n", "COMM", "OPENAT");
	for (int i = 0; i < n && i < top; i++)
		printf("%-16.16s %10llu\n", rows[i].comm, rows[i].count);
	printf("total: %llu in %d processes\n", sum, n);
	return 0;
}

static double now_s(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void usage(const char *argv0)
{
	fprintf(stderr, "usage: %s [-o OBJ] [-t COMM] [-v] [-d SEC] [-n N] [-c]\n", argv0);
}

int main(int argc, char **argv)
{
	const char *obj_path = "/usr/lib/lab11/opencount.bpf.o";
	struct config cfg = {0};
	struct bpf_object *obj = NULL;
	struct bpf_program *prog;
	struct bpf_link *link = NULL;
	struct ring_buffer *rb = NULL;
	double duration = 0, start;
	int top = 10, check_only = 0, opt, err, rc = 1;
	__u32 zero = 0;

	while ((opt = getopt(argc, argv, "o:t:vd:n:ch")) != -1) {
		switch (opt) {
		case 'o': obj_path = optarg; break;
		case 't': strncpy(cfg.target, optarg, COMM_LEN - 1); break;
		case 'v': cfg.verbose = 1; break;
		case 'd': duration = atof(optarg); break;
		case 'n': top = atoi(optarg); break;
		case 'c': check_only = 1; break;
		default: usage(argv[0]); return opt == 'h' ? 0 : 2;
		}
	}

	libbpf_set_print(print_libbpf);

	obj = bpf_object__open_file(obj_path, NULL);
	if (!obj) {
		fprintf(stderr, "opencount: cannot open %s: %s\n", obj_path, strerror(errno));
		return 1;
	}
	err = bpf_object__load(obj);	/* тут ядро запускає верифікатор */
	if (err) {
		fprintf(stderr, "opencount: load failed: %s\n", strerror(-err));
		goto out;
	}
	if (check_only) {
		printf("opencount: %s: verifier accepted all programs\n", obj_path);
		rc = 0;
		goto out;
	}

	err = bpf_map_update_elem(bpf_object__find_map_fd_by_name(obj, "config"),
				  &zero, &cfg, BPF_ANY);
	if (err) {
		fprintf(stderr, "opencount: config: %s\n", strerror(errno));
		goto out;
	}

	prog = bpf_object__find_program_by_name(obj, "on_openat");
	if (!prog) {
		fprintf(stderr, "opencount: no program on_openat in %s\n", obj_path);
		goto out;
	}
	link = bpf_program__attach(prog);
	if (!link) {
		fprintf(stderr, "opencount: attach: %s\n", strerror(errno));
		goto out;
	}

	rb = ring_buffer__new(bpf_object__find_map_fd_by_name(obj, "events"),
			      on_event, NULL, NULL);
	if (!rb) {
		fprintf(stderr, "opencount: ring buffer: %s\n", strerror(errno));
		goto out;
	}

	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);
	fprintf(stderr, "opencount: attached to syscalls/sys_enter_openat, target=%s verbose=%u\n",
		cfg.target[0] ? cfg.target : "-", cfg.verbose);

	start = now_s();
	while (!stop && (duration <= 0 || now_s() - start < duration)) {
		err = ring_buffer__poll(rb, 100 /* мс */);
		if (err < 0 && err != -EINTR) {
			fprintf(stderr, "opencount: poll: %s\n", strerror(-err));
			break;
		}
	}
	fflush(stdout);

	if (print_top(bpf_object__find_map_fd_by_name(obj, "counts"), top) == 0)
		rc = 0;
out:
	ring_buffer__free(rb);
	bpf_link__destroy(link);
	bpf_object__close(obj);
	return rc;
}
