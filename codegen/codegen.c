#include "codegen.h"

#include <ctype.h>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Analysis.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *name;
    LLVMValueRef value;
    TypeKind type;
    int pointer_level;
    int array_size;
} CodegenSymbol;

static CodegenSymbol *local_vars = NULL;
static int local_var_count = 0;
static int local_var_capacity = 0;

static CodegenSymbol *global_vars = NULL;
static int global_var_count = 0;
static int global_var_capacity = 0;

static LLVMBasicBlockRef current_break_target = NULL;
static LLVMBasicBlockRef current_continue_target = NULL;

static void add_local_var(const char *name, const LLVMValueRef value, const TypeKind type, const int pointer_level, int array_size) {
    if (local_var_count >= local_var_capacity) {
        local_var_capacity = local_var_capacity == 0 ? 16 : local_var_capacity * 2;
        local_vars = realloc(local_vars, sizeof(CodegenSymbol) * local_var_capacity);
    }

    local_vars[local_var_count].name = strdup(name);
    local_vars[local_var_count].value = value;
    local_vars[local_var_count].type = type;
    local_vars[local_var_count].pointer_level = pointer_level;
    local_vars[local_var_count].array_size = array_size;
    local_var_count++;
}

static void add_global_var(const char *name, const LLVMValueRef value, const TypeKind type, const int pointer_level, int array_size) {
    if (global_var_count >= global_var_capacity) {
        global_var_capacity = global_var_capacity == 0 ? 16 : global_var_capacity * 2;
        global_vars = realloc(global_vars, sizeof(CodegenSymbol) * global_var_capacity);
    }

    global_vars[global_var_count].name = strdup(name);
    global_vars[global_var_count].value = value;
    global_vars[global_var_count].type = type;
    global_vars[global_var_count].pointer_level = pointer_level;
    global_vars[global_var_count].array_size = array_size;
    global_var_count++;
}

static CodegenSymbol* lookup_local_var_full(const char *name) {
    // Search from the end to find most recently declared variable
    for (int i = local_var_count - 1; i >= 0; i--) {
        if (strcmp(local_vars[i].name, name) == 0) {
            return &local_vars[i];
        }
    }
    return NULL;
}

static CodegenSymbol* lookup_global_var_full(const char *name) {
    // Search from the end to find most recently declared variable
    for (int i = global_var_count - 1; i >= 0; i--) {
        if (strcmp(global_vars[i].name, name) == 0) {
            return &global_vars[i];
        }
    }

    return NULL;
}

static LLVMValueRef lookup_global_var(const char *name) {
    CodegenSymbol *sym = lookup_global_var_full(name);
    return sym ? sym->value : NULL;
}

static LLVMValueRef lookup_local_var(const char *name) {
    CodegenSymbol *sym = lookup_local_var_full(name);
    return sym ? sym->value : NULL;
}

static LLVMValueRef lookup_var(const char *name) {
    LLVMValueRef val = lookup_local_var(name);
    if (!val) val = lookup_global_var(name);
    return val;
}

static CodegenSymbol* lookup_var_full(const char *name) {
    CodegenSymbol *sym = lookup_local_var_full(name);
    if (!sym) sym = lookup_global_var_full(name);
    return sym;
}

static void clear_local_vars(void) {
    for (int i = 0; i < local_var_count; i++) {
        free(local_vars[i].name);
    }
    local_var_count = 0;
}

static LLVMModuleRef module;
static LLVMBuilderRef builder;
static LLVMContextRef context;

static LLVMTypeRef get_llvm_type(const TypeKind type) {
    switch (type) {
        case TYPE_INT:     return LLVMInt32TypeInContext(context);
        case TYPE_LONG:    return LLVMInt64TypeInContext(context);
        case TYPE_CHAR:    return LLVMInt8TypeInContext(context);
        case TYPE_FLOAT:   return LLVMFloatTypeInContext(context);
        case TYPE_DOUBLE:  return LLVMDoubleTypeInContext(context);
        case TYPE_BOOLEAN: return LLVMInt1TypeInContext(context);
        case TYPE_VOID:    return LLVMVoidTypeInContext(context);
        case TYPE_STRING:  return LLVMPointerType(LLVMInt8TypeInContext(context), 0);
        default: {
            fprintf(stderr, "Unsupported type in codegen\n");
            exit(1);
        }
    }
}

static LLVMTypeRef get_llvm_type_with_pointers(const TypeKind kind, int pointer_level) {
    LLVMTypeRef base_type = get_llvm_type(kind);
    for (int i = 0; i < pointer_level; i++) {
        base_type = LLVMPointerType(base_type, 0);
    }

    return base_type;
}

static LLVMValueRef convert_to_type(LLVMValueRef value, TypeKind from_type, TypeKind to_type) {
    if (from_type == to_type) return value;

    LLVMTypeRef from_llvm = get_llvm_type(from_type);
    LLVMTypeRef to_llvm = get_llvm_type(to_type);

    // Both are integers of different sizes
    if ((from_type == TYPE_INT || from_type == TYPE_LONG || from_type == TYPE_CHAR) &&
        (to_type == TYPE_INT || to_type == TYPE_LONG || to_type == TYPE_CHAR)) {

        unsigned from_bits = LLVMGetIntTypeWidth(from_llvm);
        unsigned to_bits = LLVMGetIntTypeWidth(to_llvm);

        if (from_bits < to_bits) {
            return LLVMBuildSExt(builder, value, to_llvm, "sext");
        } else if (from_bits > to_bits) {
            return LLVMBuildTrunc(builder, value, to_llvm, "trunc");
        }
    }

    return value;
}

