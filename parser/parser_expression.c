#include "parser.h"
#include "token_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * grammar implemented:
 *
 * expression  -> term ((+ | -) term)*
 * term        -> factor ((* | /) factor)*
 * factor      -> NUMBER | IDENTIFIER | '(' expression ')'
 */
static ExprNode *parse_expression_internal(void);
static ExprNode *parse_term(void);
static ExprNode *parse_factor(void);

static ExprNode *make_number(const float value) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_NUMBER;
    e->value = value;
    return e;
}

static ExprNode *make_var(const char *name) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_VAR;
    e->name = strdup(name);
    return e;
}

static ExprNode *make_binop(const TokenType op, ExprNode *left, ExprNode *right) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_BINOP;
    e->binop.op = op;
    e->binop.left = left;
    e->binop.right = right;
    return e;
}

/* factor -> NUMBER | IDENTIFIER | '(' expression ')' */
static ExprNode *parse_factor(void) {
    const Token t = current_token();

    if (t.type == TOK_NUMBER || t.type == TOK_DECI_NUMBER) {
        advance();
        return make_number(t.value);
    }

    if (t.type == TOK_IDENTIFIER) {
        advance();
        return make_var(t.lexeme);
    }

    if (t.type == TOK_LPAREN) {
        advance();
        ExprNode *expr = parse_expression_internal();
        expect(TOK_RPAREN);
        return expr;
    }

    fprintf(stderr, "unexpected token in expression: %d\n", t.type);
    exit(1);
}

/* term -> factor ((* | /) factor)* */
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

/* expression -> term ((+ | -) term)* */
static ExprNode *parse_expression_internal(void) {
    ExprNode *left = parse_term();

    while (current_token().type == TOK_PLUS || current_token().type == TOK_SUBTRACT) {
        const TokenType op = current_token().type;
        advance();
        ExprNode *right = parse_term();
        left = make_binop(op, left, right);
    }

    return left;
}

// public entry point
ExprNode *parse_expression(void) {
    return parse_expression_internal();
}