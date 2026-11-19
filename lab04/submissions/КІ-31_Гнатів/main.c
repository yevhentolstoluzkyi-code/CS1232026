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

static void report_error(const char *what, int err)
{
    fprintf(stderr, PROG ": %s: %s\n", what, strerror(err));
}

static int parse_count(const char *s, int *out)
{
    if (s == NULL || *s == '\0') {
        return -1;
    }

    char *end = NULL;
    errno = 0;
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

static int count_fd(int fd, struct scanner *sc, struct table *t)
{
    char buf[BUFSZ];

    for (;;) {
        ssize_t r = read(fd, buf, sizeof(buf));
        if (r > 0) {
            if (scanner_feed(sc, buf, (size_t)r, t) != 0) {
                return ENOMEM;
            }
        } else if (r == 0) {
            return 0;
        } else {
            if (errno == EINTR) {
                continue;
            }
            return errno;
        }
    }
}

static int count_file(const char *name, struct table *t)
{
    int fd;
    const char *label;

    if (name == NULL) {
        fd = STDIN_FILENO;
        label = "stdin";
    } else {
        label = name;
        fd = open(name, O_RDONLY);
        if (fd < 0) {
            report_error(label, errno);
            return -1;
        }
    }

    struct scanner sc;
    scanner_init(&sc);

    int err = count_fd(fd, &sc, t);
    if (err == 0 && scanner_finish(&sc, t) != 0) {
        err = ENOMEM;
    }

    scanner_free(&sc);

    if (err != 0) {
        report_error(label, err);
        if (name != NULL) {
            close(fd);
        }
        return -1;
    }

    if (name != NULL && close(fd) != 0) {
        report_error(label, errno);
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

    if (fflush(stdout) != 0 || ferror(stdout)) {
        report_error("stdout", errno);
        status = 1;
    }

    table_free(t);
    return status;
}
