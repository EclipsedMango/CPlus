
#include "semantic.h"
#include "typecheck.h"
#include <stdlib.h>
#include <string.h>

#include "scope.h"

// internal state
struct SemanticAnalyzer {
    DiagnosticEngine *diagnostics;
    Scope *current_scope;

    TypeKind current_function_return_type;
    int current_function_return_ptr_level;
};

static void analyze_expression(SemanticAnalyzer *analyzer, ExprNode *expr);
static bool analyze_statement(SemanticAnalyzer *analyzer, StmtNode *stmt, TypeKind expected_ret_type, int expected_ret_ptr_level);
static void analyze_function(SemanticAnalyzer *analyzer, const FunctionNode *func, Scope *global);

SemanticAnalyzer* semantic_create(DiagnosticEngine *diagnostics) {
    SemanticAnalyzer *analyzer = malloc(sizeof(SemanticAnalyzer));
    if (!analyzer) return NULL;

    analyzer->diagnostics = diagnostics;
    analyzer->current_scope = NULL;
    analyzer->current_function_return_type = TYPE_VOID;
    analyzer->current_function_return_ptr_level = 0;

    return analyzer;
}

void semantic_destroy(SemanticAnalyzer *analyzer) {
    if (!analyzer) return;
    free(analyzer);
}

bool semantic_analyze_program(SemanticAnalyzer *analyzer, ProgramNode *program) {
    if (!analyzer || !program) return false;

    Scope *global = scope_create(NULL, SCOPE_GLOBAL);
    analyzer->current_scope = global;

    for (int i = 0; i < program->function_count; i++) {
        const FunctionNode *func = program->functions[i];

        if (scope_lookup(global, func->name)) {
            diag_error(analyzer->diagnostics, func->location, "Function '%s' already declared", func->name);
            continue;
        }

        Symbol *sym = malloc(sizeof(Symbol));
        sym->name = strdup(func->name);
        sym->kind = SYM_FUNCTION;
        sym->type = func->return_type;
        sym->pointer_level = func->return_pointer_level;
        sym->is_const = false;
        sym->location = func->location;

        scope_add_symbol(global, sym);
    }

    for (int i = 0; i < program->global_count; i++) {
        const GlobalVarNode *global_var = program->globals[i];

        if (scope_lookup(global, global_var->name)) {
            diag_error(analyzer->diagnostics, global_var->location, "Global variable '%s' already declared", global_var->name);
            continue;
        }

        Symbol *sym = malloc(sizeof(Symbol));
        sym->name = strdup(global_var->name);
        sym->kind = SYM_VARIABLE;
        sym->type = global_var->kind;
        sym->is_const = global_var->is_const;
        sym->location = global_var->location;

        if (global_var->array_size > 0) {
            sym->pointer_level = global_var->pointer_level + 1;
        } else {
            sym->pointer_level = global_var->pointer_level;
        }

        // Analyze initializer if present
        if (global_var->initializer) {
            analyze_expression(analyzer, global_var->initializer);

            if (!types_compatible_with_pointers(global_var->kind, global_var->pointer_level,
                                                global_var->initializer->type,
                                                global_var->initializer->pointer_level)) {
                diag_error(analyzer->diagnostics, global_var->location,
                          "Type mismatch in initialization of '%s'. Expected '%s%s', got '%s%s'",
                          global_var->name,
                          type_to_string(global_var->kind),
                          global_var->pointer_level > 0 ? "*" : "",
                          type_to_string(global_var->initializer->type),
                          global_var->initializer->pointer_level > 0 ? "*" : ""
                );
            }
        }

        scope_add_symbol(global, sym);
    }

    for (int i = 0; i < program->function_count; i++) {
        analyze_function(analyzer, program->functions[i], global);
    }

    scope_destroy(global);
    analyzer->current_scope = NULL;

    return !diag_has_errors(analyzer->diagnostics);
}

