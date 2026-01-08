#include <stdio.h>
#include "parser/parser.h"

int main(const int argc, char *argv[]) {
    if (argc < 2) {
        printf("%s takes a file as argument\n", argv[0]);
        return 1;
    }

    if (argc > 2) {
        printf("%s has too many arguments\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) {
        printf("%s does not exist\n", argv[1]);
        return 1;
    }

    printf("compiling %s\n", argv[1]);

    set_current_file(f);

    advance();
    const int result = parse_program();

    printf("program returns %d\n", result);

    fclose(f);
    return 0;
}
