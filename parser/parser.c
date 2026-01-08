#include "parser.h"

#include <stdlib.h>

#include "../lexer/lexer.h"

int tokensSize = 5;
Token tokens[5];

FILE* file;

void update_current_token(void) {
    for (int i = tokensSize - 1; i > 0; i--) {
        tokens[i] = tokens[i - 1];
    }

    tokens[0] = next_token(file);
}

Token current_token(void) {
    return tokens[0];
}

Token get_previous_token(const int index) {
    if (index < 0 || index >= tokensSize) {
        printf("index out of bounds\n");
        exit(1);
    }

    return tokens[index];
}

FILE* get_current_file(void) {
    return file;
}

void set_current_file(FILE* f) {
    file = f;

    // initialize token buffer
    for (int i = 0; i < tokensSize; i++) {
        tokens[i] = (Token){ .type = TOK_EOF, .value = 0 };
    }
}

void advance(void) {
    update_current_token();
}

void expect(const TokenType type) {
    if (current_token().type != type) {
        fprintf(stderr, "expected token %d, got %d\n", type, current_token().type);
        exit(1);
    }

    advance();
}

int parse_return(void) {
    expect(TOK_RETURN);

    if (current_token().type != TOK_NUMBER) {
        fprintf(stderr, "expected number after return\n");
        exit(1);
    }

    const int value = (int) current_token().value;
    advance();

    expect(TOK_SEMI);
    return value;
}

int parse_program(void) {
    expect(TOK_INT);
    expect(TOK_IDENTIFIER);
    expect(TOK_LPAREN);
    expect(TOK_RPAREN);
    expect(TOK_LBRACE);
    const int result = parse_return();
    expect(TOK_RBRACE);
    expect(TOK_EOF);
    return result;
}