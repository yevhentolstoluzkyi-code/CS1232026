// SPDX-License-Identifier: GPL-2.0
/*
 * openbench — скільки коштує один openat()+close() у наносекундах.
 *
 *   openbench [-n N] [-r R] [ФАЙЛ]
 *
 * R разів поспіль виконує N пар open(ФАЙЛ)/close() і друкує час на одну
 * пару в кожному прогоні та медіану. Типово N=100000, R=5, ФАЙЛ=/dev/null.
 * Запускайте без спостерігача, з модулем і з програмою eBPF: різниця
 * медіан — ціна спостереження за кожним openat().
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define MAX_RUNS 51

static double now_ns(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1e9 + ts.tv_nsec;
}

static int cmp_double(const void *a, const void *b)
{
	double x = *(const double *)a, y = *(const double *)b;

	return (x > y) - (x < y);
}

int main(int argc, char **argv)
{
	const char *path = "/dev/null";
	long n = 100000;
	int runs = 5, opt;
	double per[MAX_RUNS];

	while ((opt = getopt(argc, argv, "n:r:")) != -1) {
		switch (opt) {
		case 'n': n = atol(optarg); break;
		case 'r': runs = atoi(optarg); break;
		default:
			fprintf(stderr, "usage: %s [-n N] [-r R] [FILE]\n", argv[0]);
			return 2;
		}
	}
	if (optind < argc)
		path = argv[optind];
	if (n <= 0 || runs <= 0 || runs > MAX_RUNS) {
		fprintf(stderr, "openbench: N > 0, 1 <= R <= %d\n", MAX_RUNS);
		return 2;
	}

	for (int r = 0; r < runs; r++) {
		double t0 = now_ns();

		for (long i = 0; i < n; i++) {
			int fd = open(path, O_RDONLY);

			if (fd < 0) {
				perror(path);
				return 1;
			}
			close(fd);
		}
		per[r] = (now_ns() - t0) / n;
		printf("run %d: %.1f ns\n", r + 1, per[r]);
	}
	qsort(per, runs, sizeof(per[0]), cmp_double);
	printf("openbench: %ld x open+close of %s, median %.1f ns\n", n, path, per[runs / 2]);
	return 0;
}
