#include "parser_internal.h"
#include <stdlib.h>
#include <string.h>

StmtNode* parse_statement(Parser *p) {
    const Token t = parser_current_token(p);

    switch (t.type) {
        case TOK_RETURN: return parse_return_stmt(p);
        case TOK_IF: return parse_if_stmt(p);
        case TOK_WHILE: return parse_while_stmt(p);
        case TOK_FOR: return parse_for_stmt(p);
        case TOK_BREAK: return parse_break_stmt(p);
        case TOK_CONTINUE: return parse_continue_stmt(p);
        case TOK_ASM: return parse_asm_stmt(p);
        case TOK_LBRACE: return parse_compound_stmt(p);

        // type keywords indicate variable declaration
        case TOK_CONST:
        case TOK_INT:
        case TOK_LONG:
        case TOK_CHAR:
        case TOK_FLOAT:
        case TOK_DOUBLE:
        case TOK_STRING_KW:
        case TOK_BOOL:
        case TOK_VOID: return parse_var_decl(p);

        default: return parse_expr_stmt(p);
    }
}

StmtNode* parse_return_stmt(Parser *p) {
    const SourceLocation loc = parser_current_token(p).location;
    parser_expect(p, TOK_RETURN);

    StmtNode *stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_RETURN;
    stmt->location = loc;

    // only parse expression if next token is NOT a semicolon
    if (parser_current_token(p).type != TOK_SEMI) {
        stmt->return_stmt.expr = parse_expression(p);
    } else {
        stmt->return_stmt.expr = NULL;
    }

    parser_expect(p, TOK_SEMI);
    return stmt;
}

StmtNode* parse_if_stmt(Parser *p) {
    const SourceLocation loc = parser_current_token(p).location;
    parser_expect(p, TOK_IF);

    StmtNode *stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_IF;
    stmt->location = loc;
    stmt->if_stmt.else_stmt = NULL;

    parser_expect(p, TOK_LPAREN);
    stmt->if_stmt.condition = parse_expression(p);
    parser_expect(p, TOK_RPAREN);

    stmt->if_stmt.then_stmt = parse_statement(p);

    if (parser_current_token(p).type == TOK_ELSE) {
        parser_advance(p);
        stmt->if_stmt.else_stmt = parse_statement(p);
    }

    return stmt;
}

StmtNode* parse_while_stmt(Parser *p) {
    const SourceLocation loc = parser_current_token(p).location;
    parser_expect(p, TOK_WHILE);
    parser_expect(p, TOK_LPAREN);
    ExprNode *cond = parse_expression(p);
    parser_expect(p, TOK_RPAREN);
    StmtNode *body = parse_statement(p);

    StmtNode *stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_WHILE;
    stmt->location = loc;
    stmt->while_stmt.condition = cond;
    stmt->while_stmt.body = body;
    return stmt;
}

StmtNode* parse_for_stmt(Parser *p) {
    const SourceLocation loc = parser_current_token(p).location;
    parser_expect(p, TOK_FOR);
    parser_expect(p, TOK_LPAREN);

    StmtNode *init = NULL;
    if (parser_current_token(p).type == TOK_SEMI) {
        parser_advance(p);
    } else {
        init = parse_statement(p);
    }

    // condition
    ExprNode *cond = NULL;
    if (parser_current_token(p).type != TOK_SEMI) {
        cond = parse_expression(p);
    }

    parser_expect(p, TOK_SEMI);

    // increment
    ExprNode *incr = NULL;
    if (parser_current_token(p).type != TOK_RPAREN) {
        incr = parse_expression(p);
    }

    parser_expect(p, TOK_RPAREN);

    StmtNode *body = parse_statement(p);

    StmtNode *stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_FOR;
    stmt->location = loc;
    stmt->for_stmt.init = init;
    stmt->for_stmt.condition = cond;
    stmt->for_stmt.increment = incr;
    stmt->for_stmt.body = body;
    return stmt;
}

