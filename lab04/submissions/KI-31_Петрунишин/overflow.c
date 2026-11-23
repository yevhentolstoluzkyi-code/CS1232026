/* overflow.c — безпечна перевірка переповнення без невизначеної поведінки.
 *
 *   ./overflow-gcc-O2 2147483647 1
 */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* Додає a і b. Повертає -1, якщо сума не вміщується в int, без UB. */
static int add_checked(int a, int b, int *out)
{
    if ((b > 0 && a > INT_MAX - b) || (b < 0 && a < INT_MIN - b)) {
        return -1;
    }

    *out = a + b;
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
