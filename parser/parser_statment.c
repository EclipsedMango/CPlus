#include "token_utils.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>

static TypeKind token_to_typekind(const TokenType token) {
    switch (token) {
        case TOK_INT: return TYPE_INT;
        case TOK_LONG: return TYPE_LONG;
        case TOK_CHAR: return TYPE_CHAR;
        case TOK_FLOAT: return TYPE_FLOAT;
        case TOK_DOUBLE: return TYPE_DOUBLE;
        case TOK_STRING_KW: return TYPE_STRING;
        case TOK_BOOL: return TYPE_BOOLEAN;
        case TOK_VOID: return TYPE_VOID;
        default: {
            fprintf(stderr, "invalid type token: %d\n", token);
            exit(1);
        }
    }
}

StmtNode* parse_return_stmt(void) {
    const SourceLocation loc = current_token().location;
    expect(TOK_RETURN);

    StmtNode* stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_RETURN;
    stmt->location = loc;

    // only parse expression if next token is NOT a semicolon
    if (current_token().type != TOK_SEMI) {
        stmt->return_stmt.expr = parse_expression();
    } else {
        stmt->return_stmt.expr = NULL;
    }

    expect(TOK_SEMI);
    return stmt;
}

StmtNode* parse_if_stmt(void) {
    const SourceLocation loc = current_token().location;
    expect(TOK_IF);

    StmtNode* stmt = malloc(sizeof *stmt);
    stmt->kind = STMT_IF;
    stmt->location = loc;
    stmt->if_stmt.else_stmt = NULL;

    expect(TOK_LPAREN);
    stmt->if_stmt.condition = parse_expression();
    expect(TOK_RPAREN);

    stmt->if_stmt.then_stmt = parse_statement();

    if (current_token().type == TOK_ELSE) {
        expect(TOK_ELSE);
        stmt->if_stmt.else_stmt = parse_statement();
    }

    return stmt;
}

StmtNode* parse_asm_stmt(void) {
    const SourceLocation loc = current_token().location;
    expect(TOK_ASM);
    expect(TOK_LPAREN);

    // expect a string literal containing assembly code
    if (current_token().type != TOK_STRING_LITERAL) {
        report_error(current_token().location, "Expected assembly string literal after 'asm('");
    }

    char *asm_code = strdup(current_token().lexeme);
    advance();

    Vector inputs = create_vector(4, sizeof(ExprNode*));
    Vector constraints = create_vector(4, sizeof(char*));

    while (current_token().type == TOK_COMMA) {
        advance();
        ExprNode *input = parse_expression();
        vector_push(&inputs, &input);

        // use "r" constraint
        char *constraint = strdup("r");
        vector_push(&constraints, &constraint);
    }

    expect(TOK_RPAREN);
    expect(TOK_SEMI);

    StmtNode *stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_ASM;
    stmt->location = loc;
    stmt->asm_stmt.assembly_code = asm_code;
    stmt->asm_stmt.inputs = inputs.elements;
    stmt->asm_stmt.input_count = inputs.length;
    stmt->asm_stmt.constraints = constraints.elements;

    return stmt;
}

StmtNode* parse_var_decl(void) {
    // read and convert type
    const Token type_tok = current_token();
    const SourceLocation loc = type_tok.location;
    const TypeKind type = token_to_typekind(type_tok.type);
    advance();

    int pointer_level = 0;
    while (current_token().type == TOK_ASTERISK) {
        pointer_level++;
        advance();
    }

    // read identifier
    const Token name_token = current_token();
    expect(TOK_IDENTIFIER);

    // check for initializer
    ExprNode *initializer = NULL;
    if (current_token().type == TOK_ASSIGN) {
        advance();
        initializer = parse_expression();
    }

    expect(TOK_SEMI);

    // build AST node
    StmtNode *stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_VAR_DECL;
    stmt->location = loc;
    stmt->var_decl.type = type;
    stmt->var_decl.pointer_level = pointer_level;
    stmt->var_decl.name = strdup(name_token.lexeme);
    stmt->var_decl.initializer = initializer;

    return stmt;
}

StmtNode* parse_expr_stmt(void) {
    const SourceLocation loc = current_token().location;

    StmtNode* stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_EXPR;
    stmt->location = loc;
    stmt->expr_stmt.expr = parse_expression();

    expect(TOK_SEMI);
    return stmt;
}

StmtNode* parse_compound_stmt(void) {
    const SourceLocation loc = current_token().location;
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
    stmt->location = loc;
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

    // if statement
    if (t.type == TOK_IF) {
        return parse_if_stmt();
    }

    // asm statement
    if (t.type == TOK_ASM) {
        return parse_asm_stmt();
    }

    // variable declaration (starts with type)
    if (t.type == TOK_INT || t.type == TOK_LONG || t.type == TOK_FLOAT || t.type == TOK_DOUBLE || t.type == TOK_STRING_KW || t.type == TOK_BOOL) {
        return parse_var_decl();
    }

    // compound statement
    if (t.type == TOK_LBRACE) {
        return parse_compound_stmt();
    }

    // assume expression statement
    return parse_expr_stmt();
}