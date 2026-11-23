/* table_hash.c — хеш-таблиця з ланцюжками. Реалізуйте в частині 4.
 *
 * Інтерфейс — той самий, що в table_naive.c (див. wordfreq.h), тож main.c
 * і scan.c не зміняться: Makefile просто компонує інший об'єктний файл.
 *
 * Вимоги:
 *   - хеш-функція FNV-1a, 64 біти (константи нижче);
 *   - кількість кошиків — степінь двійки, індекс кошика — hash & (nbuckets - 1);
 *   - коли кількість слів перевищує кількість кошиків — подвоїти кошики
 *     й перерозподілити записи (хеш не перераховувати: він зберігається);
 *   - table_free звільняє все, що виділено: valgrind і ASan мають мовчати.
 */
#include "wordfreq.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FNV_OFFSET 14695981039346656037ULL
#define FNV_PRIME  1099511628211ULL
#define INIT_BUCKETS 1024U

struct node {
    struct entry e;              /* e.word і e.count — див. wordfreq.h */
    uint64_t     hash;           /* збережений хеш: не рахувати вдруге при розширенні */
    size_t       len;            /* довжина слова: порівнювати спершу її */
    struct node *next;           /* наступний запис у тому самому кошику */
};

struct table {
    struct node **buckets;
    size_t        nbuckets;      /* степінь двійки */
    size_t        n;             /* кількість різних слів */
};

static uint64_t fnv1a(const char *p, size_t len)
{
    uint64_t h = FNV_OFFSET;
    const unsigned char *bytes = (const unsigned char *)p;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint64_t)bytes[i];
        h *= FNV_PRIME;
    }
    return h;
}

struct table *table_new(void)
{
    struct table *t = malloc(sizeof(*t));
    if (t == NULL) {
        return NULL;
    }

    t->buckets = calloc(INIT_BUCKETS, sizeof(*t->buckets));
    if (t->buckets == NULL) {
        free(t);
        return NULL;
    }

    t->nbuckets = INIT_BUCKETS;
    t->n = 0;
    return t;
}

/* Подвоює кількість кошиків. 0 або -1. */
static int grow(struct table *t)
{
    if (t->nbuckets > SIZE_MAX / 2) {
        return -1;
    }

    size_t new_nbuckets = t->nbuckets * 2;
    struct node **new_buckets = calloc(new_nbuckets, sizeof(*new_buckets));
    if (new_buckets == NULL) {
        return -1;
    }

    size_t mask = new_nbuckets - 1;
    for (size_t i = 0; i < t->nbuckets; i++) {
        struct node *cur = t->buckets[i];
        while (cur != NULL) {
            struct node *next = cur->next;
            size_t idx = (size_t)(cur->hash & mask);
            cur->next = new_buckets[idx];
            new_buckets[idx] = cur;
            cur = next;
        }
    }

    free(t->buckets);
    t->buckets = new_buckets;
    t->nbuckets = new_nbuckets;
    return 0;
}

int table_add(struct table *t, const char *word, size_t len)
{
    if (t == NULL) {
        return -1;
    }

    uint64_t h = fnv1a(word, len);
    size_t idx = (size_t)(h & (t->nbuckets - 1));

    for (struct node *cur = t->buckets[idx]; cur != NULL; cur = cur->next) {
        if (cur->hash == h && cur->len == len && memcmp(cur->e.word, word, len) == 0) {
            cur->e.count++;
            return 0;
        }
    }

    struct node *node = malloc(sizeof(*node));
    if (node == NULL) {
        return -1;
    }

    char *copy = malloc(len + 1);
    if (copy == NULL) {
        free(node);
        return -1;
    }

    if (len > 0) {
        memcpy(copy, word, len);
    }
    copy[len] = '\0';

    node->e.word = copy;
    node->e.count = 1;
    node->hash = h;
    node->len = len;
    node->next = t->buckets[idx];
    t->buckets[idx] = node;
    t->n++;

    if (t->n > t->nbuckets) {
        if (grow(t) != 0) {
            return -1;
        }
    }

    return 0;
}

size_t table_size(const struct table *t)
{
    return t->n;
}

struct entry **table_entries(const struct table *t)
{
    if (t == NULL) {
        return NULL;
    }

    struct entry **arr = NULL;
    if (t->n > 0) {
        arr = malloc(t->n * sizeof(*arr));
        if (arr == NULL) {
            return NULL;
        }
    } else {
        arr = malloc(sizeof(*arr));
        if (arr == NULL) {
            return NULL;
        }
    }

    size_t pos = 0;
    for (size_t i = 0; i < t->nbuckets; i++) {
        for (struct node *cur = t->buckets[i]; cur != NULL; cur = cur->next) {
            arr[pos++] = &cur->e;
        }
    }

    return arr;
}

void table_free(struct table *t)
{
    if (t == NULL) {
        return;
    }

    for (size_t i = 0; i < t->nbuckets; i++) {
        struct node *cur = t->buckets[i];
        while (cur != NULL) {
            struct node *next = cur->next;
            free(cur->e.word);
            free(cur);
            cur = next;
        }
    }

    free(t->buckets);
    free(t);
}
