#include "codegen.h"
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Analysis.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *name;
    LLVMValueRef value;  // LLVM value (parameter or alloca)
} CodegenSymbol;

static CodegenSymbol *local_vars = NULL;
static int local_var_count = 0;
static int local_var_capacity = 0;

static void add_local_var(const char *name, const LLVMValueRef value) {
    if (local_var_count >= local_var_capacity) {
        local_var_capacity = local_var_capacity == 0 ? 16 : local_var_capacity * 2;
        local_vars = realloc(local_vars, sizeof(CodegenSymbol) * local_var_capacity);
    }

    local_vars[local_var_count].name = strdup(name);
    local_vars[local_var_count].value = value;
    local_var_count++;
}

static LLVMValueRef lookup_local_var(const char *name) {
    for (int i = 0; i < local_var_count; i++) {
        if (strcmp(local_vars[i].name, name) == 0) {
            return local_vars[i].value;
        }
    }
    return NULL;
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
        case TYPE_INT:    return LLVMInt32TypeInContext(context);
        case TYPE_LONG:   return LLVMInt64TypeInContext(context);
        case TYPE_FLOAT:  return LLVMFloatTypeInContext(context);
        case TYPE_DOUBLE: return LLVMDoubleTypeInContext(context);
        case TYPE_BOOLEAN: return LLVMInt1TypeInContext(context);
        case TYPE_VOID:   return LLVMVoidTypeInContext(context);
        case TYPE_STRING: return LLVMPointerType(LLVMInt8TypeInContext(context), 0);
        default:
            fprintf(stderr, "Unsupported type in codegen\n");
            exit(1);
    }
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
            const LLVMValueRef var = lookup_local_var(expr->text);
            if (!var) {
                fprintf(stderr, "Codegen error: undefined variable '%s'\n", expr->text);
                exit(1);
            }
            // Load with the correct type
            LLVMTypeRef var_type = get_llvm_type(expr->type);
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

            const LLVMValueRef left = codegen_expression(expr->binop.left);
            const LLVMValueRef right = codegen_expression(expr->binop.right);

            switch (expr->binop.op) {
                case BIN_ADD:
                    return LLVMBuildAdd(builder, left, right, "addtmp");
                case BIN_SUB:
                    return LLVMBuildSub(builder, left, right, "subtmp");
                case BIN_MUL:
                    return LLVMBuildMul(builder, left, right, "multmp");
                case BIN_DIV:
                    return LLVMBuildSDiv(builder, left, right, "divtmp");
                case BIN_ASSIGN:
                    if (expr->binop.left->kind != EXPR_VAR) {
                        fprintf(stderr, "Left-hand side of assignment must be a variable\n");
                        exit(1);
                    }

                    LLVMValueRef lhs_ptr = lookup_local_var(expr->binop.left->text);
                    if (!lhs_ptr) {
                        fprintf(stderr, "Assignment to undefined variable '%s'\n", expr->binop.left->text);
                        exit(1);
                    }

                    LLVMValueRef rhs_val = codegen_expression(expr->binop.right);
                    LLVMBuildStore(builder, rhs_val, lhs_ptr);

                    return rhs_val;
                case BIN_LESS:
                    return LLVMBuildICmp(builder, LLVMIntSLT, left, right, "cmptmp");
                case BIN_GREATER:
                    return LLVMBuildICmp(builder, LLVMIntSGT, left, right, "cmptmp");
                case BIN_LESS_EQ:
                    return LLVMBuildICmp(builder, LLVMIntSLE, left, right, "cmptmp");
                case BIN_GREATER_EQ:
                    return LLVMBuildICmp(builder, LLVMIntSGE, left, right, "cmptmp");
                case BIN_EQUAL:
                    return LLVMBuildICmp(builder, LLVMIntEQ, left, right, "cmptmp");
                default:
                    fprintf(stderr, "Unsupported binary operator\n");
                    exit(1);
            }
        }
        case EXPR_CALL: {
            const LLVMValueRef func = LLVMGetNamedFunction(module, expr->call.function_name);
            if (!func) {
                fprintf(stderr, "Codegen error: undefined function '%s'\n", expr->call.function_name);
                exit(1);
            }

            // make arguments
            LLVMValueRef *args = malloc(sizeof(LLVMValueRef) * expr->call.arg_count);
            for (int i = 0; i < expr->call.arg_count; i++) {
                args[i] = codegen_expression(expr->call.args[i]);
            }

            const LLVMTypeRef func_type = LLVMGlobalGetValueType(func);
            const LLVMValueRef result = LLVMBuildCall2(builder, func_type, func, args, expr->call.arg_count, "calltmp");

            free(args);
            return result;
        }
        default:
            fprintf(stderr, "Unsupported expression kind: %d\n", expr->kind);
            exit(1);
    }
}

