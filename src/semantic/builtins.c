#include "builtins.h"

#include <stdlib.h>
#include <string.h>

void register_builtins(Scope *global_scope) {
    Symbol *print_sym = create_print_func();
    scope_add_symbol(global_scope, print_sym);
}

Symbol* create_print_func() {
    Symbol *print_sym = malloc(sizeof(Symbol));
    print_sym->name = strdup("__cplus_print_");
    print_sym->kind = SYM_FUNCTION;
    print_sym->type = TYPE_VOID;
    print_sym->pointer_level = 0;
    print_sym->parameters = create_vector(1, sizeof(Symbol*));

    // print arg 1
    Symbol *arg1 = malloc(sizeof(Symbol));
    arg1->name = strdup("message");
    arg1->kind = SYM_PARAMETER;
    arg1->type = TYPE_STRING;
    arg1->pointer_level = 0;
    arg1->is_const = true;

    vector_push(&print_sym->parameters, &arg1);
    return print_sym;
}
