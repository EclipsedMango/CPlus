#include "../lexer/lexer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static int current_line = 1;
static int current_column = 1;
static const char *current_filename = NULL;

// keywords
static char *KW_INT      = "int";
static char *KW_LONG     = "long";
static char *KW_CHAR     = "char";
static char *KW_FLOAT    = "float";
static char *KW_DOUBLE   = "double";
static char *KW_STRING   = "string";
static char *KW_BOOL     = "bool";
static char *KW_VOID     = "void";
static char *KW_RETURN   = "return";
static char *KW_IF       = "if";
static char *KW_ELSE     = "else";
static char *KW_WHILE    = "while";
static char *KW_FOR      = "for";
static char *KW_BREAK    = "break";
static char *KW_CONTINUE = "continue";
static char *KW_ASM      = "asm";

// operators
static char *OP_ASSIGN          = "=";
static char *OP_EQUAL_EQUAL     = "==";
static char *OP_GREATER         = ">";
static char *OP_GREATER_EQUALS  = ">=";
static char *OP_LESS            = "<";
static char *OP_LESS_EQUALS     = "<=";
static char *OP_PLUS            = "+";
static char *OP_MINUS           = "-";
static char *OP_ASTERISK        = "*";
static char *OP_AMPERSAND       = "&";
static char *OP_DIVIDE          = "/";
static char *OP_MODULO          = "%";
static char *OP_AND             = "&&";
static char *OP_OR              = "||";
static char *OP_NOT             = "!";
static char *OP_NOT_EQUAL       = "!=";

// punctuation
static char *P_LPAREN = "(";
static char *P_RPAREN = ")";
static char *P_LBRACE = "{";
static char *P_RBRACE = "}";
static char *P_COMMA  = ",";
static char *P_COLON  = ":";
static char *P_SEMI   = ";";

// identifiers & literals
static char *IDENTIFIER     = "identifier";
static char *STRING_LITERAL = "string literal";
static char *NUMBER         = "number";
static char *DECI_NUMBER    = "decimal number";

// special
static char *END_OF_FILE = "end of file";
static char *UNKNOWN     = "unknown token";

void lexer_init(const char *filename) {
    current_line = 1;
    current_column = 1;
    current_filename = filename;
}

static void skip_line_comment(FILE *f) {
    int c;
    while ((c = fgetc(f)) != EOF && c != '\n') {
        current_column++;
    }
    if (c == '\n') {
        current_line++;
        current_column = 1;
    }
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
    if (strcmp(lexeme, KW_INT) == 0)      tok = (Token){ .type = TOK_INT, .lexeme = KW_INT, .location = loc };
    if (strcmp(lexeme, KW_LONG) == 0)     tok = (Token){ .type = TOK_LONG, .lexeme = KW_LONG, .location = loc };
    if (strcmp(lexeme, KW_CHAR) == 0)     tok = (Token){ .type = TOK_CHAR, .lexeme = KW_CHAR, .location = loc };
    if (strcmp(lexeme, KW_FLOAT) == 0)    tok = (Token){ .type = TOK_FLOAT, .lexeme = KW_FLOAT, .location = loc };
    if (strcmp(lexeme, KW_DOUBLE) == 0)   tok = (Token){ .type = TOK_DOUBLE, .lexeme = KW_DOUBLE, .location = loc };
    if (strcmp(lexeme, KW_STRING) == 0)   tok = (Token){ .type = TOK_STRING_KW, .lexeme = KW_STRING, .location = loc };
    if (strcmp(lexeme, KW_BOOL) == 0)     tok = (Token){ .type = TOK_BOOL, .lexeme = KW_BOOL, .location = loc };
    if (strcmp(lexeme, KW_VOID) == 0)     tok = (Token){ .type = TOK_VOID, .lexeme = KW_VOID, .location = loc };
    if (strcmp(lexeme, KW_RETURN) == 0)   tok = (Token){ .type = TOK_RETURN, .lexeme = KW_RETURN, .location = loc };
    if (strcmp(lexeme, KW_IF) == 0)       tok = (Token){ .type = TOK_IF, .lexeme = KW_IF, .location = loc };
    if (strcmp(lexeme, KW_ELSE) == 0)     tok = (Token){ .type = TOK_ELSE, .lexeme = KW_ELSE, .location = loc };
    if (strcmp(lexeme, KW_WHILE) == 0)    tok = (Token){ .type = TOK_WHILE, .lexeme = KW_WHILE, .location = loc };
    if (strcmp(lexeme, KW_FOR) == 0)      tok = (Token){ .type = TOK_FOR, .lexeme = KW_FOR, .location = loc };
    if (strcmp(lexeme, KW_BREAK) == 0)    tok = (Token){ .type = TOK_BREAK, .lexeme = KW_BREAK, .location = loc };
    if (strcmp(lexeme, KW_CONTINUE) == 0) tok = (Token){ .type = TOK_CONTINUE, .lexeme = KW_CONTINUE, .location = loc };
    if (strcmp(lexeme, KW_ASM) == 0)      tok = (Token){ .type = TOK_ASM, .lexeme = KW_ASM, .location = loc };

    vector_destroy(&buffer);
    return tok;
}

