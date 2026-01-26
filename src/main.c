#include <stdio.h>
#include <string.h>

// #include "codegen/codegen_cat.h"
#include <stdlib.h>

#include "codegen/codegen.h"

#include "preprocessor/preprocessor.h"
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

    printf("Compiling %s...\n", filename);

    DiagnosticEngine *diag = diag_create();

    // pre-process
    printf("Preprocessing...\n");
    Preprocessor *prep = preprocessor_create(filename, diag);
    char *preprocessed_text = preprocessor_process_file(prep, f);
    fclose(f);

    if (diag_has_errors(diag)) {
        diag_print_all(diag);
        free(preprocessed_text);
        preprocessor_destroy(prep);
        diag_destroy(diag);
        return 1;
    }

    preprocessor_destroy(prep);

    // lexing
    printf("Lexing...\n");
    FILE *temp = tmpfile();
    fprintf(temp, "%s", preprocessed_text);
    rewind(temp);
    free(preprocessed_text);
    Lexer *lexer = lexer_create(filename, temp);

    // parsing
    printf("Parsing...\n");
    Parser *parser = parser_create(lexer, diag);
    ProgramNode *program = parser_parse_program(parser);

    if (diag_has_errors(diag)) {
        diag_print_all(diag);
        parser_destroy(parser);
        lexer_destroy(lexer);
        fclose(temp);
        diag_destroy(diag);
        return 1;
    }

    // semantic analysis
    printf("Semantic analysis...\n");
    SemanticAnalyzer *semantic = semantic_create(diag);
    semantic_analyze_program(semantic, program);

    if (diag_has_errors(diag)) {
        diag_print_all(diag);
        semantic_destroy(semantic);
        parser_destroy(parser);
        lexer_destroy(lexer);
        fclose(temp);
        diag_destroy(diag);
        return 1;
    }

    semantic_destroy(semantic);

    printf("Generating code...\n");

    if (useLLvm && !diag_has_errors(diag)) {
        codegen_program_llvm(program, "output.o");
        printf("Finished generating code.\n");
    }
    //else {
      //  codegen_program_cat(prog, "output.asm");
    //}

    parser_destroy(parser);
    lexer_destroy(lexer);
    fclose(temp);
    diag_destroy(diag);
    return 0;
}
