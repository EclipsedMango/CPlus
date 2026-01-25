#include <stdio.h>
#include <string.h>

// #include "codegen/codegen_cat.h"
#include "codegen/codegen.h"

#include "parser/parser.h"
#include "semantic/semantic.h"

int main(const int argc, char *argv[]) {
    if (argc < 2) {
        printf("%s takes a file as argument\n", argv[0]);
        return 1;
    }

    int useLLvm = 1;
    for (int i = 2; i < argc; ++i) {
        const char* token = argv[i];
        
        if (strcmp(token, "--codegen") == 0) {
            if (i + 1 >= argc) {
                printf("--codegen requires specifying the generator ('llvm' or 'cat')\n");
                return 1;
            }
            
            char* generator = argv[++i];
            if (strcmp(generator, "llvm") == 0) {
                useLLvm = 1;
                continue;
            }
            
            if (strcmp(generator, "cat") == 0) {
                useLLvm = 0;
                continue;
            }
            
            // invalid
            printf("Invalid code generator, must be one of: 'llvm', 'cat'\n");
            return 1;
        }
        
        // if we haven't continued by now it's invalid
        printf("Invalid flag: ");
        fwrite(token, 1, strlen(token), stdout);
        printf("\n");
        return 1;
    }

    char *filename = argv[1];
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("%s does not exist\n", filename);
        return 1;
    }

    printf("Compiler: compiling %s\n", filename);

    printf("Lexer: generating tokens\n");
    Lexer *lex = lexer_create("test.c", f);

    printf("Diagnostics: crating diagnostic context\n");
    DiagnosticEngine *diag = diag_create();

    printf("Parser: parsing tokens to AST\n");
    Parser *parser = parser_create(lex, diag);

    ProgramNode *prog = parser_parse_program(parser);

    if (!diag_has_errors(diag)) {
        SemanticAnalyzer *semantic = semantic_create(diag);
        printf("SemanticAnalyzer: verifying code\n");
        semantic_analyze_program(semantic, prog);
        semantic_destroy(semantic);
    }

    diag_print_all(diag);

    if (useLLvm) {
        codegen_program_llvm(prog, "output.o");
    }
    //else {
      //  codegen_program_cat(prog, "output.asm");
    //}
    printf("Compiler: codegen complete\n");

    parser_destroy(parser);
    lexer_destroy(lex);
    diag_destroy(diag);
    fclose(f);
    return 0;
}