static void analyze_expression(SemanticAnalyzer *analyzer, ExprNode *expr) {
    if (!expr) return;

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
            Symbol *sym = scope_lookup_recursive(analyzer->current_scope, expr->text);
            if (!sym) {
                diag_error(analyzer->diagnostics, expr->location, "Undefined variable '%s'", expr->text);
                expr->type = TYPE_INT;
                expr->pointer_level = 0;
                break;
            }

            expr->type = sym->type;
            expr->pointer_level = sym->pointer_level;
            break;
        }
        case EXPR_UNARY: {
            analyze_expression(analyzer, expr->unary.operand);

            switch (expr->unary.op) {
                case UNARY_NOT: {
                    if (expr->unary.operand->type == TYPE_VOID || expr->unary.operand->type == TYPE_STRING) {
                        diag_error(analyzer->diagnostics, expr->location, "Invalid type '%s' for '!' operator", type_to_string(expr->unary.operand->type));
                    }
                    expr->type = TYPE_BOOLEAN;
                    expr->pointer_level = 0;
                    break;
                }
                case UNARY_NEG: {
                    if (!is_numeric_type(expr->unary.operand->type)) {
                        diag_error(analyzer->diagnostics, expr->location, "Invalid type '%s' for unary '-' operator", type_to_string(expr->unary.operand->type));
                    }
                    expr->type = expr->unary.operand->type;
                    expr->pointer_level = expr->unary.operand->pointer_level;
                    break;
                }
                case UNARY_DEREF: {
                    if (expr->unary.operand->pointer_level == 0) {
                        diag_error(analyzer->diagnostics, expr->location, "Cannot dereference non-pointer type '%s'", type_to_string(expr->unary.operand->type));
                    }
                    expr->type = expr->unary.operand->type;
                    expr->pointer_level = expr->unary.operand->pointer_level - 1;
                    break;
                }
                case UNARY_ADDR_OF: {
                    // check if operand is an lvalue
                    if (expr->unary.operand->kind != EXPR_VAR && !(expr->unary.operand->kind == EXPR_UNARY &&  expr->unary.operand->unary.op == UNARY_DEREF) && expr->unary.operand->kind != EXPR_ARRAY_INDEX) {
                        diag_error(analyzer->diagnostics, expr->location, "Cannot take address of non-lvalue");
                    }
                    expr->type = expr->unary.operand->type;
                    expr->pointer_level = expr->unary.operand->pointer_level + 1;
                    break;
                }
            }
            break;
        }
        case EXPR_BINOP: {
            analyze_expression(analyzer, expr->binop.left);
            analyze_expression(analyzer, expr->binop.right);

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
                    diag_error(analyzer->diagnostics, expr->location,
                              "Arithmetic operator requires numeric types. Got '%s' and '%s'",
                              type_to_string(lhs), type_to_string(rhs)
                    );
                }

                expr->type = lhs;
                expr->pointer_level = 0;
                break;
            }

            if (is_comparison_op(expr->binop.op)) {
                if (!types_compatible_with_pointers(lhs, expr->binop.left->pointer_level,
                                                    rhs, expr->binop.right->pointer_level)) {
                    diag_error(analyzer->diagnostics, expr->location,
                              "Type mismatch in comparison: '%s' vs '%s'",
                              type_to_string(lhs), type_to_string(rhs));
                }
                expr->type = TYPE_BOOLEAN;
                expr->pointer_level = 0;
                break;
            }

            if (is_assignment_op(expr->binop.op)) {
                // Check if left side is an lvalue
                if (expr->binop.left->kind != EXPR_VAR &&
                    !(expr->binop.left->kind == EXPR_UNARY &&
                      expr->binop.left->unary.op == UNARY_DEREF) &&
                    expr->binop.left->kind != EXPR_ARRAY_INDEX) {
                    diag_error(analyzer->diagnostics, expr->location,
                              "Left-hand side of assignment must be a variable, dereferenced pointer, or array element");
                }

                // Check for const assignment
                if (expr->binop.left->kind == EXPR_VAR) {
                    Symbol *sym = scope_lookup_recursive(analyzer->current_scope,
                                                        expr->binop.left->text);
                    if (sym && sym->is_const) {
                        diag_error(analyzer->diagnostics, expr->location,
                                  "Cannot assign to const variable '%s'",
                                  expr->binop.left->text);
                    }
                }

                // Check type compatibility
                if (!types_compatible_with_pointers(lhs, expr->binop.left->pointer_level,
                                                    rhs, expr->binop.right->pointer_level)) {
                    diag_error(analyzer->diagnostics, expr->location,
                              "Type mismatch in assignment. Cannot assign '%s%s' to '%s%s'",
                              type_to_string(rhs),
                              expr->binop.right->pointer_level > 0 ? "*" : "",
                              type_to_string(lhs),
                              expr->binop.left->pointer_level > 0 ? "*" : "");
                }

                expr->type = lhs;
                expr->pointer_level = expr->binop.left->pointer_level;
                break;
            }

            if (is_logical_op(expr->binop.op)) {
                if (lhs != TYPE_BOOLEAN || rhs != TYPE_BOOLEAN) {
                    diag_warning(analyzer->diagnostics, expr->location,
                                "Logical operators expect boolean operands. Got '%s' and '%s'",
                                type_to_string(lhs), type_to_string(rhs));
                }
                expr->type = TYPE_BOOLEAN;
                expr->pointer_level = 0;
                break;
            }

            break;
        }
        case EXPR_CALL: {
            Symbol *func_sym = scope_lookup_recursive(analyzer->current_scope, expr->call.function_name);
            if (!func_sym) {
                diag_error(analyzer->diagnostics, expr->location, "Undefined function '%s'", expr->call.function_name);
                expr->type = TYPE_INT;
                expr->pointer_level = 0;
                break;
            }

            if (func_sym->kind != SYM_FUNCTION) {
                diag_error(analyzer->diagnostics, expr->location, "'%s' is not a function", expr->call.function_name);
                expr->type = TYPE_INT;
                expr->pointer_level = 0;
                break;
            }

            for (int i = 0; i < expr->call.arg_count; i++) {
                analyze_expression(analyzer, expr->call.args[i]);
            }

            expr->type = func_sym->type;
            expr->pointer_level = func_sym->pointer_level;
            break;
        }
        case EXPR_ARRAY_INDEX: {
            analyze_expression(analyzer, expr->array_index.array);
            analyze_expression(analyzer, expr->array_index.index);

            if (expr->array_index.array->pointer_level == 0) {
                diag_error(analyzer->diagnostics, expr->location, "Cannot index non-pointer/non-array type '%s'", type_to_string(expr->array_index.array->type));
            }

            if (!is_numeric_type(expr->array_index.index->type)) {
                diag_error(analyzer->diagnostics, expr->location, "Array index must be a numeric type, got '%s'", type_to_string(expr->array_index.index->type));
            }

            expr->type = expr->array_index.array->type;
            expr->pointer_level = expr->array_index.array->pointer_level - 1;
            break;
        }
        case EXPR_CAST: {
            analyze_expression(analyzer, expr->cast.operand);

            expr->type = expr->cast.target_type;
            expr->pointer_level = expr->cast.target_pointer_level;

            if (expr->cast.operand->pointer_level > 0 && expr->pointer_level == 0) {
                if (expr->type != TYPE_INT && expr->type != TYPE_LONG) {
                    diag_warning(analyzer->diagnostics, expr->location, "Cast from pointer to non-integer type");
                }
            }

            break;
        }
    }
}


