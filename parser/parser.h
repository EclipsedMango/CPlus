#ifndef C__PARSER_H
#define C__PARSER_H

#include "../ast/ast.h"

// entry
ProgramNode *parse_program(void);
GlobalVarNode *parse_global_var(void);

// grammar
FunctionNode *parse_function(void);
ExprNode     *parse_expression(void);

StmtNode *parse_statement(void);
StmtNode *parse_return_stmt(void);
StmtNode *parse_var_decl(void);
StmtNode *parse_expr_stmt(void);
StmtNode *parse_compound_stmt(void);

#endif //C__PARSER_H