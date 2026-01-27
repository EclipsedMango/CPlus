#include "../lexer/lexer.h"
#include "lexer_internal.h"
#include "../util/common.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const struct {
    char *text;
    TokenType type;
} keywords[] = {
    {"int", TOK_INT},
    {"long", TOK_LONG},
    {"char", TOK_CHAR},
    {"float", TOK_FLOAT},
    {"double", TOK_DOUBLE},
    {"string", TOK_STRING_KW},
    {"bool", TOK_BOOL},
    {"void", TOK_VOID},
    {"const", TOK_CONST},
    {"return", TOK_RETURN},
    {"if", TOK_IF},
    {"else", TOK_ELSE},
    {"while", TOK_WHILE},
    {"for", TOK_FOR},
    {"break", TOK_BREAK},
    {"continue", TOK_CONTINUE},
    {"asm", TOK_ASM},
    {NULL, TOK_INVALID}
};

static const char* operator_strings[] = {
    [TOK_PLUS] = "+",
    [TOK_SUBTRACT] = "-",
    [TOK_ASTERISK] = "*",
    [TOK_AMPERSAND] = "&",
    [TOK_DIVIDE] = "/",
    [TOK_MODULO] = "%",
    [TOK_ASSIGN] = "=",
    [TOK_EQUAL_EQUAL] = "==",
    [TOK_GREATER] = ">",
    [TOK_GREATER_EQUALS] = ">=",
    [TOK_LESS] = "<",
    [TOK_LESS_EQUALS] = "<=",
    [TOK_AND] = "&&",
    [TOK_OR] = "||",
    [TOK_NOT] = "!",
    [TOK_NOT_EQUAL] = "!=",
    [TOK_LPAREN] = "(",
    [TOK_RPAREN] = ")",
    [TOK_LBRACE] = "{",
    [TOK_RBRACE] = "}",
    [TOK_LSQUARE] = "[",
    [TOK_RSQUARE] = "]",
    [TOK_COMMA] = ",",
    [TOK_COLON] = ":",
    [TOK_SEMI] = ";",
};

Lexer* lexer_create(const char *filename, FILE *file) {
    Lexer *lex = malloc(sizeof(Lexer));
    if (!lex) return NULL;

    lex->file = file;
    lex->filename = filename;
    lex->current_line = 1;
    lex->current_column = 1;
    lex->last_line = 1;
    lex->last_column = 1;
    lex->has_pushback = 0;
    lex->pushback_char = 0;

    return lex;
}

// owner close file not this
void lexer_destroy(Lexer *lexer) {
    if (!lexer) return;
    free(lexer);
}

Token lexer_next_token(Lexer *lexer) {
    if (!lexer) {
        return (Token){TOK_INVALID, NULL, {0, 0, NULL}};
    }

    while (1) {
        const int c = skip_whitespace(lexer);

        if (c == EOF) { return (Token){TOK_EOF, NULL, make_location(lexer)}; }
        if (c == '"') { return lex_string_literal(lexer); }
        if (isdigit(c)) { return lex_number_literal(lexer, c); }
        if (isalpha(c) || c == '_') { return lex_identifier_or_keyword(lexer, c); }

        const Token tok = lex_operator_or_punct(lexer, c);
        // tok.type == TOK_EOF means we hit a comment, continue looping
        if (tok.type != TOK_EOF) {
            return tok;
        }
    }
}

SourceLocation lexer_current_location(const Lexer *lexer) {
    return make_location(lexer);
}

const char* token_type_to_string(TokenType type) {
    for (int i = 0; keywords[i].text != NULL; i++) {
        if (keywords[i].type == type) {
            return keywords[i].text;
        }
    }

    if (type < sizeof(operator_strings) / sizeof(operator_strings[0]) && operator_strings[type] != NULL) {
        return operator_strings[type];
    }

    switch (type) {
        case TOK_IDENTIFIER: return "identifier";
        case TOK_STRING_LITERAL: return "string literal";
        case TOK_NUMBER: return "number";
        case TOK_DECI_NUMBER: return "decimal number";
        case TOK_EOF: return "end of file";
        case TOK_INVALID: return "invalid token";
        default: return "unknown token";
    }
}

void token_free_lexeme(Token *token) {
    if (!token) return;

    // identifiers and string literals are copied so ignore them
    if (token->type == TOK_IDENTIFIER || token->type == TOK_STRING_LITERAL ||
        token->type == TOK_NUMBER || token->type == TOK_DECI_NUMBER) {
        free(token->lexeme);
        token->lexeme = NULL;
    }
}

static SourceLocation make_location(const Lexer *lex) {
    SourceLocation loc;
    loc.line = lex->current_line;
    loc.column = lex->current_column;
    loc.filename = lex->filename;
    return loc;
}

