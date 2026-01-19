
#include "semantic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_arithmetic_op(const BinaryOp op) {
    return op == BIN_ADD || op == BIN_SUB || op == BIN_MUL || op == BIN_DIV || op == BIN_MOD;
}

static int is_comparison_op(const BinaryOp op) {
    return op == BIN_EQUAL || op == BIN_GREATER || op == BIN_LESS || op == BIN_GREATER_EQ || op == BIN_LESS_EQ;
}

static int is_assignment_op(const BinaryOp op) {
    return op == BIN_ASSIGN;
}

static int is_numeric_type(const TypeKind t) {
    return t == TYPE_INT || t == TYPE_LONG || t == TYPE_FLOAT || t == TYPE_DOUBLE;
}


Scope* scope_create(Scope* parent) {
    Scope* new_scope = malloc(sizeof(Scope));
    new_scope->parent = parent;
    new_scope->symbol_count = 0;
    new_scope->symbols = malloc(sizeof(Symbol*) * 16);
    return new_scope;
}

void scope_destroy(Scope* scope) {
    for (int i = 0; i < scope->symbol_count; i++) {
        free(scope->symbols[i]->name);
        free(scope->symbols[i]);
    }

    free(scope->symbols);
    free(scope);
}

void scope_add_symbol(Scope* scope, Symbol* symbol) {
    if (scope->symbol_count >= 16) {
        fprintf(stderr, "too many symbols (max 16)\n");
        exit(1);
    }

    scope->symbols[scope->symbol_count] = symbol;
    scope->symbol_count++;
}

Symbol* scope_lookup(const Scope* scope, const char* name) {
    for (int i = 0; i < scope->symbol_count; i++) {
        if (strcmp(scope->symbols[i]->name, name) == 0) {
            return scope->symbols[i];
        }
    }

    return NULL;
}

Symbol* scope_lookup_recursive(const Scope* scope, const char* name) {
    // try current scope first
    Symbol* sym = scope_lookup(scope, name);
    if (sym != NULL) {
        return sym;
    }

    // if not found and parent exists, search parent
    if (scope->parent != NULL) {
        return scope_lookup_recursive(scope->parent, name);
    }

    // not found anywhere
    return NULL;
}

static void analyze_expression(ExprNode* expr, Scope* scope) {
    switch (expr->kind) {
        case EXPR_NUMBER:
            expr->type = TYPE_INT;
            break;
        case EXPR_STRING_LITERAL:
            expr->type = TYPE_STRING;
            break;
        case EXPR_VAR: {
            // look up variable
            Symbol *sym = scope_lookup_recursive(scope, expr->text);
            if (!sym) {
                fprintf(stderr, "Error: undefined variable '%s'\n", expr->text);
                exit(1);
            }

            expr->type = sym->type;
            break;
        }
        case EXPR_BINOP:
            analyze_expression(expr->binop.left, scope);
            analyze_expression(expr->binop.right, scope);

            const TypeKind lhs = expr->binop.left->type;
            const TypeKind rhs = expr->binop.right->type;
            const BinaryOp op = expr->binop.op;

            if (is_arithmetic_op(op)) {
                if (!is_numeric_type(lhs) || !is_numeric_type(rhs)) {
                    fprintf(stderr, "Error: arithmetic on non-numeric types\n");
                    exit(1);
                }

                if (lhs != rhs) {
                    fprintf(stderr, "Error: type mismatch in binary expression\n");
                    exit(1);
                }

                expr->type = lhs;
                break;
            }

            if (is_comparison_op(op)) {
                if (lhs != rhs) {
                    fprintf(stderr, "Error: type mismatch in binary expression\n");
                    exit(1);
                }

                expr->type = TYPE_BOOLEAN;
                break;
            }

            if (is_assignment_op(op)) {
                if (expr->binop.left->kind != EXPR_VAR) {
                    fprintf(stderr, "Error: left-hand side of assignment is not assignable\n");
                    exit(1);
                }

                if (lhs != rhs) {
                    fprintf(stderr, "Error: type mismatch in assignment\n");
                    exit(1);
                }

                expr->type = lhs;
                break;
            }

            fprintf(stderr, "Error: unknown binary operator\n");
            exit(1);
        case EXPR_CALL: {
            // look up function
            Symbol *func_sym = scope_lookup_recursive(scope, expr->call.function_name);
            if (!func_sym) {
                fprintf(stderr, "Error: undefined function '%s'\n", expr->call.function_name);
                exit(1);
            }

            if (func_sym->kind != SYM_FUNCTION) {
                fprintf(stderr, "Error: '%s' is not a function\n", expr->call.function_name);
                exit(1);
            }

            // arguments
            for (int i = 0; i < expr->call.arg_count; i++) {
                analyze_expression(expr->call.args[i], scope);
            }

            expr->type = func_sym->type;
            break;
        }
        default: {
            fprintf(stderr, "Unknown expression kind: %d\n", expr->kind);
            exit(1);
        }
    }
}

