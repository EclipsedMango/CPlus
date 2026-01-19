
#include "semantic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* type_to_str(const TypeKind t) {
    switch(t) {
        case TYPE_INT: return "int";
        case TYPE_LONG: return "long";
        case TYPE_FLOAT: return "float";
        case TYPE_DOUBLE: return "double";
        case TYPE_STRING: return "string";
        case TYPE_BOOLEAN: return "bool";
        case TYPE_VOID: return "void";
        default: return "unknown";
    }
}

static int is_arithmetic_op(const BinaryOp op) {
    return op == BIN_ADD || op == BIN_SUB || op == BIN_MUL || op == BIN_DIV || op == BIN_MOD;
}

static int is_comparison_op(const BinaryOp op) {
    return op == BIN_EQUAL || op == BIN_GREATER || op == BIN_LESS || op == BIN_GREATER_EQ || op == BIN_LESS_EQ || op == BIN_NOT_EQUAL;
}

static int is_logical_op(const BinaryOp op) {
    return op == BIN_LOGICAL_AND || op == BIN_LOGICAL_OR;
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
        case EXPR_NUMBER: {
            expr->type = TYPE_INT;
            break;
        }
        case EXPR_STRING_LITERAL: {
            expr->type = TYPE_STRING;
            break;
        }
        case EXPR_VAR: {
            Symbol *sym = scope_lookup_recursive(scope, expr->text);
            if (!sym) {
                report_error(expr->location, "Undefined variable '%s'", expr->text);
            }

            expr->type = sym->type;
            break;
        }
        case EXPR_UNARY: {
            analyze_expression(expr->unary.operand, scope);

            if (expr->unary.op == UNARY_NOT) {
                if (expr->unary.operand->type == TYPE_VOID || expr->unary.operand->type == TYPE_STRING) {
                    report_error(expr->location, "Invalid type '%s' for '!' operator. Expected boolean or number.", type_to_str(expr->unary.operand->type));
                }

                expr->type = TYPE_BOOLEAN;
            }

            else if (expr->unary.op == UNARY_NEG) {
                if (!is_numeric_type(expr->unary.operand->type)) {
                    report_error(expr->location, "Invalid type '%s' for unary '-' operator. Expected numeric type.", type_to_str(expr->unary.operand->type));
                }

                expr->type = expr->unary.operand->type;
            }

            break;
        }
        case EXPR_BINOP: {
            analyze_expression(expr->binop.left, scope);
            analyze_expression(expr->binop.right, scope);

            const TypeKind lhs = expr->binop.left->type;
            const TypeKind rhs = expr->binop.right->type;

            if (is_arithmetic_op(expr->binop.op)) {
                if (!is_numeric_type(lhs) || !is_numeric_type(rhs)) {
                    report_error(expr->location, "Arithmetic operator requires numeric types. Got '%s' and '%s'.", type_to_str(lhs), type_to_str(rhs));
                }

                if (lhs != rhs) {
                    report_error(expr->location, "Type mismatch in arithmetic expression: '%s' vs '%s'", type_to_str(lhs), type_to_str(rhs));
                }

                expr->type = lhs;
                break;
            }

            if (is_comparison_op(expr->binop.op)) {
                if (lhs != rhs) {
                    report_error(expr->location, "Type mismatch in comparison: '%s' vs '%s'", type_to_str(lhs), type_to_str(rhs));
                }

                expr->type = TYPE_BOOLEAN;
                break;
            }

            if (is_assignment_op(expr->binop.op)) {
                if (expr->binop.left->kind != EXPR_VAR) {
                    report_error(expr->location, "Left-hand side of assignment must be a variable.");
                }

                if (lhs != rhs) {
                    report_error(expr->location, "Type mismatch in assignment. Cannot assign '%s' to variable of type '%s'.", type_to_str(rhs), type_to_str(lhs));
                }

                expr->type = lhs;
                break;
            }

            if (is_logical_op(expr->binop.op)) {
                // maybe allow numeric types as well?
                if (lhs != TYPE_BOOLEAN || rhs != TYPE_BOOLEAN) {
                    report_error(expr->location, "Logical operators require boolean operands. Got '%s' and '%s'.", type_to_str(lhs), type_to_str(rhs));
                }

                expr->type = TYPE_BOOLEAN;
                break;
            }

            break;
        }
        case EXPR_CALL: {
            // look up function
            Symbol *func_sym = scope_lookup_recursive(scope, expr->call.function_name);
            if (!func_sym) {
                report_error(expr->location, "Undefined function '%s'", expr->call.function_name);
            }

            if (func_sym->kind != SYM_FUNCTION) {
                report_error(expr->location, "'%s' is not a function", expr->call.function_name);
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
                report_error(stmt->location, "If condition must be boolean. Got '%s'.", type_to_str(stmt->if_stmt.condition->type));
            }

            if (stmt->if_stmt.then_stmt) {
                analyze_statement(stmt->if_stmt.then_stmt, scope);
            }

            if (stmt->if_stmt.else_stmt) {
                analyze_statement(stmt->if_stmt.else_stmt, scope);
            }

            break;
        }
        case STMT_ASM: {
            break;
        }
        case STMT_VAR_DECL: {
            // check if variable already exists in current scope
            const Symbol *existing = scope_lookup(scope, stmt->var_decl.name);
            if (existing) {
                report_error(stmt->location, "Variable '%s' already declared in this scope (previous declaration at line %d)", stmt->var_decl.name, existing->location.line);
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
                    report_error(stmt->location, "Type mismatch in initialization of '%s'. Expected '%s', got '%s'.",
                        stmt->var_decl.name,
                        type_to_str(stmt->var_decl.type),
                        type_to_str(stmt->var_decl.initializer->type)
                    );
                }
            }
            break;
        }
        case STMT_EXPR: {
            // analyze the expression
            analyze_expression(stmt->expr_stmt.expr, scope);
            break;
        }
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
        default: {
            fprintf(stderr, "Unknown statement kind: %d\n", stmt->kind);
            exit(1);
        }
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
