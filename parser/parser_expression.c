#include "parser.h"
#include "token_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * grammar implemented:
 *
 * expression  -> comparison
 * comparison  -> additive ((< | > | <= | >= | ==) additive)*
 * additive    -> term ((+ | -) term)*
 * term        -> factor ((* | / | %) factor)*
 */
static ExprNode *parse_additive(void);
static ExprNode *parse_term(void);
static ExprNode *parse_factor(void);

// helpers
static ExprNode *make_number(const char *text);
static ExprNode *make_var(const char *name);
static ExprNode *make_binop(TokenType op, ExprNode *left, ExprNode *right);
static ExprNode *make_string_literal(const char *text);
static ExprNode *make_call(const char *name, ExprNode **args, int count);

static BinaryOp token_to_binop(const TokenType token) {
    switch (token) {
        case TOK_PLUS: return BIN_ADD;
        case TOK_SUBTRACT: return BIN_SUB;
        case TOK_MULTIPLY: return BIN_MUL;
        case TOK_DIVIDE: return BIN_DIV;
        case TOK_MODULO: return BIN_MOD;
        case TOK_EQUAL_EQUAL: return BIN_EQUAL;
        case TOK_GREATER: return BIN_GREATER;
        case TOK_LESS: return BIN_LESS;
        case TOK_GREATER_EQUALS: return BIN_GREATER_EQ;
        case TOK_LESS_EQUALS: return BIN_LESS_EQ;
        case TOK_ASSIGN: return BIN_ASSIGN;
        default:
            fprintf(stderr, "invalid binary operator token: %d\n", token);
            exit(1);
    }
}

static ExprNode *make_number(const char *text) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_NUMBER;
    e->text = strdup(text);
    return e;
}

static ExprNode *make_var(const char *name) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_VAR;
    e->text = strdup(name);
    return e;
}

static ExprNode *make_binop(const TokenType op, ExprNode *left, ExprNode *right) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_BINOP;
    e->binop.op = token_to_binop(op);
    e->binop.left = left;
    e->binop.right = right;
    return e;
}

static ExprNode *make_string_literal(const char *text) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_STRING_LITERAL;
    e->text = strdup(text);
    return e;
}

static ExprNode *make_call(const char *name, ExprNode **args, const int count) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_CALL;
    e->call.function_name = strdup(name);
    e->call.args = args;
    e->call.arg_count = count;
    return e;
}

// factor -> NUMBER | IDENTIFIER | '(' expression ')'
static ExprNode *parse_factor(void) {
    const Token t = current_token();

    if (t.type == TOK_NUMBER || t.type == TOK_DECI_NUMBER) {
        advance();
        return make_number(t.lexeme);
    }

    if (t.type == TOK_STRING_LITERAL) {
        advance();
        return make_string_literal(t.lexeme);
    }

    if (t.type == TOK_IDENTIFIER) {
        const Token name_tok = t;
        advance();

        if (current_token().type == TOK_LPAREN) {
            advance();

            // parse arguments
            ExprNode **args = malloc(sizeof(ExprNode*) * 16);
            int arg_count = 0;

            if (current_token().type != TOK_RPAREN) {
                do {
                    args[arg_count++] = parse_expression();

                    if (current_token().type == TOK_COMMA) {
                        advance();
                    } else {
                        break;
                    }
                } while (current_token().type != TOK_RPAREN);
            }

            expect(TOK_RPAREN);

            return make_call(name_tok.lexeme, args, arg_count);
        }

        return make_var(name_tok.lexeme);
    }

    if (t.type == TOK_LPAREN) {
        advance();
        ExprNode *expr = parse_additive();
        expect(TOK_RPAREN);
        return expr;
    }

    fprintf(stderr, "unexpected token in expression: %d\n", t.type);
    exit(1);
}

// term -> factor ((* | /) factor)*
static ExprNode *parse_term(void) {
    ExprNode *left = parse_factor();

    while (current_token().type == TOK_MULTIPLY || current_token().type == TOK_DIVIDE) {
        const TokenType op = current_token().type;
        advance();
        ExprNode *right = parse_factor();
        left = make_binop(op, left, right);
    }

    return left;
}

// expression -> term ((+ | -) term)*
static ExprNode *parse_additive(void) {
    ExprNode *left = parse_term();

    while (current_token().type == TOK_PLUS || current_token().type == TOK_SUBTRACT) {
        const TokenType op = current_token().type;
        advance();
        ExprNode *right = parse_term();
        left = make_binop(op, left, right);
    }

    return left;
}

static ExprNode *parse_comparison(void) {
    ExprNode *left = parse_additive();

    while (current_token().type == TOK_LESS || current_token().type == TOK_GREATER || current_token().type == TOK_LESS_EQUALS ||
           current_token().type == TOK_GREATER_EQUALS || current_token().type == TOK_EQUAL_EQUAL) {

        const TokenType tok = current_token().type;
        advance();

        ExprNode *right = parse_additive();
        left = make_binop(tok, left, right);
    }

    return left;
}

// public entry point
ExprNode *parse_expression(void) {
    return parse_comparison();
}