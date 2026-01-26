#ifndef C__PARSER_INTERNAL_H
#define C__PARSER_INTERNAL_H

#include "parser.h"
#include "../lexer/token.h"
#include "../util/vector.h"

#define TOKEN_BUFFER_SIZE 50

// internal state
struct Parser {
    Lexer *lexer;
    DiagnosticEngine *diagnostics;
    Token token_buffer[TOKEN_BUFFER_SIZE];
    Vector retired_tokens;
};

void parser_init_token_buffer(Parser *p);
void parser_update_token_buffer(Parser *p);

Token parser_current_token(const Parser *p);
Token parser_peek_token(const Parser *p, int n);
void parser_advance(Parser *p);
void parser_expect(Parser *p, TokenType type);

TypeKind token_to_typekind(const Parser *p, TokenType token);

// forward decl, implemented in different files but shared internally

// from parse_expr.c
ExprNode* parse_expression(Parser *p);
ExprNode* parse_assignment(Parser *p);
ExprNode* parse_logical_or(Parser *p);
ExprNode* parse_logical_and(Parser *p);
ExprNode* parse_equality(Parser *p);
ExprNode* parse_relational(Parser *p);
ExprNode* parse_additive(Parser *p);
ExprNode* parse_term(Parser *p);
ExprNode* parse_unary(Parser *p);
ExprNode* parse_postfix(Parser *p);

// from parse_stmt.c
StmtNode* parse_statement(Parser *p);
StmtNode* parse_return_stmt(Parser *p);
StmtNode* parse_if_stmt(Parser *p);
StmtNode* parse_while_stmt(Parser *p);
StmtNode* parse_for_stmt(Parser *p);
StmtNode* parse_break_stmt(Parser *p);
StmtNode* parse_continue_stmt(Parser *p);
StmtNode* parse_asm_stmt(Parser *p);
StmtNode* parse_var_decl(Parser *p);
StmtNode* parse_expr_stmt(Parser *p);
StmtNode* parse_compound_stmt(Parser *p);

// from parse_decl.c
FunctionNode* parse_function(Parser *p);
GlobalVarNode* parse_global_var(Parser *p);
void parse_parameter_list(Parser *p, ParamNode **params_out, int *count_out);

#endif //C__PARSER_INTERNAL_H