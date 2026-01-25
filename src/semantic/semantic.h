#ifndef C__SEMANTIC_H
#define C__SEMANTIC_H

#include "../ast/ast.h"
#include "../util/diagnostics.h"

typedef struct SemanticAnalyzer SemanticAnalyzer;

SemanticAnalyzer* semantic_create(DiagnosticEngine *diagnostics);
void semantic_destroy(SemanticAnalyzer *analyzer);

bool semantic_analyze_program(SemanticAnalyzer *analyzer, ProgramNode *program);

#endif //C__SEMANTIC_H