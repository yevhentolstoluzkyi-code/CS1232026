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
 * Зверніть увагу: код помилки передається параметром, а не читається з errno
 * всередині. Чому так — питання для звіту 3. */
static void report_error(const char *what, int err)
{
    fprintf(stderr, PROG ": %s: %s\n", what, strerror(err));
}

/* Розбирає N для -n. Повертає 0 і записує значення в *out або -1,
 * якщо рядок не є цілим числом від 1 до INT_MAX.
 *
 * TODO 1. Реалізуйте через strtol. Некоректними мають вважатися:
 *   ""  "abc"  "5x"  " "  "0"  "-3"  "99999999999999999999"
 * Підказка: man 3 strtol, розділ про errno і endptr. */
static int parse_count(const char *s, int *out)
{
    char *end;
    long value;

    if (s == NULL || *s == '\0') {
        return -1;
    }

    errno = 0;
    value = strtol(s, &end, 10);

    if (errno == ERANGE || end == s || *end != '\0') {
        return -1;
    }

    if (value < 1 || value > INT_MAX) {
        return -1;
   }

    *out = (int)value;
    return 0;
}

/* Читає весь дескриптор fd і передає дані сканеру.
 * Повертає 0 або код помилки (значення errno), якщо читання не вдалося.
 *
 * TODO 2. Цикл read(). Врахуйте три випадки результату read і те,
 * що виклик може бути перерваний сигналом (EINTR) — тоді його просто
 * повторюють. Помилка пам'яті в scanner_feed — теж помилка (ENOMEM). */
static int count_fd(int fd, struct scanner *sc, struct table *t)
{
    char buf[BUFSZ];

    for (;;) {
        ssize_t n = read(fd, buf, sizeof(buf));

        if (n > 0) {
            if (scanner_feed(sc, buf, (size_t)n, t) != 0) {
                return ENOMEM;
            }
        } else if (n == 0) {
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
 * Повертає 0 або -1 (повідомлення вже надруковано).
 *
 * TODO 3. open → count_fd → scanner_finish → close.
 * Кожен крок може не вдатися; кожна помилка — повідомлення через
 * report_error з іменем файла («stdin» для stdin). Дескриптор має бути
 * закритий за будь-якого результату (stdin не закривайте). */
static int count_file(const char *name, struct table *t)
{
    int fd;
    int err = 0;
    struct scanner sc;

    scanner_init(&sc);

    if (name == NULL) {
        fd = STDIN_FILENO;
    } else {
        fd = open(name, O_RDONLY);

        if (fd == -1) {
            report_error(name, errno);
            return -1;
        }
    }

    err = count_fd(fd, &sc, t);

    if (err == 0) {
        if (scanner_finish(&sc, t) != 0) {
            err = ENOMEM;
        }
    }

    scanner_free(&sc);

    if (name != NULL) {
        if (close(fd) == -1 && err == 0) {
            err = errno;
        }
    }

    if (err != 0) {
        report_error(name == NULL ? "stdin" : name, err);
        return -1;
    }

    return 0;
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
    int i = optind;

    while (i < argc) {
        if (count_file(argv[i], t) != 0) {
            status = 1;
        }
        i++;
    }
}
    /* TODO 4. Якщо файлів немає (optind == argc) — count_file(NULL, t),
     * інакше — count_file для кожного argv[optind..argc-1].
     * Помилка з одним файлом робить status = 1, але не зупиняє цикл. */

    if (report_top(t, (size_t)n) != 0) {
        report_error("report", ENOMEM);
        status = 1;
    }

    /* TODO 5. Переконайтеся, що вивід справді записано. printf лише
     * кладе байти в буфер stdio; справжній write відбувається пізніше.
     * Перевірте цей момент і, якщо він не вдався, надрукуйте
     * «wordfreq: stdout: опис помилки» і поверніть 1.
     * Перевірка: ./wordfreq ФАЙЛ > /dev/full */

if (fflush(stdout) == EOF) {
    report_error("stdout", errno);
    status = 1;
}
    table_free(t);
    return status;
}
