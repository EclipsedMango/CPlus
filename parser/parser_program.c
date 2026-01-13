#include <stdlib.h>

#include "parser.h"
#include "token_utils.h"

ProgramNode* parse_program(void) {
    Vector functions = create_vector(16, sizeof(FunctionNode*));

    // parse all functions until EOF
    while (current_token().type != TOK_EOF) {
        FunctionNode *fn = parse_function();
        vector_push(&functions, &fn);
    }

    ProgramNode *program = malloc(sizeof(ProgramNode));
    program->functions = functions.elements;
    program->function_count = functions.length;

    return program;
}
