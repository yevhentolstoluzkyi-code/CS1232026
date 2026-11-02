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
    size_t i = 0;

    while (i < len) {
        h ^= (unsigned char)p[i];
        h *= FNV_PRIME;
        i++;
    }

    return h;
}
struct table *table_new(void)
{
    struct table *t = malloc(sizeof(*t));

    if (t == NULL) {
        return NULL;
    }

    t->nbuckets = 1024;
    t->n = 0;

    t->buckets = calloc(t->nbuckets, sizeof(*t->buckets));

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
        calloc(new_nbuckets, sizeof(*new_buckets));

    if (new_buckets == NULL) {
        return -1;
    }

    size_t i = 0;

    while (i < t->nbuckets) {
        struct node *node = t->buckets[i];

        while (node != NULL) {
            struct node *next = node->next;
            size_t index = node->hash & (new_nbuckets - 1);

            node->next = new_buckets[index];
            new_buckets[index] = node;

            node = next;
        }

        i++;
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
    struct node *node = t->buckets[index];

    while (node != NULL) {
        if (node->hash == hash &&
            node->len == len &&
            memcmp(node->e.word, word, len) == 0) {
            node->e.count++;
            return 0;
        }

        node = node->next;
    }

    struct node *new_node = malloc(sizeof(*new_node));

    if (new_node == NULL) {
        return -1;
    }

    new_node->e.word = malloc(len + 1);

    if (new_node->e.word == NULL) {
        free(new_node);
        return -1;
    }

    memcpy(new_node->e.word, word, len);
    new_node->e.word[len] = '\0';

    new_node->e.count = 1;
    new_node->hash = hash;
    new_node->len = len;

    new_node->next = t->buckets[index];
    t->buckets[index] = new_node;
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
    struct entry **entries = malloc(t->n * sizeof(*entries));

    if (entries == NULL && t->n != 0) {
        return NULL;
    }

    size_t k = 0;
    size_t i = 0;

    while (i < t->nbuckets) {
        struct node *node = t->buckets[i];

        while (node != NULL) {
            entries[k] = &node->e;
            k++;
            node = node->next;
        }

        i++;
    }

    return entries;
}
void table_free(struct table *t)
{
    if (t == NULL) {
        return;
    }

    size_t i = 0;

    while (i < t->nbuckets) {
        struct node *node = t->buckets[i];

        while (node != NULL) {
            struct node *next = node->next;

            free(node->e.word);
            free(node);

            node = next;
        }

        i++;
    }

    free(t->buckets);
    free(t);
}

