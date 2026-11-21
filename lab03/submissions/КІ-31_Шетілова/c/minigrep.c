#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Використання: %s ШАБЛОН ФАЙЛ\n", argv[0]);
        return 2;
    }

    FILE *f = fopen(argv[2], "r");
    if (f == NULL) {
        perror(argv[2]);
        return 2;
    }

    char *line = NULL;
    size_t cap = 0;
    ssize_t len;
    unsigned long no = 0;
    int found = 0;

    while ((len = getline(&line, &cap, f)) != -1) {
        no++;
        if (strstr(line, argv[1]) != NULL) {
            /* getline лишає '\n' у кінці; якщо в останньому рядку його немає, додаємо */
            printf("%lu:%s%s", no, line, line[len - 1] == '\n' ? "" : "\n");
            found = 1;
        }
    }

    free(line);
    fclose(f);
    return found ? 0 : 1;
}
