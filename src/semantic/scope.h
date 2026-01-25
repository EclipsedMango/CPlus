#ifndef C__SCOPE_H
#define C__SCOPE_H


#include "../ast/ast.h"
#include "../util/common.h"

#include "../util/vector.h"

typedef enum {
    SYM_VARIABLE,
    SYM_FUNCTION,
    SYM_PARAMETER,
    SYM_TYPE,
} SymbolKind;

typedef struct Symbol {
    char *name;
    SymbolKind kind;
    TypeKind type;
    int pointer_level;
    bool is_const;
    SourceLocation location;
} Symbol;

typedef enum {
    SCOPE_GLOBAL,
    SCOPE_FUNCTION,
    SCOPE_BLOCK,
    SCOPE_LOOP,
} ScopeType;

typedef struct Scope {
    Vector symbols;
    struct Scope *parent;
    ScopeType scope_type;
} Scope;

Scope* scope_create(Scope *parent, ScopeType type);
void scope_destroy(Scope *scope);

void scope_add_symbol(Scope *scope, Symbol *symbol);
Symbol* scope_lookup(const Scope *scope, const char *name);
Symbol* scope_lookup_recursive(const Scope *scope, const char *name);

bool scope_in_loop(const Scope *scope);
Scope* scope_find_function(const Scope *scope);


#endif //C__SCOPE_H