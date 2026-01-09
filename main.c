#include <stdio.h>

#include "parser/parser.h"
#include "parser/token_utils.h"

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

    const ProgramNode *program = parse_program();
    printf("Functions parsed: %d\n", program->function_count);

    for (int i = 0; i < program->function_count; i++) {
        const FunctionNode *func = program->functions[i];
        printf("\nFunction: %s\n", func->name);
        printf("  Return type: %d\n", func->return_type);
        printf("  Parameters: %d\n", func->param_count);

        for (int j = 0; j < func->param_count; j++) {
            printf("    Param %d: type=%d, name=%s\n", j, func->params[j].type, func->params[j].name);
        }

        printf("  Body: %s\n", func->body->kind == STMT_COMPOUND ? "compound statement" : "other");
        if (func->body->kind == STMT_COMPOUND) {
            printf("  Body statements: %d\n", func->body->compound.count);
        }
    }

    fclose(f);
    return 0;
}
