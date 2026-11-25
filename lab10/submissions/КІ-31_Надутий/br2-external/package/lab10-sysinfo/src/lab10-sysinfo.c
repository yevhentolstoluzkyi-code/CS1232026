/*
 * lab10-sysinfo — короткий звіт про систему, на якій запущено програму.
 *
 *   lab10-sysinfo           повний звіт (ядро, libc, час роботи, пам'ять,
 *                           процеси, модулі, коренева ФС)
 *   lab10-sysinfo -m NAME   чи завантажено модуль ядра NAME:
 *                           код виходу 0 — так, 1 — ні, 2 — помилка
 *
 * Усі дані — з /proc і системних викликів, без зовнішніх бібліотек:
 * програма має однаково працювати на робочій машині й у мінімальному
 * образі Buildroot, де немає нічого, крім BusyBox і libc.
 */
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <unistd.h>

#ifdef __GLIBC__
#include <gnu/libc-version.h>
#endif

#define VERSION "1.0"

/* Секунди від завантаження ядра: перше число в /proc/uptime. */
static int read_uptime(double *seconds)
{
	FILE *f = fopen("/proc/uptime", "r");
	if (!f)
		return -1;
	int ok = fscanf(f, "%lf", seconds) == 1;
	fclose(f);
	return ok ? 0 : -1;
}

/* MemTotal і MemAvailable з /proc/meminfo, у КіБ. */
static int read_meminfo(long *total_kib, long *avail_kib)
{
	FILE *f = fopen("/proc/meminfo", "r");
	if (!f)
		return -1;
	char key[64];
	long value;
	int found = 0;
	*total_kib = *avail_kib = -1;
	while (fscanf(f, "%63s %ld kB", key, &value) == 2) {
		if (strcmp(key, "MemTotal:") == 0) {
			*total_kib = value;
			found++;
		} else if (strcmp(key, "MemAvailable:") == 0) {
			*avail_kib = value;
			found++;
		}
	}
	fclose(f);
	return found == 2 ? 0 : -1;
}

/* Чи складається рядок лише з цифр (і не порожній). */
static int all_digits(const char *s)
{
	if (*s == '\0')
		return 0;
	for (; *s; s++)
		if (!isdigit((unsigned char)*s))
			return 0;
	return 1;
}

/*
 * TODO 4a. Кількість процесів у системі.
 * Кожен процес — каталог /proc/<PID>, ім'я якого складається лише з цифр.
 * Повертає кількість або -1, якщо /proc не вдалося відкрити.
 */
static int count_processes(void)
{
	(void)all_digits; /* приберіть цей рядок, коли використаєте all_digits() */
	return -1;
}

/*
 * TODO 4b. Модулі ядра з /proc/modules.
 * Кожен рядок файла описує один модуль; перше поле — його назва:
 *   lab10_hello 12288 0 - Live 0xffffffffc0000000 (O)
 * Повертає кількість завантажених модулів (або -1, якщо файл не відкрито);
 * якщо want != NULL, у *found записує 1, коли серед них є модуль want.
 */
static int scan_modules(const char *want, int *found)
{
	(void)want;
	if (found)
		*found = 0;
	return -1;
}

static const char *libc_name(void)
{
#ifdef __GLIBC__
	static char buf[32];
	snprintf(buf, sizeof buf, "glibc %s", gnu_get_libc_version());
	return buf;
#else
	return "non-glibc (musl or uClibc-ng)";
#endif
}

static int check_module(const char *name)
{
	int found;
	if (scan_modules(name, &found) < 0) {
		fprintf(stderr, "lab10-sysinfo: /proc/modules: %s\n", strerror(errno));
		return 2;
	}
	printf("%s: %s\n", name, found ? "loaded" : "not loaded");
	return found ? 0 : 1;
}

static void report(void)
{
	struct utsname u;
	double up;
	long total, avail;
	struct statvfs fs;
	int found;

	printf("lab10-sysinfo %s\n", VERSION);

	if (uname(&u) == 0) {
		printf("host:    %s\n", u.nodename);
		printf("kernel:  %s %s %s\n", u.sysname, u.release, u.machine);
	}
	printf("libc:    %s\n", libc_name());

	if (read_uptime(&up) == 0)
		printf("uptime:  %.2f s\n", up);

	if (read_meminfo(&total, &avail) == 0)
		printf("memory:  total %ld MiB, available %ld MiB\n",
		       total / 1024, avail / 1024);

	int procs = count_processes();
	if (procs >= 0)
		printf("procs:   %d\n", procs);

	int mods = scan_modules("lab10_hello", &found);
	if (mods >= 0)
		printf("modules: %d loaded, lab10_hello: %s\n",
		       mods, found ? "yes" : "no");

	if (statvfs("/", &fs) == 0) {
		double size = (double)fs.f_blocks * fs.f_frsize / (1024.0 * 1024.0);
		double used = (double)(fs.f_blocks - fs.f_bfree) * fs.f_frsize
			      / (1024.0 * 1024.0);
		printf("rootfs:  used %.1f of %.1f MiB\n", used, size);
	}
}

int main(int argc, char **argv)
{
	if (argc == 3 && strcmp(argv[1], "-m") == 0)
		return check_module(argv[2]);
	if (argc != 1) {
		fprintf(stderr, "usage: %s [-m MODULE]\n", argv[0]);
		return 2;
	}
	report();
	return 0;
}