static SourceLocation make_last_location(const Lexer *lex) {
    SourceLocation loc;
    loc.line = lex->last_line;
    loc.column = lex->last_column;
    loc.filename = lex->filename;
    return loc;
}

static int next_char(Lexer *lex) {
    lex->last_line = lex->current_line;
    lex->last_column = lex->current_column;

    int c;
    if (lex->has_pushback) {
        c = lex->pushback_char;
        lex->has_pushback = 0;
    } else {
        c = fgetc(lex->file);
    }

    if (c == EOF) return EOF;

    if (c == '\n') {
        lex->current_line++;
        lex->current_column = 1;
    } else {
        lex->current_column++;
    }

    return c;
}

static void unread_char(Lexer *lex, const int c) {
    if (c == EOF) return;
    if (lex->has_pushback) {
        report_error(make_location(lex), "Internal lexer error: pushback buffer overflow");
    }

    lex->has_pushback = 1;
    lex->pushback_char = c;

    lex->current_line = lex->last_line;
    lex->current_column = lex->last_column;
}

static void skip_line_comment(Lexer *lex) {
    int c;
    while ((c = next_char(lex)) != EOF && c != '\n') {}
}

static int skip_whitespace(Lexer *lex) {
    int c;
    while ((c = next_char(lex)) != EOF) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        return c;

    }
    return EOF;
}

static Token lex_identifier_or_keyword(Lexer *lex, const int first_char) {
    const SourceLocation start = make_last_location(lex);

    Vector buffer = create_vector(8, sizeof(char));

    char ch = (char)first_char;
    vector_push(&buffer, &ch);

    int c;
    while ((c = next_char(lex)) != EOF && (isalnum(c) || c == '_')) {
        ch = (char)c;
        vector_push(&buffer, &ch);
    }

    ch = '\0';
    vector_push(&buffer, &ch);
    unread_char(lex, c);

    const char *lexeme = buffer.elements;

    // keywords
    for (int i = 0; keywords[i].text != NULL; ++i) {
        if (strcmp(keywords[i].text, lexeme) == 0) {
            const Token tok = {
                .type = keywords[i].type,
                .lexeme = keywords[i].text,
                .location = start
            };

            vector_destroy(&buffer);
            return tok;
        }
    }

    // if not keyword then identifier
    const Token tok = {
        .type = TOK_IDENTIFIER,
        .lexeme = strdup(lexeme),
        .location = start
    };

    vector_destroy(&buffer);
    return tok;
}

static Token lex_number_literal(Lexer *lex, const int first_char) {
    const SourceLocation start = make_last_location(lex);

    Vector buf = create_vector(8, sizeof(char));
    int hasDecimal = 0;
    char ch = (char)first_char;

    vector_push(&buf, &ch);
    int c = next_char(lex);

    // read digits
    while (c != EOF && (isdigit(c) || c == '.')) {
        if (c == '.') {
            const SourceLocation dot_loc = make_last_location(lex);

            if (hasDecimal) {
                vector_destroy(&buf);
                report_error(dot_loc, "Invalid number: multiple decimal points");
                return (Token){TOK_INVALID, NULL, dot_loc};
            }

            hasDecimal = 1;
        }

        ch = (char)c;
        vector_push(&buf, &ch);
        c = next_char(lex);
    }

    ch = '\0';
    vector_push(&buf, &ch);
    unread_char(lex, c);

    const char *lexeme = buf.elements;

    const Token tok = {
        .type = hasDecimal ? TOK_DECI_NUMBER : TOK_NUMBER,
        .lexeme = strdup(lexeme),
        .location = start
    };

    vector_destroy(&buf);
    return tok;
}

static Token lex_string_literal(Lexer *lex) {
    Vector buf = create_vector(16, sizeof(char));
    int c;

    const SourceLocation start = make_last_location(lex);

    while ((c = next_char(lex)) != EOF) {
        if (c == '"') {
            const char ch = '\0';
            vector_push(&buf, &ch);

            const Token tok = {
                TOK_STRING_LITERAL,
                strdup(buf.elements),
                start
            };

            vector_destroy(&buf);
            return tok;
        }

        if (c == '\n') {
            vector_destroy(&buf);
            report_error(make_last_location(lex), "Unterminated string literal (newlines not allowed)");
            return (Token){TOK_INVALID, NULL, make_last_location(lex)};
        }

        if (c == '\\') {
            c = next_char(lex);
            char ch;

            switch (c) {
                case 'n':  ch = '\n'; break;
                case '0':  ch = '\0'; break;
                case 't':  ch = '\t'; break;
                case '"':  ch = '"';  break;
                case '\\': ch = '\\'; break;
                case '\n':
                    // line continuation
                    continue;
                default: {
                    vector_destroy(&buf);
                    report_error(make_last_location(lex), "Unknown escape sequence: \\%c", c);
                    return (Token){TOK_INVALID, NULL, make_last_location(lex)};
                }
            }

            vector_push(&buf, &ch);
        } else {
            char ch = (char)c;
            vector_push(&buf, &ch);
        }
    }

    vector_destroy(&buf);
    report_error(make_location(lex), "Unterminated string literal at EOF");
    return (Token){TOK_INVALID, NULL, make_location(lex)};
}

