#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "token_utils.h"

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
            fprintf(stderr, "invalid type token: %s\n", token_type_to_string(token));
            exit(1);
        }
    }
}

ProgramNode* parse_program(void) {
    Vector global_vars = create_vector(16, sizeof(GlobalVarNode*));
    Vector functions = create_vector(16, sizeof(FunctionNode*));

    // parse all functions and global vars until EOF
    while (current_token().type != TOK_EOF) {
        if (current_token().type == TOK_CONST) {
            GlobalVarNode *gl = parse_global_var();
            vector_push(&global_vars, &gl);
        } else {
            int lookahead_pos = 1;

            if (peek_token(lookahead_pos).type == TOK_LSQUARE) {
                lookahead_pos++;
                while (peek_token(lookahead_pos).type != TOK_RSQUARE) {
                    lookahead_pos++;
                }

                lookahead_pos++;
            }

            while (peek_token(lookahead_pos).type == TOK_ASTERISK) {
                lookahead_pos++;
            }

            lookahead_pos++;

            if (peek_token(lookahead_pos).type == TOK_LPAREN) {
                FunctionNode *fn = parse_function();
                vector_push(&functions, &fn);
            } else if (peek_token(lookahead_pos).type == TOK_SEMI || peek_token(lookahead_pos).type == TOK_ASSIGN) {
                GlobalVarNode *gl = parse_global_var();
                vector_push(&global_vars, &gl);
            } else {
                report_error(peek_token(lookahead_pos).location,
                    "Expected '(' (for function) or ';' or '=' (for global variable), got '%s'",
                    token_type_to_string(peek_token(lookahead_pos).type)
                );
            }
        }
    }

    ProgramNode *program = malloc(sizeof(ProgramNode));
    program->functions = functions.elements;
    program->function_count = functions.length;
    program->globals = global_vars.elements;
    program->global_count = global_vars.length;

    return program;
}

GlobalVarNode *parse_global_var(void) {
    int is_const = 0;
    if (current_token().type == TOK_CONST) {
        is_const = 1;
        advance();
    }

    const Token type_tok = current_token();
    const SourceLocation loc = type_tok.location;
    const TypeKind type = token_to_typekind(type_tok.type);
    advance();

    int array_size = 0;
    if (current_token().type == TOK_LSQUARE) {
        advance();
        if (current_token().type == TOK_NUMBER) {
            array_size = atoi(current_token().lexeme);
            advance();
        } else {
            report_error(current_token().location, "Expected array size");
        }
        expect(TOK_RSQUARE);
    }

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
