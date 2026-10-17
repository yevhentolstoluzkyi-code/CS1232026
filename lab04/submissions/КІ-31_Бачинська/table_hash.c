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

/* Подвоює кількість кошиків. 0 або -1. */
static int grow(struct table *t)
{
    size_t new_nbuckets = t->nbuckets * 2;
    struct node **new_buckets =
        calloc(new_nbuckets, sizeof *new_buckets);

    if (new_buckets == NULL)
        return -1;

    for (size_t i = 0; i < t->nbuckets; i++) {
        struct node *node = t->buckets[i];

        while (node != NULL) {
            struct node *next = node->next;
            size_t index = node->hash & (new_nbuckets - 1);

            node->next = new_buckets[index];
            new_buckets[index] = node;

            node = next;
        }
    }

    free(t->buckets);
    t->buckets = new_buckets;
    t->nbuckets = new_nbuckets;

    return 0;
}

int table_add(struct table *t, const char *word, size_t len)
{
    uint64_t hash = fnv1a(word, len);
    size_t index = hash & (t->nbuckets - 1);

    for (struct node *node = t->buckets[index];
         node != NULL;
         node = node->next) {

        if (node->hash == hash &&
            node->len == len &&
            memcmp(node->e.word, word, len) == 0) {
            node->e.count++;
            return 0;
        }
    }

    struct node *node = malloc(sizeof *node);
    if (node == NULL)
        return -1;

    node->e.word = malloc(len + 1);
    if (node->e.word == NULL) {
        free(node);
        return -1;
    }

    memcpy(node->e.word, word, len);
    node->e.word[len] = '\0';

    node->e.count = 1;
    node->hash = hash;
    node->len = len;

    node->next = t->buckets[index];
    t->buckets[index] = node;
    t->n++;

    if (t->n > t->nbuckets) {
        if (grow(t) != 0)
            return -1;
    }

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

    size_t j = 0;

    for (size_t i = 0; i < t->nbuckets; i++) {
        for (struct node *node = t->buckets[i];
             node != NULL;
             node = node->next) {
            v[j++] = &node->e;
        }
    }

    return v;
}

void table_free(struct table *t)
{
    if (t == NULL)
        return;

    for (size_t i = 0; i < t->nbuckets; i++) {
        struct node *node = t->buckets[i];

        while (node != NULL) {
            struct node *next = node->next;
            free(node->e.word);
            free(node);
            node = next;
        }
    }

    free(t->buckets);
    free(t);
}
