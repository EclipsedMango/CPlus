#ifndef C__SEMANTIC_H
#define C__SEMANTIC_H

#include "../ast/ast.h"
#include "../util/common.h"

typedef struct Symbol {
    char *name;
    enum {
        SYM_VARIABLE,
        SYM_FUNCTION,
        SYM_PARAMETER
    } kind;
    TypeKind type;
    int pointer_level;
    int is_const;
    SourceLocation location;
} Symbol;

typedef struct Scope {
    Vector symbols;
    struct Scope *parent; // NULL for global
} Scope;

// scope management
Scope* scope_create(Scope* parent);
void scope_destroy(Scope* scope);

// symbol operations
void scope_add_symbol(Scope* scope, Symbol* symbol);
Symbol* scope_lookup(const Scope* scope, const char* name);       // current scope only
Symbol* scope_lookup_recursive(const Scope* scope, const char* name);  // search up

void analyze_program(const ProgramNode *program);

#endif //C__SEMANTIC_H