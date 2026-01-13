#include "token_utils.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>

static TypeKind token_to_typekind(const TokenType token) {
    switch (token) {
        case TOK_INT: return TYPE_INT;
        case TOK_LONG: return TYPE_LONG;
        case TOK_FLOAT: return TYPE_FLOAT;
        case TOK_DOUBLE: return TYPE_DOUBLE;
        case TOK_STRING_KW: return TYPE_STRING;
        default:
            fprintf(stderr, "invalid type token: %d\n", token);
            exit(1);
    }
}

StmtNode* parse_return_stmt(void) {
    expect(TOK_RETURN);

    StmtNode* stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_RETURN;
    stmt->return_stmt.expr = parse_expression();

    expect(TOK_SEMI);
    return stmt;
}

StmtNode* parse_var_decl(void) {
    // read and convert type
    const Token type_tok = current_token();
    const TypeKind type = token_to_typekind(type_tok.type);
    advance();

    // read identifier
    const Token name_token = current_token();
    expect(TOK_IDENTIFIER);

    // check for initializer
    ExprNode *initializer = nullptr;
    if (current_token().type == TOK_ASSIGN) {
        advance();
        initializer = parse_expression();
    }

    expect(TOK_SEMI);

    // build AST node
    StmtNode *stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_VAR_DECL;
    stmt->var_decl.type = type;
    stmt->var_decl.name = strdup(name_token.lexeme);
    stmt->var_decl.initializer = initializer;

    return stmt;
}

StmtNode* parse_expr_stmt(void) {
    StmtNode* stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_EXPR;
    stmt->expr_stmt.expr = parse_expression();

    expect(TOK_SEMI);
    return stmt;
}

StmtNode* parse_compound_stmt(void) {
    expect(TOK_LBRACE);

    // allocate statement list (grow dynamically or use fixed size)
    Vector stmts = create_vector(8, sizeof(StmtNode*));

    while (current_token().type != TOK_RBRACE && current_token().type != TOK_EOF) {
        StmtNode *s = parse_statement();
        vector_push(&stmts, &s);
    }

    expect(TOK_RBRACE);

    StmtNode* stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_COMPOUND;
    stmt->compound.stmts = stmts.elements;
    stmt->compound.count = stmts.length;

    return stmt;
}

StmtNode* parse_statement(void) {
    const Token t = current_token();

    // return statement
    if (t.type == TOK_RETURN) {
        return parse_return_stmt();
    }

    // variable declaration (starts with type)
    if (t.type == TOK_INT || t.type == TOK_LONG || t.type == TOK_FLOAT || t.type == TOK_DOUBLE || t.type == TOK_STRING_KW) {
        return parse_var_decl();
    }

    // compound statement
    if (t.type == TOK_LBRACE) {
        return parse_compound_stmt();
    }

    // assume expression statement
    return parse_expr_stmt();
}