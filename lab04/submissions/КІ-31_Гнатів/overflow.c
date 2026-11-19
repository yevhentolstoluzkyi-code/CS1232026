#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static int parse_int(const char *s, int *out)
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
    if (val < INT_MIN || val > INT_MAX) {
        return -1;
    }

    *out = (int)val;
    return 0;
}

static int add_checked(int a, int b, int *out)
{
    if ((b > 0 && a > INT_MAX - b) || (b < 0 && a < INT_MIN - b)) {
        return -1;
    }
    *out = a + b;
    return 0;
}

int main(int argc, char *argv[])
{
    int a, b, r;

    if (argc != 3 || parse_int(argv[1], &a) != 0 || parse_int(argv[2], &b) != 0) {
        fprintf(stderr, "usage: overflow A B\n");
        return 2;
    }

    if (add_checked(a, b, &r) != 0) {
        puts("overflow detected");
    } else {
        printf("sum = %d\n", r);
    }

    return 0;
}
