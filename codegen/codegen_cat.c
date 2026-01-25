#include "codegen_cat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROLOGUE_STACK_SIZE 20  // including ret pointer
const char* prologue = "    ; prologue\n"
                       "    push r4\n"
                       "    push r5\n"
                       "    push r6\n"
                       "    push r7\n"
                       "    mov r7, sp\n\n";

const char* epilogue = "\n    ; epilogue\n"
                       ".end:\n"
                       "    mov sp, r7\n"
                       "    pop r7\n"
                       "    pop r6\n"
                       "    pop r5\n"
                       "    pop r4\n"
                       "    ret\n";

const int arg_registers[3] = {1, 2, 3};

char** strings[256];
int string_count = 0;
int branch_num = 0;

Map* variables;  // key is name, value is offset from r7
int current_r7_offset = 0;

#define USABLE_REGISTERS 7
const int usable_registers[] = {0, 1, 2, 3, 4, 5, 6};
int register_statuses[USABLE_REGISTERS];  // 0 is unused, 1 is used

int get_reg_index(const int reg) {
    for (int i = 0; i < USABLE_REGISTERS; ++i) {
        if (usable_registers[i] == reg) {
            return i;
        }
    }
    
    fprintf(stderr, "Error: Tried to get index of register that isn't usable.");
    exit(1);
}

int borrow_register(int* outMustPreserve, const int not) {
    for (int i = 0; i < USABLE_REGISTERS; ++i) {
        if (usable_registers[i] == not) continue;
        
        if (!register_statuses[i]) {
            *outMustPreserve = 0;  // don't need to preserve
            register_statuses[i] = 1;
            return usable_registers[i];
        }
    }
    
    // okay we couldn't find an unborrowed one
    // give them one, but they must preserve it
    *outMustPreserve = 1;
    for (int i = 0; i < USABLE_REGISTERS; ++i) {
        if (usable_registers[i] == not) continue;
        
        return usable_registers[i];
    }
    
    // couldn't find anything
    fprintf(stderr, "Error: Ran out of registers D:");
    exit(1);
}

// returns whether is must be preserved
int borrow_specific_register(const int reg) {
    const int i = get_reg_index(reg);
    if (register_statuses[i]) {
        return 1;  // in use
    }
    
    register_statuses[i] = 1;
    return 0;  // not in use
}

void return_register(const int reg, const int preserved) {
    const int index = get_reg_index(reg);
    
    if (preserved) {
        // do nothing
    } else {
        register_statuses[index] = 0;
    }
}

void expr_in_reg(ExprNode* expr, FILE* file, int reg);
void codegen_call(const ExprNode* expr, FILE* file, int returnIn);

