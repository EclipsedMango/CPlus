#include <stdio.h>

#include "codegen/codegen.h"
#include "parser/parser.h"
#include "parser/token_utils.h"
#include "semantic/semantic.h"

int main(const int argc, char *argv[]) {
    if (argc < 2) {
        printf("%s takes a file as argument\n", argv[0]);
        return 1;
    }

    if (argc > 2) {
        printf("%s has too many arguments\n", argv[0]);
        return 1;
    }

    char *filename = argv[1];
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("%s does not exist\n", filename);
        return 1;
    }

    printf("compiling %s\n", filename);

    lexer_init(filename);
    set_current_file(f);

    ProgramNode *program = parse_program();
    printf("parsing complete\n");

    analyze_program(program);
    printf("semantic analysis complete\n");

    codegen_program(program, "output.o");
    printf("codegen complete\n");

    fclose(f);
    return 0;
}
