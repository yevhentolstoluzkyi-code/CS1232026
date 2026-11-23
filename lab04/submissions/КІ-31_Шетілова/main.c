/* main.c — wordfreq: частотний словник текстових файлів.
 *   wordfreq [-n N] [ФАЙЛ...]
 * Коди повернення: 0 — ок; 1 — помилка файла/запису/пам'яті; 2 — аргументи.
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

/* errno приходить параметром: між помилкою і друком можуть бути інші
 * виклики (fprintf, close), які змінять errno. */
static void report_error(const char *what, int err)
{
    fprintf(stderr, PROG ": %s: %s\n", what, strerror(err));
}

/* TODO 1: розбір N. Некоректно: "", "abc", "5x", " ", "0", "-3", переповнення. */
static int parse_count(const char *s, int *out)
{
    char *end;
    errno = 0;                          /* щоб відрізнити ERANGE від старого errno */
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0')       /* жодної цифри або сміття в кінці */
        return -1;
    if (errno == ERANGE)                /* не вміщується в long */
        return -1;
    if (v < 1 || v > INT_MAX)           /* поза діапазоном 1..INT_MAX */
        return -1;
    *out = (int)v;
    return 0;
}

/* TODO 2: читає fd до кінця. Повертає 0 або errno. */
static int count_fd(int fd, struct scanner *sc, struct table *t)
{
    char buf[BUFSZ];

    for (;;) {
        ssize_t r = read(fd, buf, sizeof buf);
        if (r > 0) {                    /* прочитали r байтів (можливо менше ніж BUFSZ) */
            if (scanner_feed(sc, buf, (size_t)r, t) != 0)
                return ENOMEM;
        } else if (r == 0) {            /* кінець файла */
            return 0;
        } else {                        /* r == -1 */
            if (errno == EINTR)         /* перервано сигналом — повторити */
                continue;
            return errno;
        }
    }
}

/* TODO 3: один файл. name == NULL — stdin. Повертає 0 або -1. */
static int count_file(const char *name, struct table *t)
{
    const char *label = name ? name : "stdin";
    int fd = STDIN_FILENO;
    int rc = 0;

    if (name != NULL) {
        fd = open(name, O_RDONLY);
        if (fd < 0) {
            report_error(label, errno);
            return -1;
        }
    }

    struct scanner sc;
    scanner_init(&sc);

    int err = count_fd(fd, &sc, t);
    if (err != 0) {
        report_error(label, err);
        rc = -1;
    } else if (scanner_finish(&sc, t) != 0) {
        report_error(label, ENOMEM);
        rc = -1;
    }
    scanner_free(&sc);

    if (name != NULL && close(fd) != 0) {   /* stdin не закриваємо */
        report_error(label, errno);
        rc = -1;
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
        default:
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

    /* TODO 4: stdin або кожен файл; помилка не зупиняє цикл */
    if (optind == argc) {
        if (count_file(NULL, t) != 0)
            status = 1;
    } else {
        for (int i = optind; i < argc; i++)
            if (count_file(argv[i], t) != 0)
                status = 1;
    }

    if (report_top(t, (size_t)n) != 0) {
        report_error("report", ENOMEM);
        status = 1;
    }

    /* TODO 5: printf лише пише в буфер; реальний write — тут */
    if (fflush(stdout) != 0 || ferror(stdout)) {
        report_error("stdout", errno);
        status = 1;
    }

    table_free(t);
    return status;
}
