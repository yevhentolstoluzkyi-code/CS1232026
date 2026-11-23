/* main.c — wordfreq: частотний словник текстових файлів.
 *
 *   wordfreq [-n N] [ФАЙЛ...]
 *
 * Специфікація — у тексті ЛР 4, розділ 4.1. Коротко:
 *   - без файлів читає stdin;
 *   - друкує N (за замовчуванням 10) найчастіших слів;
 *   - файл, який не вдалося відкрити чи прочитати, не зупиняє роботу:
 *     повідомлення «wordfreq: ФАЙЛ: опис помилки» у stderr і перехід
 *     до наступного файла;
 *   - код повернення: 0 — усе гаразд; 1 — була помилка файла, запису
 *     або пам'яті; 2 — неправильні аргументи.
 *
 * Шукайте позначки TODO. Код поза ними можна змінювати, якщо розумієте навіщо.
 */
#include "wordfreq.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PROG "wordfreq"
#define BUFSZ 65536

static void usage(void)
{
    fprintf(stderr, "usage: " PROG " [-n N] [FILE...]\n");
}

/* Повідомлення про помилку у форматі, який перевіряє tools/wordfreq-test.py.
 * Код помилки передається параметром, а не читається з errno всередині
 * (див. питання для звіту 3). */
static void report_error(const char *what, int err)
{
    fprintf(stderr, PROG ": %s: %s\n", what, strerror(err));
}

/* Розбирає N для -n. Повертає 0 і записує значення в *out або -1,
 * якщо рядок не є цілим числом від 1 до INT_MAX. */
static int parse_count(const char *s, int *out)
{
    if (s == NULL || *s == '\0') {
        return -1;
    }

    errno = 0;
    char *end = NULL;
    long val = strtol(s, &end, 10);

    if (errno == ERANGE || end == s || *end != '\0') {
        return -1;
    }
    if (val < 1 || val > INT_MAX) {
        return -1;
    }

    *out = (int)val;
    return 0;
}

/* Читає весь дескриптор fd і передає дані сканеру.
 * Повертає 0 або код помилки (значення errno), якщо читання не вдалося. */
static int count_fd(int fd, struct scanner *sc, struct table *t)
{
    char buf[BUFSZ];

    for (;;) {
        ssize_t nread = read(fd, buf, sizeof(buf));
        if (nread > 0) {
            if (scanner_feed(sc, buf, (size_t)nread, t) != 0) {
                return ENOMEM;
            }
        } else if (nread == 0) {
            return 0;
        } else {
            if (errno == EINTR) {
                continue;
            }
            return errno;
        }
    }
}

/* Обробляє один файл. name == NULL означає stdin.
 * Повертає 0 або -1 (повідомлення вже надруковано). */
static int count_file(const char *name, struct table *t)
{
    const char *label = (name != NULL) ? name : "stdin";
    int fd;

    if (name != NULL) {
        fd = open(name, O_RDONLY);
        if (fd < 0) {
            report_error(label, errno);
            return -1;
        }
    } else {
        fd = STDIN_FILENO;
    }

    struct scanner sc;
    scanner_init(&sc);

    int rc = 0;
    int rd_err = count_fd(fd, &sc, t);
    if (rd_err != 0) {
        report_error(label, rd_err);
        rc = -1;
    } else if (scanner_finish(&sc, t) != 0) {
        report_error(label, ENOMEM);
        rc = -1;
    }

    scanner_free(&sc);

    if (name != NULL && close(fd) != 0) {
        int cl_err = errno;
        if (rc == 0) {
            report_error(label, cl_err);
            rc = -1;
        }
    }

    return rc;
}

int main(int argc, char *argv[])
{
    int n = 10;
    int opt;

    while ((opt = getopt(argc, argv, "n:")) != -1) {
        switch (opt) {
        case 'n':
            if (parse_count(optarg, &n) != 0) {
                fprintf(stderr, PROG ": invalid count: '%s'\n", optarg);
                usage();
                return 2;
            }
            break;
        default:        /* getopt уже надрукував, що не так */
            usage();
            return 2;
        }
    }

    struct table *t = table_new();
    if (t == NULL) {
        report_error("table", ENOMEM);
        return 1;
    }

    int status = 0;

    if (optind == argc) {
        if (count_file(NULL, t) != 0) {
            status = 1;
        }
    } else {
        for (int i = optind; i < argc; i++) {
            if (count_file(argv[i], t) != 0) {
                status = 1;
            }
        }
    }

    if (report_top(t, (size_t)n) != 0) {
        report_error("report", ENOMEM);
        status = 1;
    }

    errno = 0;
    if (fflush(stdout) != 0) {
        int out_err = errno ? errno : EIO;
        report_error("stdout", out_err);
        status = 1;
    } else if (ferror(stdout)) {
        int out_err = errno ? errno : EIO;
        report_error("stdout", out_err);
        status = 1;
    }

    table_free(t);
    return status;
}
