#include "parser_internal.h"
#include <stdlib.h>
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
ExprNode* parse_expression(Parser *p) {
    return parse_assignment(p);
}

ExprNode* parse_assignment(Parser *p) {
    ExprNode *left = parse_logical_or(p);

    if (parser_current_token(p).type == TOK_ASSIGN) {
        const SourceLocation loc = parser_current_token(p).location;
        parser_advance(p);
        ExprNode *right = parse_assignment(p);

        ExprNode *expr = malloc(sizeof(ExprNode));
        expr->kind = EXPR_BINOP;
        expr->location = loc;
        expr->binop.op = BIN_ASSIGN;
        expr->binop.left = left;
        expr->binop.right = right;
        expr->pointer_level = 0;

        return expr;
    }

    return left;
}

ExprNode* parse_logical_or(Parser *p) {
    ExprNode *left = parse_logical_and(p);

    while (parser_current_token(p).type == TOK_OR) {
        const SourceLocation loc = parser_current_token(p).location;
        parser_advance(p);
        ExprNode *right = parse_logical_and(p);

        ExprNode *expr = malloc(sizeof(ExprNode));
        expr->kind = EXPR_BINOP;
        expr->location = loc;
        expr->binop.op = BIN_LOGICAL_OR;
        expr->binop.left = left;
        expr->binop.right = right;
        expr->pointer_level = 0;

        left = expr;
    }

    return left;
}

ExprNode* parse_logical_and(Parser *p) {
    ExprNode *left = parse_equality(p);

    while (parser_current_token(p).type == TOK_AND) {
        const SourceLocation loc = parser_current_token(p).location;
        parser_advance(p);
        ExprNode *right = parse_equality(p);

        ExprNode *expr = malloc(sizeof(ExprNode));
        expr->kind = EXPR_BINOP;
        expr->location = loc;
        expr->binop.op = BIN_LOGICAL_AND;
        expr->binop.left = left;
        expr->binop.right = right;
        expr->pointer_level = 0;

        left = expr;
    }

    return left;
}

ExprNode* parse_equality(Parser *p) {
    ExprNode *left = parse_relational(p);

    while (parser_current_token(p).type == TOK_EQUAL_EQUAL || parser_current_token(p).type == TOK_NOT_EQUAL) {
        const SourceLocation loc = parser_current_token(p).location;
        const TokenType op = parser_current_token(p).type;

        parser_advance(p);
        ExprNode *right = parse_relational(p);

        ExprNode *expr = malloc(sizeof(ExprNode));
        expr->kind = EXPR_BINOP;
        expr->location = loc;
        expr->binop.op = (op == TOK_EQUAL_EQUAL) ? BIN_EQUAL : BIN_NOT_EQUAL;
        expr->binop.left = left;
        expr->binop.right = right;
        expr->pointer_level = 0;

        left = expr;
    }

    return left;
}

ExprNode* parse_relational(Parser *p) {
    ExprNode *left = parse_additive(p);

    while (parser_current_token(p).type == TOK_LESS || parser_current_token(p).type == TOK_GREATER ||
           parser_current_token(p).type == TOK_LESS_EQUALS || parser_current_token(p).type == TOK_GREATER_EQUALS) {
        const SourceLocation loc = parser_current_token(p).location;
        const TokenType op_tok = parser_current_token(p).type;

        parser_advance(p);
        ExprNode *right = parse_additive(p);

        BinaryOp op;
        switch (op_tok) {
            case TOK_LESS: op = BIN_LESS; break;
            case TOK_GREATER: op = BIN_GREATER; break;
            case TOK_LESS_EQUALS: op = BIN_LESS_EQ; break;
            case TOK_GREATER_EQUALS: op = BIN_GREATER_EQ; break;
            default: op = BIN_LESS; break;  // should not happen
        }

        ExprNode *expr = malloc(sizeof(ExprNode));
        expr->kind = EXPR_BINOP;
        expr->location = loc;
        expr->binop.op = op;
        expr->binop.left = left;
        expr->binop.right = right;
        expr->pointer_level = 0;

        left = expr;
    }

    return left;
}

ExprNode* parse_additive(Parser *p) {
    ExprNode *left = parse_term(p);

    while (parser_current_token(p).type == TOK_PLUS || parser_current_token(p).type == TOK_SUBTRACT) {
        const SourceLocation loc = parser_current_token(p).location;
        const BinaryOp op = (parser_current_token(p).type == TOK_PLUS) ? BIN_ADD : BIN_SUB;
        parser_advance(p);
        ExprNode *right = parse_term(p);

        ExprNode *expr = malloc(sizeof(ExprNode));
        expr->kind = EXPR_BINOP;
        expr->location = loc;
        expr->binop.op = op;
        expr->binop.left = left;
        expr->binop.right = right;
        expr->pointer_level = 0;

        left = expr;
    }

    return left;
}

ExprNode* parse_term(Parser *p) {
    ExprNode *left = parse_unary(p);

    while (parser_current_token(p).type == TOK_ASTERISK || parser_current_token(p).type == TOK_DIVIDE || parser_current_token(p).type == TOK_MODULO) {
        const SourceLocation loc = parser_current_token(p).location;
        const TokenType op_tok = parser_current_token(p).type;
        parser_advance(p);
        ExprNode *right = parse_unary(p);

        BinaryOp op;
        switch (op_tok) {
            case TOK_ASTERISK: op = BIN_MUL; break;
            case TOK_DIVIDE: op = BIN_DIV; break;
            case TOK_MODULO: op = BIN_MOD; break;
            default: op = BIN_MUL; break;
        }

        ExprNode *expr = malloc(sizeof(ExprNode));
        expr->kind = EXPR_BINOP;
        expr->location = loc;
        expr->binop.op = op;
        expr->binop.left = left;
        expr->binop.right = right;
        expr->pointer_level = 0;

        left = expr;
    }

    return left;
}

