#include "token_utils.h"
#include <stdio.h>
#include <stdlib.h>

static Token tokens[TOKEN_BUFFER_SIZE];
static FILE *current_file = nullptr;

/*
 * fill the token buffer by shifting left and
 * pulling a new token from the lexer.
 */
static void update_token_buffer(void) {
    for (int i = 0; i < TOKEN_BUFFER_SIZE - 1; i++) {
        tokens[i] = tokens[i + 1];
    }

    tokens[TOKEN_BUFFER_SIZE - 1] = next_token(current_file);
}


// init token stream for a file
void set_current_file(FILE *f) {
    current_file = f;

    // prime the token buffer
    for (int i = 0; i < TOKEN_BUFFER_SIZE; i++) {
        tokens[i] = next_token(current_file);
    }
}

FILE *get_current_file(void) {
    return current_file;
}

Token current_token(void) {
    return tokens[0];
}

// peek ahead n tokens (0 = current)
Token peek_token(const int n) {
    if (n < 0 || n >= TOKEN_BUFFER_SIZE) {
        fprintf(stderr, "peek_token: index out of bounds\n");
        exit(1);
    }
    return tokens[n];
}

// consume the current token
void advance(void) {
    update_token_buffer();
}


// expect a token type or error
void expect(const TokenType type) {
    if (current_token().type != type) {
        fprintf(stderr, "parser error: expected token %d, got %d\n", type, current_token().type);
        exit(1);
    }
    advance();
}