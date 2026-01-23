#include "parser.h"
#include "token_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * grammar implemented:
 *
 * expression  -> assignment
 * assignment  -> logical_or (= assignment)?
 * logical_or  -> logical_and (|| logical_and)*
 * logical_and -> equality (&& equality)*
 * equality    -> relational ((== | !=) relational)*
 * relational  -> additive ((< | > | <= | >=) additive)*
 * additive    -> term ((+ | -) term)*
 * term        -> factor ((* | / | %) factor)*
 * factor      -> unary
 * unary       -> (* | & | - | !)* postfix
 * postfix     -> primary ('[' expression ']')*
 * primary     -> NUMBER | STRING | IDENTIFIER | call | '(' expression ')'
 */
static ExprNode *parse_assignment(void);
static ExprNode *parse_logical_or(void);
static ExprNode *parse_logical_and(void);
static ExprNode *parse_equality(void);
static ExprNode *parse_relational(void);
static ExprNode *parse_unary(void);
static ExprNode *parse_postfix(void);
static ExprNode *parse_additive(void);
static ExprNode *parse_term(void);
static ExprNode *parse_factor(void);

// helpers
static ExprNode *make_number(const char *text, SourceLocation loc);
static ExprNode *make_var(const char *name, SourceLocation loc);
static ExprNode *make_binop(TokenType op, ExprNode *left, ExprNode *right, SourceLocation loc);
static ExprNode *make_unaryop(UnaryOp op, ExprNode *operand, SourceLocation loc);
static ExprNode *make_string_literal(const char *text, SourceLocation loc);
static ExprNode *make_call(const char *name, ExprNode **args, int count, SourceLocation loc);
static ExprNode *make_array_index(ExprNode *array, ExprNode* array_index);

// public entry point
ExprNode *parse_expression(void) {
    return parse_assignment();
}

static ExprNode *parse_assignment(void) {
    ExprNode *left = parse_logical_or();

    if (current_token().type == TOK_ASSIGN) {
        SourceLocation loc = current_token().location;
        const TokenType op = current_token().type;
        advance();
        ExprNode *right = parse_assignment(); // recursive on right side
        left = make_binop(op, left, right, loc);
    }

    return left;
}

static ExprNode *parse_logical_or(void) {
    ExprNode *left = parse_logical_and();
    while (current_token().type == TOK_OR) {
        SourceLocation loc = current_token().location;
        const TokenType op = current_token().type;
        advance();
        ExprNode *right = parse_logical_and();
        left = make_binop(op, left, right, loc);
    }
    return left;
}

static ExprNode *parse_logical_and(void) {
    ExprNode *left = parse_equality();
    while (current_token().type == TOK_AND) {
        SourceLocation loc = current_token().location;
        const TokenType op = current_token().type;
        advance();
        ExprNode *right = parse_equality();
        left = make_binop(op, left, right, loc);
    }
    return left;
}

static ExprNode *parse_equality(void) {
    ExprNode *left = parse_relational();
    while (current_token().type == TOK_EQUAL_EQUAL || current_token().type == TOK_NOT_EQUAL) {
        SourceLocation loc = current_token().location;
        const TokenType op = current_token().type;
        advance();
        ExprNode *right = parse_relational();
        left = make_binop(op, left, right, loc);
    }
    return left;
}

static ExprNode *parse_relational(void) {
    ExprNode *left = parse_additive();
    while (current_token().type == TOK_LESS || current_token().type == TOK_GREATER ||
           current_token().type == TOK_LESS_EQUALS || current_token().type == TOK_GREATER_EQUALS) {
        SourceLocation loc = current_token().location;
        const TokenType op = current_token().type;
        advance();
        ExprNode *right = parse_additive();
        left = make_binop(op, left, right, loc);
    }
    return left;
}

static ExprNode *parse_unary(void) {
    SourceLocation loc = current_token().location;
    if (current_token().type == TOK_ASTERISK) {
        advance();
        ExprNode *operand = parse_unary();
        return make_unaryop(UNARY_DEREF, operand, loc);
    }

    if (current_token().type == TOK_AMPERSAND) {
        advance();
        ExprNode *operand = parse_unary();
        return make_unaryop(UNARY_ADDR_OF, operand, loc);
    }

    if (current_token().type == TOK_SUBTRACT) {
        advance();
        ExprNode *operand = parse_unary();
        return make_unaryop(UNARY_NEG, operand, loc);
    }

    if (current_token().type == TOK_NOT) {
        advance();
        ExprNode *operand = parse_unary();
        return make_unaryop(UNARY_NOT, operand, loc);
    }

    return parse_postfix();
}