StmtNode* parse_break_stmt(Parser *p) {
    const SourceLocation loc = parser_current_token(p).location;
    parser_expect(p, TOK_BREAK);
    parser_expect(p, TOK_SEMI);

    StmtNode *stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_BREAK;
    stmt->location = loc;
    return stmt;
}

StmtNode* parse_continue_stmt(Parser *p) {
    const SourceLocation loc = parser_current_token(p).location;
    parser_expect(p, TOK_CONTINUE);
    parser_expect(p, TOK_SEMI);

    StmtNode *stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_CONTINUE;
    stmt->location = loc;
    return stmt;
}

StmtNode* parse_var_decl(Parser *p) {
    int is_const = 0;
    if (parser_current_token(p).type == TOK_CONST) {
        is_const = 1;
        parser_advance(p);
    }

    const Token type_tok = parser_current_token(p);
    const SourceLocation loc = type_tok.location;
    const TypeKind type = token_to_typekind(p, type_tok.type);
    parser_advance(p);

    int array_size = 0;
    if (parser_current_token(p).type == TOK_LSQUARE) {
        parser_advance(p);
        if (parser_current_token(p).type == TOK_NUMBER) {
            array_size = atoi(parser_current_token(p).lexeme);
            parser_advance(p);
        } else {
            diag_error(p->diagnostics, parser_current_token(p).location, "Expected array size");
        }

        parser_expect(p, TOK_RSQUARE);
    }

    int pointer_level = 0;
    while (parser_current_token(p).type == TOK_ASTERISK) {
        pointer_level++;
        parser_advance(p);
    }

    const Token name_token = parser_current_token(p);
    parser_expect(p, TOK_IDENTIFIER);

    ExprNode *initializer = NULL;
    if (parser_current_token(p).type == TOK_ASSIGN) {
        parser_advance(p);
        initializer = parse_expression(p);
    }

    parser_expect(p, TOK_SEMI);

    StmtNode *stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_VAR_DECL;
    stmt->location = loc;
    stmt->var_decl.type = type;
    stmt->var_decl.pointer_level = pointer_level;
    stmt->var_decl.array_size = array_size;
    stmt->var_decl.name = strdup(name_token.lexeme);
    stmt->var_decl.initializer = initializer;
    stmt->var_decl.is_const = is_const;

    return stmt;
}

StmtNode* parse_expr_stmt(Parser *p) {
    const SourceLocation loc = parser_current_token(p).location;

    StmtNode *stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_EXPR;
    stmt->location = loc;
    stmt->expr_stmt.expr = parse_expression(p);

    parser_expect(p, TOK_SEMI);
    return stmt;
}

StmtNode* parse_compound_stmt(Parser *p) {
    const SourceLocation loc = parser_current_token(p).location;
    parser_expect(p, TOK_LBRACE);

    Vector stmts = create_vector(8, sizeof(StmtNode*));

    while (parser_current_token(p).type != TOK_RBRACE && parser_current_token(p).type != TOK_EOF) {
        StmtNode *s = parse_statement(p);
        vector_push(&stmts, &s);
    }

    parser_expect(p, TOK_RBRACE);

    StmtNode *stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_COMPOUND;
    stmt->location = loc;
    stmt->compound.stmts = (StmtNode**)stmts.elements;
    stmt->compound.count = stmts.length;

    return stmt;
}

