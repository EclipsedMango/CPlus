
#include "semantic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* type_to_str(const TypeKind t) {
    switch(t) {
        case TYPE_INT: return "int";
        case TYPE_LONG: return "long";
        case TYPE_CHAR: return "char";
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
    return t == TYPE_INT || t == TYPE_LONG || t == TYPE_FLOAT || t == TYPE_DOUBLE || t == TYPE_CHAR;
}

static int types_compatible(const TypeKind target, const TypeKind source) {
    if (target == source) return 1;

    // numeric types are compatible with each other (e.g., int to float)
    if (is_numeric_type(target) && is_numeric_type(source)) return 1;
    return 0;
}

static int types_compatible_with_pointers(const TypeKind target_type, const int target_ptr_level, const TypeKind source_type, const int source_ptr_level) {
    // string type is compatible with char*
    if (target_type == TYPE_STRING && source_type == TYPE_CHAR && source_ptr_level == 1) return 1;
    if (target_type == TYPE_CHAR && target_ptr_level == 1 && source_type == TYPE_STRING) return 1;

    // void* compatibility (generic pointer)
    if (target_ptr_level > 0 && source_ptr_level > 0) {
        if (target_type == TYPE_VOID || source_type == TYPE_VOID) return 1;
    }

    // int to pointer implicit conversion
    if (target_type == TYPE_INT && source_ptr_level > 0) return 1;
    if (source_type == TYPE_INT && target_ptr_level > 0) return 1;

    // otherwise, pointer levels must match and types must be compatible
    if (target_ptr_level != source_ptr_level) return 0;
    return types_compatible(target_type, source_type);
}

Scope* scope_create(Scope* parent) {
    Scope* new_scope = malloc(sizeof(Scope));
    new_scope->parent = parent;
    new_scope->symbols = create_vector(16, sizeof(Symbol*));
    return new_scope;
}

void scope_destroy(Scope* scope) {
    for (int i = 0; i < scope->symbols.length; i++) {
        Symbol **sym_ptr = vector_get(&scope->symbols, i);
        if (sym_ptr && *sym_ptr) {
            free((*sym_ptr)->name);
            free(*sym_ptr);
        }
    }

    vector_destroy(&scope->symbols);
    free(scope);
}

void scope_add_symbol(Scope* scope, Symbol* symbol) {
    vector_push(&scope->symbols, &symbol);
}

