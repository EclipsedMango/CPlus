#include "parser.h"
#include "token_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * grammar implemented:
 *
 * expression  -> comparison
 * comparison  -> additive ((< | > | <= | >= | == | !=) additive)*
 * additive    -> term ((+ | -) term)*
 * term        -> factor ((* | / | %) factor)*
 * logical     -> comparison ((&& | ||) comparison)*
 */
static ExprNode *parse_assignment(void);
static ExprNode *parse_logical_or(void);
static ExprNode *parse_logical_and(void);
static ExprNode *parse_equality(void);
static ExprNode *parse_relational(void);
static ExprNode *parse_unary(void);
static ExprNode *parse_additive(void);
static ExprNode *parse_term(void);
static ExprNode *parse_factor(void);

// helpers
static ExprNode *make_number(const char *text);
static ExprNode *make_var(const char *name);
static ExprNode *make_binop(TokenType op, ExprNode *left, ExprNode *right);
static ExprNode *make_unaryop(UnaryOp op, ExprNode *operand);
static ExprNode *make_string_literal(const char *text);
static ExprNode *make_call(const char *name, ExprNode **args, int count);

// public entry point
ExprNode *parse_expression(void) {
    return parse_assignment();
}

static ExprNode *parse_assignment(void) {
    ExprNode *left = parse_logical_or();

    if (current_token().type == TOK_ASSIGN) {
        const TokenType op = current_token().type;
        advance();
        ExprNode *right = parse_assignment(); // recursive on right side
        left = make_binop(op, left, right);
    }

    return left;
}

static ExprNode *parse_logical_or(void) {
    ExprNode *left = parse_logical_and();
    while (current_token().type == TOK_OR) {
        const TokenType op = current_token().type;
        advance();
        ExprNode *right = parse_logical_and();
        left = make_binop(op, left, right);
    }
    return left;
}

static ExprNode *parse_logical_and(void) {
    ExprNode *left = parse_equality();
    while (current_token().type == TOK_AND) {
        const TokenType op = current_token().type;
        advance();
        ExprNode *right = parse_equality();
        left = make_binop(op, left, right);
    }
    return left;
}

static ExprNode *parse_equality(void) {
    ExprNode *left = parse_relational();
    while (current_token().type == TOK_EQUAL_EQUAL || current_token().type == TOK_NOT_EQUAL) {
        const TokenType op = current_token().type;
        advance();
        ExprNode *right = parse_relational();
        left = make_binop(op, left, right);
    }
    return left;
}

static ExprNode *parse_relational(void) {
    ExprNode *left = parse_additive();
    while (current_token().type == TOK_LESS || current_token().type == TOK_GREATER ||
           current_token().type == TOK_LESS_EQUALS || current_token().type == TOK_GREATER_EQUALS) {
        const TokenType op = current_token().type;
        advance();
        ExprNode *right = parse_additive();
        left = make_binop(op, left, right);
    }
    return left;
}

static ExprNode *parse_unary(void) {
    if (current_token().type == TOK_ASTERISK) {
        advance();
        ExprNode *operand = parse_unary();
        return make_unaryop(UNARY_DEREF, operand);
    }

    if (current_token().type == TOK_AMPERSAND) {
        advance();
        ExprNode *operand = parse_unary();
        return make_unaryop(UNARY_ADDR_OF, operand);
    }

    if (current_token().type == TOK_SUBTRACT) {
        advance();
        ExprNode *operand = parse_unary();
        return make_unaryop(UNARY_NEG, operand);
    }

    if (current_token().type == TOK_NOT) {
        advance();
        ExprNode *operand = parse_unary();
        return make_unaryop(UNARY_NOT, operand);
    }

    return parse_factor();
}

// term -> factor ((* | /) factor)*
static ExprNode *parse_term(void) {
    ExprNode *left = parse_unary();

    while (current_token().type == TOK_ASTERISK || current_token().type == TOK_DIVIDE || current_token().type == TOK_MODULO) {
        const TokenType op = current_token().type;
        advance();
        ExprNode *right = parse_unary();
        left = make_binop(op, left, right);
    }

    return left;
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
            Vector args = create_vector(4, sizeof(ExprNode*));

            if (current_token().type != TOK_RPAREN) {
                do {
                    ExprNode *arg = parse_expression();
                    vector_push(&args, &arg);

                    if (current_token().type == TOK_COMMA) {
                        advance();
                    } else {
                        break;
                    }
                } while (current_token().type != TOK_RPAREN);
            }

            expect(TOK_RPAREN);

            return make_call(name_tok.lexeme, args.elements, args.length);
        }

        return make_var(name_tok.lexeme);
    }

    if (t.type == TOK_LPAREN) {
        advance();
        ExprNode *expr = parse_expression();
        expect(TOK_RPAREN);
        return expr;
    }

    report_error(t.location, "Unexpected token in expression: '%s'", token_type_to_string(t.type));
    exit(1);
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

static BinaryOp token_to_binop(const TokenType token) {
    switch (token) {
        case TOK_PLUS: return BIN_ADD;
        case TOK_SUBTRACT: return BIN_SUB;
        case TOK_ASTERISK: return BIN_MUL;
        case TOK_DIVIDE: return BIN_DIV;
        case TOK_MODULO: return BIN_MOD;
        case TOK_EQUAL_EQUAL: return BIN_EQUAL;
        case TOK_GREATER: return BIN_GREATER;
        case TOK_LESS: return BIN_LESS;
        case TOK_GREATER_EQUALS: return BIN_GREATER_EQ;
        case TOK_LESS_EQUALS: return BIN_LESS_EQ;
        case TOK_ASSIGN: return BIN_ASSIGN;
        case TOK_AND: return BIN_LOGICAL_AND;
        case TOK_OR: return BIN_LOGICAL_OR;
        case TOK_NOT_EQUAL: return BIN_NOT_EQUAL;
        default: report_error(current_token().location, "Parser logic error: Invalid binary operator token");
    }
}

static ExprNode *make_number(const char *text) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_NUMBER;
    e->text = strdup(text);
    e->pointer_level = 0;
    return e;
}

static ExprNode *make_var(const char *name) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_VAR;
    e->text = strdup(name);
    e->pointer_level = 0;
    return e;
}

static ExprNode *make_binop(const TokenType op, ExprNode *left, ExprNode *right) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_BINOP;
    e->binop.op = token_to_binop(op);
    e->binop.left = left;
    e->binop.right = right;
    e->pointer_level = 0;
    return e;
}

static ExprNode *make_unaryop(const UnaryOp op, ExprNode *operand) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_UNARY;
    e->unary.op = op;
    e->unary.operand = operand;
    e->pointer_level = 0;
    return e;
}

static ExprNode *make_string_literal(const char *text) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_STRING_LITERAL;
    e->text = strdup(text);
    e->pointer_level = 0;
    return e;
}

static ExprNode *make_call(const char *name, ExprNode **args, const int count) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_CALL;
    e->call.function_name = strdup(name);
    e->call.args = args;
    e->call.arg_count = count;
    e->pointer_level = 0;
    return e;
}