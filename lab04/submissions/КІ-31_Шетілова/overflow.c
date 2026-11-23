/* overflow.c — перевірка переповнення БЕЗ невизначеної поведінки. */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* Спосіб 1: перевірити ДО додавання. Переповнення int — UB, тому
 * порівнюємо з межею, не виконуючи самого додавання. */
static int add_checked(int a, int b, int *out)
{
    if ((b > 0 && a > INT_MAX - b) || (b < 0 && a < INT_MIN - b))
        return -1;
    *out = a + b;
    return 0;
}

/* Спосіб 2: вбудована функція компілятора, яка повідомляє про переповнення. */
static int add_checked_builtin(int a, int b, int *out)
{
    return __builtin_add_overflow(a, b, out) ? -1 : 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: overflow A B\n");
        return 2;
    }
    int a = atoi(argv[1]), b = atoi(argv[2]);
    int r, r2;
    int e1 = add_checked(a, b, &r);
    int e2 = add_checked_builtin(a, b, &r2);
    if (e1 != 0 || e2 != 0)
        puts("overflow detected");
    else
        printf("sum = %d\n", r);
    return 0;
}