static LLVMValueRef codegen_expression(const ExprNode* expr) {
    if (!expr) {
        fprintf(stderr, "Error: null expression in codegen\n");
        exit(1);
    }
    switch (expr->kind) {
        case EXPR_NUMBER: {
            if (expr->type == TYPE_BOOLEAN) {
                const int value = atoi(expr->text);
                return LLVMConstInt(LLVMInt1TypeInContext(context), value != 0, 0);
            }

            const int value = atoi(expr->text);
            return LLVMConstInt(LLVMInt32TypeInContext(context), value, 0);
        }
        case EXPR_STRING_LITERAL: {
            // create a global string constant
            LLVMValueRef str_const = LLVMBuildGlobalStringPtr(builder, expr->text, "strtmp");
            return str_const;
        }
        case EXPR_VAR: {
            CodegenSymbol *sym = lookup_var_full(expr->text);

            if (!sym) {
                fprintf(stderr, "Codegen error: undefined variable '%s'\n", expr->text);
                exit(1);
            }

            LLVMValueRef var = sym->value;

            // If it is a stack array, decay to pointer to first element
            if (sym->array_size > 0) {
                LLVMTypeRef element_type = get_llvm_type_with_pointers(sym->type, sym->pointer_level - 1);
                LLVMTypeRef array_type = LLVMArrayType(element_type, sym->array_size);

                LLVMValueRef indices[2];
                indices[0] = LLVMConstInt(LLVMInt32TypeInContext(context), 0, 0);
                indices[1] = LLVMConstInt(LLVMInt32TypeInContext(context), 0, 0);

                return LLVMBuildGEP2(builder, array_type, var, indices, 2, "arraydecay");
            }

            // Normal variable: Load with the correct type including pointer level
            LLVMTypeRef var_type = get_llvm_type_with_pointers(expr->type, expr->pointer_level);
            return LLVMBuildLoad2(builder, var_type, var, "loadtmp");
        }
        case EXPR_BINOP: {
            if (!expr->binop.left) {
                fprintf(stderr, "Error: binary op has null left operand\n");
                exit(1);
            }
            if (!expr->binop.right) {
                fprintf(stderr, "Error: binary op has null right operand\n");
                exit(1);
            }

            LLVMValueRef left = codegen_expression(expr->binop.left);
            LLVMValueRef right = codegen_expression(expr->binop.right);

            if (expr->binop.op == BIN_EQUAL || expr->binop.op == BIN_NOT_EQUAL ||
                expr->binop.op == BIN_LESS || expr->binop.op == BIN_GREATER ||
                expr->binop.op == BIN_LESS_EQ || expr->binop.op == BIN_GREATER_EQ) {

                TypeKind left_type = expr->binop.left->type;
                TypeKind right_type = expr->binop.right->type;

                if (left_type != right_type) {
                    if (left_type == TYPE_CHAR && right_type == TYPE_INT) {
                        left = convert_to_type(left, TYPE_CHAR, TYPE_INT);
                    } else if (left_type == TYPE_INT && right_type == TYPE_CHAR) {
                        right = convert_to_type(right, TYPE_CHAR, TYPE_INT);
                    }
                }
            }

            switch (expr->binop.op) {
                case BIN_ADD: {
                    LLVMTypeRef left_type = LLVMTypeOf(left);
                    LLVMTypeRef right_type = LLVMTypeOf(right);

                    if (LLVMGetTypeKind(left_type) == LLVMPointerTypeKind) {
                        // Pointer + integer: use GEP
                        // Use i8 as the element type for pointer arithmetic
                        LLVMTypeRef i8_type = LLVMInt8TypeInContext(context);
                        return LLVMBuildGEP2(builder, i8_type, left, &right, 1, "addtmp");
                    } else if (LLVMGetTypeKind(right_type) == LLVMPointerTypeKind) {
                        // Integer + pointer: use GEP
                        LLVMTypeRef i8_type = LLVMInt8TypeInContext(context);
                        return LLVMBuildGEP2(builder, i8_type, right, &left, 1, "addtmp");
                    } else {
                        // Regular integer addition
                        return LLVMBuildAdd(builder, left, right, "addtmp");
                    }
                }
                case BIN_SUB: {
                    // Check if left operand is a pointer
                    LLVMTypeRef left_type = LLVMTypeOf(left);

                    if (LLVMGetTypeKind(left_type) == LLVMPointerTypeKind) {
                        // Pointer - integer: negate the offset and use GEP
                        LLVMValueRef neg_right = LLVMBuildNeg(builder, right, "negtmp");
                        LLVMTypeRef i8_type = LLVMInt8TypeInContext(LLVMGetGlobalContext());
                        return LLVMBuildGEP2(builder, i8_type, left, &neg_right, 1, "subtmp");
                    } else {
                        // Regular integer subtraction
                        return LLVMBuildSub(builder, left, right, "subtmp");
                    }
                }
                case BIN_MUL: return LLVMBuildMul(builder, left, right, "multmp");
                case BIN_DIV: return LLVMBuildSDiv(builder, left, right, "divtmp");
                case BIN_ASSIGN: {
                    LLVMValueRef rhs_val = codegen_expression(expr->binop.right);
                    LLVMValueRef lhs_ptr;

                    if (expr->binop.left->kind == EXPR_VAR) {
                        // Assigning to a variable: x = 5
                        lhs_ptr = lookup_var(expr->binop.left->text);

                        if (!lhs_ptr) {
                            fprintf(stderr, "Assignment to undefined variable '%s'\n", expr->binop.left->text);
                            exit(1);
                        }
                    } else if (expr->binop.left->kind == EXPR_UNARY && expr->binop.left->unary.op == UNARY_DEREF) {
                        // Dereferenced pointer assignment: *ptr = value
                        // Just get the pointer value (don't load from it)
                        lhs_ptr = codegen_expression(expr->binop.left->unary.operand);
                    } else if (expr->binop.left->kind == EXPR_ARRAY_INDEX) {
                        // Array element assignment: arr[i] = value
                        // We need to compute the element pointer without loading the value

                        ExprNode *array_expr = expr->binop.left->array_index.array;
                        ExprNode *index_expr = expr->binop.left->array_index.index;

                        LLVMValueRef array_ptr;

                        if (array_expr->kind == EXPR_VAR) {
                            array_ptr = lookup_var(array_expr->text);

                            if (!array_ptr) {
                                fprintf(stderr, "Codegen error: undefined array variable '%s'\n", array_expr->text);
                                exit(1);
                            }
                        } else {
                            array_ptr = codegen_expression(array_expr);
                        }

                        LLVMValueRef index_val = codegen_expression(index_expr);

                        LLVMTypeRef element_type = get_llvm_type_with_pointers(
                            expr->binop.left->type,
                            expr->binop.left->pointer_level
                        );

                        // Check if this is an actual array or a pointer
                        CodegenSymbol *sym = NULL;
                        if (array_expr->kind == EXPR_VAR) {
                            sym = lookup_var_full(array_expr->text);
                        }

                        if (sym && sym->array_size > 0) {
                            // Actual array - use two-index GEP
                            // Reconstruct array type [Size x ElementType]
                            LLVMTypeRef base_element_type = get_llvm_type_with_pointers(sym->type, sym->pointer_level - 1);
                            LLVMTypeRef array_type = LLVMArrayType(base_element_type, sym->array_size);

                            LLVMValueRef indices[2];
                            indices[0] = LLVMConstInt(LLVMInt32TypeInContext(context), 0, 0);
                            indices[1] = index_val;
                            lhs_ptr = LLVMBuildGEP2(builder, array_type, array_ptr, indices, 2, "arrayidx");
                        } else {
                            // Pointer - load then single-index GEP
                            LLVMTypeRef ptr_type = get_llvm_type_with_pointers(array_expr->type, array_expr->pointer_level);
                            LLVMValueRef loaded_ptr = LLVMBuildLoad2(builder, ptr_type, array_ptr, "loadptr");
                            lhs_ptr = LLVMBuildGEP2(builder, element_type, loaded_ptr, &index_val, 1, "arrayidx");
                        }
                    } else {
                        fprintf(stderr, "Invalid lvalue in assignment\n");
                        exit(1);
                    }

                    LLVMBuildStore(builder, rhs_val, lhs_ptr);
                    return rhs_val;
                }
                case BIN_LESS: return LLVMBuildICmp(builder, LLVMIntSLT, left, right, "cmptmp");
                case BIN_GREATER: return LLVMBuildICmp(builder, LLVMIntSGT, left, right, "cmptmp");
                case BIN_LESS_EQ: return LLVMBuildICmp(builder, LLVMIntSLE, left, right, "cmptmp");
                case BIN_GREATER_EQ: return LLVMBuildICmp(builder, LLVMIntSGE, left, right, "cmptmp");
                case BIN_EQUAL: return LLVMBuildICmp(builder, LLVMIntEQ, left, right, "cmptmp");
                case BIN_NOT_EQUAL: return LLVMBuildICmp(builder, LLVMIntNE, left, right, "neqtmp");
                case BIN_LOGICAL_AND: {
                    // LLVM Logical And works on i1 (1-bit integers)
                    // Convert operands to boolean (i1) by checking != 0
                    LLVMValueRef l_bool = LLVMBuildIsNotNull(builder, left, "l_bool");
                    LLVMValueRef r_bool = LLVMBuildIsNotNull(builder, right, "r_bool");
                    return LLVMBuildAnd(builder, l_bool, r_bool, "andtmp");
                }
                case BIN_LOGICAL_OR: {
                    LLVMValueRef l_bool = LLVMBuildIsNotNull(builder, left, "l_bool");
                    LLVMValueRef r_bool = LLVMBuildIsNotNull(builder, right, "r_bool");
                    return LLVMBuildOr(builder, l_bool, r_bool, "ortmp");
                }
                default: {
                    fprintf(stderr, "Unsupported binary operator\n");
                    exit(1);
                }
            }
        }
        case EXPR_UNARY: {
            if (expr->unary.op == UNARY_DEREF) {
                // Evaluate the operand to get the pointer value.
                // Note: For EXPR_VAR referring to an array, this correctly returns the decayed pointer.
                LLVMValueRef ptr = codegen_expression(expr->unary.operand);

                LLVMTypeRef deref_type = get_llvm_type_with_pointers(expr->type, expr->pointer_level);
                return LLVMBuildLoad2(builder, deref_type, ptr, "deref");
            }

            if (expr->unary.op == UNARY_ADDR_OF) {
                if (expr->unary.operand->kind == EXPR_VAR) {
                    // Return the pointer to the variable (don't load it)
                    LLVMValueRef var = lookup_var(expr->unary.operand->text);
                    return var;
                } else if (expr->unary.operand->kind == EXPR_ARRAY_INDEX) {
                    // Taking address of array element: &arr[i]
                    // We need to compute the element pointer without loading the value

                    ExprNode *array_expr = expr->unary.operand->array_index.array;
                    ExprNode *index_expr = expr->unary.operand->array_index.index;

                    LLVMValueRef array_ptr;

                    if (array_expr->kind == EXPR_VAR) {
                        array_ptr = lookup_var(array_expr->text);

                        if (!array_ptr) {
                            fprintf(stderr, "Codegen error: undefined array variable '%s'\n", array_expr->text);
                            exit(1);
                        }
                    } else {
                        // array_ptr here is the alloca (value)
                        // Should probably be handled? But context above suggests logic for simple vars.
                        // For EXPR_ARRAY_INDEX on non-var, we probably can't take address easily unless it's an lvalue
                    }

                    LLVMValueRef index_val = codegen_expression(index_expr);

                    LLVMTypeRef element_type = get_llvm_type_with_pointers(
                        expr->unary.operand->type,
                        expr->unary.operand->pointer_level
                    );

                    // Check if this is an actual array or a pointer
                    CodegenSymbol *sym = NULL;
                    if (array_expr->kind == EXPR_VAR) {
                        sym = lookup_var_full(array_expr->text);
                    }

                    LLVMValueRef element_ptr;

                    if (sym && sym->array_size > 0) {
                        // Actual array - use two-index GEP
                        LLVMTypeRef base_element_type = get_llvm_type_with_pointers(sym->type, sym->pointer_level - 1);
                        LLVMTypeRef array_type = LLVMArrayType(base_element_type, sym->array_size);

                        LLVMValueRef indices[2];
                        indices[0] = LLVMConstInt(LLVMInt32TypeInContext(context), 0, 0);
                        indices[1] = index_val;
                        element_ptr = LLVMBuildGEP2(builder, array_type, array_ptr, indices, 2, "arrayaddr");
                    } else {
                        // Pointer - load then single-index GEP
                        LLVMTypeRef ptr_type = get_llvm_type_with_pointers(array_expr->type, array_expr->pointer_level);
                        LLVMValueRef loaded_ptr = LLVMBuildLoad2(builder, ptr_type, array_ptr, "loadptr");
                        element_ptr = LLVMBuildGEP2(builder, element_type, loaded_ptr, &index_val, 1, "arrayaddr");
                    }

                    // Return the pointer (don't load the value)
                    return element_ptr;
                } else if (expr->unary.operand->kind == EXPR_UNARY && expr->unary.operand->unary.op == UNARY_DEREF) {
                    // Taking address of dereferenced pointer: &(*ptr)
                    // This just evaluates to the pointer itself
                    return codegen_expression(expr->unary.operand->unary.operand);
                }

                fprintf(stderr, "Codegen error: cannot take address of this expression\n");
                exit(1);
            }

            // For other unary ops, evaluate the operand normally
            LLVMValueRef operand = codegen_expression(expr->unary.operand);

            if (expr->unary.op == UNARY_NOT) {
                LLVMValueRef zero = LLVMConstNull(LLVMTypeOf(operand));
                return LLVMBuildICmp(builder, LLVMIntEQ, operand, zero, "nottmp");
            }

            if (expr->unary.op == UNARY_NEG) {
                // check if we need float negation or integer negation
                if (expr->type == TYPE_FLOAT || expr->type == TYPE_DOUBLE) {
                    return LLVMBuildFNeg(builder, operand, "negtmp");
                }

                return LLVMBuildNeg(builder, operand, "negtmp");
            }

            break;
        }
        case EXPR_CALL: {
            const LLVMValueRef func = LLVMGetNamedFunction(module, expr->call.function_name);
            if (!func) {
                fprintf(stderr, "Codegen error: undefined function '%s'\n", expr->call.function_name);
                exit(1);
            }

            LLVMValueRef *args = malloc(sizeof(LLVMValueRef) * expr->call.arg_count);
            for (int i = 0; i < expr->call.arg_count; i++) {
                args[i] = codegen_expression(expr->call.args[i]);
            }

            const LLVMTypeRef func_type = LLVMGlobalGetValueType(func);

            // Check if function returns void
            LLVMTypeRef ret_type = LLVMGetReturnType(func_type);
            const char *call_name = (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind) ? "" : "calltmp";

            const LLVMValueRef result = LLVMBuildCall2(builder, func_type, func, args, expr->call.arg_count, call_name);

            free(args);
            return result;
        }
        case EXPR_ARRAY_INDEX: {
            LLVMValueRef array_ptr;

            if (expr->array_index.array->kind == EXPR_VAR) {
                // if it's a variable, get its pointer (don't load it yet)
                array_ptr = lookup_var(expr->array_index.array->text);

                if (!array_ptr) {
                    fprintf(stderr, "Codegen error: undefined array variable '%s'\n", expr->array_index.array->text);
                    exit(1);
                }
            } else {
                // for other expressions (like nested array access: arr[i][j])
                array_ptr = codegen_expression(expr->array_index.array);
            }

            LLVMValueRef index_val = codegen_expression(expr->array_index.index);
            LLVMTypeRef element_type = get_llvm_type_with_pointers(expr->type, expr->pointer_level);

            // for arrays declared as int[N], we need two indices: [0, index]
            // for pointers (int*), we need one index: [index]
            LLVMValueRef indices[2];
            LLVMValueRef element_ptr;

            // check if this is an array (allocated with alloca) or a pointer
            CodegenSymbol *sym = NULL;
            if (expr->array_index.array->kind == EXPR_VAR) {
                sym = lookup_var_full(expr->array_index.array->text);
            }

            if (sym && sym->array_size > 0) {
                // this is an actual array (pointer_level was increased during allocation)
                // use two-index GEP: first index 0 to get array start, second is actual index
                LLVMTypeRef base_element_type = get_llvm_type_with_pointers(sym->type, sym->pointer_level - 1);
                LLVMTypeRef array_type = LLVMArrayType(base_element_type, sym->array_size);

                indices[0] = LLVMConstInt(LLVMInt32TypeInContext(context), 0, 0);
                indices[1] = index_val;

                element_ptr = LLVMBuildGEP2(builder, array_type, array_ptr, indices, 2, "arrayidx");
            } else {
                LLVMTypeRef ptr_type = get_llvm_type_with_pointers(expr->array_index.array->type, expr->array_index.array->pointer_level);
                LLVMValueRef loaded_ptr = LLVMBuildLoad2(builder, ptr_type, array_ptr, "loadptr");
                element_ptr = LLVMBuildGEP2(builder, element_type, loaded_ptr, &index_val, 1, "arrayidx");
            }

            // Load the value from the computed address
            return LLVMBuildLoad2(builder, element_type, element_ptr, "arrayval");
        }
        default: {
            fprintf(stderr, "Unsupported expression kind: %d\n", expr->kind);
            exit(1);
        }
    }
}