void expr_in_reg(ExprNode* expr, FILE* file, const int reg) {
    switch (expr->kind) {
        case EXPR_NUMBER: {
            fprintf(file, "    mov r%d, %s\n", reg, expr->text);
            break;
        }
        case EXPR_STRING_LITERAL: {
            strings[string_count] = &expr->text;
            fprintf(file, "    mov r%d, str_%d\n", reg, string_count);
            string_count++;
            break;
        }
        case EXPR_VAR: {
            const int offset = map_get(variables, expr->text);
            if (offset == -1) {
                fprintf(stderr, "Error: Variable not found %s", expr->text);
                exit(1);
            }
            fprintf(file, "    ; getting variable %s\n", expr->text);  // our var is at r7+ the offset to it
            fprintf(file, "    mov r%d, r7\n", reg);  // our var is at r7+ the offset to it
            fprintf(file, "    sub r%d, %d\n", reg, offset);  // add the offset
            fprintf(file, "    mov r%d, @r%d\n", reg, reg);  // dereference to get value
            break;
        }
        case EXPR_BINOP: {
            const BinaryOp op = expr->binop.op;
            if (op < BIN_ASSIGN || op > BIN_ASSIGN) {  // compare or maths
                // okay we need two temp registers
                int preserve1;
                int preserve2;
                const int scratch1 = borrow_register(&preserve1, reg);
                const int scratch2 = borrow_register(&preserve2, reg);
                
                if (preserve1) fprintf(file, "    push r%d\n", scratch1);
                if (preserve2) fprintf(file, "    push r%d\n", scratch2);
                expr_in_reg(expr->binop.left, file, scratch1);
                expr_in_reg(expr->binop.right, file, scratch2);
                    
                if (op < BIN_EQUAL || op > BIN_ASSIGN) {  // maths
                    char* op_instr = NULL;
                    switch (op) {
                        case BIN_ADD:
                            op_instr = "add";
                            break;
                        case BIN_SUB:
                            op_instr = "sub";
                            break;
                        case BIN_MUL:
                            op_instr = "umul";
                            break;
                        case BIN_DIV:
                            op_instr = "udiv";
                            break;
                        case BIN_LOGICAL_AND:
                            op_instr = "and";
                            break;
                        case BIN_LOGICAL_OR:
                            op_instr = "or";
                            break;
                        default:
                            fprintf(stderr, "Error: Invalid OP, was one added without me knowing?");
                            exit(1);
                    }
                    fprintf(file, "    %s r%d, r%d\n", op_instr, scratch1, scratch2);  // perform the op
                    fprintf(file, "    mov r%d, r%d\n", reg, scratch1);
                } else {  // compare
                    fprintf(file, "    cmp r%d, r%d  ; compare our values for bin op\n", scratch1, scratch2);
                    
                    char* jmp_instr = NULL;
                    switch (op) {
                        case BIN_EQUAL:
                            jmp_instr = "je";
                            break;
                        case BIN_NOT_EQUAL:
                            jmp_instr = "jne";
                            break;
                        case BIN_GREATER:
                            jmp_instr = "jug";
                            break;
                        case BIN_LESS:
                            jmp_instr = "jul";
                            break;
                        case BIN_GREATER_EQ:
                            jmp_instr = "juge";
                            break;
                        case BIN_LESS_EQ:
                            jmp_instr = "jule";
                            break;
                        default:
                            fprintf(stderr, "Error: Invalid OP, was one added without me knowing?");
                            exit(1);
                    }
                    
                    // now we have the jump instruction, later we'll make a label for 
                    // the true and false branches, so jump occordingly.
                    // They will be named .cmpbranch_INDEX_true and false
                    fprintf(file, "    %s .cmpbranch_%d_true\n", jmp_instr, branch_num);
                    fprintf(file, "    jmp .cmpbranch_%d_false\n", branch_num);
                    
                    // true branch (set to 1)
                    fprintf(file, ".cmpbranch_%d_true:\n", branch_num);
                    fprintf(file, "    mov r%d, 1\n", reg);  // true
                    fprintf(file, "    jmp .cmpbranch_%d_end\n\n", branch_num);
                    
                    // false branch (set to 0)
                    fprintf(file, ".cmpbranch_%d_false:\n", branch_num);
                    fprintf(file, "    mov r%d, 0\n\n", reg);  // false
                    
                    // end
                    fprintf(file, ".cmpbranch_%d_end:\n", branch_num);
                    
                    branch_num++;
                }
                
                // restore registers
                if (preserve2) fprintf(file, "    pop r%d\n", scratch2);
                if (preserve1) fprintf(file, "    pop r%d\n", scratch1);
                return_register(scratch2, preserve2);
                return_register(scratch1, preserve1);
            } else {  // assignment
                char* varName;
                ExprNode* arrayIndex = NULL;
                int dereferenceCount = 0;
                const ExprNode* currentExpr = expr->binop.left;
                while (1) {
                    if (currentExpr->kind == EXPR_VAR) {
                        varName = currentExpr->text;
                        break;
                    }
                    
                    if (currentExpr->kind == EXPR_ARRAY_INDEX) {
                        arrayIndex = currentExpr->array_index.index;
                        if (currentExpr->array_index.array->kind != EXPR_VAR) {
                            fprintf(stderr, "Error: array must be var.");
                            exit(1);
                        }
                        varName = currentExpr->array_index.array->text;
                        break;
                    }
                    
                    if (currentExpr->kind != EXPR_UNARY || currentExpr->unary.op != UNARY_DEREF) {
                        fprintf(stderr, "Error: Non variable assignment target must be deref binop.");
                        exit(1);
                    }
                    
                    // okay now we can recursively dereference
                    dereferenceCount++;
                    currentExpr = currentExpr->unary.operand;
                }
                
                const int offset = map_get(variables, varName);
                if (offset == -1) {
                    fprintf(stderr, "Error: Variable not found %s", varName);
                    exit(1);
                }
                
                fprintf(file, "    ; setting variable %s\n", varName);
                
                // we need 2 scratch registers
                int preserve;
                const int scratch = borrow_register(&preserve, reg);
                if (preserve) fprintf(file, "    push r%d\n", scratch);
                
                if (arrayIndex == NULL) {
                    fprintf(file, "    mov r%d, r7\n", reg);  // our var is at r7+ the offset to it
                    fprintf(file, "    sub r%d, %d\n", reg, offset);  // add the offset
                } else {
                    expr_in_reg(arrayIndex, file, scratch);
                    fprintf(file, "    umul r%d, 4\n", scratch);
                    fprintf(file, "    mov r%d, r7\n", reg);
                    fprintf(file, "    sub r%d, %d\n", reg, offset);
                    fprintf(file, "    mov r%d, @r%d\n", reg, reg);
                    fprintf(file, "    add r%d, r%d\n", reg, scratch);
                }

                for (int i = 0; i < dereferenceCount; ++i) {
                    fprintf(file, "    mov r%d, @r%d\n", reg, reg);  // dereference to get pointer out of memory
                }
                
                // get value we're setting to
                expr_in_reg(expr->binop.right, file, scratch);  // place value into scratch register
                
                fprintf(file, "    mov @r%d, r%d\n", reg, scratch);  // set the variable
                
                if (preserve) fprintf(file, "    pop r%d\n", scratch);
                return_register(scratch, preserve);
                
                // we can then pop r6 because nothing is being evaluated
                // after pushing r6, which means the stack is safe
                // that's why we start by getting the value in our allocated
                // register 'reg'.
            }
            break;
        }
        case EXPR_CALL: {
            codegen_call(expr, file, reg);  // returns in r0
            break;
        }
        case EXPR_UNARY: {
            switch (expr->unary.op) {
                case UNARY_NEG: {
                    expr_in_reg(expr->unary.operand, file, reg);
                    fprintf(file, "    not r%d\n", reg);
                    fprintf(file, "    add r%d, 1\n", reg);
                    break;
                }
                case UNARY_NOT: {
                    expr_in_reg(expr->unary.operand, file, reg);
                    fprintf(file, "    cmp r%d, 0\n", reg);
                    fprintf(file, "    je .donenot%d\n", branch_num);
                    fprintf(file, "    mov r%d, 1\n", reg);
                    fprintf(file, ".donenot%d:\n", branch_num);
                    branch_num++;
                    break;
                }
                case UNARY_DEREF: {
                    expr_in_reg(expr->unary.operand, file, reg);
                    fprintf(file, "    mov r%d, @r%d\n", reg, reg);
                    break;
                }
                case UNARY_ADDR_OF: {
                    if (expr->unary.operand->kind != EXPR_VAR) {
                        fprintf(stderr, "Error: Address of operand must be variable.");
                        exit(1);
                    }
                    
                    const int offset = map_get(variables, expr->unary.operand->text);
                    if (offset == -1) {
                        fprintf(stderr, "Error: Variable not found %s", expr->unary.operand->text);
                        exit(1);
                    }
                    
                    fprintf(file, "    mov r%d, r7\n", reg);
                    fprintf(file, "    sub r%d, %d\n", reg, offset);
                    break;
                }
            }
            break;
        }
        case EXPR_ARRAY_INDEX: {
            if (expr->array_index.array->kind != EXPR_VAR) {
                fprintf(stderr, "Error: array index array type is not variable, is: %d.", expr->array_index.array->kind);
                exit(1);
            }
            
            const int offset = map_get(variables, expr->array_index.array->text);
            if (offset == -1) {
                fprintf(stderr, "Error: Variable not found %s", expr->array_index.array->text);
                exit(1);
            }
            
            // we need a scratch register
            int preserve;
            const int scratch = borrow_register(&preserve, reg);
            if (preserve) fprintf(file, "    push r%d\n", scratch);
            
            fprintf(file, "    ; getting value in array %s\n", expr->array_index.array->text);
            expr_in_reg(expr->array_index.index, file, scratch);
            fprintf(file, "    mov r%d, r7\n", reg);
            fprintf(file, "    sub r%d, %d\n", reg, offset);
            fprintf(file, "    mov r%d, @r%d\n", reg, reg);
            fprintf(file, "    umul r%d, 4\n", scratch);
            fprintf(file, "    add r%d, r%d\n", reg, scratch);
            fprintf(file, "    mov r%d, @r%d\n", reg, reg);
            
            if (preserve) fprintf(file, "    pop r%d\n", scratch);
            return_register(scratch, preserve);
            break;
        }
        default: {
            fprintf(stderr, "Error: invalid expression type: %d.", expr->kind);
            exit(1);
        }
    }
}

