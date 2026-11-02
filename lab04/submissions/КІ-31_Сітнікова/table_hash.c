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
    /* TODO: h = FNV_OFFSET; для кожного байта: h ^= байт; h *= FNV_PRIME. */
    /*(void)p;
    (void)len;
    return 0;
    */

    uint64_t h = FNV_OFFSET;

    for (size_t i = 0; i < len; i++) {
        h ^= (unsigned char)p[i];
        h *= FNV_PRIME;
    }

    return h;
}

struct table *table_new(void)
{
    /* TODO: таблиця з 1024 порожніми кошиками. */
    /*return NULL;*/
    struct table *t = malloc(sizeof(struct table));

    if (t == NULL) {
        return NULL;
    }

    t->nbuckets = 1024;
    t->n = 0;

    t->buckets = calloc(t->nbuckets, sizeof(struct node *));

    if (t->buckets == NULL) {
        free(t);
        return NULL;
    }

    return t;
}

/* Подвоює кількість кошиків. 0 або -1. */
static int grow(struct table *t)
{
    /*(void)t;
       TODO
    return 0;*/

    if (t->nbuckets > SIZE_MAX / 2) {
        return -1;
    }

    size_t new_size = t->nbuckets * 2;

    struct node **new_buckets =
        calloc(new_size, sizeof(struct node *));

    if (new_buckets == NULL) {
        return -1;
    }

    for (size_t i = 0; i < t->nbuckets; i++) {
        struct node *current = t->buckets[i];

        while (current != NULL) {
            struct node *next = current->next;

            size_t index = current->hash & (new_size - 1);

            current->next = new_buckets[index];
            new_buckets[index] = current;

            current = next;
        }
    }

    free(t->buckets);

    t->buckets = new_buckets;
    t->nbuckets = new_size;

    return 0;
}

int table_add(struct table *t, const char *word, size_t len)
{
    /* TODO: знайти в кошику запис з тим самим hash, len і вмістом —
     * збільшити count; інакше створити новий вузол на початку ланцюжка,
     * а якщо слів стало більше, ніж кошиків, — grow(). */
    /*(void)fnv1a;
    (void)grow;
    (void)t;
    (void)word;
    (void)len;
    return -1;
    */

    uint64_t hash = fnv1a(word, len);
    size_t index = hash & (t->nbuckets - 1);

    struct node *current = t->buckets[index];

    while (current != NULL) {
        if (current->hash == hash &&
            current->len == len &&
            memcmp(current->e.word, word, len) == 0) {

            current->e.count++;
            return 0;
        }

        current = current->next;
    }

    struct node *new_node = malloc(sizeof(struct node));

    if (new_node == NULL) {
        return -1;
    }

    char *copy = malloc(len + 1);

    if (copy == NULL) {
        free(new_node);
        return -1;
    }

    memcpy(copy, word, len);
    copy[len] = '\0';

    new_node->e.word = copy;
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
    /* TODO: масив із t->n вказівників &node->e. */
    /*(void)t;
    return NULL;
    */

    struct entry **entries =
        malloc((t->n ? t->n : 1) * sizeof *entries);

    if (entries == NULL) {
        return NULL;
    }

    size_t k = 0;

    for (size_t i = 0; i < t->nbuckets; i++) {
        struct node *current = t->buckets[i];

        while (current != NULL) {
            entries[k] = &current->e;
            k++;
            current = current->next;
        }
    }

    return entries;
}

void table_free(struct table *t)
{
    /* TODO */
    /*(void)t;*/

    if (t == NULL) {
        return;
    }

    for (size_t i = 0; i < t->nbuckets; i++) {
        struct node *current = t->buckets[i];

        while (current != NULL) {
            struct node *next = current->next;

            free(current->e.word);
            free(current);

            current = next;
        }
    }

    free(t->buckets);
    free(t);
}