static void codegen_statement(const StmtNode* stmt) {
    switch (stmt->kind) {
        case STMT_RETURN: {
            if (stmt->return_stmt.expr) {
                const LLVMValueRef ret_val = codegen_expression(stmt->return_stmt.expr);
                LLVMBuildRet(builder, ret_val);
            } else {
                LLVMBuildRetVoid(builder);
            }

            break;
        }
        case STMT_IF: {
            // evaluate condition
            LLVMValueRef cond_val = codegen_expression(stmt->if_stmt.condition);

            // LLVM expects i1 for condition
            cond_val = LLVMBuildTrunc(builder, cond_val, LLVMInt1TypeInContext(context), "ifcond");

            // create blocks
            LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
            LLVMBasicBlockRef then_block = LLVMAppendBasicBlockInContext(context, func, "then");
            LLVMBasicBlockRef else_block = stmt->if_stmt.else_stmt ? LLVMAppendBasicBlockInContext(context, func, "else") : NULL;
            LLVMBasicBlockRef merge_block = LLVMAppendBasicBlockInContext(context, func, "ifcont");

            // conditional branch
            if (else_block) {
                LLVMBuildCondBr(builder, cond_val, then_block, else_block);
            } else {
                LLVMBuildCondBr(builder, cond_val, then_block, merge_block);
            }

            // generate 'then' block
            LLVMPositionBuilderAtEnd(builder, then_block);
            codegen_statement(stmt->if_stmt.then_stmt);
            // Only add branch if block is not already terminated
            if (!LLVMGetBasicBlockTerminator(then_block)) {
                LLVMBuildBr(builder, merge_block);
            }

            // generate 'else' block if present
            if (else_block) {
                LLVMPositionBuilderAtEnd(builder, else_block);
                codegen_statement(stmt->if_stmt.else_stmt);
                // Only add branch if block is not already terminated
                if (!LLVMGetBasicBlockTerminator(else_block)) {
                    LLVMBuildBr(builder, merge_block);
                }
            }

            // continue after if
            LLVMPositionBuilderAtEnd(builder, merge_block);
            break;
        }
        case STMT_WHILE: {
            LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

            LLVMBasicBlockRef cond_block = LLVMAppendBasicBlockInContext(context, func, "while_cond");
            LLVMBasicBlockRef body_block = LLVMAppendBasicBlockInContext(context, func, "while_body");
            LLVMBasicBlockRef end_block  = LLVMAppendBasicBlockInContext(context, func, "while_end");

            // jump to condition
            LLVMBuildBr(builder, cond_block);

            // Condition Block
            LLVMPositionBuilderAtEnd(builder, cond_block);
            LLVMValueRef cond_val = codegen_expression(stmt->while_stmt.condition);
            // Ensure i1
            cond_val = LLVMBuildTrunc(builder, cond_val, LLVMInt1TypeInContext(context), "booltmp");
            LLVMBuildCondBr(builder, cond_val, body_block, end_block);

            // Body Block
            LLVMPositionBuilderAtEnd(builder, body_block);

            // Save previous loop targets (recursion support)
            LLVMBasicBlockRef old_break = current_break_target;
            LLVMBasicBlockRef old_cont  = current_continue_target;
            current_break_target = end_block;
            current_continue_target = cond_block;

            codegen_statement(stmt->while_stmt.body);

            // Loop back if not terminated
            if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(builder))) {
                LLVMBuildBr(builder, cond_block);
            }

            // Restore targets
            current_break_target = old_break;
            current_continue_target = old_cont;

            // End Block
            LLVMPositionBuilderAtEnd(builder, end_block);
            break;
        }
        case STMT_FOR: {
            LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));

            // IMPORTANT: If init is a variable declaration, we need to hoist the
            // allocation outside the loop but keep the initialization inside
            LLVMValueRef init_alloca = NULL;
            StmtNode *init_stmt = stmt->for_stmt.init;

            if (init_stmt && init_stmt->kind == STMT_VAR_DECL) {
                // Allocate the variable in the current block (before loop)
                LLVMTypeRef var_type;

                if (init_stmt->var_decl.array_size > 0) {
                    LLVMTypeRef element_type = get_llvm_type_with_pointers(
                       init_stmt->var_decl.type,
                       init_stmt->var_decl.pointer_level
                    );
                    var_type = LLVMArrayType(element_type, init_stmt->var_decl.array_size);

                    init_alloca = LLVMBuildAlloca(builder, var_type, init_stmt->var_decl.name);
                    // Note: Arrays decay to pointers, so store with pointer_level + 1
                    add_local_var(init_stmt->var_decl.name, init_alloca, init_stmt->var_decl.type, init_stmt->var_decl.pointer_level + 1, init_stmt->var_decl.array_size);
                } else {
                    var_type = get_llvm_type_with_pointers(
                       init_stmt->var_decl.type,
                       init_stmt->var_decl.pointer_level
                    );
                    init_alloca = LLVMBuildAlloca(builder, var_type, init_stmt->var_decl.name);
                    add_local_var(init_stmt->var_decl.name, init_alloca, init_stmt->var_decl.type, init_stmt->var_decl.pointer_level, 0);
                }

                // Initialize the variable if there's an initializer
                if (init_stmt->var_decl.initializer) {
                    if (init_stmt->var_decl.array_size > 0) {
                        fprintf(stderr, "Array initializers not yet supported in for-loop\n");
                        exit(1);
                    }

                    LLVMValueRef init_val = codegen_expression(init_stmt->var_decl.initializer);
                    if (init_stmt->var_decl.pointer_level == 0) {
                        init_val = convert_to_type(init_val, init_stmt->var_decl.initializer->type, init_stmt->var_decl.type);
                    }

                    LLVMBuildStore(builder, init_val, init_alloca);
                }
            } else if (init_stmt) {
                // Regular statement (like expression statement)
                codegen_statement(init_stmt);
            }

            LLVMBasicBlockRef cond_block = LLVMAppendBasicBlockInContext(context, func, "for_cond");
            LLVMBasicBlockRef body_block = LLVMAppendBasicBlockInContext(context, func, "for_body");
            LLVMBasicBlockRef inc_block  = LLVMAppendBasicBlockInContext(context, func, "for_inc");
            LLVMBasicBlockRef end_block  = LLVMAppendBasicBlockInContext(context, func, "for_end");

            LLVMBuildBr(builder, cond_block);

            // Condition
            LLVMPositionBuilderAtEnd(builder, cond_block);
            if (stmt->for_stmt.condition) {
                LLVMValueRef cond_val = codegen_expression(stmt->for_stmt.condition);
                cond_val = LLVMBuildTrunc(builder, cond_val, LLVMInt1TypeInContext(context), "booltmp");
                LLVMBuildCondBr(builder, cond_val, body_block, end_block);
            } else {
                LLVMBuildBr(builder, body_block);
            }

            // Body
            LLVMPositionBuilderAtEnd(builder, body_block);

            LLVMBasicBlockRef old_break = current_break_target;
            LLVMBasicBlockRef old_cont  = current_continue_target;
            current_break_target = end_block;
            current_continue_target = inc_block;

            codegen_statement(stmt->for_stmt.body);

            if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(builder))) {
                LLVMBuildBr(builder, inc_block);
            }

            current_break_target = old_break;
            current_continue_target = old_cont;

            // Increment
            LLVMPositionBuilderAtEnd(builder, inc_block);
            if (stmt->for_stmt.increment) {
                codegen_expression(stmt->for_stmt.increment);
            }
            LLVMBuildBr(builder, cond_block);

            // End
            LLVMPositionBuilderAtEnd(builder, end_block);

            // Remove the loop variable from local_vars if it was declared in init
            if (stmt->for_stmt.init && stmt->for_stmt.init->kind == STMT_VAR_DECL) {
                const char *var_name = stmt->for_stmt.init->var_decl.name;
                // Find and remove this variable
                for (int i = local_var_count - 1; i >= 0; i--) {
                    if (strcmp(local_vars[i].name, var_name) == 0) {
                        free(local_vars[i].name);
                        // Shift remaining variables down
                        for (int j = i; j < local_var_count - 1; j++) {
                            local_vars[j] = local_vars[j + 1];
                        }
                        local_var_count--;
                        break;
                    }
                }
            }

            break;
        }
        case STMT_BREAK: {
            if (!current_break_target) {
                fprintf(stderr, "Codegen Error: Break outside loop (logic error)\n");
                exit(1);
            }
            LLVMBuildBr(builder, current_break_target);
            // Create a dummy block so subsequent code doesn't crash LLVM builder
            // and position the builder there for any following code
            LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
            LLVMBasicBlockRef dead_block = LLVMAppendBasicBlockInContext(context, func, "dead_after_break");
            LLVMPositionBuilderAtEnd(builder, dead_block);
            // Add an unreachable instruction to terminate this dead block
            LLVMBuildUnreachable(builder);
            break;
        }
        case STMT_CONTINUE: {
            if (!current_continue_target) {
                fprintf(stderr, "Codegen Error: Continue outside loop (logic error)\n");
                exit(1);
            }
            LLVMBuildBr(builder, current_continue_target);
            // Create a dummy block
            LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
            LLVMBasicBlockRef dead_block = LLVMAppendBasicBlockInContext(context, func, "dead_after_continue");
            LLVMPositionBuilderAtEnd(builder, dead_block);
            // Add an unreachable instruction to terminate this dead block
            LLVMBuildUnreachable(builder);
            break;
        }
        case STMT_ASM: {
            // Build output operands
            LLVMValueRef *output_vals = NULL;
            LLVMTypeRef *output_types = NULL;
            if (stmt->asm_stmt.output_count > 0) {
                output_vals = malloc(sizeof(LLVMValueRef) * stmt->asm_stmt.output_count);
                output_types = malloc(sizeof(LLVMTypeRef) * stmt->asm_stmt.output_count);

                for (size_t i = 0; i < stmt->asm_stmt.output_count; i++) {
                    ExprNode *output_expr = stmt->asm_stmt.outputs[i];
                    if (output_expr->kind == EXPR_VAR) {
                        output_vals[i] = lookup_var(output_expr->text);
                        CodegenSymbol *sym = lookup_var_full(output_expr->text);
                        output_types[i] = get_llvm_type(sym->type);
                    } else {
                        fprintf(stderr, "Output operand must be a variable\n");
                        exit(1);
                    }
                }
            }

            // Build input operands
            LLVMValueRef *input_vals = NULL;
            if (stmt->asm_stmt.input_count > 0) {
                input_vals = malloc(sizeof(LLVMValueRef) * stmt->asm_stmt.input_count);
                for (size_t i = 0; i < stmt->asm_stmt.input_count; i++) {
                    input_vals[i] = codegen_expression(stmt->asm_stmt.inputs[i]);

                    // Zero-extend 32-bit integers to 64-bit for inline asm compatibility
                    LLVMTypeRef input_type = LLVMTypeOf(input_vals[i]);
                    if (LLVMGetTypeKind(input_type) == LLVMIntegerTypeKind &&
                        LLVMGetIntTypeWidth(input_type) == 32) {
                        input_vals[i] = LLVMBuildZExt(builder, input_vals[i], LLVMInt64Type(), "zext");
                    }
                }
            }

            // Convert assembly code from $ syntax to ${} syntax for LLVM Intel dialect
            // $0, $1, etc. -> ${0}, ${1}, etc.
            size_t asm_len = strlen(stmt->asm_stmt.assembly_code);
            char *converted_asm = malloc(asm_len * 3 + 1); // worst case: each $ becomes ${N}
            size_t write_pos = 0;

            for (size_t i = 0; i < asm_len; i++) {
                if (stmt->asm_stmt.assembly_code[i] == '$' &&
                    i + 1 < asm_len &&
                    isdigit(stmt->asm_stmt.assembly_code[i + 1])) {
                    // Replace $N with ${N}
                    converted_asm[write_pos++] = '$';
                    converted_asm[write_pos++] = '{';
                    // Copy all consecutive digits
                    while (i + 1 < asm_len && isdigit(stmt->asm_stmt.assembly_code[i + 1])) {
                        i++;
                        converted_asm[write_pos++] = stmt->asm_stmt.assembly_code[i];
                    }
                    converted_asm[write_pos++] = '}';
                } else {
                    converted_asm[write_pos++] = stmt->asm_stmt.assembly_code[i];
                }
            }
            converted_asm[write_pos] = '\0';

            // Build constraint string - calculate total size first
            size_t total_len = 0;
            for (size_t i = 0; i < stmt->asm_stmt.output_count; i++) {
                total_len += strlen(stmt->asm_stmt.output_constraints[i]) + 1;
            }
            for (size_t i = 0; i < stmt->asm_stmt.input_count; i++) {
                total_len += strlen(stmt->asm_stmt.input_constraints[i]) + 1;
            }
            for (size_t i = 0; i < stmt->asm_stmt.clobber_count; i++) {
                total_len += strlen(stmt->asm_stmt.clobbers[i]) + 5;
            }

            char *constraint_str = malloc(total_len + 1);
            constraint_str[0] = '\0';

            // Add output constraints
            for (size_t i = 0; i < stmt->asm_stmt.output_count; i++) {
                strcat(constraint_str, stmt->asm_stmt.output_constraints[i]);
                if (i < stmt->asm_stmt.output_count - 1 || stmt->asm_stmt.input_count > 0 || stmt->asm_stmt.clobber_count > 0) {
                    strcat(constraint_str, ",");
                }
            }

            // Add input constraints
            for (size_t i = 0; i < stmt->asm_stmt.input_count; i++) {
                strcat(constraint_str, stmt->asm_stmt.input_constraints[i]);
                if (i < stmt->asm_stmt.input_count - 1 || stmt->asm_stmt.clobber_count > 0) {
                    strcat(constraint_str, ",");
                }
            }

            // Add clobbers
            for (size_t i = 0; i < stmt->asm_stmt.clobber_count; i++) {
                strcat(constraint_str, "~{");
                strcat(constraint_str, stmt->asm_stmt.clobbers[i]);
                strcat(constraint_str, "}");
                if (i < stmt->asm_stmt.clobber_count - 1) {
                    strcat(constraint_str, ",");
                }
            }

            // Determine return type based on outputs
            LLVMTypeRef return_type;
            if (stmt->asm_stmt.output_count == 0) {
                return_type = LLVMVoidType();
            } else if (stmt->asm_stmt.output_count == 1) {
                return_type = output_types[0];
            } else {
                return_type = LLVMStructType(output_types, stmt->asm_stmt.output_count, 0);
            }

            // Build function type
            LLVMTypeRef *param_types = NULL;
            if (stmt->asm_stmt.input_count > 0) {
                param_types = malloc(sizeof(LLVMTypeRef) * stmt->asm_stmt.input_count);
                for (size_t i = 0; i < stmt->asm_stmt.input_count; i++) {
                    param_types[i] = LLVMTypeOf(input_vals[i]);
                }
            }

            LLVMTypeRef asm_func_type = LLVMFunctionType(
                return_type,
                param_types,
                stmt->asm_stmt.input_count,
                0
            );

            // Create inline assembly
            LLVMValueRef asm_val = LLVMGetInlineAsm(
                asm_func_type,
                converted_asm,
                strlen(converted_asm),
                constraint_str,
                strlen(constraint_str),
                1, // hasSideEffects
                0, // isAlignStack
                LLVMInlineAsmDialectIntel,
                0  // canThrow
            );

            // Call the inline assembly
            LLVMValueRef result = LLVMBuildCall2(
                builder,
                asm_func_type,
                asm_val,
                input_vals,
                stmt->asm_stmt.input_count,
                ""
            );

            // Store results to output variables
            if (stmt->asm_stmt.output_count == 1) {
                LLVMBuildStore(builder, result, output_vals[0]);
            } else if (stmt->asm_stmt.output_count > 1) {
                for (size_t i = 0; i < stmt->asm_stmt.output_count; i++) {
                    LLVMValueRef extracted = LLVMBuildExtractValue(builder, result, i, "");
                    LLVMBuildStore(builder, extracted, output_vals[i]);
                }
            }

            // Cleanup
            free(converted_asm);
            free(constraint_str);
            if (output_vals) free(output_vals);
            if (output_types) free(output_types);
            if (input_vals) free(input_vals);
            if (param_types) free(param_types);

            break;
        }
        case STMT_VAR_DECL: {
            LLVMValueRef alloca;
            LLVMTypeRef var_type;

            if (stmt->var_decl.array_size > 0) {
                // Array declaration: int[5] arr;
                LLVMTypeRef element_type = get_llvm_type_with_pointers(
                    stmt->var_decl.type,
                    stmt->var_decl.pointer_level
                );
                var_type = LLVMArrayType(element_type, stmt->var_decl.array_size);

                // Check if variable already exists at this scope level
                LLVMValueRef existing = lookup_var(stmt->var_decl.name);

                if (existing) {
                    alloca = existing;
                } else {
                    alloca = LLVMBuildAlloca(builder, var_type, stmt->var_decl.name);
                    // Arrays decay to pointers, so we store them with pointer_level + 1
                    // Track array_size for proper GEP generation later
                    add_local_var(stmt->var_decl.name, alloca, stmt->var_decl.type, stmt->var_decl.pointer_level + 1, stmt->var_decl.array_size);
                }

                // Array initializer support (if provided)
                if (stmt->var_decl.initializer) {
                    // For now, we'll handle simple cases
                    // You could extend this to handle initializer lists
                    fprintf(stderr, "Array initializers not yet supported\n");
                    exit(1);
                }
            } else {
                // Regular variable declaration
                var_type = get_llvm_type_with_pointers(stmt->var_decl.type, stmt->var_decl.pointer_level);

                // Check if variable already exists at this scope level
                LLVMValueRef existing = lookup_var(stmt->var_decl.name);

                if (existing) {
                    alloca = existing;
                } else {
                    alloca = LLVMBuildAlloca(builder, var_type, stmt->var_decl.name);
                    add_local_var(stmt->var_decl.name, alloca, stmt->var_decl.type, stmt->var_decl.pointer_level, 0);
                }

                if (stmt->var_decl.initializer) {
                    LLVMValueRef init_val = codegen_expression(stmt->var_decl.initializer);

                    if (stmt->var_decl.pointer_level == 0) {
                        init_val = convert_to_type(init_val, stmt->var_decl.initializer->type, stmt->var_decl.type);
                    }

                    LLVMBuildStore(builder, init_val, alloca);
                }
            }

            break;
        }
        case STMT_EXPR: {
            codegen_expression(stmt->expr_stmt.expr);
            break;
        }
        case STMT_COMPOUND: {
            for (int i = 0; i < stmt->compound.count; i++) {
                // Check if current block is already terminated (e.g., by return)
                LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
                if (LLVMGetBasicBlockTerminator(current_block)) {
                    // Block already terminated, skip remaining statements
                    break;
                }

                codegen_statement(stmt->compound.stmts[i]);
            }
            break;
        }
        default: {
            fprintf(stderr, "Unsupported statement kind: %d\n", stmt->kind);
            exit(1);
        }
    }
}

