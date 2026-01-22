#include <stdio.h>
#include <string.h>

#include "codegen/codegen_cat.h"
#include "codegen/codegen.h"

#include "parser/parser.h"
#include "parser/token_utils.h"
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

    printf("compiling %s\n", filename);

    lexer_init(filename);
    set_current_file(f);

    ProgramNode *program = parse_program();
    printf("parsing complete\n");

    analyze_program(program);
    printf("semantic analysis complete\n");

    if (useLLvm) {
        codegen_program_llvm(program, "output.o");
    } else {
        codegen_program_cat(program, "output.asm");
    }
    printf("codegen complete\n");

    fclose(f);
    return 0;
}
