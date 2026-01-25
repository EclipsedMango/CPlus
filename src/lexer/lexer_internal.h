#ifndef C__LEXER_INTERNAL_H
#define C__LEXER_INTERNAL_H

#include "lexer.h"
#include "../util/vector.h"

// internal state
struct Lexer {
    FILE *file;
    const char *filename;
    int current_line;
    int current_column;
};

static void skip_line_comment(Lexer *lex);
static int skip_whitespace(Lexer *lex);
static int next_char(Lexer *lex);
static SourceLocation make_location(const Lexer *lex);

static Token lex_identifier_or_keyword(Lexer *lex, int first_char);
static Token lex_number_literal(Lexer *lex, int first_char);
static Token lex_string_literal(Lexer *lex);
static Token lex_operator_or_punct(Lexer *lex, int c);

#endif // C__LEXER_INTERNAL_H
