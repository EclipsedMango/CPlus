#include "codegen_cat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

const char* arg_registers[3] = {"r1", "r2", "r3"};

char** strings[256];
int string_count = 0;
int branch_num = 0;

Map* variables;  // key is name, value is offset from r7
int current_r7_offset = 0;

void expr_in_reg(ExprNode* expr, FILE* file, int reg);
void codegen_call(const ExprNode* expr, FILE* file);

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
        case EXPR_VAR:
            const int offset = map_get(variables, expr->text);
            if (offset == -1) {
                fprintf(stderr, "Error: Variable not found");
                exit(1);
            }
            fprintf(file, "    mov r%d, r7\n", reg);  // our var is at r7+ the offset to it
            fprintf(file, "    add r%d, %d\n", reg, offset);  // add the offset
            fprintf(file, "    mov r%d, @r%d\n", reg, reg);  // dereference to get value
            break;
        case EXPR_BINOP: {
            const BinaryOp op = expr->binop.op;
            if (op < BIN_ASSIGN || op > BIN_ASSIGN) {  // compare or maths
                // let's just use r4 and r5 (we need to preserve them in case of nested ops)
                fprintf(file, "    push r4\n"
                              "    push r5\n");
                expr_in_reg(expr->binop.left, file, 4);
                expr_in_reg(expr->binop.right, file, 5);
                    
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
                    fprintf(file, "    %s r4, r5\n", op_instr);  // perform the op
                    fprintf(file, "    mov r%d, r4\n", reg);
                } else {  // compare
                    fprintf(file, "    cmp r4, r5  ; compare our values for bin op\n");
                    
                    char* jmp_instr = NULL;
                    switch (op) {
                        case BIN_EQUAL:
                            jmp_instr = "je";
                            break;
                        case BIN_NOT_EQUAL:
                            jmp_instr = "jne";
                            break;
                        case BIN_GREATER:
                            jmp_instr = "ja";
                            break;
                        case BIN_LESS:
                            jmp_instr = "jb";
                            break;
                        case BIN_GREATER_EQ:
                            jmp_instr = "jae";
                            break;
                        case BIN_LESS_EQ:
                            jmp_instr = "jbe";
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
                fprintf(file, "    pop r5\n"
                              "    pop r4\n");
            } else {  // assignment
                if (expr->binop.left->kind != EXPR_VAR) {
                    fprintf(stderr, "Error: What? Assignment left of var assign isn't variable. It's: %d. Var is %s. Source line is %d.\n", expr->binop.left->kind, expr->binop.left->text, op);
                    exit(1);
                }
                
                const int offset = map_get(variables, expr->binop.left->text);
                if (offset == -1) {
                    fprintf(stderr, "Error: Variable not found");
                    exit(1);
                }
                
                expr_in_reg(expr->binop.right, file, reg);  // place value into register
                
                // get value we're setting to (in r6)
                fprintf(file, "    ; setting variable %s\n", expr->binop.left->text);
                fprintf(file, "    push r6\n");  // preserve it
                fprintf(file, "    mov r6, r%d\n", reg);  // place target value into r6 to save
                
                fprintf(file, "    mov r%d, r7\n", reg);  // our var is at r7+ the offset to it
                fprintf(file, "    add r%d, %d\n", reg, offset);  // add the offset
                fprintf(file, "    mov @r%d, r6\n", reg);  // place r6 val into variable
                
                fprintf(file, "    pop r6\n\n");  // put r6 back
                
                // we can then pop r6 because nothing is being evaluated
                // after pushing r6, which means the stack is safe
                // that's why we start by getting the value in our allocated
                // register 'reg'.
            }
            break;
        }
        case EXPR_CALL:
            codegen_call(expr, file);  // returns in r0
            if (reg != 0) {
                fprintf(file, "    mov r%d, r0\n", reg);
            }
            break;
    }
}

