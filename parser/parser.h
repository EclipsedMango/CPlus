#ifndef C__PARSER_H
#define C__PARSER_H

#include "../lexer/lexer.h"

FILE* get_current_file(void);
void set_current_file(FILE* f);

void advance(void);
void expect(TokenType type);
int parse_return(void);
int parse_program(void);

#endif //C__PARSER_H