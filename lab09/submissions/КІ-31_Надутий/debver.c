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
    /* TODO 1: розбір [epoch:]upstream[-revision]. */
    memset(v, 0, sizeof *v);

    if (s == NULL || *s == '\0')
        return fail(err, "empty version");

    const char *upstream = s;
    const char *colon = strchr(s, ':');

    if (colon != NULL) {
        if (colon == s)
            return fail(err, "empty epoch");

        unsigned epoch = 0;

        for (const char *p = s; p < colon; p++) {
            if (!isdigit((unsigned char)*p))
                return fail(err, "invalid epoch");

            unsigned digit = (unsigned)(*p - '0');

            if (epoch > ((unsigned)INT_MAX - digit) / 10)
                return fail(err, "epoch too large");

            epoch = epoch * 10 + digit;
        }

        v->epoch = epoch;
        upstream = colon + 1;
    }

    const char *dash = strrchr(upstream, '-');
    size_t upstream_len = dash
        ? (size_t)(dash - upstream)
        : strlen(upstream);

    if (upstream_len == 0)
        return fail(err, "empty upstream");

    if (upstream_len >= DEBVER_MAX)
        return fail(err, "upstream too long");

    memcpy(v->upstream, upstream, upstream_len);
    v->upstream[upstream_len] = '\0';

    if (!isdigit((unsigned char)v->upstream[0]))
        return fail(err, "upstream must start with digit");

    if (!allowed(v->upstream, ".+~-:"))
        return fail(err, "invalid upstream");

    if (dash != NULL) {
        const char *revision = dash + 1;
        size_t revision_len = strlen(revision);

        if (revision_len == 0)
            return fail(err, "empty revision");

        if (revision_len >= DEBVER_MAX)
            return fail(err, "revision too long");

        if (!allowed(revision, ".+~"))
            return fail(err, "invalid revision");

        memcpy(v->revision, revision, revision_len + 1);
    }

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
    /* TODO 2: порівнюємо нецифрові й цифрові частини за dpkg. */
    while (*a || *b) {
        while ((*a && !isdigit((unsigned char)*a)) ||
               (*b && !isdigit((unsigned char)*b))) {
            int oa = order((unsigned char)*a);
            int ob = order((unsigned char)*b);

            if (oa != ob)
                return oa < ob ? -1 : 1;

            if (*a)
                a++;
            if (*b)
                b++;
        }

        /* Числа порівнюються без перетворення на int. */
        while (*a == '0')
            a++;
        while (*b == '0')
            b++;

        const char *da = a;
        const char *db = b;

        while (isdigit((unsigned char)*a))
            a++;
        while (isdigit((unsigned char)*b))
            b++;

        size_t la = (size_t)(a - da);
        size_t lb = (size_t)(b - db);

        if (la != lb)
            return la < lb ? -1 : 1;

        for (size_t i = 0; i < la; i++) {
            if (da[i] != db[i])
                return da[i] < db[i] ? -1 : 1;
        }
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