// causes r0 override
void codegen_call(const ExprNode* expr, FILE* file, int returnIn) {
    // preserve the arg registers just in case of nested call
    fprintf(file, "\n    ; calling %s\n", expr->call.function_name);
    if (returnIn != 0) {
        fprintf(file, "    push r0\n");  // preserve return register
    }
    
    // get all arguments
    Vector preservedRegs = create_vector(3, sizeof(int));
    Vector nonPreservedRegs = create_vector(3, sizeof(int));
    int argsSpace = 0;
    for (int i = 0; i < expr->call.arg_count; ++i) {
        if (i < 3) {  // named register args, simple
            const int reg = arg_registers[i];
            const int mustPreserve = borrow_specific_register(reg);
            if (mustPreserve) {
                vector_push(&preservedRegs, &arg_registers[i]);
                fprintf(file, "    push r%d\n", reg);
            } else {
                vector_push(&nonPreservedRegs, &arg_registers[i]);
            }
            
            expr_in_reg(expr->call.args[i], file, reg);  // replace the arg value into the register
            continue;
        }
        
        // it needs to go on the stack
        // we're going to temporarily put into r0 because we're already saving it
        // and, we can't wrap this call in a push/pop for a scratch register.
        expr_in_reg(expr->call.args[i], file, 0);
        fprintf(file, "    push r0\n");  // push onto stack
        argsSpace += 4;
    }
    
    fprintf(file, "    call %s\n\n", expr->call.function_name);
    if (argsSpace > 0) {  // we allocated space for args, we need to restore it
        fprintf(file, "    add sp, %d  ; restore space used by args\n\n", argsSpace);
    }
    
    // return our arg registers
    while (1) {
        const int* reg = (int*)vector_pop(&preservedRegs);
        if (reg == NULL) {
            break;
        }
        
        fprintf(file, "    pop r%d\n", *reg);
        return_register(*reg, 1);
    }
    vector_destroy(&preservedRegs);
    
    while (1) {
        const int* reg = (int*)vector_pop(&nonPreservedRegs);
        if (reg == NULL) {
            break;
        }
        
        return_register(*reg, 0);
    }
    vector_destroy(&nonPreservedRegs);
    
    
    // return value will be left in r0, we need to change to our goal (returnIn)
    if (returnIn != 0) {
        fprintf(file, "    mov r%d, r0\n", returnIn);
        fprintf(file, "    pop r0\n");
    }
}

