#include "codegen_cat.h"

#include <stdio.h>
#include <stdlib.h>

const char* prologue = "\n; prologue\n"
                       "    push r4\n"
                       "    push r5\n"
                       "    push r6\n"
                       "    push r7\n"
                       "    mov r7, sp\n";

const char* epilogue = "\n; epilogue\n"
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
            // todo
            break;
        case EXPR_BINOP: {
            const BinaryOp op = expr->binop.op;
            if (op < BIN_ASSIGN) {  // compare or maths
                // let's just use r4 and r5 (we need to preserve them in case of nested ops)
                fprintf(file, "    push r4\n"
                              "    push r5\n");
                expr_in_reg(expr->binop.left, file, 4);
                expr_in_reg(expr->binop.right, file, 5);
                    
                if (op < BIN_EQUAL) {  // maths
                    char** op_instr = nullptr;
                    switch (op) {
                        case BIN_ADD:
                            *op_instr = "add";
                            break;
                        case BIN_SUB:
                            *op_instr = "sub";
                            break;
                        case BIN_MUL:
                            *op_instr = "umul";
                            break;
                        case BIN_DIV:
                            *op_instr = "udiv";
                            break;
                    }
                    fprintf(file, "    %s r4, r5\n", *op_instr);  // perform the op
                    fprintf(file, "    mov r%d, r4"
                                  "    pop r5\n"
                                  "    pop r4\n", reg);
                } else {  // compare
                    
                }
            } else {  // assign
                // fuck no
                fprintf(stderr, "Error: fuck no");
                exit(69);
            }
                
            expr->call.args[i]->binop.left;
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
    fprintf(file, "    pop r3\n"
                  "    pop r2\n"
                  "    pop r1\n");
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
            break;
        }
        case STMT_EXPR: {
            if (stmt->expr_stmt.expr->kind != EXPR_CALL) {
                fprintf(stderr, "Error: statement expression is not call statement\n");
                exit(1);
            }

            codegen_call(stmt->expr_stmt.expr, file);  // places result in r0 but will be ignored
            break;
        }
        case STMT_COMPOUND: {
            for (int i = 0; i < stmt->compound.count; ++i) {
                codegen_statement(stmt->compound.stmts[i], file);
            }
            break;
        }
    }
}

void codegen_function(const FunctionNode* function, FILE* file) {
    fprintf(file, "%s:\n", function->name);
    fprintf(file, "%s", prologue);

    for (int i = 0; i < function->param_count; i++) {
        if (i >= 3) {
            break;
        }

        fprintf(file, "    push %s\n", arg_registers[i]);
    }

    codegen_statement(function->body, file);
    fprintf(file, "%s", epilogue);
}

void codegen_program(const ProgramNode* program, const char* output_file) {
    FILE *output = fopen(output_file, "w");

    if (output == nullptr) {
        fprintf(stderr, "Error writing object file: %s\n", output_file);
        exit(1);
    }

    fprintf(output, "; GENERATED FROM C+ BY C+ COMPILER\n");
    fprintf(output, "jmp main\n");

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