static ExprNode *parse_postfix(void) {
    const Token t = current_token();
    ExprNode *expr = NULL;

    if (t.type == TOK_NUMBER || t.type == TOK_DECI_NUMBER) {
        advance();
        expr = make_number(t.lexeme, t.location);
    } else if (t.type == TOK_STRING_LITERAL) {
        advance();
        expr = make_string_literal(t.lexeme, t.location);
    } else if (t.type == TOK_IDENTIFIER) {
        const Token name_tok = t;
        advance();
        if (current_token().type == TOK_LPAREN) {
            // function call
            advance();
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
            expr = make_call(name_tok.lexeme, args.elements, args.length, name_tok.location);
        } else {
            expr = make_var(name_tok.lexeme, name_tok.location);
        }
    } else if (t.type == TOK_LPAREN) {
        advance();
        expr = parse_expression();
        expect(TOK_RPAREN);
    } else {
        report_error(t.location, "Unexpected token in expression: '%s'", token_type_to_string(t.type));
        exit(1);
    }

    // handle postfix operators (array indexing)
    while (current_token().type == TOK_LSQUARE) {
        advance();
        ExprNode *index = parse_expression();
        expect(TOK_RSQUARE);
        expr = make_array_index(expr, index);
    }

    return expr;
}

// term -> factor ((* | /) factor)*
static ExprNode *parse_term(void) {
    ExprNode *left = parse_factor(); // note: factor handles unary
    while (current_token().type == TOK_ASTERISK || current_token().type == TOK_DIVIDE || current_token().type == TOK_MODULO) {
        SourceLocation loc = current_token().location;
        const TokenType op = current_token().type;
        advance();
        ExprNode *right = parse_factor(); // right side of mult/div is factor (which includes unary)
        left = make_binop(op, left, right, loc);
    }

    return left;
}

// factor -> NUMBER | IDENTIFIER | '(' expression ')'
static ExprNode *parse_factor(void) {
    return parse_unary();
}

// expression -> term ((+ | -) term)*
static ExprNode *parse_additive(void) {
    ExprNode *left = parse_term();
    while (current_token().type == TOK_PLUS || current_token().type == TOK_SUBTRACT) {
        SourceLocation loc = current_token().location;
        const TokenType op = current_token().type;
        advance();
        ExprNode *right = parse_term();
        left = make_binop(op, left, right, loc);
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

static ExprNode *make_number(const char *text, const SourceLocation loc) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_NUMBER;
    e->text = strdup(text);
    e->pointer_level = 0;
    e->location = loc;
    return e;
}

static ExprNode *make_var(const char *name, SourceLocation loc) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_VAR;
    e->text = strdup(name);
    e->pointer_level = 0;
    e->location = loc;
    return e;
}

static ExprNode *make_binop(const TokenType op, ExprNode *left, ExprNode *right, SourceLocation loc) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_BINOP;
    e->binop.op = token_to_binop(op);
    e->binop.left = left;
    e->binop.right = right;
    e->pointer_level = max(left->pointer_level, right->pointer_level);
    e->location = loc;
    return e;
}

static ExprNode *make_unaryop(const UnaryOp op, ExprNode *operand, SourceLocation loc) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_UNARY;
    e->unary.op = op;
    e->unary.operand = operand;
    e->pointer_level = 0;
    e->location = loc;
    return e;
}

static ExprNode *make_string_literal(const char *text, SourceLocation loc) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_STRING_LITERAL;
    e->text = strdup(text);
    e->pointer_level = 0;
    e->location = loc;
    return e;
}

static ExprNode *make_call(const char *name, ExprNode **args, const int count, SourceLocation loc) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_CALL;
    e->call.function_name = strdup(name);
    e->call.args = args;
    e->call.arg_count = count;
    e->pointer_level = 0;
    e->location = loc;
    return e;
}

static ExprNode *make_array_index(ExprNode *array, ExprNode* array_index) {
    ExprNode *e = malloc(sizeof(ExprNode));
    e->kind = EXPR_ARRAY_INDEX;
    e->array_index.array = array;
    e->array_index.index = array_index;
    e->pointer_level = 0;
    return e;
}