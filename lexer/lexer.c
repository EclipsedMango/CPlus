#include "../lexer/lexer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static int current_line = 1;
static int current_column = 1;
static const char *current_filename = nullptr;

// keywords
static char *KW_INT    = "int";
static char *KW_LONG   = "long";
static char *KW_FLOAT  = "float";
static char *KW_DOUBLE = "double";
static char *KW_STRING = "string";
static char *KW_RETURN = "return";
static char *KW_IF     = "if";

// operators
static char *OP_ASSIGN          = "=";
static char *OP_EQUAL_EQUAL     = "==";
static char *OP_GREATER         = ">";
static char *OP_GREATER_EQUALS  = ">=";
static char *OP_LESS            = "<";
static char *OP_LESS_EQUALS     = "<=";
static char *OP_PLUS            = "+";
static char *OP_MINUS           = "-";
static char *OP_MULTIPLY        = "*";
static char *OP_DIVIDE          = "/";
static char *OP_MODULO          = "%";

// punctuation
static char *P_LPAREN = "(";
static char *P_RPAREN = ")";
static char *P_LBRACE = "{";
static char *P_RBRACE = "}";
static char *P_COMMA  = ",";
static char *P_SEMI   = ";";

void lexer_init(const char *filename) {
    current_line = 1;
    current_column = 1;
    current_filename = filename;
}

static int skip_whitespace(FILE *f) {
    int c;
    do {
        c = fgetc(f);
        if (c == '\n') {
            current_line++;
            current_column = 1;
        } else if (c == EOF) {
            current_column++;
        }
    } while (c == ' ' || c == '\n' || c == '\t' || c == '\r');
    return c;
}

static int next_char(FILE *f) {
    const int c = fgetc(f);
    if (c == '\n') {
        current_line++;
        current_column = 1;
    } else if (c != EOF) {
        current_column++;
    }
    return c;
}

static SourceLocation make_location(void) {
    SourceLocation loc;
    loc.line = current_line;
    loc.column = current_column;
    loc.filename = current_filename;
    return loc;
}

static Token lex_identifier_or_keyword(FILE *f, const int first) {
    Vector buffer = create_vector(8, sizeof(char));

    char ch = (char)first;
    vector_push(&buffer, &ch);

    int c;
    while ((c = next_char(f)) != EOF && (isalnum(c) || c == '_')) {
        ch = (char)c;
        vector_push(&buffer, &ch);
    }

    ch = '\0';
    vector_push(&buffer, &ch);

    ungetc(c, f);

    const char *lexeme = buffer.elements;
    const SourceLocation loc = make_location();

    Token tok = (Token){ .type = TOK_IDENTIFIER, .lexeme = strdup(lexeme), .location = loc };
    if (strcmp(lexeme, KW_INT) == 0)    tok = (Token){ .type = TOK_INT, .lexeme = KW_INT, .location = loc };
    if (strcmp(lexeme, KW_LONG) == 0)   tok = (Token){ .type = TOK_LONG, .lexeme = KW_LONG, .location = loc };
    if (strcmp(lexeme, KW_FLOAT) == 0)  tok = (Token){ .type = TOK_FLOAT, .lexeme = KW_FLOAT, .location = loc };
    if (strcmp(lexeme, KW_DOUBLE) == 0) tok = (Token){ .type = TOK_DOUBLE, .lexeme = KW_DOUBLE, .location = loc };
    if (strcmp(lexeme, KW_STRING) == 0) tok = (Token){ .type = TOK_STRING_KW, .lexeme = KW_STRING, .location = loc };
    if (strcmp(lexeme, KW_RETURN) == 0) tok = (Token){ .type = TOK_RETURN, .lexeme = KW_RETURN, .location = loc };
    if (strcmp(lexeme, KW_IF) == 0)     tok = (Token){ .type = TOK_IF, .lexeme = KW_IF, .location = loc };

    vector_destroy(&buffer);
    return tok;
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
        c = next_char(f);
    }

    buf[i] = '\0';
    ungetc(c, f);

    return (Token){
        .type = hasDecimal ? TOK_DECI_NUMBER : TOK_NUMBER,
        .lexeme = strdup(buf),
        .location = make_location()
    };
}

static Token lex_string_literal(FILE *f) {
    char buf[256];
    int i = 0;
    int c;

    while ((c = next_char(f)) != EOF) {
        if (c == '"') {
            buf[i] = '\0';
            return (Token){ .type = TOK_STRING_LITERAL, .lexeme = strdup(buf), .location = make_location() };
        }

        if (c == '\n') {
            fprintf(stderr, "unterminated string literal\n");
            exit(1);
        }

        if (c == '\\') {
            c = next_char(f);
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
    const SourceLocation loc = make_location();

    switch (c) {
        // operators with possible lookahead
        case '=':
            next = next_char(f);
            if (next == '=') return (Token){TOK_EQUAL_EQUAL, OP_EQUAL_EQUAL, loc };
            ungetc(next, f);
            return (Token){TOK_ASSIGN, OP_ASSIGN, loc };

        case '>':
            next = next_char(f);
            if (next == '=') return (Token){TOK_GREATER_EQUALS, OP_GREATER_EQUALS, loc };
            ungetc(next, f);
            return (Token){TOK_GREATER, OP_GREATER, loc };

        case '<':
            next = next_char(f);
            if (next == '=') return (Token){TOK_LESS_EQUALS, OP_LESS_EQUALS, loc };
            ungetc(next, f);
            return (Token){TOK_LESS, OP_LESS, loc};

            // single-char operators
        case '+': return (Token){.type = TOK_PLUS, OP_PLUS, loc};
        case '-': return (Token){.type = TOK_SUBTRACT, OP_MINUS, loc};
        case '*': return (Token){.type = TOK_MULTIPLY, OP_MULTIPLY, loc};
        case '/': return (Token){.type = TOK_DIVIDE, OP_DIVIDE, loc};
        case '%': return (Token){.type = TOK_MODULO, OP_MODULO, loc};

            // punctuation / delimiters
        case '(': return (Token){TOK_LPAREN, P_LPAREN, loc};
        case ')': return (Token){TOK_RPAREN, P_RPAREN, loc};
        case '{': return (Token){TOK_LBRACE, P_LBRACE, loc};
        case '}': return (Token){TOK_RBRACE, P_RBRACE, loc};
        case ',': return (Token){TOK_COMMA, P_COMMA, loc};
        case ';': return (Token){TOK_SEMI, P_SEMI, loc};

        default:
            return (Token){TOK_EOF,nullptr, loc}; // should never happen
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