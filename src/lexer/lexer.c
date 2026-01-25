#include "../lexer/lexer.h"
#include "lexer_internal.h"
#include "../util/common.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const struct {
    const char *text;
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

    return lex;
}

// owner close file not this
void lexer_destroy(Lexer *lexer) {
    if (!lexer) return;
    free(lexer);
}

Token lexer_next_token(Lexer *lex) {
    if (!lex) {
        return (Token){TOK_INVALID, NULL, {0, 0, NULL}};
    }

    while (1) {
        const int c = skip_whitespace(lex);

        if (c == EOF) { return (Token){TOK_EOF, NULL, make_location(lex)}; }
        if (c == '"') { return lex_string_literal(lex); }
        if (isdigit(c)) { return lex_number_literal(lex, c); }
        if (isalpha(c) || c == '_') { return lex_identifier_or_keyword(lex, c); }

        const Token tok = lex_operator_or_punct(lex, c);
        // tok.type == TOK_EOF means we hit a comment, continue looping
        if (tok.type != TOK_EOF) {
            return tok;
        }
    }
}

SourceLocation lexer_current_location(const Lexer *lex) {
    return make_location(lex);
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

static int next_char(Lexer *lex) {
    int c = fgetc(lex->file);
    if (c == '\n') {
        lex->current_line++;
        lex->current_column = 1;
    } else if (c != EOF) {
        lex->current_column++;
    }
    return c;
}

static void skip_line_comment(Lexer *lex) {
    int c;
    while ((c = fgetc(lex->file)) != EOF && c != '\n') {
        lex->current_column++;
    }

    if (c == '\n') {
        lex->current_line++;
        lex->current_column = 1;
    }
}

static int skip_whitespace(Lexer *lex) {
    int c;
    while (1) {
        c = fgetc(lex->file);

        if (c == '\n') {
            lex->current_line++;
            lex->current_column = 1;
        } else if (c == ' ' || c == '\t' || c == '\r') {
            lex->current_column++;
        } else {
            break;
        }
    }

    return c;
}

static Token lex_identifier_or_keyword(Lexer *lex, const int first_char) {
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

    ungetc(c, lex->file);

    const char *lexeme = buffer.elements;
    const SourceLocation loc = make_location(lex);

    // keywords
    for (int i = 0; keywords[i].text != NULL; ++i) {
        if (strcmp(keywords[i].text, lexeme) == 0) {
            const Token tok = {
                .type = keywords[i].type,
                .lexeme = keywords[i].text,
                .location = loc
            };

            vector_destroy(&buffer);
            return tok;
        }
    }

    // if not keyword then identifier
    const Token tok = {
        .type = TOK_IDENTIFIER,
        .lexeme = strdup(lexeme),
        .location = loc
    };

    vector_destroy(&buffer);
    return tok;
}

static Token lex_number_literal(Lexer *lex, const int first_char) {
    Vector buf = create_vector(8, sizeof(char));
    int hasDecimal = 0;
    char ch = (char)first_char;

    vector_push(&buf, &ch);
    int c = next_char(lex);

    // read digits
    while (c != EOF && (isdigit(c) || c == '.')) {
        if (c == '.') {
            if (hasDecimal) {
                vector_destroy(&buf);
                report_error(make_location(lex), "Invalid number: multiple decimal points");
                return (Token){TOK_INVALID, NULL, make_location(lex)};
            }

            hasDecimal = 1;
        }

        ch = (char)c;
        vector_push(&buf, &ch);
        c = next_char(lex);
    }

    ch = '\0';
    vector_push(&buf, &ch);
    ungetc(c, lex->file);

    const char *lexeme = buf.elements;
    const SourceLocation loc = make_location(lex);

    const Token tok = {
        .type = hasDecimal ? TOK_DECI_NUMBER : TOK_NUMBER,
        .lexeme = strdup(lexeme),
        .location = loc
    };

    vector_destroy(&buf);
    return tok;
}

static Token lex_string_literal(Lexer *lex) {
    Vector buf = create_vector(16, sizeof(char));
    int c;

    while ((c = next_char(lex)) != EOF) {
        if (c == '"') {
            constexpr char ch = '\0';
            vector_push(&buf, &ch);

            const Token tok = {
                TOK_STRING_LITERAL,
                strdup(buf.elements),
                make_location(lex)
            };

            vector_destroy(&buf);
            return tok;
        }

        if (c == '\n') {
            vector_destroy(&buf);
            report_error(make_location(lex), "Unterminated string literal (newlines not allowed)");
            return (Token){TOK_INVALID, NULL, make_location(lex)};
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
                    report_error(make_location(lex), "Unknown escape sequence: \\%c", c);
                    return (Token){TOK_INVALID, NULL, make_location(lex)};
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
    const SourceLocation loc = make_location(lex);
    int next;

    switch (c) {
        // operators with possible lookahead
        case '=': {
            next = next_char(lex);
            if (next == '=') return (Token) { TOK_EQUAL_EQUAL, "==", loc };
            ungetc(next, lex->file);

            return (Token){ TOK_ASSIGN, "=", loc };
        }
        case '>': {
            next = next_char(lex);
            if (next == '=') return (Token){TOK_GREATER_EQUALS, ">=", loc };
            ungetc(next, lex->file);

            return (Token){TOK_GREATER, ">", loc };
        }
        case '<': {
            next = next_char(lex);
            if (next == '=') return (Token){TOK_LESS_EQUALS, "<=", loc };
            ungetc(next, lex->file);

            return (Token){TOK_LESS, "<", loc};
        }
        case '&': {
            next = next_char(lex);
            if (next == '&') return (Token){TOK_AND, "&&", loc };
            ungetc(next, lex->file);

            return (Token){TOK_AMPERSAND, "&", loc};
        }
        case '|': {
            next = next_char(lex);
            if (next == '|') return (Token){TOK_OR, "||", loc };
            ungetc(next, lex->file);

            // maybe add bitwise OR later?
            report_error(loc, "Unexpected character '|'. Did you mean '||'?");
            return (Token){TOK_INVALID, NULL, loc};
        }
        case '!': {
            next = next_char(lex);
            if (next == '=') return (Token){TOK_NOT_EQUAL, "!=", loc };
            ungetc(next, lex->file);

            return (Token){TOK_NOT, "!", loc };
        }
        case '/': {
            next = next_char(lex);
            if (next == '/') {
                skip_line_comment(lex);
                return (Token){TOK_EOF, NULL, loc };
            }

            ungetc(next, lex->file);
            return (Token){.type = TOK_DIVIDE, "/", loc};
        }

        // single-char operators
        case '+': return (Token){TOK_PLUS, "+", loc};
        case '-': return (Token){TOK_SUBTRACT, "-", loc};
        case '*': return (Token){TOK_ASTERISK, "*", loc};
        case '%': return (Token){TOK_MODULO, "%", loc};

        // Punctuation
        case '(': return (Token){TOK_LPAREN, "(", loc};
        case ')': return (Token){TOK_RPAREN, ")", loc};
        case '{': return (Token){TOK_LBRACE, "{", loc};
        case '}': return (Token){TOK_RBRACE, "}", loc};
        case '[': return (Token){TOK_LSQUARE, "[", loc};
        case ']': return (Token){TOK_RSQUARE, "]", loc};
        case ':': return (Token){TOK_COLON, ":", loc};
        case ',': return (Token){TOK_COMMA, ",", loc};
        case ';': return (Token){TOK_SEMI, ";", loc};

        default:
            report_error(loc, "Unexpected character: '%c'", c);
            return (Token){TOK_INVALID, NULL, loc};
    }
}
