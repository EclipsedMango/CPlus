#ifndef C__LEXER_H
#define C__LEXER_H
#include <stdio.h>

typedef enum {
    // keywords
    TOK_INT,
    TOK_LONG,

    TOK_FLOAT,
    TOK_DOUBLE,

    TOK_STRING_KW,
    TOK_RETURN,

    // identifiers & literals
    TOK_IDENTIFIER,
    TOK_STRING_LITERAL,
    TOK_NUMBER,
    TOK_DECI_NUMBER,

    // operators
    TOK_PLUS,
    TOK_SUBTRACT,

    TOK_MULTIPLY,
    TOK_DIVIDE,
    TOK_MODULO,

    TOK_ASSIGN,
    TOK_EQUAL_EQUAL,

    TOK_GREATER,
    TOK_LESS,

    TOK_GREATER_EQUALS,
    TOK_LESS_EQUALS,

    // punctuation / delimiters
    TOK_LPAREN,
    TOK_RPAREN,

    TOK_LBRACE,
    TOK_RBRACE,

    TOK_SEMI,
    TOK_COMMA,

    // special
    TOK_EOF,
    TOK_INVALID,
} TokenType;

typedef struct Token {
    TokenType type;
    char *lexeme;  // for identifiers and string literals
} Token;

Token next_token(FILE *f);


#endif //C__LEXER_H