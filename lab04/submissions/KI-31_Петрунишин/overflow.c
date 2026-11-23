/* overflow.c — «захист» від переповнення, який компілятор має право видалити.
 *
 *   ./overflow-gcc-O0 2147483647 1
 *
 * Зберіть чотири варіанти (gcc і clang, -O0 і -O2) і порівняйте вивід.
 */
#include <stdio.h>
#include <stdlib.h>

/* Додає a і b. Повертає -1, якщо сума не вміщується в int. */
static int add_checked(int a, int b, int *out)
{
    int s = a + b;
    if (b > 0 && s < a)         /* «якщо додали додатне, а стало менше» */
        return -1;
    *out = s;
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: overflow A B\n");
        return 2;
    }
    int r;
    if (add_checked(atoi(argv[1]), atoi(argv[2]), &r) != 0)
        puts("overflow detected");
    else
        printf("sum = %d\n", r);
    return 0;
}
