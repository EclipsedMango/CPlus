#ifndef C__TOKEN_H
#define C__TOKEN_H

#include "../util/common.h"

typedef enum {
    // keywords
    TOK_INT,
    TOK_LONG,
    TOK_CHAR,

    TOK_FLOAT,
    TOK_DOUBLE,

    TOK_STRING_KW,
    TOK_BOOL,

    TOK_VOID,
    TOK_CONST,

    TOK_RETURN,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_BREAK,
    TOK_CONTINUE,
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

    TOK_LSQUARE,
    TOK_RSQUARE,

    TOK_ARROW,
    TOK_DOT,

    TOK_COLON,

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

const char* token_type_to_string(TokenType type);
void token_free_lexeme(Token *token);

#endif //C__TOKEN_H

