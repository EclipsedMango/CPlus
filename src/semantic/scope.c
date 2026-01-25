#include "scope.h"
#include <stdlib.h>
#include <string.h>

Scope* scope_create(Scope *parent, ScopeType type) {
    Scope *scope = malloc(sizeof(Scope));
    if (!scope) return NULL;

    scope->parent = parent;
    scope->scope_type = type;
    scope->symbols = create_vector(16, sizeof(Symbol*));

    return scope;
}

void scope_destroy(Scope *scope) {
    if (!scope) return;

    for (int i = 0; i < scope->symbols.length; i++) {
        Symbol **sym_ptr = vector_get(&scope->symbols, i);
        if (sym_ptr && *sym_ptr) {
            free((*sym_ptr)->name);
            free(*sym_ptr);
        }
    }

    vector_destroy(&scope->symbols);
    free(scope);
}

void scope_add_symbol(Scope *scope, Symbol *symbol) {
    if (!scope || !symbol) return;
    vector_push(&scope->symbols, &symbol);
}

Symbol* scope_lookup(const Scope *scope, const char *name) {
    if (!scope || !name) return NULL;

    for (int i = 0; i < scope->symbols.length; i++) {
        Symbol **sym_ptr = vector_get(&scope->symbols, i);
        if (sym_ptr && *sym_ptr && strcmp((*sym_ptr)->name, name) == 0) {
            return *sym_ptr;
        }
    }

    return NULL;
}

Symbol* scope_lookup_recursive(const Scope *scope, const char *name) {
    if (!scope || !name) return NULL;

    Symbol *sym = scope_lookup(scope, name);
    if (sym) return sym;

    if (scope->parent) {
        return scope_lookup_recursive(scope->parent, name);
    }

    return NULL;
}

bool scope_in_loop(const Scope *scope) {
    if (!scope) return false;

    if (scope->scope_type == SCOPE_LOOP) {
        return true;
    }

    if (scope->parent) {
        return scope_in_loop(scope->parent);
    }

    return false;
}

Scope* scope_find_function(const Scope *scope) {
    if (!scope) return NULL;

    if (scope->scope_type == SCOPE_FUNCTION) {
        return (Scope*)scope;
    }

    if (scope->parent) {
        return scope_find_function(scope->parent);
    }

    return NULL;
}