static void codegen_function(const FunctionNode* func) {
    // build parameter types array
    LLVMTypeRef *param_types = malloc(sizeof(LLVMTypeRef) * func->param_count);
    for (int i = 0; i < func->param_count; i++) {
        param_types[i] = get_llvm_type_with_pointers(func->params[i].type, func->params[i].pointer_level);
    }

    // function type
    const LLVMTypeRef ret_type = get_llvm_type_with_pointers(func->return_type, func->return_pointer_level);
    const LLVMTypeRef func_type = LLVMFunctionType(ret_type, param_types, func->param_count, 0);

    const LLVMValueRef llvm_func = LLVMAddFunction(module, func->name, func_type);

    // entry block
    const LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(context, llvm_func, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    // add parameters as local variables
    clear_local_vars();
    for (int i = 0; i < func->param_count; i++) {
        LLVMValueRef param = LLVMGetParam(llvm_func, i);
        LLVMSetValueName(param, func->params[i].name);

        LLVMTypeRef param_type = get_llvm_type_with_pointers(func->params[i].type, func->params[i].pointer_level);
        LLVMValueRef alloca = LLVMBuildAlloca(builder, param_type, func->params[i].name);
        LLVMBuildStore(builder, param, alloca);

        add_local_var(func->params[i].name, alloca, func->params[i].type, func->params[i].pointer_level, 0);
    }

    codegen_statement(func->body);

    // Add default return if block is not terminated
    LLVMBasicBlockRef current_block = LLVMGetInsertBlock(builder);
    if (!LLVMGetBasicBlockTerminator(current_block)) {
        if (func->return_type == TYPE_VOID && func->return_pointer_level == 0) {
            LLVMBuildRetVoid(builder);
        } else {
            // Return default value (0 for int, nullptr for pointers, etc.)
            LLVMValueRef default_val = LLVMConstNull(ret_type);
            LLVMBuildRet(builder, default_val);
        }
    }

    free(param_types);
}

void codegen_program_llvm(const ProgramNode* program, const char* output_file) {
    // init
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    // create context, module, builder
    context = LLVMContextCreate();
    module = LLVMModuleCreateWithNameInContext("my_module", context);
    builder = LLVMCreateBuilderInContext(context);

    printf("LLVM codegen initialized\n");

    // global variables
    for (int i = 0; i < program->global_count; ++i) {
        const GlobalVarNode *global_var = program->globals[i];

        LLVMTypeRef var_type;
        if (global_var->array_size > 0) {
            LLVMTypeRef element_type = get_llvm_type_with_pointers(global_var->kind, global_var->pointer_level);
            var_type = LLVMArrayType(element_type, global_var->array_size);
        } else {
            var_type = get_llvm_type_with_pointers(global_var->kind, global_var->pointer_level);
        }

        // Create the global variable in the module
        LLVMValueRef llvm_global = LLVMAddGlobal(module, var_type, global_var->name);

        LLVMValueRef init_value;
        if (global_var->initializer) {
            // for now, only handle constant expressions
            if (global_var->initializer->kind == EXPR_NUMBER) {
                // It's a number literal
                int value = atoi(global_var->initializer->text);
                init_value = LLVMConstInt(var_type, value, 0);
            } else if (global_var->initializer->kind == EXPR_STRING_LITERAL) {
                // It's a string literal
                init_value = LLVMConstString(global_var->initializer->text, strlen(global_var->initializer->text), 0);
            } else {
                // Non-constant initializer - error for now
                fprintf(stderr, "Error: Global variable '%s' has non-constant initializer\n", global_var->name);
                exit(1);
            }
        } else {
            init_value = LLVMConstNull(var_type);
        }

        LLVMSetInitializer(llvm_global, init_value);

        // Handle const
        if (global_var->is_const) {
            LLVMSetGlobalConstant(llvm_global, 1);
        }

        add_global_var(global_var->name, llvm_global, global_var->kind, global_var->pointer_level, global_var->array_size);
    }

    // code for each function
    for (int i = 0; i < program->function_count; i++) {
        codegen_function(program->functions[i]);
    }

    printf("All functions generated\n");

    char *error = NULL;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);

    // print LLVM IR to file (for debugging)
    char ir_file[256];
    snprintf(ir_file, sizeof(ir_file), "%s.ll", output_file);
    if (LLVMPrintModuleToFile(module, ir_file, &error) != 0) {
        fprintf(stderr, "Error writing IR: %s\n", error);
        LLVMDisposeMessage(error);
    } else {
        printf("LLVM IR written to %s\n", ir_file);
    }

    // write object file
    LLVMTargetRef target;
    if (LLVMGetTargetFromTriple(LLVMGetDefaultTargetTriple(), &target, &error) != 0) {
        fprintf(stderr, "Error getting target: %s\n", error);
        LLVMDisposeMessage(error);
        exit(1);
    }

    const LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
        target,
        LLVMGetDefaultTargetTriple(),
        "generic",
        "",
        LLVMCodeGenLevelDefault,
        LLVMRelocPIC,  // Change this from LLVMRelocDefault to LLVMRelocPIC
        LLVMCodeModelDefault
    );

    if (LLVMTargetMachineEmitToFile(machine, module, (char*)output_file, LLVMObjectFile, &error) != 0) {
        fprintf(stderr, "Error writing object file: %s\n", error);
        LLVMDisposeMessage(error);
        exit(1);
    }

    printf("Object file written to %s\n", output_file);

    // cleanup
    LLVMDisposeTargetMachine(machine);
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
    LLVMContextDispose(context);
}