static bool analyze_statement(SemanticAnalyzer *analyzer, StmtNode *stmt, const TypeKind expected_ret_type, const int expected_ret_ptr_level) {
    if (!stmt) return false;

    switch (stmt->kind) {
        case STMT_RETURN: {
            if (stmt->return_stmt.expr) {
                if (expected_ret_type == TYPE_VOID && expected_ret_ptr_level == 0) {
                    diag_error(analyzer->diagnostics, stmt->location, "Void function cannot return a value");
                }

                analyze_expression(analyzer, stmt->return_stmt.expr);

                if (!types_compatible_with_pointers(expected_ret_type, expected_ret_ptr_level,
                                                    stmt->return_stmt.expr->type,
                                                    stmt->return_stmt.expr->pointer_level)) {
                    diag_error(analyzer->diagnostics, stmt->location,
                              "Return type mismatch. Expected '%s%s', got '%s%s'",
                              type_to_string(expected_ret_type),
                              expected_ret_ptr_level > 0 ? "*" : "",
                              type_to_string(stmt->return_stmt.expr->type),
                              stmt->return_stmt.expr->pointer_level > 0 ? "*" : ""
                    );
                }
            } else {
                if (expected_ret_type != TYPE_VOID || expected_ret_ptr_level > 0) {
                    diag_error(analyzer->diagnostics, stmt->location,
                              "Non-void function must return a value of type '%s%s'",
                              type_to_string(expected_ret_type),
                              expected_ret_ptr_level > 0 ? "*" : ""
                    );
                }
            }
            return true;
        }
        case STMT_IF: {
            analyze_expression(analyzer, stmt->if_stmt.condition);

            if (stmt->if_stmt.condition->type != TYPE_BOOLEAN &&
                !is_numeric_type(stmt->if_stmt.condition->type)) {
                diag_warning(analyzer->diagnostics, stmt->location, "If condition should be boolean or numeric. Got '%s'", type_to_string(stmt->if_stmt.condition->type));
            }

            const bool then_returns = analyze_statement(analyzer, stmt->if_stmt.then_stmt, expected_ret_type, expected_ret_ptr_level);

            bool else_returns = false;
            if (stmt->if_stmt.else_stmt) {
                else_returns = analyze_statement(analyzer, stmt->if_stmt.else_stmt, expected_ret_type, expected_ret_ptr_level);
            }

            return then_returns && else_returns;
        }
        case STMT_WHILE: {
            analyze_expression(analyzer, stmt->while_stmt.condition);

            if (stmt->while_stmt.condition->type != TYPE_BOOLEAN && !is_numeric_type(stmt->while_stmt.condition->type)) {
                diag_warning(analyzer->diagnostics, stmt->location, "While condition should be boolean or numeric");
            }

            Scope *loop_scope = scope_create(analyzer->current_scope, SCOPE_LOOP);
            Scope *old_scope = analyzer->current_scope;
            analyzer->current_scope = loop_scope;

            analyze_statement(analyzer, stmt->while_stmt.body, expected_ret_type, expected_ret_ptr_level);

            analyzer->current_scope = old_scope;
            scope_destroy(loop_scope);

            return false;
        }
        case STMT_FOR: {
            Scope *loop_scope = scope_create(analyzer->current_scope, SCOPE_LOOP);
            Scope *old_scope = analyzer->current_scope;
            analyzer->current_scope = loop_scope;

            if (stmt->for_stmt.init) {
                analyze_statement(analyzer, stmt->for_stmt.init, expected_ret_type, expected_ret_ptr_level);
            }

            // condition
            if (stmt->for_stmt.condition) {
                analyze_expression(analyzer, stmt->for_stmt.condition);
                if (stmt->for_stmt.condition->type != TYPE_BOOLEAN && !is_numeric_type(stmt->for_stmt.condition->type)) {
                    diag_warning(analyzer->diagnostics, stmt->location, "For condition should be boolean or numeric");
                }
            }

            // increment
            if (stmt->for_stmt.increment) {
                analyze_expression(analyzer, stmt->for_stmt.increment);
            }

            // body
            analyze_statement(analyzer, stmt->for_stmt.body, expected_ret_type, expected_ret_ptr_level);

            analyzer->current_scope = old_scope;
            scope_destroy(loop_scope);

            return false;
        }
        case STMT_BREAK:
        case STMT_CONTINUE: {
            if (!scope_in_loop(analyzer->current_scope)) {
                diag_error(analyzer->diagnostics, stmt->location, "'%s' statement can only be used inside a loop", stmt->kind == STMT_BREAK ? "break" : "continue");
            }
            return false;
        }
        case STMT_ASM: {
            for (int i = 0; i < stmt->asm_stmt.output_count; i++) {
                analyze_expression(analyzer, stmt->asm_stmt.outputs[i]);
            }

            for (int i = 0; i < stmt->asm_stmt.input_count; i++) {
                analyze_expression(analyzer, stmt->asm_stmt.inputs[i]);
            }

            return false;
        }
        case STMT_VAR_DECL: {
            if (stmt->var_decl.type == TYPE_VOID && stmt->var_decl.pointer_level == 0) {
                diag_error(analyzer->diagnostics, stmt->location,
                          "Variable '%s' declared as void. Variables cannot be void (did you mean 'void*'?)",
                          stmt->var_decl.name);
            }

            // check if variable already exists in current scope
            const Symbol *existing = scope_lookup(analyzer->current_scope, stmt->var_decl.name);
            if (existing) {
                diag_error(analyzer->diagnostics, stmt->location,
                          "Variable '%s' already declared in this scope (previous declaration at line %d)",
                          stmt->var_decl.name, existing->location.line);
            }

            Symbol *sym = malloc(sizeof(Symbol));
            sym->name = strdup(stmt->var_decl.name);
            sym->kind = SYM_VARIABLE;
            sym->type = stmt->var_decl.type;
            sym->is_const = stmt->var_decl.is_const;

            if (stmt->var_decl.array_size > 0) {
                sym->pointer_level = stmt->var_decl.pointer_level + 1;
            } else {
                sym->pointer_level = stmt->var_decl.pointer_level;
            }

            sym->location = stmt->location;
            scope_add_symbol(analyzer->current_scope, sym);

            if (stmt->var_decl.initializer) {
                analyze_expression(analyzer, stmt->var_decl.initializer);

                if (!types_compatible_with_pointers(stmt->var_decl.type,
                                                    stmt->var_decl.pointer_level,
                                                    stmt->var_decl.initializer->type,
                                                    stmt->var_decl.initializer->pointer_level)) {
                    diag_error(analyzer->diagnostics, stmt->location,
                              "Type mismatch in initialization of '%s'. Expected '%s%s', got '%s%s'",
                              stmt->var_decl.name,
                              type_to_string(stmt->var_decl.type),
                              stmt->var_decl.pointer_level > 0 ? "*" : "",
                              type_to_string(stmt->var_decl.initializer->type),
                              stmt->var_decl.initializer->pointer_level > 0 ? "*" : ""
                    );
                }
            }

            return false;
        }
        case STMT_EXPR: {
            analyze_expression(analyzer, stmt->expr_stmt.expr);
            return false;
        }
        case STMT_COMPOUND: {
            Scope *block_scope = scope_create(analyzer->current_scope, SCOPE_BLOCK);
            Scope *old_scope = analyzer->current_scope;
            analyzer->current_scope = block_scope;

            bool does_return = false;
            for (int i = 0; i < stmt->compound.count; i++) {
                if (analyze_statement(analyzer, stmt->compound.stmts[i],
                                    expected_ret_type, expected_ret_ptr_level)) {
                    does_return = true;
                }
            }

            analyzer->current_scope = old_scope;
            scope_destroy(block_scope);

            return does_return;
        }
        default: {
            diag_error(analyzer->diagnostics, stmt->location, "Unknown statement kind: %d", stmt->kind);
            return false;
        }
    }

    return false;
}

