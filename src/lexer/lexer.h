#ifndef C__LEXER_H
#define C__LEXER_H

#include <stdio.h>
#include "token.h"

typedef struct Lexer Lexer;

Lexer* lexer_create(const char *filename, FILE *file);
void lexer_destroy(Lexer *lexer);

Token lexer_next_token(Lexer *lexer);

SourceLocation lexer_current_location(const Lexer *lexer);

#endif //C__LEXER_H