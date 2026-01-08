#ifndef C__AST_H
#define C__AST_H

// needs to match lexer
typedef enum BinaryOp {
    // arithmetic
    BIN_ADD,
    BIN_SUB,
    BIN_MUL,
    BIN_DIV,
    BIN_MOD,

    // comparison
    BIN_EQUAL,
    BIN_GREATER,
    BIN_LESS,
    BIN_GREATER_EQ,
    BIN_LESS_EQ,

    // assignment
    BIN_ASSIGN
} BinaryOp;

// needs to match lexer
typedef enum TypeKind {
    TYPE_INT,
    TYPE_LONG,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_VOID
} TypeKind;

// needs to match lexer
typedef enum UnaryOp {
    UNARY_NEG,
    UNARY_NOT
} UnaryOp;

typedef struct ExprNode {
    enum {
        EXPR_NUMBER,
        EXPR_STRING_LITERAL,
        EXPR_VAR,
        EXPR_BINOP,
        EXPR_CALL
    } kind;
    union {
        char *text;
        struct {
            struct ExprNode *left;
            struct ExprNode *right;
            BinaryOp op;
        } binop;
        struct {
            char *function_name;
            struct ExprNode **args;
            int arg_count;
        } call;
    };
} ExprNode;

typedef struct StmtNode {
    enum {
        STMT_RETURN,
        STMT_VAR_DECL,
        STMT_EXPR,
        STMT_COMPOUND,
    } kind;
    union {
        struct {
            ExprNode *expr;
        } return_stmt;

        struct {
            TypeKind type;
            char *name;
            ExprNode *initializer;  // NULL if no initializer
        } var_decl;

        struct {
            ExprNode *expr;
        } expr_stmt;

        struct {
            struct StmtNode **stmts;
            int count;
        } compound;
    };
} StmtNode;

typedef struct ParamNode {
    TypeKind type;
    char *name;
} ParamNode;

typedef struct FunctionNode {
    char *name;
    TypeKind return_type;
    ParamNode *params;
    int param_count;
    StmtNode *body;
} FunctionNode;

typedef struct ProgramNode {
    FunctionNode **functions;
    int function_count;
} ProgramNode;

#endif //C__AST_H