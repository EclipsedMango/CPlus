#include "parser.h"

FunctionNode* parse_program(void) {
    // for now, program = single function
    return parse_function();
}