ExprNode* parse_unary(Parser *p) {
    const SourceLocation loc = parser_current_token(p).location;

    if (parser_current_token(p).type == TOK_ASTERISK) {
        parser_advance(p);
        ExprNode *operand = parse_unary(p);

        ExprNode *expr = malloc(sizeof(ExprNode));
        expr->kind = EXPR_UNARY;
        expr->location = loc;
        expr->unary.op = UNARY_DEREF;
        expr->unary.operand = operand;
        expr->pointer_level = 0;
        return expr;
    }

    if (parser_current_token(p).type == TOK_AMPERSAND) {
        parser_advance(p);
        ExprNode *operand = parse_unary(p);

        ExprNode *expr = malloc(sizeof(ExprNode));
        expr->kind = EXPR_UNARY;
        expr->location = loc;
        expr->unary.op = UNARY_ADDR_OF;
        expr->unary.operand = operand;
        expr->pointer_level = 0;
        return expr;
    }

    if (parser_current_token(p).type == TOK_SUBTRACT) {
        parser_advance(p);
        ExprNode *operand = parse_unary(p);

        ExprNode *expr = malloc(sizeof(ExprNode));
        expr->kind = EXPR_UNARY;
        expr->location = loc;
        expr->unary.op = UNARY_NEG;
        expr->unary.operand = operand;
        expr->pointer_level = 0;
        return expr;
    }

    if (parser_current_token(p).type == TOK_NOT) {
        parser_advance(p);
        ExprNode *operand = parse_unary(p);

        ExprNode *expr = malloc(sizeof(ExprNode));
        expr->kind = EXPR_UNARY;
        expr->location = loc;
        expr->unary.op = UNARY_NOT;
        expr->unary.operand = operand;
        expr->pointer_level = 0;
        return expr;
    }

    return parse_postfix(p);
}

ExprNode* parse_postfix(Parser *p) {
    Token t = parser_current_token(p);
    ExprNode *expr = NULL;

    // primary expression
    if (t.type == TOK_NUMBER || t.type == TOK_DECI_NUMBER) {
        parser_advance(p);
        expr = malloc(sizeof(ExprNode));
        expr->kind = EXPR_NUMBER;
        expr->text = strdup(t.lexeme);
        expr->location = t.location;
        expr->pointer_level = 0;

    } else if (t.type == TOK_STRING_LITERAL) {
        parser_advance(p);
        expr = malloc(sizeof(ExprNode));
        expr->kind = EXPR_STRING_LITERAL;
        expr->text = strdup(t.lexeme);
        expr->location = t.location;
        expr->pointer_level = 0;

    } else if (t.type == TOK_IDENTIFIER) {
        const Token name_tok = t;
        parser_advance(p);

        // check for function call
        if (parser_current_token(p).type == TOK_LPAREN) {
            parser_advance(p);
            Vector args = create_vector(4, sizeof(ExprNode*));

            if (parser_current_token(p).type != TOK_RPAREN) {
                do {
                    ExprNode *arg = parse_expression(p);
                    vector_push(&args, &arg);

                    if (parser_current_token(p).type == TOK_COMMA) {
                        parser_advance(p);
                    } else {
                        break;
                    }
                } while (parser_current_token(p).type != TOK_RPAREN);
            }

            parser_expect(p, TOK_RPAREN);

            expr = malloc(sizeof(ExprNode));
            expr->kind = EXPR_CALL;
            expr->call.function_name = strdup(name_tok.lexeme);
            expr->call.args = (ExprNode**)args.elements;
            expr->call.arg_count = args.length;
            expr->location = name_tok.location;
            expr->pointer_level = 0;

        } else {
            // variable
            expr = malloc(sizeof(ExprNode));
            expr->kind = EXPR_VAR;
            expr->text = strdup(name_tok.lexeme);
            expr->location = name_tok.location;
            expr->pointer_level = 0;

        }
    } else if (t.type == TOK_LPAREN) {
        parser_advance(p);
        expr = parse_expression(p);
        parser_expect(p, TOK_RPAREN);

    } else {
        diag_error(p->diagnostics, t.location, "Unexpected token in expression: '%s'", token_type_to_string(t.type));

        // error recovery: create dummy expression and then skip bad token
        expr = malloc(sizeof(ExprNode));
        expr->kind = EXPR_NUMBER;
        expr->text = strdup("0");
        expr->location = t.location;
        expr->pointer_level = 0;

        parser_advance(p);
        return expr;
    }

    // postfix operators
    while (parser_current_token(p).type == TOK_LSQUARE) {
        parser_advance(p);
        ExprNode *index = parse_expression(p);
        parser_expect(p, TOK_RSQUARE);

        ExprNode *array_expr = malloc(sizeof(ExprNode));
        array_expr->kind = EXPR_ARRAY_INDEX;
        array_expr->array_index.array = expr;
        array_expr->array_index.index = index;
        array_expr->location = expr->location;
        array_expr->pointer_level = 0;

        expr = array_expr;
    }

    return expr;
}