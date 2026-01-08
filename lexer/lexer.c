#include "../lexer/lexer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static int skip_whitespace(FILE *f) {
    int c;
    do {
        c = fgetc(f);
    } while (c == ' ' || c == '\n' || c == '\t' || c == '\r');
    return c;
}

static Token lex_identifier_or_keyword(FILE *f, const int first) {
    char buf[32];
    int i = 0;

    buf[i++] = first;

    int c;
    while ((c = fgetc(f)) != EOF && (isalnum(c) || c == '_')) {
        if (i < 31) buf[i++] = c;
    }

    buf[i] = '\0';
    ungetc(c, f);

    if (strcmp(buf, "int") == 0)    return (Token){ .type = TOK_INT, .lexeme = strdup("int") };
    if (strcmp(buf, "long") == 0)   return (Token){ .type = TOK_LONG, .lexeme = strdup("long") };
    if (strcmp(buf, "float") == 0)  return (Token){ .type = TOK_FLOAT, .lexeme = strdup("float") };
    if (strcmp(buf, "double") == 0) return (Token){ .type = TOK_DOUBLE, .lexeme = strdup("double") };
    if (strcmp(buf, "string") == 0) return (Token){ .type = TOK_STRING_KW, .lexeme = strdup("string") };
    if (strcmp(buf, "return") == 0) return (Token){ .type = TOK_RETURN, .lexeme = strdup("return") };

    return (Token){
        .type = TOK_IDENTIFIER,
        .lexeme = strdup(buf)
    };
}

static Token lex_number_literal(FILE *f, int c) {
    char buf[64]; // for literal text
    int i = 0;
    bool hasDecimal = false;

    // read digits
    while (c != EOF && (isdigit(c) || c == '.')) {
        if (c == '.') {
            if (hasDecimal) {
                fprintf(stderr,"invalid number\n");
                exit(1);
            }

            hasDecimal = true;
        }

        buf[i++] = c;
        c = fgetc(f);
    }

    buf[i] = '\0';
    ungetc(c, f);

    return (Token){
        .type = hasDecimal ? TOK_DECI_NUMBER : TOK_NUMBER,
        .lexeme = strdup(buf)
    };
}

static Token lex_string_literal(FILE *f) {
    char buf[256];
    int i = 0;
    int c;

    while ((c = fgetc(f)) != EOF) {
        if (c == '"') {
            buf[i] = '\0';
            return (Token){ .type = TOK_STRING_LITERAL, .lexeme = strdup(buf) };
        }

        if (c == '\n') {
            fprintf(stderr, "unterminated string literal\n");
            exit(1);
        }

        if (c == '\\') {
            c = fgetc(f);
            switch (c) {
                case 'n':  buf[i++] = '\n'; break;
                case 't':  buf[i++] = '\t'; break;
                case '"':  buf[i++] = '"';  break;
                case '\\': buf[i++] = '\\'; break;
                default:
                    fprintf(stderr, "unknown escape: \\%c\n", c);
                    exit(1);
            }
        } else {
            buf[i++] = c;
        }

        if (i >= 255) {
            fprintf(stderr, "string literal too long\n");
            exit(1);
        }
    }

    fprintf(stderr, "unterminated string literal\n");
    exit(1);
}

static Token lex_operator_or_punct(FILE *f, const int c) {
    int next;

    switch (c) {
        // operators with possible lookahead
        case '=':
            next = fgetc(f);
            if (next == '=') return (Token){TOK_EQUAL_EQUAL, .lexeme = strdup("==")};
            ungetc(next, f);
            return (Token){TOK_ASSIGN, .lexeme = strdup("=")};

        case '>':
            next = fgetc(f);
            if (next == '=') return (Token){TOK_GREATER_EQUALS, .lexeme = strdup(">=")};
            ungetc(next, f);
            return (Token){TOK_GREATER, .lexeme = strdup(">")};

        case '<':
            next = fgetc(f);
            if (next == '=') return (Token){TOK_LESS_EQUALS, .lexeme = strdup("<=")};
            ungetc(next, f);
            return (Token){TOK_LESS, .lexeme = strdup("<")};

            // single-char operators
        case '+': return (Token){.type = TOK_PLUS, .lexeme = strdup("+")};
        case '-': return (Token){.type = TOK_SUBTRACT, .lexeme = strdup("-")};
        case '*': return (Token){.type = TOK_MULTIPLY, .lexeme = strdup("*")};
        case '/': return (Token){.type = TOK_DIVIDE, .lexeme = strdup("/")};
        case '%': return (Token){.type = TOK_MODULO, .lexeme = strdup("%")};

            // punctuation / delimiters
        case '(': return (Token){TOK_LPAREN, .lexeme = strdup("(")};
        case ')': return (Token){TOK_RPAREN, .lexeme = strdup(")")};
        case '{': return (Token){TOK_LBRACE, .lexeme = strdup("{")};
        case '}': return (Token){TOK_RBRACE, .lexeme = strdup("}")};
        case ',': return (Token){TOK_COMMA, .lexeme = strdup(",")};
        case ';': return (Token){TOK_SEMI, .lexeme = strdup(";")};

        default:
            return (Token){TOK_EOF}; // should never happen
    }
}

Token next_token(FILE *f) {
    const int c = skip_whitespace(f);

    if (c == EOF) return (Token){TOK_EOF, nullptr};
    if (c == '"') return lex_string_literal(f);

    if (isdigit(c)) return lex_number_literal(f, c);
    if (isalpha(c)) return lex_identifier_or_keyword(f, c);

    const Token t = lex_operator_or_punct(f, c);
    if (t.type != TOK_EOF) return t;

    fprintf(stderr, "unexpected char: %c\n", c);
    exit(1);
}