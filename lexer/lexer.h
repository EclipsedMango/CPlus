#ifndef C__LEXER_H
#define C__LEXER_H
#include "../common.h"
#include <stdio.h>

typedef enum {
    // keywords
    TOK_INT,
    TOK_LONG,

    TOK_FLOAT,
    TOK_DOUBLE,

    TOK_STRING_KW,
    TOK_BOOL,

    TOK_RETURN,
    TOK_IF,
    TOK_ELSE,
    TOK_ASM,

    // identifiers & literals
    TOK_IDENTIFIER,
    TOK_STRING_LITERAL,
    TOK_NUMBER,
    TOK_DECI_NUMBER,

    // operators
    TOK_PLUS,
    TOK_SUBTRACT,

    TOK_DIVIDE,
    TOK_MODULO,

    TOK_ASTERISK,
    TOK_AMPERSAND,

    TOK_ASSIGN,
    TOK_EQUAL_EQUAL,

    TOK_GREATER,
    TOK_LESS,

    TOK_GREATER_EQUALS,
    TOK_LESS_EQUALS,

    // logical operators
    TOK_AND,
    TOK_OR,
    TOK_NOT,
    TOK_NOT_EQUAL,

    // punctuation / delimiters
    TOK_LPAREN,
    TOK_RPAREN,

    TOK_LBRACE,
    TOK_RBRACE,

    TOK_ARROW,
    TOK_DOT,

    TOK_SEMI,
    TOK_COMMA,

    // special
    TOK_EOF,
    TOK_INVALID,
} TokenType;

typedef struct Token {
    TokenType type;
    char *lexeme;  // for identifiers and string literals
    SourceLocation location;
} Token;

Token next_token(FILE *f);
void lexer_init(const char *filename);
const char* token_type_to_string(TokenType type);


#endif //C__LEXER_H