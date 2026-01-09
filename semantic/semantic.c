
#include "semantic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Scope* scope_create(Scope* parent) {
    Scope* new_scope = malloc(sizeof(Scope));
    new_scope->parent = parent;
    new_scope->symbol_count = 0;
    new_scope->symbols = malloc(sizeof(Symbol*) * 16);
    return new_scope;
}

void scope_destroy(Scope* scope) {
    for (int i = 0; i < scope->symbol_count; i++) {
        free(scope->symbols[i]->name);
        free(scope->symbols[i]);
    }

    free(scope->symbols);
    free(scope);
}

void scope_add_symbol(Scope* scope, Symbol* symbol) {
    if (scope->symbol_count >= 16) {
        fprintf(stderr, "too many symbols (max 16)\n");
        exit(1);
    }

    scope->symbols[scope->symbol_count] = symbol;
    scope->symbol_count++;
}

Symbol* scope_lookup(const Scope* scope, const char* name) {
    for (int i = 0; i < scope->symbol_count; i++) {
        if (strcmp(scope->symbols[i]->name, name) == 0) {
            return scope->symbols[i];
        }
    }

    return nullptr;
}

Symbol* scope_lookup_recursive(const Scope* scope, const char* name) {
    // try current scope first
    Symbol* sym = scope_lookup(scope, name);
    if (sym != nullptr) {
        return sym;
    }

    // if not found and parent exists, search parent
    if (scope->parent != nullptr) {
        return scope_lookup_recursive(scope->parent, name);
    }

    // not found anywhere
    return nullptr;
}
