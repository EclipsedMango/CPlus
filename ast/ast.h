#ifndef C__AST_H
#define C__AST_H

#include <stddef.h>

#include "../common.h"

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
    BIN_NOT_EQUAL,

    // assignment
    BIN_ASSIGN,

    // logical operators
    BIN_LOGICAL_AND,
    BIN_LOGICAL_OR,
} BinaryOp;

// needs to match lexer
typedef enum TypeKind {
    TYPE_INT,
    TYPE_LONG,
    TYPE_CHAR,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_BOOLEAN,
    TYPE_VOID
} TypeKind;

// needs to match lexer
typedef enum UnaryOp {
    UNARY_NEG,
    UNARY_NOT,
    UNARY_DEREF,
    UNARY_ADDR_OF
} UnaryOp;

typedef struct ExprNode {
    enum {
        EXPR_NUMBER,
        EXPR_STRING_LITERAL,
        EXPR_VAR,
        EXPR_BINOP,
        EXPR_UNARY,
        EXPR_CALL,
        EXPR_ARRAY_INDEX,
    } kind;
    SourceLocation location;
    TypeKind type;
    int pointer_level;
    union {
        char *text;
        struct {
            struct ExprNode *left;
            struct ExprNode *right;
            BinaryOp op;
        } binop;
        struct {
            UnaryOp op;
            struct ExprNode *operand;
        } unary;
        struct {
            char *function_name;
            struct ExprNode **args;
            int arg_count;
        } call;
        struct {
            struct ExprNode *array;
            struct ExprNode *index;
        } array_index;
    };
} ExprNode;

typedef struct StmtNode {
    enum {
        STMT_RETURN,
        STMT_IF,
        STMT_WHILE,
        STMT_FOR,
        STMT_BREAK,
        STMT_CONTINUE,
        STMT_VAR_DECL,
        STMT_EXPR,
        STMT_COMPOUND,
        STMT_ASM,
    } kind;
    SourceLocation location;
    union {
        struct {
            ExprNode *expr;
        } return_stmt;
        struct {
            ExprNode *condition;
            struct StmtNode *then_stmt;
            struct StmtNode *else_stmt;
        } if_stmt;
        struct {
            ExprNode *condition;
            struct StmtNode *body;
        } while_stmt;
        struct {
            struct StmtNode *init; // can be VarDecl or ExprStmt
            ExprNode *condition;   // can be NULL
            ExprNode *increment;   // can be NULL
            struct StmtNode *body;
        } for_stmt;
        struct {
            TypeKind type;
            int pointer_level;
            int array_size;
            char *name;
            ExprNode *initializer;  // nullptr if no initializer
            int is_const;
        } var_decl;
        struct {
            ExprNode *expr;
        } expr_stmt;
        struct {
            struct StmtNode **stmts;
            int count;
        } compound;
        struct {
            char *assembly_code;
            ExprNode **outputs;
            size_t output_count;
            char **output_constraints;
            ExprNode **inputs;
            size_t input_count;
            char **input_constraints;
            char **clobbers;
            size_t clobber_count;
        } asm_stmt;
    };
} StmtNode;

typedef struct GlobalVarNode {
    TypeKind kind;
    int pointer_level;
    int array_size;
    char *name;
    ExprNode *initializer;
    bool is_const;
    SourceLocation location;
} GlobalVarNode;

typedef struct ParamNode {
    TypeKind type;
    int pointer_level;
    SourceLocation location;
    char *name;
    int is_const;
} ParamNode;

typedef struct FunctionNode {
    char *name;
    TypeKind return_type;
    int return_pointer_level;
    SourceLocation location;
    ParamNode *params;
    int param_count;
    StmtNode *body;
} FunctionNode;

typedef struct ProgramNode {
    FunctionNode **functions;
    int function_count;
    GlobalVarNode **globals;
    int global_count;
} ProgramNode;

#endif //C__AST_H