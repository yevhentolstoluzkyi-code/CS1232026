/* scan.c — розбиття на слова і звіт. Готовий код, змінювати не потрібно. */
#include "wordfreq.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_word_byte(unsigned char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c >= 0x80;
}

void scanner_init(struct scanner *s)
{
    s->buf = NULL;
    s->len = s->cap = 0;
}

void scanner_free(struct scanner *s)
{
    free(s->buf);
    scanner_init(s);
}

/* Дописати байти до незавершеного слова. */
static int append(struct scanner *s, const char *p, size_t n)
{
    if (s->len + n > s->cap) {
        size_t cap = s->cap ? s->cap : 64;
        while (cap < s->len + n)
            cap *= 2;
        char *nb = realloc(s->buf, cap);
        if (nb == NULL)
            return -1;
        s->buf = nb;
        s->cap = cap;
    }
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)p[i];
        s->buf[s->len++] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : (char)c;
    }
    return 0;
}

int scanner_feed(struct scanner *s, const char *data, size_t n, struct table *t)
{
    size_t i = 0;
    while (i < n) {
        if (!is_word_byte((unsigned char)data[i])) {
            /* роздільник: якщо слово було почате, воно закінчилося */
            if (s->len > 0) {
                if (table_add(t, s->buf, s->len) != 0)
                    return -1;
                s->len = 0;
            }
            i++;
            continue;
        }
        size_t start = i;
        while (i < n && is_word_byte((unsigned char)data[i]))
            i++;
        if (append(s, data + start, i - start) != 0)
            return -1;
    }
    return 0;
}

int scanner_finish(struct scanner *s, struct table *t)
{
    if (s->len > 0) {
        if (table_add(t, s->buf, s->len) != 0)
            return -1;
        s->len = 0;
    }
    return 0;
}

static int by_count_then_word(const void *a, const void *b)
{
    const struct entry *x = *(const struct entry *const *)a;
    const struct entry *y = *(const struct entry *const *)b;
    if (x->count != y->count)
        return x->count > y->count ? -1 : 1;
    return strcmp(x->word, y->word);
}

int report_top(const struct table *t, size_t n)
{
    size_t total = table_size(t);
    if (total == 0)
        return 0;
    struct entry **v = table_entries(t);
    if (v == NULL)
        return -1;
    qsort(v, total, sizeof v[0], by_count_then_word);
    for (size_t i = 0; i < total && i < n; i++)
        printf("%lu %s\n", v[i]->count, v[i]->word);
    free(v);
    return 0;
}
