#ifndef C__CODEGEN_H
#define C__CODEGEN_H

#include "../ast/ast.h"

void codegen_program(const ProgramNode* program, const char* output_file);

#endif //C__CODEGEN_H