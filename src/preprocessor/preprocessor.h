#ifndef C__PREPROCESSOR_H
#define C__PREPROCESSOR_H

#include "macro.h"
#include "../util/diagnostics.h"
#include <stdio.h>

// preprocessor context
typedef struct Preprocessor Preprocessor;

Preprocessor* preprocessor_create(const char *filename, DiagnosticEngine *diag);
void preprocessor_destroy(Preprocessor *prep);

// returns preprocessed text (caller must free)
char* preprocessor_process(Preprocessor *prep, const char *input);

char* preprocessor_process_file(Preprocessor *prep, FILE *file);


#endif //C__PREPROCESSOR_H