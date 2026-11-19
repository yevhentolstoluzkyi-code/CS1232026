/* vercmp — порівняння версій пакетів Debian, як dpkg --compare-versions.
 * ЛР 9, «Системне програмне забезпечення».
 *
 *   vercmp A OP B      OP: lt le eq ne ge gt  або  << <= = >= >>
 *                      код виходу 0 — істина, 1 — хиба, 2 — помилка
 *   vercmp show V      друкує епоху, upstream і ревізію
 *   vercmp sort        читає версії зі stdin, друкує за зростанням
 */
#include "debver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_or_die(const char *s, struct debver *v)
{
    const char *why = NULL;
    if (debver_parse(s, v, &why) != 0) {
        fprintf(stderr, "vercmp: bad version '%s': %s\n", s, why);
        exit(2);
    }
    return 0;
}

static int op_holds(const char *op, int r)
{
    if (!strcmp(op, "lt") || !strcmp(op, "<<")) return r < 0;
    if (!strcmp(op, "le") || !strcmp(op, "<="))  return r <= 0;
    if (!strcmp(op, "eq") || !strcmp(op, "="))   return r == 0;
    if (!strcmp(op, "ne"))                       return r != 0;
    if (!strcmp(op, "ge") || !strcmp(op, ">="))  return r >= 0;
    if (!strcmp(op, "gt") || !strcmp(op, ">>"))  return r > 0;
    fprintf(stderr, "vercmp: unknown operator '%s'\n", op);
    exit(2);
}

struct item { struct debver v; char text[2 * DEBVER_MAX + 16]; };

static int item_cmp(const void *x, const void *y)
{
    const struct item *a = x, *b = y;
    int r = debver_cmp(&a->v, &b->v);
    return r ? r : strcmp(a->text, b->text);
}

static int cmd_sort(void)
{
    size_t n = 0, cap = 64;
    struct item *it = malloc(cap * sizeof *it);
    char line[2 * DEBVER_MAX + 16];
    if (!it) { perror("malloc"); return 2; }
    while (fgets(line, sizeof line, stdin)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0')
            continue;
        if (n == cap) {
            cap *= 2;
            struct item *t = realloc(it, cap * sizeof *it);
            if (!t) { perror("realloc"); free(it); return 2; }
            it = t;
        }
        parse_or_die(line, &it[n].v);
        snprintf(it[n].text, sizeof it[n].text, "%s", line);
        n++;
    }
    qsort(it, n, sizeof *it, item_cmp);
    for (size_t i = 0; i < n; i++)
        puts(it[i].text);
    free(it);
    return 0;
}

static void usage(void)
{
    fprintf(stderr, "usage: vercmp A OP B | vercmp show V | vercmp sort\n");
    exit(2);
}

int main(int argc, char **argv)
{
    struct debver a, b;
    if (argc == 2 && !strcmp(argv[1], "sort"))
        return cmd_sort();
    if (argc == 3 && !strcmp(argv[1], "show")) {
        char buf[2 * DEBVER_MAX + 16];
        parse_or_die(argv[2], &a);
        debver_format(&a, buf, sizeof buf);
        printf("epoch %u\nupstream %s\nrevision %s\ncanonical %s\n",
               a.epoch, a.upstream, a.revision[0] ? a.revision : "-", buf);
        return 0;
    }
    if (argc != 4)
        usage();
    parse_or_die(argv[1], &a);
    parse_or_die(argv[3], &b);
    return op_holds(argv[2], debver_cmp(&a, &b)) ? 0 : 1;
}
