#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s PATTERN FILE\n", argv[0]);
        return 2;
    }

    FILE *file = fopen(argv[2], "r");
    if (file == NULL) {
        perror("fopen");
        return 2;
    }

    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    int found = 0;
    size_t line_number = 0;

    while ((length = getline(&line, &capacity, file)) != -1) {
        line_number++;

        if (strstr(line, argv[1]) != NULL) {
            printf("%zu:%s", line_number, line);
            found = 1;
        }
    }
    free(line);

    fclose(file);

    return found ? 0 : 1;
}
