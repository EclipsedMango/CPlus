#ifndef C__PARSER_H
#define C__PARSER_H

#include "../ast/ast.h"

// entry
FunctionNode *parse_program(void);

// grammar
FunctionNode *parse_function(void);
StmtNode     *parse_statement(void);
StmtNode     *parse_return_stmt(void);
ExprNode     *parse_expression(void);

#endif //C__PARSER_H