void codegen_statement(const StmtNode* stmt, FILE* file, const int loop) {
    switch (stmt->kind) {
        case STMT_RETURN: {
            if (!stmt->return_stmt.expr) {
                fprintf(file, "    jmp .end\n");
            } else {
                expr_in_reg(stmt->return_stmt.expr, file, 0);  // move result into return register
                fprintf(file, "    jmp .end\n");
            }
            break;
        }
        case STMT_VAR_DECL: {
            const int varOffset = current_r7_offset;
            map_add(variables, stmt->var_decl.name, varOffset);
            current_r7_offset += 4;
            
            fprintf(file, "    sub sp, 4  ; space for %s (r7 - %d)\n", stmt->var_decl.name, varOffset);  // allocate space
            
            // get two scratch registers
            int preserve1;
            int preserve2;
            const int scratch1 = borrow_register(&preserve1, -1);
            const int scratch2 = borrow_register(&preserve2, -1);
            if (preserve1) {
                fprintf(stderr, "Error: preserving vardec registers is not currently supported.");  // TODO
                exit(1);
                fprintf(file, "    push r%d\n", scratch1);
            }
            if (preserve2) {
                fprintf(stderr, "Error: preserving vardec registers is not currently supported.");  // TODO
                exit(1);
                fprintf(file, "    push r%d\n", scratch2);
            }

            if (stmt->var_decl.array_size > 0) {
                // the initialiser is basically the pointer value
                fprintf(file, "    ; initialising %s (setting array pointer)\n", stmt->var_decl.name);
                fprintf(file, "    mov r%d, sp\n", scratch2);
                fprintf(file, "    sub r%d, %d\n", scratch2, stmt->var_decl.array_size * 4);
                fprintf(file, "    mov @sp, r%d\n", scratch2);
                
                fprintf(file, "    sub sp, %d  ; ARRAY[%d] space for %s\n", 
                    stmt->var_decl.array_size * 4, 
                    stmt->var_decl.array_size,
                    stmt->var_decl.name);  // allocate array space
                current_r7_offset += stmt->var_decl.array_size * 4;
            }
            
            if (stmt->var_decl.initializer == NULL) {
                if (preserve2) fprintf(file, "    pop r%d\n", scratch2);
                if (preserve1) fprintf(file, "    pop r%d\n", scratch1);
                return_register(scratch2, preserve2);
                return_register(scratch1, preserve1);
                break;
            }
            
            if (stmt->var_decl.array_size != 0) {
                fprintf(stderr, "Error: array initialisation is not supported.");
                exit(1);
            }
            
            // actually set value
            expr_in_reg(stmt->var_decl.initializer, file, scratch1);
            
            fprintf(file, "    ; initialising %s\n", stmt->var_decl.name);
            fprintf(file, "    mov r%d, r7\n", scratch2);
            fprintf(file, "    sub r%d, %d\n", scratch2, varOffset);
            fprintf(file, "    mov @r%d, r%d\n", scratch2, scratch1);
            
            if (preserve2) fprintf(file, "    pop r%d\n", scratch2);
            if (preserve1) fprintf(file, "    pop r%d\n", scratch1);
            return_register(scratch2, preserve2);
            return_register(scratch1, preserve1);
            break;
        }
        case STMT_EXPR: {
            if (stmt->expr_stmt.expr->kind == EXPR_CALL) {
                codegen_call(stmt->expr_stmt.expr, file, 0);  // places result in r0 but will be ignored
                break;
            }

            if (stmt->expr_stmt.expr->kind == EXPR_BINOP && stmt->expr_stmt.expr->binop.op == BIN_ASSIGN) {
                int preserve;
                const int scratch = borrow_register(&preserve, -1);
                if (preserve) fprintf(file, "    push r%d\n", scratch);
                expr_in_reg(stmt->expr_stmt.expr, file, scratch);  // we can clobber scratch
                if (preserve) fprintf(file, "    pop r%d\n", scratch);
                return_register(scratch, preserve);
                break;
            }
                
            fprintf(stderr, "Error: statement expression is not valid for statement. It's: %d.\n", stmt->expr_stmt.expr->kind);
            exit(1);
        }
        case STMT_COMPOUND: {
            for (int i = 0; i < stmt->compound.count; ++i) {
                codegen_statement(stmt->compound.stmts[i], file, loop);
            }
            break;
        }
        case STMT_IF: {
            int preserve;
            const int scratch = borrow_register(&preserve, -1);
            if (preserve) fprintf(file, "    push r%d\n", scratch);
            
            expr_in_reg(stmt->if_stmt.condition, file, scratch);
            const int branch = branch_num++;
            fprintf(file, "    cmp r%d, 1\n", scratch);
            fprintf(file, "    je .true_%d\n", branch);
            fprintf(file, "    ; false\n");
            if (stmt->if_stmt.else_stmt != NULL) {
                codegen_statement(stmt->if_stmt.else_stmt, file, loop);
            }
            fprintf(file, "    jmp .done_%d\n", branch);
            fprintf(file, ".true_%d:\n", branch);
            codegen_statement(stmt->if_stmt.then_stmt, file, loop);
            fprintf(file, ".done_%d:\n", branch);
            
            if (preserve) fprintf(file, "    pop r%d\n", scratch);
            return_register(scratch, preserve);
            break;
        }
        case STMT_ASM: {
            fprintf(file, "\n; BEGIN INLINE ASM\n");

            Vector preserveRegs = create_vector(8, sizeof(int));
            Vector nonPreserveRegs = create_vector(8, sizeof(int));
            for (int i = 0; i < stmt->asm_stmt.clobber_count; ++i) {
                const int reg = stmt->asm_stmt.clobbers[i][1] - '0';
                const int mustPreserve = borrow_specific_register(reg);
                
                if (!mustPreserve) {
                    vector_push(&nonPreserveRegs, &reg);
                    continue;
                }

                fprintf(file, "push r%d\n", reg);
                vector_push(&preserveRegs, &reg);
            }
            
            if (stmt->asm_stmt.input_count > 3) {
                fprintf(stderr, "Error: currently only 3 inputs are allowed.\n");
                exit(1);
            }

            for (int i = 0; i < stmt->asm_stmt.input_count; ++i) {
                const int regNum = stmt->asm_stmt.input_constraints[i][1] - '0';
                expr_in_reg(stmt->asm_stmt.inputs[i], file, regNum);
            }
            
            fprintf(file, "\n");
            fwrite(stmt->asm_stmt.assembly_code, 1, strlen(stmt->asm_stmt.assembly_code), file);
            fprintf(file, "\n");
            
            if (stmt->asm_stmt.output_count > 4) {
                fprintf(stderr, "Error: currently only 4 outputs are allowed.\n");
                exit(1);
            }

            for (int i = 0; i < stmt->asm_stmt.output_count; ++i) {
                const ExprNode* output = stmt->asm_stmt.outputs[i];
                if (output->kind != EXPR_VAR) {
                    fprintf(stderr, "Error: michael is a liar, asm output is not variable.\n");
                    exit(1);
                }
                
                const int offset = map_get(variables, output->text);
                if (offset == -1) {
                    fprintf(stderr, "Error: Variable not found %s", output->text);
                    exit(1);
                }
                fprintf(file, "; outputting %s\n", output->text);
                fprintf(file, "mov r6, r7\n");  // our var is at r7+ the offset to it
                fprintf(file, "sub r6, %d\n", offset);  // add the offset
                fprintf(file, "mov @r6, %s\n", stmt->asm_stmt.output_constraints[i]);  // dereference to get value
            }

            while (1) {
                const int* regPointer = vector_pop(&preserveRegs);
                if (regPointer == NULL) {
                    break;
                }
                
                fprintf(file, "pop %d\n", *regPointer);
                return_register(*regPointer, 1);
            }
            vector_destroy(&preserveRegs);
            while (1) {
                const int* regPointer = vector_pop(&nonPreserveRegs);
                if (regPointer == NULL) {
                    break;
                }
                
                return_register(*regPointer, 0);
            }
            vector_destroy(&nonPreserveRegs);
            
            fprintf(file, "\n; END INLINE ASM\n");
            break;
        }
        case STMT_WHILE: {
            int preserve;
            const int scratch = borrow_register(&preserve, -1);
            if (preserve) fprintf(file, "    push r%d\n", scratch);
            
            const int branch = branch_num++;
            fprintf(file, ".loop%d:\n", branch);
            expr_in_reg(stmt->while_stmt.condition, file, scratch);
            fprintf(file, "    cmp r%d, 0\n", scratch);
            fprintf(file, "    je .doneloop%d\n\n", branch);
            codegen_statement(stmt->while_stmt.body, file, branch);
            fprintf(file, ".continueloop%d:\n", branch);
            fprintf(file, "\n    jmp .loop%d\n", branch);
            fprintf(file, ".doneloop%d:\n", branch);
            
            if (preserve) fprintf(file, "    pop r%d\n", scratch);
            return_register(scratch, preserve);
            break;
        }
        case STMT_FOR: {
            int preserve;
            const int scratch = borrow_register(&preserve, -1);
            if (preserve) fprintf(file, "    push r%d\n", scratch);
            
            codegen_statement(stmt->for_stmt.init, file, -1);
            const int branch = branch_num++;
            fprintf(file, ".loop%d:\n", branch);
            expr_in_reg(stmt->for_stmt.condition, file, scratch);
            fprintf(file, "    cmp r%d, 0\n", scratch);
            fprintf(file, "    je .doneloop%d\n\n", branch);
            codegen_statement(stmt->for_stmt.body, file, branch);
            fprintf(file, ".continueloop%d:\n", branch);
            expr_in_reg(stmt->for_stmt.increment, file, scratch);
            fprintf(file, "\n    jmp .loop%d\n", branch);
            fprintf(file, ".doneloop%d:\n", branch);
            
            if (preserve) fprintf(file, "    pop r%d\n", scratch);
            return_register(scratch, preserve);
            break;
        }
        case STMT_BREAK: {
            if (loop == -1) {
                fprintf(stderr, "Error: no loop to break from.\n");
                exit(1);
            }
            
            fprintf(file, "    jmp .doneloop%d\n", loop);
            break;
        }
        case STMT_CONTINUE: {
            if (loop == -1) {
                fprintf(stderr, "Error: no loop to break from.\n");
                exit(1);
            }
            
            fprintf(file, "    jmp .continueloop%d\n", loop);
            break;
        }
        default: {
            fprintf(stderr, "Error: invalid statement type: %d.\n", stmt->kind);
            exit(1);
        }
    }
}

