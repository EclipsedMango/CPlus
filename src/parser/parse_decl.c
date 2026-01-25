#include "parser_internal.h"
#include <stdlib.h>
#include <string.h>

GlobalVarNode* parse_global_var(Parser *p) {
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

    // parse initializer if present
    ExprNode *initializer = NULL;
    if (parser_current_token(p).type == TOK_ASSIGN) {
        parser_advance(p);
        initializer = parse_expression(p);
    }

    parser_expect(p, TOK_SEMI);

    GlobalVarNode *global = malloc(sizeof(GlobalVarNode));
    global->kind = type;
    global->location = loc;
    global->name = strdup(name_token.lexeme);
    global->pointer_level = pointer_level;
    global->array_size = array_size;
    global->initializer = initializer;
    global->is_const = is_const;

    return global;
}

void parse_parameter_list(Parser *p, ParamNode **params_out, int *count_out) {
    parser_expect(p, TOK_LPAREN);

    if (parser_current_token(p).type == TOK_RPAREN) {
        *params_out = NULL;
        *count_out = 0;
        parser_advance(p);
        return;
    }

    Vector params = create_vector(4, sizeof(ParamNode));

    do {
        int param_is_const = 0;
        if (parser_current_token(p).type == TOK_CONST) {
            param_is_const = 1;
            parser_advance(p);
        }

        const Token type_tok = parser_current_token(p);
        const TypeKind type = token_to_typekind(p, type_tok.type);
        parser_advance(p);

        int pointer_level = 0;
        while (parser_current_token(p).type == TOK_ASTERISK) {
            pointer_level++;
            parser_advance(p);
        }

        const Token name_tok = parser_current_token(p);
        parser_expect(p, TOK_IDENTIFIER);

        ParamNode param = {
            .type = type,
            .pointer_level = pointer_level,
            .name = strdup(name_tok.lexeme),
            .is_const = param_is_const,
            .location = type_tok.location
        };

        vector_push(&params, &param);

        // check for more params
        if (parser_current_token(p).type == TOK_COMMA) {
            parser_advance(p);
        } else {
            break;
        }
    } while (parser_current_token(p).type != TOK_RPAREN);

    parser_expect(p, TOK_RPAREN);

    *params_out = (ParamNode*)params.elements;
    *count_out = params.length;
}

FunctionNode* parse_function(Parser *p) {
    const Token type_token = parser_current_token(p);
    const TypeKind return_type = token_to_typekind(p, type_token.type);
    parser_advance(p);

    int return_pointer_level = 0;
    while (parser_current_token(p).type == TOK_ASTERISK) {
        return_pointer_level++;
        parser_advance(p);
    }

    const Token name_token = parser_current_token(p);
    parser_expect(p, TOK_IDENTIFIER);

    ParamNode *params;
    int param_count = 0;
    parse_parameter_list(p, &params, &param_count);

    StmtNode *body = parse_compound_stmt(p);

    FunctionNode *func = malloc(sizeof(FunctionNode));
    func->name = strdup(name_token.lexeme);
    func->return_type = return_type;
    func->return_pointer_level = return_pointer_level;
    func->params = params;
    func->param_count = param_count;
    func->body = body;
    func->location = type_token.location;

    return func;
}
