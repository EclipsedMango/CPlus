#ifndef C__TOKEN_UTILS_H
#define C__TOKEN_UTILS_H

#include "../lexer/lexer.h"

// how many tokens of lookahead we keep
#define TOKEN_BUFFER_SIZE 5

// initialization
void set_current_file(FILE *f);
FILE *get_current_file(void);

// token access
Token current_token(void);
Token peek_token(int n);

// token movement
void advance(void);
void expect(TokenType type);

#endif //C__TOKEN_UTILS_H