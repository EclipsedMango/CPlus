#include "token_utils.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

FunctionNode* parse_function(void) {
    // return type
    const Token type_token = current_token();
    if (type_token.type != TOK_INT && type_token.type != TOK_FLOAT && type_token.type != TOK_DOUBLE) {
        fprintf(stderr, "expected function return type\n");
        exit(1);
    }

    advance();

    // function name
    const Token name_token = current_token();
    expect(TOK_IDENTIFIER);

    // parameters (empty for now)
    expect(TOK_LPAREN);
    expect(TOK_RPAREN);

    // body
    expect(TOK_LBRACE);

    StmtNode **body = malloc(sizeof(StmtNode*) * 16); // max 16 for now
    int count = 0;

    while (current_token().type != TOK_RBRACE && current_token().type != TOK_EOF) {
        if (current_token().type == TOK_RETURN) {
            body[count++] = parse_return_stmt();
        } else {
            fprintf(stderr, "unexpected statement in function body\n");
            exit(1);
        }
    }

    expect(TOK_RBRACE);

    FunctionNode* func = malloc(sizeof(FunctionNode));
    func->name = strdup(name_token.lexeme);
    func->return_type = type_token.type;
    func->body = body;
    func->body_count = count;

    return func;
}