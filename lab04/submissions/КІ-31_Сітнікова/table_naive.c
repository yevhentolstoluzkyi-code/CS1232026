/* table_naive.c — найпростіша таблиця: масив записів і лінійний пошук.
 * Готовий і правильний код. Ваше завдання в частині 4 — з'ясувати
 * вимірюванням, чим він поганий, а не здогадатися. */
#include "wordfreq.h"

#include <stdlib.h>
#include <string.h>

struct table {
    struct entry *items;
    size_t        n, cap;
};

struct table *table_new(void)
{
    return calloc(1, sizeof(struct table));
}

/* Шукає слово в масиві. Повертає індекс або -1. */
static long table_find(const struct table *t, const char *word, size_t len)
{
    for (size_t i = 0; i < t->n; i++)
        if (strncmp(t->items[i].word, word, len) == 0 && t->items[i].word[len] == '\0')
            return (long)i;
    return -1;
}

int table_add(struct table *t, const char *word, size_t len)
{
    long i = table_find(t, word, len);
    if (i >= 0) {
        t->items[i].count++;
        return 0;
    }
    if (t->n == t->cap) {
        size_t cap = t->cap ? t->cap * 2 : 256;
        struct entry *ni = realloc(t->items, cap * sizeof *ni);
        if (ni == NULL)
            return -1;
        t->items = ni;
        t->cap = cap;
    }
    char *copy = malloc(len + 1);
    if (copy == NULL)
        return -1;
    memcpy(copy, word, len);
    copy[len] = '\0';
    t->items[t->n].word = copy;
    t->items[t->n].count = 1;
    t->n++;
    return 0;
}

size_t table_size(const struct table *t)
{
    return t->n;
}

struct entry **table_entries(const struct table *t)
{
    struct entry **v = malloc((t->n ? t->n : 1) * sizeof *v);
    if (v == NULL)
        return NULL;
    for (size_t i = 0; i < t->n; i++)
        v[i] = &t->items[i];
    return v;
}

void table_free(struct table *t)
{
    if (t == NULL)
        return;
    for (size_t i = 0; i < t->n; i++)
        free(t->items[i].word);
    free(t->items);
    free(t);
}
