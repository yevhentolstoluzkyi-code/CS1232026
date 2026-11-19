#include "wordfreq.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FNV_OFFSET 14695981039346656037ULL
#define FNV_PRIME  1099511628211ULL

struct node {
    struct entry e;
    uint64_t     hash;
    size_t       len;
    struct node *next;
};

struct table {
    struct node **buckets;
    size_t        nbuckets;
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
    struct table *t = malloc(sizeof *t);
    if (t == NULL)
        return NULL;
    t->nbuckets = 1024;
    t->n = 0;
    t->buckets = calloc(t->nbuckets, sizeof *t->buckets);
    if (t->buckets == NULL) {
        free(t);
        return NULL;
    }
    return t;
}

static int grow(struct table *t)
{
    size_t new_nbuckets = t->nbuckets * 2;
    struct node **new_buckets = calloc(new_nbuckets, sizeof *new_buckets);
    if (new_buckets == NULL)
        return -1;
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
    uint64_t h = fnv1a(word, len);
    size_t idx = (size_t)(h & (t->nbuckets - 1));
    for (struct node *cur = t->buckets[idx]; cur != NULL; cur = cur->next) {
        if (cur->hash == h && cur->len == len && memcmp(cur->e.word, word, len) == 0) {
            cur->e.count++;
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
    node->hash = h;
    node->len = len;
    node->next = t->buckets[idx];
    t->buckets[idx] = node;
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
    size_t count = t->n;
    struct entry **arr = malloc((count > 0 ? count : 1) * sizeof *arr);
    if (arr == NULL)
        return NULL;
    size_t k = 0;
    for (size_t i = 0; i < t->nbuckets; i++)
        for (struct node *cur = t->buckets[i]; cur != NULL; cur = cur->next)
            arr[k++] = &cur->e;
    return arr;
}

void table_free(struct table *t)
{
    if (t == NULL)
        return;
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
