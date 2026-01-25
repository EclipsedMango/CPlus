#ifndef C__PARSER_H
#define C__PARSER_H

#include "../ast/ast.h"
#include "../lexer/lexer.h"
#include "../util/diagnostics.h"

typedef struct Parser Parser;

Parser* parser_create(Lexer *lexer, DiagnosticEngine *diagnostics);
void parser_destroy(Parser *parser);

ProgramNode* parser_parse_program(Parser *parser);

#endif //C__PARSER_H