void codegen_function(const FunctionNode* function, FILE* file) {
    int oldr7off = current_r7_offset;
    current_r7_offset = 4;  // start at 4 because reasons okay
    
    fprintf(file, "%s:\n", function->name);
    fprintf(file, "%s", prologue);
    
    if (function->param_count > 0) fprintf(file, "    ; Save arguments on stack\n");
    for (int i = 0; i < function->param_count; i++) {
        const ParamNode param = function->params[i];
        
        if (i >= 3) {
            // already on the stack
            // so what's the offset?
            map_add(variables, param.name, -PROLOGUE_STACK_SIZE - (function->param_count-3) * 4 + (i - 2) * 4);
            continue;
        }

        fprintf(file, "    push r%d\n", arg_registers[i]);
        
        // record param
        map_add(variables, param.name, current_r7_offset);
        current_r7_offset += 4;
    }
    fprintf(file, "\n");

    codegen_statement(function->body, file, -1);
    fprintf(file, "%s", epilogue);
    
    current_r7_offset = oldr7off;
}

void codegen_program_cat(const ProgramNode* program, const char* output_file) {
    FILE *output = fopen(output_file, "w");

    if (output == NULL) {
        fprintf(stderr, "Error writing object file: %s\n", output_file);
        exit(1);
    }

    fprintf(output, "; GENERATED FROM C+ BY C+ COMPILER\n");
    fprintf(output, "jmp main\n");
    
    // initialise variables
    Map varMap = create_map(16);
    // ReSharper disable once CppDFALocalValueEscapesFunction, won't happen :D
    variables = &varMap;

    for (int i = 0; i < program->function_count; i++) {
        fprintf(output, "\n");
        codegen_function(program->functions[i], output);
        fprintf(output, "\n");
    }

    fprintf(output, "; Application Strings\n");
    for (int i = 0; i < string_count; i++) {
        fprintf(output, "str_%d:\n", i);
        const char *str = *strings[i];
        
        // build data string hex array, (0x7B, 0x82, etc.)
        const size_t strLen = strlen(*strings[i]);
        char data[6*strLen-2];
        data[0] = '\0';
        for (size_t j = 0; j < strLen; ++j) {
            char buf[8];
            snprintf(buf, sizeof(buf), "0x%02X", (unsigned char)str[j]);
            strcat(data, buf);
            if (j < strLen - 1) {
                strcat(data, ", ");
            }
        }
        
        fprintf(output, "    d8 %s, 0\n", data);
    }
}