static void analyze_function(SemanticAnalyzer *analyzer, const FunctionNode *func, Scope *global) {
    Scope *func_scope = scope_create(global, SCOPE_FUNCTION);
    analyzer->current_scope = func_scope;

    analyzer->current_function_return_type = func->return_type;
    analyzer->current_function_return_ptr_level = func->return_pointer_level;

    for (int i = 0; i < func->param_count; i++) {
        const ParamNode *param = &func->params[i];

        if (scope_lookup(func_scope, param->name)) {
            diag_error(analyzer->diagnostics, param->location, "Duplicate parameter '%s'", param->name);
            continue;
        }

        Symbol *sym = malloc(sizeof(Symbol));
        sym->name = strdup(param->name);
        sym->kind = SYM_PARAMETER;
        sym->type = param->type;
        sym->pointer_level = param->pointer_level;
        sym->is_const = param->is_const;
        sym->location = param->location;

        scope_add_symbol(func_scope, sym);
    }

    const bool returns_value = analyze_statement(analyzer, func->body, func->return_type, func->return_pointer_level);

    if (func->return_type != TYPE_VOID && !returns_value) {
        diag_warning(analyzer->diagnostics, func->location,
                    "Function '%s' is declared to return '%s' but not all control paths return a value",
                    func->name, type_to_string(func->return_type)
        );
    }

    scope_destroy(func_scope);
    analyzer->current_scope = global;
}
