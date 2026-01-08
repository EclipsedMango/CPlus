#include <stdlib.h>

#include "parser.h"
#include "token_utils.h"

ProgramNode* parse_program(void) {
    // allocate function list
    FunctionNode **functions = malloc(sizeof(FunctionNode*) * 16);
    int count = 0;

    // parse all functions until EOF
    while (current_token().type != TOK_EOF) {
        functions[count++] = parse_function();

        if (count >= 16) {
            fprintf(stderr, "too many functions (max 16)\n");
            exit(1);
        }
    }

    ProgramNode *program = malloc(sizeof(ProgramNode));
    program->functions = functions;
    program->function_count = count;

    return program;
}
