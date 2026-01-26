
#include "parser_internal.h"
#include <stdlib.h>
#include <string.h>

Parser* parser_create(Lexer *lexer, DiagnosticEngine *diagnostics) {
    Parser *p = malloc(sizeof(Parser));
    if (!p) return NULL;

    p->lexer = lexer;
    p->diagnostics = diagnostics;
    p->retired_tokens = create_vector(128, sizeof(Token));

    parser_init_token_buffer(p);
    return p;
}

void parser_destroy(Parser *parser) {
    if (!parser) return;

    // free any owned token lexemes in the buffer
    for (int i = 0; i < TOKEN_BUFFER_SIZE; i++) {
        token_free_lexeme(&parser->token_buffer[i]);
    }

    for (int i = 0; i < parser->retired_tokens.length; i++) {
        Token *t = vector_get(&parser->retired_tokens, i);
        token_free_lexeme(t);
    }

    vector_destroy(&parser->retired_tokens);

    free(parser);
}

ProgramNode* parser_parse_program(Parser *parser) {
    Vector global_vars = create_vector(16, sizeof(GlobalVarNode*));
    Vector functions = create_vector(16, sizeof(FunctionNode*));

    while (parser_current_token(parser).type != TOK_EOF) {
        if (parser_current_token(parser).type == TOK_CONST) {
            GlobalVarNode *global = parse_global_var(parser);
            vector_push(&global_vars, &global);
            continue;
        }

        int lookahead_pos = 1;

        if (parser_peek_token(parser, lookahead_pos).type == TOK_LSQUARE) {
            lookahead_pos++;

            while (parser_peek_token(parser, lookahead_pos).type != TOK_RSQUARE) {
                lookahead_pos++;
            }

            lookahead_pos++;
        }

        while (parser_peek_token(parser, lookahead_pos).type == TOK_ASTERISK) {
            lookahead_pos++;
        }

        lookahead_pos++;

        const TokenType next = parser_peek_token(parser, lookahead_pos).type;
        if (next == TOK_LPAREN) {
            // function decl
            FunctionNode *fn = parse_function(parser);
            vector_push(&functions, &fn);
        } else if (next == TOK_SEMI || next == TOK_ASSIGN) {
            // global var decl
            GlobalVarNode *global = parse_global_var(parser);
            vector_push(&global_vars, &global);
        } else {
            diag_error(parser->diagnostics, parser_peek_token(parser, lookahead_pos).location,
                      "Expected '(' (for function) or ';' or '=' (for global variable), got '%s'",
                      token_type_to_string(next)
            );

            while (parser_current_token(parser).type != TOK_SEMI && parser_current_token(parser).type != TOK_LBRACE && parser_current_token(parser).type != TOK_EOF) {
                parser_advance(parser);
            }

            if (parser_current_token(parser).type == TOK_SEMI) {
                parser_advance(parser);
            }
        }
    }

    ProgramNode *program = malloc(sizeof(ProgramNode));
    program->functions = (FunctionNode**)functions.elements;
    program->function_count = functions.length;
    program->globals = (GlobalVarNode**)global_vars.elements;
    program->global_count = global_vars.length;

    return program;
}

void parser_init_token_buffer(Parser *p) {
    for (int i = 0; i < TOKEN_BUFFER_SIZE; i++) {
        p->token_buffer[i] = lexer_next_token(p->lexer);
    }
}

void parser_update_token_buffer(Parser *p) {
    vector_push(&p->retired_tokens, &p->token_buffer[0]);

    // shift tokens to the left
    for (int i = 0; i < TOKEN_BUFFER_SIZE - 1; i++) {
        p->token_buffer[i] = p->token_buffer[i + 1];
    }

    p->token_buffer[TOKEN_BUFFER_SIZE - 1] = lexer_next_token(p->lexer);
}

Token parser_current_token(const Parser *p) {
    return p->token_buffer[0];
}

Token parser_peek_token(const Parser *p, const int n) {
    if (n < 0 || n >= TOKEN_BUFFER_SIZE) {
        diag_error(p->diagnostics, lexer_current_location(p->lexer), "Internal error: peek_token index %d out of bounds", n);
        return (Token){TOK_INVALID, NULL, {0, 0, NULL}};
    }

    return p->token_buffer[n];
}

void parser_advance(Parser *p) {
    parser_update_token_buffer(p);
}

void parser_expect(Parser *p, const TokenType type) {
    if (parser_current_token(p).type != type) {
        diag_error(p->diagnostics, parser_current_token(p).location, "Expected '%s', but got '%s'",
                  token_type_to_string(type),
                  token_type_to_string(parser_current_token(p).type)
        );

        return;
    }

    parser_advance(p);
}

TypeKind token_to_typekind(const Parser *p, const TokenType token) {
    switch (token) {
        case TOK_INT: return TYPE_INT;
        case TOK_LONG: return TYPE_LONG;
        case TOK_CHAR: return TYPE_CHAR;
        case TOK_FLOAT: return TYPE_FLOAT;
        case TOK_DOUBLE: return TYPE_DOUBLE;
        case TOK_STRING_KW: return TYPE_STRING;
        case TOK_BOOL: return TYPE_BOOLEAN;
        case TOK_VOID: return TYPE_VOID;
        default:
            diag_error(p->diagnostics, parser_current_token(p).location, "Invalid type token: %s", token_type_to_string(token));
            return TYPE_INT;  // error recovery, default to int
    }
}