static Token lex_number_literal(FILE *f, int c) {
    Vector buf = create_vector(8, sizeof(char));
    int hasDecimal = 0;
    char ch = (char)c;

    // read digits
    while (c != EOF && (isdigit(c) || c == '.')) {
        if (c == '.') {
            if (hasDecimal) {
                fprintf(stderr,"invalid number\n");
                exit(1);
            }

            hasDecimal = 1;
        }

        vector_push(&buf, &ch);
        c = next_char(f);
        ch = (char)c;
    }

    ch = '\0';
    vector_push(&buf, &ch);
    ungetc(c, f);

    const char *lexeme = buf.elements;
    const SourceLocation loc = make_location();
    const Token tok = {
        .type = hasDecimal ? TOK_DECI_NUMBER : TOK_NUMBER,
        .lexeme = strdup(lexeme),
        .location = loc
    };

    vector_destroy(&buf);
    return tok;
}

static Token lex_string_literal(FILE *f) {
    Vector buf = create_vector(16, sizeof(char));
    int c;

    while ((c = next_char(f)) != EOF) {
        char ch;
        if (c == '"') {
            ch = '\0';
            vector_push(&buf, &ch);
            const Token tok = { TOK_STRING_LITERAL, strdup(buf.elements), make_location() };
            vector_destroy(&buf);
            return tok;
        }

        if (c == '\n') {
            report_error(make_location(), "Unterminated string literal (newlines not allowed)");
        }

        if (c == '\\') {
            c = next_char(f);
            switch (c) {
                case 'n':  ch = '\n'; break;
                case '0':  ch = '\0'; break;
                case 't':  ch = '\t'; break;
                case '"':  ch = '"';  break;
                case '\\': ch = '\\'; break;
                case '\n':
                    // line continuation: backslash followed by newline
                    // skip the newline and continue parsing the string
                    continue;
                default: {
                    fprintf(stderr, "unknown escape: \\%c\n", c);
                    exit(1);
                }
            }
        } else {
            ch = (char)c;
        }

        vector_push(&buf, &ch);
    }

    report_error(make_location(), "Unterminated string literal at EOF");
    exit(1);
}

static Token lex_operator_or_punct(FILE *f, const int c) {
    int next;
    const SourceLocation loc = make_location();

    switch (c) {
        // operators with possible lookahead
        case '=': {
            next = next_char(f);
            if (next == '=') return (Token){TOK_EQUAL_EQUAL, OP_EQUAL_EQUAL, loc };
            ungetc(next, f);
            return (Token){TOK_ASSIGN, OP_ASSIGN, loc };
        }
        case '>': {
            next = next_char(f);
            if (next == '=') return (Token){TOK_GREATER_EQUALS, OP_GREATER_EQUALS, loc };
            ungetc(next, f);
            return (Token){TOK_GREATER, OP_GREATER, loc };
        }
        case '<': {
            next = next_char(f);
            if (next == '=') return (Token){TOK_LESS_EQUALS, OP_LESS_EQUALS, loc };
            ungetc(next, f);
            return (Token){TOK_LESS, OP_LESS, loc};
        }
        case '&': {
            next = next_char(f);
            if (next == '&') return (Token){TOK_AND, "&&", loc };
            ungetc(next, f);
            return (Token){TOK_AMPERSAND, OP_AMPERSAND, loc};
        }
        case '|': {
            next = next_char(f);
            if (next == '|') return (Token){TOK_OR, "||", loc };
            ungetc(next, f);
            // maybe add bitwise OR later?
            report_error(loc, "Unexpected character '|'. Did you mean '||'?");
            exit(1);
        }
        case '!': {
            next = next_char(f);
            if (next == '=') return (Token){TOK_NOT_EQUAL, "!=", loc };
            ungetc(next, f);
            return (Token){TOK_NOT, "!", loc };
        }
        case '/': {
            next = next_char(f);
            if (next == '/') {
                skip_line_comment(f);
                return (Token){TOK_EOF, NULL, loc}; // signal to get next token
            }
            ungetc(next, f);
            return (Token){.type = TOK_DIVIDE, OP_DIVIDE, loc};
        }

        // single-char operators
        case '+': return (Token){.type = TOK_PLUS, OP_PLUS, loc};
        case '-': return (Token){.type = TOK_SUBTRACT, OP_MINUS, loc};
        case '*': return (Token){.type = TOK_ASTERISK, OP_ASTERISK, loc};
        case '%': return (Token){.type = TOK_MODULO, OP_MODULO, loc};

        // punctuation / delimiters
        case '(': return (Token){TOK_LPAREN, P_LPAREN, loc};
        case ')': return (Token){TOK_RPAREN, P_RPAREN, loc};
        case '{': return (Token){TOK_LBRACE, P_LBRACE, loc};
        case '}': return (Token){TOK_RBRACE, P_RBRACE, loc};
        case ':': return (Token){TOK_COLON, P_COLON, loc};
        case ',': return (Token){TOK_COMMA, P_COMMA, loc};
        case ';': return (Token){TOK_SEMI, P_SEMI, loc};

        default: return (Token){TOK_EOF,NULL, loc}; // should never happen
    }
}