static void analyze_statement(const StmtNode* stmt, Scope* scope) {
    switch (stmt->kind) {
        case STMT_RETURN: {
            if (stmt->return_stmt.expr) {
                analyze_expression(stmt->return_stmt.expr, scope);
            }
            break;
        }
        case STMT_IF: {
            analyze_expression(stmt->if_stmt.condition, scope);

            if (stmt->if_stmt.condition->type != TYPE_BOOLEAN) {
                fprintf(stderr, "Error: if condition must be integer (boolean)\n");
                exit(1);
            }

            if (stmt->if_stmt.then_stmt) {
                analyze_statement(stmt->if_stmt.then_stmt, scope);
            }

            if (stmt->if_stmt.else_stmt) {
                analyze_statement(stmt->if_stmt.else_stmt, scope);
            }

            break;
        }
        case STMT_VAR_DECL: {
            // check if variable already exists in current scope
            const Symbol *existing = scope_lookup(scope, stmt->var_decl.name);
            if (existing) {
                fprintf(stderr, "Error: variable '%s' already declared in this scope\n", stmt->var_decl.name);
                exit(1);
            }

            // add variable to scope
            Symbol *sym = malloc(sizeof(Symbol));
            sym->name = strdup(stmt->var_decl.name);
            sym->kind = SYM_VARIABLE;
            sym->type = stmt->var_decl.type;
            sym->location = stmt->location;

            scope_add_symbol(scope, sym);

            // analyze initializer if present
            if (stmt->var_decl.initializer) {
                analyze_expression(stmt->var_decl.initializer, scope);

                if (stmt->var_decl.initializer->type != stmt->var_decl.type) {
                    fprintf(stderr, "Error: type mismatch in assignment\n");
                    exit(1);
                }
            }
            break;
        }
        case STMT_EXPR:
            // analyze the expression
            analyze_expression(stmt->expr_stmt.expr, scope);
            break;
        case STMT_COMPOUND: {
            // create new scope for block
            Scope *block_scope = scope_create(scope);

            // analyze each statement in the block
            for (int i = 0; i < stmt->compound.count; i++) {
                analyze_statement(stmt->compound.stmts[i], block_scope);
            }

            scope_destroy(block_scope);
            break;
        }
        default:
            fprintf(stderr, "Unknown statement kind: %d\n", stmt->kind);
            exit(1);
    }
}

static void analyze_function(const FunctionNode *func, Scope *global) {
    Scope *func_scope = scope_create(global);

    for (int i = 0; i < func->param_count; i++) {
        const ParamNode* param = &func->params[i];

        // check for duplicate parameter names
        if (scope_lookup(func_scope, param->name)) {
            fprintf(stderr, "Error: duplicate parameter '%s'\n", param->name);
        }

        Symbol *sym = malloc(sizeof(Symbol));
        sym->name = strdup(param->name);
        sym->kind = SYM_PARAMETER;
        sym->type = param->type;
        sym->location = param->location;

        scope_add_symbol(func_scope, sym);
    }

    analyze_statement(func->body, func_scope);
    scope_destroy(func_scope);
}

void analyze_program(const ProgramNode *program) {
    Scope* global = scope_create(NULL);

    for (int i = 0; i < program->function_count; ++i) {
        const FunctionNode* func = program->functions[i];

        const Symbol* existing = scope_lookup(global, func->name);
        if (existing) {
            fprintf(stderr, "Error: function '%s' already declared\n", func->name);
            exit(1);
        }

        Symbol *sym = malloc(sizeof(Symbol));
        sym->name = strdup(func->name);
        sym->kind = SYM_FUNCTION;
        sym->type = func->return_type;
        sym->location = func->location;

        scope_add_symbol(global, sym);
    }

    for (int i = 0; i < program->function_count; ++i) {
        analyze_function(program->functions[i], global);
    }

    scope_destroy(global);
}
