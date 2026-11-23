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
    (void)p;
    (void)len;
    return 0;
}

struct table *table_new(void)
{
    /* TODO: таблиця з 1024 порожніми кошиками. */
    return NULL;
}

/* Подвоює кількість кошиків. 0 або -1. */
static int grow(struct table *t)
{
    (void)t;
    /* TODO */
    return 0;
}

int table_add(struct table *t, const char *word, size_t len)
{
    /* TODO: знайти в кошику запис з тим самим hash, len і вмістом —
     * збільшити count; інакше створити новий вузол на початку ланцюжка,
     * а якщо слів стало більше, ніж кошиків, — grow(). */
    (void)fnv1a;
    (void)grow;
    (void)t;
    (void)word;
    (void)len;
    return -1;
}

size_t table_size(const struct table *t)
{
    return t->n;
}

struct entry **table_entries(const struct table *t)
{
    /* TODO: масив із t->n вказівників &node->e. */
    (void)t;
    return NULL;
}

void table_free(struct table *t)
{
    /* TODO */
    (void)t;
}