static Token lex_operator_or_punct(Lexer *lex, const int c) {
    const SourceLocation loc = make_last_location(lex);
    int next;

    switch (c) {
        // operators with possible lookahead
        case '=': {
            next = next_char(lex);
            if (next == '=') return (Token) { TOK_EQUAL_EQUAL, .lexeme = "==", loc };
            unread_char(lex, next);

            return (Token){ TOK_ASSIGN, .lexeme = "=", loc };
        }
        case '>': {
            next = next_char(lex);
            if (next == '=') return (Token){TOK_GREATER_EQUALS, .lexeme =">=", loc };
            unread_char(lex, next);

            return (Token){TOK_GREATER, .lexeme = ">", loc };
        }
        case '<': {
            next = next_char(lex);
            if (next == '=') return (Token){TOK_LESS_EQUALS, .lexeme = "<=", loc };
            unread_char(lex, next);

            return (Token){TOK_LESS, .lexeme = "<", loc};
        }
        case '&': {
            next = next_char(lex);
            if (next == '&') return (Token){TOK_AND, .lexeme = "&&", loc };
            unread_char(lex, next);

            return (Token){TOK_AMPERSAND, .lexeme = "&", loc};
        }
        case '|': {
            next = next_char(lex);
            if (next == '|') return (Token){TOK_OR, .lexeme = "||", loc };
            unread_char(lex, next);

            // maybe add bitwise OR later?
            report_error(loc, "Unexpected character '|'. Did you mean '||'?");
            return (Token){TOK_INVALID, NULL, loc};
        }
        case '!': {
            next = next_char(lex);
            if (next == '=') return (Token){TOK_NOT_EQUAL, .lexeme = "!=", loc };
            unread_char(lex, next);

            return (Token){TOK_NOT, .lexeme = "!", loc };
        }
        case '/': {
            next = next_char(lex);
            if (next == '/') {
                skip_line_comment(lex);
                return (Token){TOK_EOF, NULL, loc };
            }

            if (next == '=') {
                return (Token){.type = TOK_DIVIDE_EQUALS, .lexeme = "/=", loc};
            }

            unread_char(lex, next);
            return (Token){.type = TOK_DIVIDE, .lexeme = "/", loc};
        }
        case '+': {
            next = next_char(lex);
            if (next == '+') {
                return (Token){TOK_PLUS_PLUS, .lexeme = "++", loc};
            }

            if (next == '=') {
                return (Token){TOK_PLUS_EQUALS, .lexeme = "+=", loc};
            }

            unread_char(lex, next);
            return (Token){TOK_PLUS, .lexeme = "+", loc};
        }
        case '-': {
            next = next_char(lex);
            if (next == '-') {
                return (Token){TOK_SUBTRACT_SUBTRACT, .lexeme = "-", loc};
            }

            if (next == '=') {
                return (Token){TOK_SUBTRACT_EQUALS, .lexeme = "-=", loc};
            }

            unread_char(lex, next);
            return (Token){TOK_SUBTRACT, .lexeme = "-", loc};
        }

        // single-char operators
        case '*': {
            next = next_char(lex);
            if (next == '=') {
                return (Token){TOK_ASTERISK_EQUALS, .lexeme = "*=", loc};
            }

            unread_char(lex, next);
            return (Token){TOK_ASTERISK, .lexeme = "*", loc};
        }
        case '%': {
            next = next_char(lex);
            if (next == '=') {
                return (Token){TOK_MODULO_EQUALS, .lexeme = "%=", loc};
            }

            unread_char(lex, next);
            return (Token){TOK_MODULO, .lexeme = "%", loc};
        }

        // Punctuation
        case '(': return (Token){TOK_LPAREN, .lexeme = "(", loc};
        case ')': return (Token){TOK_RPAREN, .lexeme = ")", loc};
        case '{': return (Token){TOK_LBRACE, .lexeme = "{", loc};
        case '}': return (Token){TOK_RBRACE, .lexeme = "}", loc};
        case '[': return (Token){TOK_LSQUARE, .lexeme = "[", loc};
        case ']': return (Token){TOK_RSQUARE, .lexeme = "]", loc};
        case ':': return (Token){TOK_COLON, .lexeme = ":", loc};
        case ',': return (Token){TOK_COMMA, .lexeme = ",", loc};
        case ';': return (Token){TOK_SEMI, .lexeme = ";", loc};

        default:
            report_error(loc, "Unexpected character: '%c'", c);
            return (Token){TOK_INVALID, NULL, loc};
    }
}