Symbol* scope_lookup(const Scope* scope, const char* name) {
    for (int i = 0; i < scope->symbols.length; i++) {
        Symbol **sym_ptr = vector_get(&scope->symbols, i);
        if (sym_ptr && *sym_ptr && strcmp((*sym_ptr)->name, name) == 0) {
            return *sym_ptr;
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
            expr->pointer_level = 0;
            break;
        }
        case EXPR_STRING_LITERAL: {
            expr->type = TYPE_STRING;
            expr->pointer_level = 0;
            break;
        }
        case EXPR_VAR: {
            Symbol *sym = scope_lookup_recursive(scope, expr->text);
            if (!sym) {
                report_error(expr->location, "Undefined variable '%s'", expr->text);
            }

            expr->type = sym->type;
            expr->pointer_level = sym->pointer_level;
            break;
        }
        case EXPR_UNARY: {
            analyze_expression(expr->unary.operand, scope);

            if (expr->unary.op == UNARY_NOT) {
                if (expr->unary.operand->type == TYPE_VOID || expr->unary.operand->type == TYPE_STRING) {
                    report_error(expr->location, "Invalid type '%s' for '!' operator.", type_to_str(expr->unary.operand->type));
                }

                expr->pointer_level = 0;
                expr->type = TYPE_BOOLEAN;
            } else if (expr->unary.op == UNARY_NEG) {
                if (!is_numeric_type(expr->unary.operand->type)) {
                    report_error(expr->location, "Invalid type '%s' for unary '-' operator.", type_to_str(expr->unary.operand->type));
                }

                expr->type = expr->unary.operand->type;
                expr->pointer_level = expr->unary.operand->pointer_level;
            } else if (expr->unary.op == UNARY_DEREF) {
                if (expr->unary.operand->pointer_level == 0) {
                    report_error(expr->location, "Cannot dereference non-pointer type '%s'.", type_to_str(expr->unary.operand->type));
                }

                expr->type = expr->unary.operand->type;
                expr->pointer_level = expr->unary.operand->pointer_level - 1;
            } else if (expr->unary.op == UNARY_ADDR_OF) {
                if (expr->unary.operand->kind != EXPR_VAR &&
        !(expr->unary.operand->kind == EXPR_UNARY && expr->unary.operand->unary.op == UNARY_DEREF) && expr->unary.operand->kind != EXPR_ARRAY_INDEX) {
                    report_error(expr->location, "Cannot take address of non-lvalue");
                }

                expr->type = expr->unary.operand->type;
                expr->pointer_level = expr->unary.operand->pointer_level + 1;
            }

            break;
        }
        case EXPR_BINOP: {
            analyze_expression(expr->binop.left, scope);
            analyze_expression(expr->binop.right, scope);

            const TypeKind lhs = expr->binop.left->type;
            const TypeKind rhs = expr->binop.right->type;

            if (is_arithmetic_op(expr->binop.op)) {
                if (expr->binop.left->pointer_level > 0 && is_numeric_type(rhs)) {
                    expr->type = lhs;
                    expr->pointer_level = expr->binop.left->pointer_level;
                    break;
                }

                if (is_numeric_type(lhs) && expr->binop.right->pointer_level > 0) {
                    expr->type = rhs;
                    expr->pointer_level = expr->binop.right->pointer_level;
                    break;
                }

                if (!is_numeric_type(lhs) || !is_numeric_type(rhs)) {
                    report_error(expr->location, "Arithmetic operator requires numeric types. Got '%s' and '%s'.", type_to_str(lhs), type_to_str(rhs));
                }

                if (!types_compatible(lhs, rhs)) {
                    report_error(expr->location, "Type mismatch in arithmetic expression: '%s' vs '%s'", type_to_str(lhs), type_to_str(rhs));
                }

                expr->type = lhs;
                expr->pointer_level = 0;
                break;
            }

            if (is_comparison_op(expr->binop.op)) {
                if (!types_compatible_with_pointers(lhs, expr->binop.left->pointer_level, rhs, expr->binop.right->pointer_level)) {
                    report_error(expr->location, "Type mismatch in comparison: '%s' vs '%s'", type_to_str(lhs), type_to_str(rhs));
                }

                expr->type = TYPE_BOOLEAN;
                expr->pointer_level = 0;
                break;
            }

            if (is_assignment_op(expr->binop.op)) {
                if (expr->binop.left->kind != EXPR_VAR && !(expr->binop.left->kind == EXPR_UNARY && expr->binop.left->unary.op == UNARY_DEREF) && expr->binop.left->kind != EXPR_ARRAY_INDEX) {
                    report_error(expr->location, "Left-hand side of assignment must be a variable, dereferenced pointer, or array element.");
                }

                if (!types_compatible_with_pointers(lhs, expr->binop.left->pointer_level, rhs, expr->binop.right->pointer_level)) {
                    report_error(expr->location, "Type mismatch in assignment. Cannot assign '%s%s' to '%s%s'.",
                        type_to_str(rhs), expr->binop.right->pointer_level > 0 ? "*" : "",
                        type_to_str(lhs), expr->binop.left->pointer_level > 0 ? "*" : "");
                }

                expr->type = lhs;
                expr->pointer_level = max(expr->binop.left->pointer_level, expr->binop.right->pointer_level);
                break;
            }

            if (is_logical_op(expr->binop.op)) {
                // maybe allow numeric types as well?
                if (lhs != TYPE_BOOLEAN || rhs != TYPE_BOOLEAN) {
                    report_error(expr->location, "Logical operators require boolean operands. Got '%s' and '%s'.", type_to_str(lhs), type_to_str(rhs));
                }

                expr->type = TYPE_BOOLEAN;
                expr->pointer_level = 0;
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
            expr->pointer_level = func_sym->pointer_level;
            break;
        }
        case EXPR_ARRAY_INDEX: {
            analyze_expression(expr->array_index.array, scope);
            analyze_expression(expr->array_index.index, scope);

            if (expr->array_index.array->pointer_level == 0) {
                report_error(expr->location, "Cannot index non-pointer/non-array type '%s'", type_to_str(expr->array_index.array->type));
            }

            if (!is_numeric_type(expr->array_index.index->type)) {
                report_error(expr->location, "Array index must be a numeric type, got '%s'", type_to_str(expr->array_index.index->type));
            }

            expr->type = expr->array_index.array->type;
            expr->pointer_level = expr->array_index.array->pointer_level - 1;
            break;
        }
        default: {
            fprintf(stderr, "Unknown expression kind: %d\n", expr->kind);
            exit(1);
        }
    }
}

static bool analyze_statement(const StmtNode* stmt, Scope* scope, const TypeKind expected_ret_type, const int expected_ret_ptr_level, bool in_loop)  {
    switch (stmt->kind) {
        case STMT_RETURN: {
            if (stmt->return_stmt.expr) {
                if (expected_ret_type == TYPE_VOID && expected_ret_ptr_level == 0) {
                    report_error(stmt->location, "Void function cannot return a value.");
                }

                analyze_expression(stmt->return_stmt.expr, scope);

                if (!types_compatible_with_pointers(expected_ret_type, expected_ret_ptr_level,
                                                    stmt->return_stmt.expr->type, stmt->return_stmt.expr->pointer_level)) {
                    report_error(stmt->location,
                        "Return type mismatch. Expected '%s%s', got '%s%s'.",
                        type_to_str(expected_ret_type),
                        expected_ret_ptr_level > 0 ? "*" : "",
                        type_to_str(stmt->return_stmt.expr->type),
                        stmt->return_stmt.expr->pointer_level > 0 ? "*" : ""
                    );
                }
            } else {
                if (expected_ret_type != TYPE_VOID || expected_ret_ptr_level > 0) {
                    report_error(stmt->location, "Non-void function must return a value of type '%s%s'.",
                        type_to_str(expected_ret_type),
                        expected_ret_ptr_level > 0 ? "*" : ""
                    );
                }
            }
            return true;
        }
        case STMT_IF: {
            analyze_expression(stmt->if_stmt.condition, scope);

            if (stmt->if_stmt.condition->type != TYPE_BOOLEAN && !is_numeric_type(stmt->if_stmt.condition->type)) {
                report_error(stmt->location, "If condition must be boolean or numeric. Got '%s'.", type_to_str(stmt->if_stmt.condition->type));
            }

            bool then_returns = false;
            if (stmt->if_stmt.then_stmt) {
                then_returns = analyze_statement(stmt->if_stmt.then_stmt, scope, expected_ret_type, expected_ret_ptr_level, in_loop);
            }

            bool else_returns = false;
            if (stmt->if_stmt.else_stmt) {
                else_returns = analyze_statement(stmt->if_stmt.else_stmt, scope, expected_ret_type, expected_ret_ptr_level, in_loop);
            }

            return then_returns && else_returns;
        }
        case STMT_WHILE: {
            analyze_expression(stmt->while_stmt.condition, scope);
            if (stmt->while_stmt.condition->type != TYPE_BOOLEAN && !is_numeric_type(stmt->while_stmt.condition->type)) {
                report_error(stmt->location, "While condition must be boolean or numeric.");
            }

            analyze_statement(stmt->while_stmt.body, scope, expected_ret_type, expected_ret_ptr_level, true);
            return false;
        }
        case STMT_FOR: {
            Scope *loop_scope = scope_create(scope);

            if (stmt->for_stmt.init) {
                analyze_statement(stmt->for_stmt.init, loop_scope, expected_ret_type, expected_ret_ptr_level, false);
            }

            if (stmt->for_stmt.condition) {
                analyze_expression(stmt->for_stmt.condition, loop_scope);
                if (stmt->for_stmt.condition->type != TYPE_BOOLEAN && !is_numeric_type(stmt->for_stmt.condition->type)) {
                    report_error(stmt->location, "For condition must be boolean or numeric.");
                }
            }

            if (stmt->for_stmt.increment) {
                analyze_expression(stmt->for_stmt.increment, loop_scope);
            }

            analyze_statement(stmt->for_stmt.body, loop_scope, expected_ret_type, expected_ret_ptr_level, true);

            scope_destroy(loop_scope);
            return false;
        }
        case STMT_BREAK:
        case STMT_CONTINUE: {
            if (!in_loop) {
                report_error(stmt->location, "Statement can only be used inside a loop.");
            }
            return false;
        }
        case STMT_ASM: {
            for (int i = 0; i < stmt->asm_stmt.input_count; i++) {
                analyze_expression(stmt->asm_stmt.inputs[i], scope);
            }

            return false;
        }
        case STMT_VAR_DECL: {
            if (stmt->var_decl.type == TYPE_VOID && stmt->var_decl.pointer_level == 0) {
                report_error(stmt->location, "Variable '%s' declared as void. Variables cannot be void (did you mean 'void*'?)", stmt->var_decl.name);
            }

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

            if (stmt->var_decl.array_size > 0) {
                sym->pointer_level = stmt->var_decl.pointer_level + 1;
            } else {
                sym->pointer_level = stmt->var_decl.pointer_level;
            }

            sym->location = stmt->location;

            scope_add_symbol(scope, sym);

            // analyze initializer if present
            if (stmt->var_decl.initializer) {
                analyze_expression(stmt->var_decl.initializer, scope);

                if (!types_compatible_with_pointers(stmt->var_decl.type, stmt->var_decl.pointer_level,
                                                    stmt->var_decl.initializer->type, stmt->var_decl.initializer->pointer_level)) {
                    report_error(stmt->location, "Type mismatch in initialization of '%s'. Expected '%s%s', got '%s%s'.",
                        stmt->var_decl.name,
                        type_to_str(stmt->var_decl.type),
                        stmt->var_decl.pointer_level > 0 ? "*" : "",
                        type_to_str(stmt->var_decl.initializer->type),
                        stmt->var_decl.initializer->pointer_level > 0 ? "*" : ""
                    );
                }
            }

            return false;
        }
        case STMT_EXPR: {
            analyze_expression(stmt->expr_stmt.expr, scope);
            return false;
        }
        case STMT_COMPOUND: {
            Scope *block_scope = scope_create(scope);
            bool does_return = false;

            for (int i = 0; i < stmt->compound.count; i++) {
                // if one returns, the block returns.
                if (analyze_statement(stmt->compound.stmts[i], block_scope, expected_ret_type, expected_ret_ptr_level, in_loop)) {
                    does_return = true;
                }
            }

            scope_destroy(block_scope);
            return does_return;
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
        sym->pointer_level = param->pointer_level;
        sym->location = param->location;

        scope_add_symbol(func_scope, sym);
    }

    const bool returns_value = analyze_statement(func->body, func_scope, func->return_type, func->return_pointer_level, false);
    if (func->return_type != TYPE_VOID && !returns_value) {
        report_error(func->location, "Function '%s' is declared to return '%s' but not all control paths return a value.", func->name, type_to_str(func->return_type));
    }

    scope_destroy(func_scope);
}

void analyze_program(const ProgramNode *program) {
    Scope* global = scope_create(NULL);

    for (int i = 0; i < program->function_count; ++i) {
        const FunctionNode* func = program->functions[i];
        if (scope_lookup(global, func->name)) {
            fprintf(stderr, "Error: function '%s' already declared\n", func->name);
            exit(1);
        }

        Symbol *sym = malloc(sizeof(Symbol));
        sym->name = strdup(func->name);
        sym->kind = SYM_FUNCTION;
        sym->type = func->return_type;
        sym->pointer_level = func->return_pointer_level;
        sym->location = func->location;

        scope_add_symbol(global, sym);
    }

    for (int i = 0; i < program->function_count; ++i) {
        analyze_function(program->functions[i], global);
    }

    scope_destroy(global);
}