Token next_token(FILE *f) {
    while (1) {
        const int c = skip_whitespace(f);

        if (c == EOF) return (Token){TOK_EOF, NULL};
        if (c == '"') return lex_string_literal(f);

        if (isdigit(c)) return lex_number_literal(f, c);
        if (isalpha(c) || c == '_') return lex_identifier_or_keyword(f, c);

        const Token t = lex_operator_or_punct(f, c);
        if (t.type != TOK_EOF) return t;

        // if we got TOK_EOF from a comment, loop again to get the next real token
    }
}

const char* token_type_to_string(const TokenType type) {
    switch (type) {
        case TOK_INT: return KW_INT;
        case TOK_LONG: return KW_LONG;
        case TOK_CHAR: return KW_CHAR;
        case TOK_FLOAT: return KW_FLOAT;
        case TOK_DOUBLE: return KW_DOUBLE;
        case TOK_STRING_KW: return KW_STRING;
        case TOK_BOOL: return KW_BOOL;
        case TOK_VOID: return KW_VOID;
        case TOK_RETURN: return KW_RETURN;
        case TOK_IF: return KW_IF;
        case TOK_ELSE: return KW_ELSE;
        case TOK_WHILE: return KW_WHILE;
        case TOK_FOR: return KW_FOR;
        case TOK_BREAK: return KW_BREAK;
        case TOK_CONTINUE: return KW_CONTINUE;
        case TOK_ASM: return KW_ASM;
        case TOK_IDENTIFIER: return IDENTIFIER;
        case TOK_STRING_LITERAL: return STRING_LITERAL;
        case TOK_NUMBER: return NUMBER;
        case TOK_DECI_NUMBER: return DECI_NUMBER;
        case TOK_PLUS: return OP_PLUS;
        case TOK_SUBTRACT: return OP_MINUS;
        case TOK_ASTERISK: return OP_ASTERISK;
        case TOK_AMPERSAND: return OP_AMPERSAND;
        case TOK_DIVIDE: return OP_DIVIDE;
        case TOK_MODULO: return OP_MODULO;
        case TOK_ASSIGN: return OP_ASSIGN;
        case TOK_EQUAL_EQUAL: return OP_EQUAL_EQUAL;
        case TOK_GREATER: return OP_GREATER;
        case TOK_LESS: return OP_LESS;
        case TOK_GREATER_EQUALS: return OP_GREATER_EQUALS;
        case TOK_LESS_EQUALS: return OP_LESS_EQUALS;
        case TOK_AND: return OP_AND;
        case TOK_OR: return OP_OR;
        case TOK_NOT: return OP_NOT;
        case TOK_NOT_EQUAL: return OP_NOT_EQUAL;
        case TOK_LPAREN: return P_LPAREN;
        case TOK_RPAREN: return P_RPAREN;
        case TOK_LBRACE: return P_LBRACE;
        case TOK_RBRACE: return P_RBRACE;
        case TOK_COLON: return P_COLON;
        case TOK_SEMI: return P_SEMI;
        case TOK_COMMA: return P_COMMA;
        case TOK_EOF: return END_OF_FILE;
        default: return UNKNOWN;
    }
}