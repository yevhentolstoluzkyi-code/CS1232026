/* table_hash.c — хеш-таблиця з ланцюжками, FNV-1a 64 біти. */
#include "wordfreq.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FNV_OFFSET 14695981039346656037ULL
#define FNV_PRIME  1099511628211ULL

struct node {
    struct entry e;
    uint64_t     hash;           /* збережений хеш: не рахуємо вдруге при розширенні */
    size_t       len;
    struct node *next;
};

struct table {
    struct node **buckets;
    size_t        nbuckets;      /* степінь двійки */
    size_t        n;
};

static uint64_t fnv1a(const char *p, size_t len)
{
    uint64_t h = FNV_OFFSET;
    for (size_t i = 0; i < len; i++) {
        h ^= (unsigned char)p[i];
        h *= FNV_PRIME;
    }
    return h;
}

struct table *table_new(void)
{
    struct table *t = calloc(1, sizeof *t);
    if (t == NULL)
        return NULL;
    t->nbuckets = 1024;
    t->buckets = calloc(t->nbuckets, sizeof *t->buckets);
    if (t->buckets == NULL) {
        free(t);
        return NULL;
    }
    return t;
}

/* Подвоює кількість кошиків. 0 або -1 (стара таблиця лишається цілою). */
static int grow(struct table *t)
{
    size_t nb = t->nbuckets * 2;
    struct node **nbk = calloc(nb, sizeof *nbk);
    if (nbk == NULL)
        return -1;
    for (size_t i = 0; i < t->nbuckets; i++) {
        struct node *p = t->buckets[i];
        while (p != NULL) {
            struct node *next = p->next;
            size_t idx = p->hash & (nb - 1);
            p->next = nbk[idx];
            nbk[idx] = p;
            p = next;
        }
    }
    free(t->buckets);
    t->buckets = nbk;
    t->nbuckets = nb;
    return 0;
}

int table_add(struct table *t, const char *word, size_t len)
{
    uint64_t h = fnv1a(word, len);
    size_t idx = h & (t->nbuckets - 1);

    for (struct node *p = t->buckets[idx]; p != NULL; p = p->next) {
        if (p->hash == h && p->len == len && memcmp(p->e.word, word, len) == 0) {
            p->e.count++;
            return 0;
        }
    }

    struct node *nd = malloc(sizeof *nd);
    if (nd == NULL)
        return -1;
    char *copy = malloc(len + 1);
    if (copy == NULL) {
        free(nd);                /* не лишаємо витоку на шляху помилки */
        return -1;
    }
    memcpy(copy, word, len);
    copy[len] = '\0';

    nd->e.word = copy;
    nd->e.count = 1;
    nd->hash = h;
    nd->len = len;
    nd->next = t->buckets[idx];
    t->buckets[idx] = nd;
    t->n++;

    if (t->n > t->nbuckets && grow(t) != 0)
        return -1;
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
    size_t k = 0;
    for (size_t i = 0; i < t->nbuckets; i++)
        for (struct node *p = t->buckets[i]; p != NULL; p = p->next)
            v[k++] = &p->e;
    return v;
}

void table_free(struct table *t)
{
    if (t == NULL)
        return;
    for (size_t i = 0; i < t->nbuckets; i++) {
        struct node *p = t->buckets[i];
        while (p != NULL) {
            struct node *next = p->next;
            free(p->e.word);
            free(p);
            p = next;
        }
    }
    free(t->buckets);
    free(t);
}
