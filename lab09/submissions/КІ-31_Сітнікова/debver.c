/* debver.c — розбір і порівняння версій Debian. ЛР 9, «Системне програмне забезпечення».
 * TODO 1 — debver_parse, TODO 2 — debver_part_cmp. */
#include "debver.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static int fail(const char **err, const char *why)
{
    if (err)
        *err = why;
    return -1;
}

static int allowed(const char *s, const char *extra)
{
    for (; *s; s++)
        if (!isalnum((unsigned char)*s) && !strchr(extra, *s))
            return 0;
    return 1;
}

int debver_parse(const char *s, struct debver *v, const char **err)
{
    memset(v, 0, sizeof *v);
    if (s == NULL || *s == '\0')
        return fail(err, "empty version");
    for (const char *p = s; *p; p++)
        if (isspace((unsigned char)*p))
            return fail(err, "whitespace in version");

    /* Епоха: усе до ПЕРШОЇ двокрапки. Немає двокрапки — епоха 0. */
    const char *rest = s;
    const char *colon = strchr(s, ':');
    if (colon) {
        size_t n = (size_t)(colon - s);
        if (n == 0)
            return fail(err, "empty epoch");
        unsigned long e = 0;
        for (size_t i = 0; i < n; i++) {
            if (!isdigit((unsigned char)s[i]))
                return fail(err, "epoch is not a number");
            unsigned d = (unsigned)(s[i] - '0');
            if (e > (INT_MAX - d) / 10)
                return fail(err, "epoch too large");
            e = e * 10 + d;
        }
        v->epoch = (unsigned)e;
        rest = colon + 1;
    }

    /* Ревізія: усе після ОСТАННЬОГО дефіса в залишку. Немає дефіса — "". */
    const char *dash = strrchr(rest, '-');
    size_t ulen = dash ? (size_t)(dash - rest) : strlen(rest);
    const char *rev = dash ? dash + 1 : "";
    if (dash && *rev == '\0')
        return fail(err, "empty revision");
    if (!allowed(rev, ".+~"))
        return fail(err, "bad character in revision");
    if (strlen(rev) >= DEBVER_MAX)
        return fail(err, "revision too long");

    /* Версія автора: те, що між епохою і ревізією. */
    if (ulen == 0)
        return fail(err, "empty upstream version");
    if (!isdigit((unsigned char)*rest))
        return fail(err, "upstream version must start with a digit");
    if (ulen >= DEBVER_MAX)
        return fail(err, "upstream version too long");
    memcpy(v->upstream, rest, ulen);
    v->upstream[ulen] = '\0';
    if (!allowed(v->upstream, ".+~-:"))
        return fail(err, "bad character in upstream version");

    snprintf(v->revision, sizeof v->revision, "%s", rev);
    return 0;
}

/* Вага символу в нецифровій частині: ~ менша за все, навіть за кінець
 * рядка; кінець рядка — 0; літери — за кодом; решта — після літер. */
static int order(int c)
{
    if (c == '~')
        return -1;
    if (c == '\0' || isdigit(c))
        return 0;
    if (isalpha(c))
        return c;
    return c + 256;
}

int debver_part_cmp(const char *a, const char *b)
{
    while (*a || *b) {
        /* 1. Нецифрові шматки: посимвольно за вагою order(). */
        while ((*a && !isdigit((unsigned char)*a)) ||
               (*b && !isdigit((unsigned char)*b))) {
            int ac = order((unsigned char)*a);
            int bc = order((unsigned char)*b);
            if (ac != bc)
                return ac < bc ? -1 : 1;
            a++;
            b++;
        }

        /* 2. Цифрові шматки: як числа, але без перетворення на int. */
        while (*a == '0')
            a++;
        while (*b == '0')
            b++;
        int first_diff = 0;
        while (isdigit((unsigned char)*a) && isdigit((unsigned char)*b)) {
            if (!first_diff)
                first_diff = *a - *b;
            a++;
            b++;
        }
        if (isdigit((unsigned char)*a))
            return 1;                      /* довше число — більше */
        if (isdigit((unsigned char)*b))
            return -1;
        if (first_diff)
            return first_diff < 0 ? -1 : 1;
    }
    return 0;
}

int debver_cmp(const struct debver *a, const struct debver *b)
{
    if (a->epoch != b->epoch)
        return a->epoch < b->epoch ? -1 : 1;
    int r = debver_part_cmp(a->upstream, b->upstream);
    if (r)
        return r;
    return debver_part_cmp(a->revision, b->revision);
}

void debver_format(const struct debver *v, char *buf, unsigned long size)
{
    if (v->epoch)
        snprintf(buf, size, "%u:%s%s%s", v->epoch, v->upstream,
                 v->revision[0] ? "-" : "", v->revision);
    else
        snprintf(buf, size, "%s%s%s", v->upstream,
                 v->revision[0] ? "-" : "", v->revision);
}
