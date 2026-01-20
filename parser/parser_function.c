#include "token_utils.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static TypeKind token_to_typekind(const TokenType token) {
    switch (token) {
        case TOK_INT: return TYPE_INT;
        case TOK_LONG: return TYPE_LONG;
        case TOK_FLOAT: return TYPE_FLOAT;
        case TOK_DOUBLE: return TYPE_DOUBLE;
        case TOK_STRING_KW: return TYPE_STRING;
        default: {
            fprintf(stderr, "invalid type token: %d\n", token);
            exit(1);
        }
    }
}

static void parse_parameter_list(ParamNode **params_out, int *count_out) {
    expect(TOK_LPAREN);

    // handle empty parameter list
    if (current_token().type == TOK_RPAREN) {
        *params_out = NULL;
        *count_out = 0;
        advance();
        return;
    }

    // allocate parameter array
    Vector params = create_vector(4, sizeof(ParamNode));

    // parse first parameter
    do {
        const Token type_tok = current_token();
        const TypeKind type = token_to_typekind(type_tok.type);
        advance();

        int point_level = 0;
        while (current_token().type == TOK_ASTERISK) {
            point_level++;
            advance();
        }

        const Token name_tok = current_token();
        expect(TOK_IDENTIFIER);

        ParamNode p = {
            .type = type,
            .pointer_level = point_level,
            .name = strdup(name_tok.lexeme),
            .location = type_tok.location
        };
        vector_push(&params, &p);

        // check for more parameters
        if (current_token().type == TOK_COMMA) {
            advance();
        } else {
            break;
        }
    } while (current_token().type != TOK_RPAREN);

    expect(TOK_RPAREN);

    *params_out = params.elements;
    *count_out = params.length;
}

FunctionNode* parse_function(void) {
    // return type
    const Token type_token = current_token();
    const TypeKind return_type = token_to_typekind(type_token.type);
    advance();

    const Token name_token = current_token();
    expect(TOK_IDENTIFIER);

    // parameters
    ParamNode *params;
    int param_count = 0;
    parse_parameter_list(&params, &param_count);

    // body
    StmtNode *body = parse_compound_stmt();

    FunctionNode* func = malloc(sizeof(FunctionNode));
    func->name = strdup(name_token.lexeme);
    func->return_type = return_type;
    func->params = params;
    func->param_count = param_count;
    func->body = body;

    return func;
}