StmtNode* parse_asm_stmt(Parser *p) {
    const SourceLocation loc = parser_current_token(p).location;
    parser_expect(p, TOK_ASM);
    parser_expect(p, TOK_LPAREN);

    if (parser_current_token(p).type != TOK_STRING_LITERAL) {
        diag_error(p->diagnostics, parser_current_token(p).location, "Expected assembly string literal after 'asm('");
    }

    char *asm_code = strdup(parser_current_token(p).lexeme);
    parser_advance(p);

    Vector outputs = create_vector(4, sizeof(ExprNode*));
    Vector output_constraints = create_vector(4, sizeof(char*));
    Vector inputs = create_vector(4, sizeof(ExprNode*));
    Vector input_constraints = create_vector(4, sizeof(char*));
    Vector clobbers = create_vector(4, sizeof(char*));

    // parse outputs (if colon present)
    if (parser_current_token(p).type == TOK_COLON) {
        parser_advance(p);

        if (parser_current_token(p).type != TOK_COLON && parser_current_token(p).type != TOK_RPAREN) {
            while (parser_current_token(p).type != TOK_COLON && parser_current_token(p).type != TOK_RPAREN && parser_current_token(p).type != TOK_EOF) {
                if (parser_current_token(p).type != TOK_STRING_LITERAL) {
                    diag_error(p->diagnostics, parser_current_token(p).location, "Expected output constraint");
                }

                char *constraint = strdup(parser_current_token(p).lexeme);
                parser_advance(p);

                parser_expect(p, TOK_LPAREN);
                ExprNode *output = parse_expression(p);
                vector_push(&outputs, &output);
                vector_push(&output_constraints, &constraint);
                parser_expect(p, TOK_RPAREN);

                if (parser_current_token(p).type == TOK_COMMA) {
                    parser_advance(p);
                } else {
                    break;
                }
            }
        }
    }

    // parse inputs (if second colon present)
    if (parser_current_token(p).type == TOK_COLON) {
        parser_advance(p);

        if (parser_current_token(p).type != TOK_COLON && parser_current_token(p).type != TOK_RPAREN) {
            while (parser_current_token(p).type != TOK_COLON && parser_current_token(p).type != TOK_RPAREN && parser_current_token(p).type != TOK_EOF) {
                if (parser_current_token(p).type != TOK_STRING_LITERAL) {
                    diag_error(p->diagnostics, parser_current_token(p).location, "Expected input constraint");
                }

                char *constraint = strdup(parser_current_token(p).lexeme);
                parser_advance(p);

                parser_expect(p, TOK_LPAREN);
                ExprNode *input = parse_expression(p);
                vector_push(&inputs, &input);
                vector_push(&input_constraints, &constraint);
                parser_expect(p, TOK_RPAREN);

                if (parser_current_token(p).type == TOK_COMMA) {
                    parser_advance(p);
                } else {
                    break;
                }
            }
        }
    }

    // parse clobbers (if third colon present)
    if (parser_current_token(p).type == TOK_COLON) {
        parser_advance(p);

        while (parser_current_token(p).type != TOK_RPAREN && parser_current_token(p).type != TOK_EOF) {
            if (parser_current_token(p).type != TOK_STRING_LITERAL) {
                diag_error(p->diagnostics, parser_current_token(p).location, "Expected clobber");
            }

            char *clobber = strdup(parser_current_token(p).lexeme);
            vector_push(&clobbers, &clobber);
            parser_advance(p);

            if (parser_current_token(p).type == TOK_COMMA) {
                parser_advance(p);
            } else {
                break;
            }
        }
    }

    parser_expect(p, TOK_RPAREN);
    parser_expect(p, TOK_SEMI);

    StmtNode *stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_ASM;
    stmt->location = loc;
    stmt->asm_stmt.assembly_code = asm_code;
    stmt->asm_stmt.outputs = (ExprNode**)outputs.elements;
    stmt->asm_stmt.output_count = outputs.length;
    stmt->asm_stmt.output_constraints = (char**)output_constraints.elements;
    stmt->asm_stmt.inputs = (ExprNode**)inputs.elements;
    stmt->asm_stmt.input_count = inputs.length;
    stmt->asm_stmt.input_constraints = (char**)input_constraints.elements;
    stmt->asm_stmt.clobbers = (char**)clobbers.elements;
    stmt->asm_stmt.clobber_count = clobbers.length;

    return stmt;
}