#include "token_utils.h"
#include "parser.h"
#include <stdlib.h>

StmtNode* parse_return_stmt(void) {
    expect(TOK_RETURN);

    StmtNode* stmt = malloc(sizeof(StmtNode));
    stmt->kind = STMT_RETURN;

    stmt->return_stmt.expr = parse_expression();

    expect(TOK_SEMI);
    return stmt;
}