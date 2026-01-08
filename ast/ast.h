#ifndef C__AST_H
#define C__AST_H

#include "../lexer/lexer.h"

typedef struct ExprNode {
    enum {
        EXPR_NUMBER,
        EXPR_VAR,
        EXPR_BINOP
    } kind;
    union {
        float value;              // EXPR_NUMBER
        char *name;               // EXPR_VAR
        struct {
            struct ExprNode *left;
            struct ExprNode *right;
            TokenType op;         // +, -, *, /
        } binop;                  // EXPR_BINOP
    };
} ExprNode;

typedef struct StmtNode {
    enum {
        STMT_RETURN,
        STMT_VAR_DECL,
        STMT_EXPR
    } kind;
    union {
        struct {
            ExprNode *expr;
        } return_stmt; // return <expr>;
    };
} StmtNode;

typedef struct FunctionNode {
    char *name;
    TokenType return_type;
    StmtNode **body;
    int body_count;
} FunctionNode;

#endif //C__AST_H