// causes r0 override
void codegen_call(const ExprNode* expr, FILE* file) {
    // preserve the arg registers just in case of nested call
    fprintf(file, "\n    ; calling %s\n"
                  "    push r1\n"
                  "    push r2\n"
                  "    push r3\n", expr->call.function_name);
    current_r7_offset += 4*3;   // space for 3 args
    
    for (int i = 0; i < expr->call.arg_count; ++i) {
        if (i >= 3) {
            fprintf(stderr, "Error: too many arguments");
            exit(1);
        }
        
        expr_in_reg(expr->call.args[i], file, i+1);  // replace the arg value into the register
    }
    
    fprintf(file, "    call %s\n\n", expr->call.function_name);
    // return value will be left in r0
    
    // restore existing arg registers
    // we can't restore the stack space because we could overwrite variables (yes it's dumb)
    fprintf(file, "    pop r3\n"
                  "    pop r2\n"
                  "    pop r1\n"
                  "    sub sp, 12\n");
}

void codegen_statement(const StmtNode* stmt, FILE* file) {
    switch (stmt->kind) {
        case STMT_RETURN: {
            if (!stmt->return_stmt.expr) {
                fprintf(stderr, "Error: return statement has no expression\n");
                exit(1);
            }

            expr_in_reg(stmt->return_stmt.expr, file, 0);  // move result into return register
            fprintf(file, "    jmp .end\n");
            break;
        }
        case STMT_VAR_DECL: {
            fprintf(file, "    sub sp, 4\n");  // allocate space
            map_add(variables, stmt->var_decl.name, current_r7_offset);
            current_r7_offset += 4;
            break;
        }
        case STMT_EXPR: {
            if (stmt->expr_stmt.expr->kind == EXPR_CALL) {
                codegen_call(stmt->expr_stmt.expr, file);  // places result in r0 but will be ignored
                break;
            }

            if (stmt->expr_stmt.expr->kind == EXPR_BINOP && stmt->expr_stmt.expr->binop.op == BIN_ASSIGN) {
                expr_in_reg(stmt->expr_stmt.expr, file, 1);  // this will set, and clobber r1
                break;
            }
                
            fprintf(stderr, "Error: statement expression is not valid for statement. It's: %d.\n", stmt->expr_stmt.expr->kind);
            exit(1);
        }
        case STMT_COMPOUND: {
            for (int i = 0; i < stmt->compound.count; ++i) {
                codegen_statement(stmt->compound.stmts[i], file);
            }
            break;
        }
        case STMT_IF: {
            expr_in_reg(stmt->if_stmt.condition, file, 1);
            const int branch = branch_num++;
            fprintf(file, "    cmp r1, 1\n");
            fprintf(file, "    je .true\n");
            fprintf(file, "    ; false\n");
            if (stmt->if_stmt.else_stmt != NULL) {
                codegen_statement(stmt->if_stmt.else_stmt, file);
            }
            fprintf(file, "    jmp .done%d\n", branch);
            fprintf(file, ".true_%d:\n", branch);
            codegen_statement(stmt->if_stmt.then_stmt, file);
            fprintf(file, ".done_%d:\n", branch);
            break;
        }
        case STMT_ASM: {
            fprintf(file, "\n; BEGIN INLINE ASM\n");
            fwrite(stmt->asm_stmt.assembly_code, 1, strlen(stmt->asm_stmt.assembly_code), file);
            fprintf(file, "\n; END INLINE ASM\n");
            break;
        }
    }
}

void codegen_function(const FunctionNode* function, FILE* file) {
    int oldr7off = current_r7_offset;
    current_r7_offset = 0;
    
    fprintf(file, "%s:\n", function->name);
    fprintf(file, "%s", prologue);

    fprintf(file, "    ; Save arguments on stack\n");
    for (int i = 0; i < function->param_count; i++) {
        if (i >= 3) {
            break;
        }

        fprintf(file, "    push %s\n", arg_registers[i]);
        
        // record param
        ParamNode param = function->params[i];
        map_add(variables, param.name, current_r7_offset);
        current_r7_offset += 4;
    }
    fprintf(file, "\n");

    codegen_statement(function->body, file);
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
        fprintf(output, "    dstr %s\\0\n", *strings[i]);
    }
}