static void codegen_statement(const StmtNode* stmt) {
    switch (stmt->kind) {
        case STMT_RETURN: {
            if (!stmt->return_stmt.expr) {
                fprintf(stderr, "Error: return statement has no expression\n");
                exit(1);
            }

            const LLVMValueRef ret_val = codegen_expression(stmt->return_stmt.expr);
            LLVMBuildRet(builder, ret_val);
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
        case STMT_ASM: {
            // create inline assembly
            LLVMTypeRef void_type = LLVMVoidTypeInContext(context);
            LLVMTypeRef asm_fn_type = LLVMFunctionType(void_type, NULL, 0, 0);

            LLVMValueRef asm_val = LLVMGetInlineAsm(
                asm_fn_type,
                stmt->asm_stmt.assembly_code,
                strlen(stmt->asm_stmt.assembly_code),
                "",  // constraints (empty for basic asm)
                0,   // constraints length
                1,   // hasSideEffects (set to 1 to be safe)
                0,   // isAlignStack
                LLVMInlineAsmDialectIntel,  // intel syntax
                0    // canThrow
            );

            LLVMBuildCall2(builder, asm_fn_type, asm_val, NULL, 0, "");
            break;
        }
        case STMT_VAR_DECL: {
            // allocate space for variable
            const LLVMTypeRef var_type = get_llvm_type(stmt->var_decl.type);
            const LLVMValueRef alloca = LLVMBuildAlloca(builder, var_type, stmt->var_decl.name);

            // store initializer if present
            if (stmt->var_decl.initializer) {
                const LLVMValueRef init_val = codegen_expression(stmt->var_decl.initializer);
                LLVMBuildStore(builder, init_val, alloca);
            }

            add_local_var(stmt->var_decl.name, alloca);
            break;
        }
        case STMT_EXPR: {
            codegen_expression(stmt->expr_stmt.expr);
            break;
        }
        case STMT_COMPOUND: {
            for (int i = 0; i < stmt->compound.count; i++) {
                codegen_statement(stmt->compound.stmts[i]);
            }
            break;
        }
        default:
            fprintf(stderr, "Unsupported statement kind: %d\n", stmt->kind);
            exit(1);
    }
}

static void codegen_function(const FunctionNode* func) {
    // build parameter types array
    LLVMTypeRef *param_types = malloc(sizeof(LLVMTypeRef) * func->param_count);
    for (int i = 0; i < func->param_count; i++) {
        param_types[i] = get_llvm_type(func->params[i].type);
    }

    // function type
    const LLVMTypeRef ret_type = get_llvm_type(func->return_type);
    const LLVMTypeRef func_type = LLVMFunctionType(ret_type, param_types, func->param_count, 0);

    const LLVMValueRef llvm_func = LLVMAddFunction(module, func->name, func_type);

    // entry block
    const LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(context, llvm_func, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    // add parameters as local variables
    clear_local_vars();
    for (int i = 0; i < func->param_count; i++) {
        const LLVMValueRef param = LLVMGetParam(llvm_func, i);
        LLVMSetValueName(param, func->params[i].name);

        // allocate stack space and store parameter
        const LLVMTypeRef param_type = get_llvm_type(func->params[i].type);
        const LLVMValueRef alloca = LLVMBuildAlloca(builder, param_type, func->params[i].name);
        LLVMBuildStore(builder, param, alloca);

        add_local_var(func->params[i].name, alloca);
    }

    codegen_statement(func->body);
    free(param_types);
}

void codegen_program(const ProgramNode* program, const char* output_file) {
    // init
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    // create context, module, builder
    context = LLVMContextCreate();
    module = LLVMModuleCreateWithNameInContext("my_module", context);
    builder = LLVMCreateBuilderInContext(context);

    printf("LLVM codegen initialized\n");

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