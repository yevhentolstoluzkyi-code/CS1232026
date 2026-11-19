#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 1. Копія рядка у верхньому регістрі. */
static char *dup_upper(const char *s)
{
    /* Виправлення 1: виділяємо strlen(s) + 1 байт, щоб умістився завершальний '\0'. */
    char *r = malloc(strlen(s) + 1);
    if (r == NULL)
        return NULL;
    strcpy(r, s);
    for (char *p = r; *p; p++)
        if (*p >= 'a' && *p <= 'z')
            *p -= 32;
    return r;
}

static void scenario1(void)
{
    char *s = dup_upper("wordfreq");
    if (s != NULL) {
        printf("%s\n", s);
        free(s);
    }
}

/* 2. Однозв'язний список і його звільнення. */
struct node { int value; struct node *next; };

static void scenario2(void)
{
    struct node *head = NULL;
    for (int i = 0; i < 5; i++) {
        struct node *n = malloc(sizeof *n);
        if (n == NULL)
            break;
        n->value = i;
        n->next = head;
        head = n;
    }
    int sum = 0;
    for (struct node *p = head; p; p = p->next)
        sum += p->value;
    /* Виправлення 2: зберігаємо вказівник next до виклику free(p), щоб уникнути use-after-free. */
    for (struct node *p = head; p != NULL; ) {
        struct node *next = p->next;
        free(p);
        p = next;
    }
    printf("sum = %d\n", sum);
}

/* 3. Гістограма довжин слів. Слова довші за MAXLEN рахуються як MAXLEN. */
#define MAXLEN 8

static void scenario3(void)
{
    const char *words[] = { "a", "system", "software", "is", "wordfreq" };
    /* Виправлення 3: масив на MAXLEN + 1 елементів і вивід до <= MAXLEN, бо індекс len == MAXLEN валідний. */
    int hist[MAXLEN + 1] = { 0 };
    int canary = 12345;
    for (size_t i = 0; i < sizeof words / sizeof words[0]; i++) {
        size_t len = strlen(words[i]);
        if (len > MAXLEN)
            len = MAXLEN;
        hist[len]++;
    }
    for (int i = 1; i <= MAXLEN; i++)
        printf("%d:%d ", i, hist[i]);
    printf("\ncanary = %d\n", canary);
}

/* 4. Середня довжина слова. */
static void scenario4(int argc)
{
    const char *words[] = { "kernel", "module", "driver" };
    /* Виправлення 4: ініціалізуємо total нулем перед накопиченням суми. */
    int total = 0;
    for (int i = 0; i < 3; i++)
        total += (int)strlen(words[i]);
    if (argc > 100)
        total = 0;
    if (total > 10)
        printf("average = %d\n", total / 3);
    else
        printf("short words\n");
}

/* 5. Номер кошика для рядка (як у хеш-таблиці). */
#define NBUCKETS 16

static void scenario5(void)
{
    const char *s = "the quick brown fox jumps over the lazy dog";
    /* Виправлення 5: використовуємо unsigned int для хешу, бо переповнення signed int є UB. */
    unsigned int h = 0;
    for (const char *p = s; *p; p++)
        h = h * 31u + (unsigned char)*p;
    unsigned idx = h % NBUCKETS;
    printf("bucket = %u\n", idx);
}

/* 6. Рядок звіту. */
static char *make_line(const char *word, int count)
{
    char *line = malloc(64);
    if (line != NULL)
        snprintf(line, 64, "%d %s", count, word);
    return line;
}

static void scenario6(void)
{
    for (int i = 0; i < 3; i++) {
        char *line = make_line("kernel", i);
        if (line != NULL) {
            printf("%s\n", line);
            /* Виправлення 6: звільняємо виділений у make_line буфер на кожній ітерації циклу. */
            free(line);
        }
    }
}

int main(int argc, char *argv[])
{
    int n = argc > 1 ? atoi(argv[1]) : 0;
    switch (n) {
    case 1: scenario1(); break;
    case 2: scenario2(); break;
    case 3: scenario3(); break;
    case 4: scenario4(argc); break;
    case 5: scenario5(); break;
    case 6: scenario6(); break;
    default:
        fprintf(stderr, "usage: buggy N   (N = 1..6)\n");
        return 2;
    }
    return 0;
}
