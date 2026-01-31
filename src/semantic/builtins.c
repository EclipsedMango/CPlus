#include "builtins.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static void add_builtin(Scope *scope, const char *name, TypeKind ret_type, int ret_ptr, int param_count, ...) {
    Symbol *sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->kind = SYM_FUNCTION;
    sym->type = ret_type;
    sym->pointer_level = ret_ptr;
    sym->parameters = create_vector(param_count, sizeof(Symbol*));

    va_list args;
    va_start(args, param_count);

    for (int i = 0; i < param_count; i++) {
        const TypeKind p_type = (TypeKind)va_arg(args, int);
        const int p_ptr = va_arg(args, int);

        Symbol *param = malloc(sizeof(Symbol));
        param->name = strdup("arg");
        param->kind = SYM_PARAMETER;
        param->type = p_type;
        param->pointer_level = p_ptr;
        param->is_const = 1;

        vector_push(&sym->parameters, &param);
    }

    va_end(args);
    scope_add_symbol(scope, sym);
}

void register_builtins(Scope *global_scope) {
    add_builtin(global_scope, "__cplus_input_", TYPE_STRING, 0, 0);
    add_builtin(global_scope, "__cplus_to_int_", TYPE_INT, 0, 1, TYPE_STRING, 0);
    add_builtin(global_scope, "__cplus_to_float_", TYPE_FLOAT, 0, 1, TYPE_STRING, 0);
    add_builtin(global_scope, "__cplus_int_to_string_", TYPE_STRING, 0, 1, TYPE_INT, 0);
    add_builtin(global_scope, "__cplus_float_to_string_", TYPE_STRING, 0, 1, TYPE_FLOAT, 0);

    add_builtin(global_scope, "__cplus_print_", TYPE_VOID, 0, 1, TYPE_STRING, 0);
    add_builtin(global_scope, "__cplus_str_concat", TYPE_STRING, 0, 2, TYPE_STRING, 0, TYPE_STRING, 0);
    add_builtin(global_scope, "__cplus_strcmp_", TYPE_BOOLEAN, 0, 2, TYPE_STRING, 0, TYPE_STRING, 0);

    add_builtin(global_scope, "__cplus_substr_", TYPE_STRING, 0, 3, TYPE_STRING, 0, TYPE_INT, 0, TYPE_INT, 0);
    add_builtin(global_scope, "__cplus_char_at_", TYPE_CHAR, 0, 2, TYPE_STRING, 0, TYPE_INT, 0);

    add_builtin(global_scope, "__cplus_memcpy_", TYPE_VOID, 0, 3, TYPE_VOID, 1, TYPE_VOID, 1, TYPE_INT, 0);
    add_builtin(global_scope, "__cplus_memset_", TYPE_VOID, 0, 3, TYPE_VOID, 1, TYPE_INT, 0, TYPE_INT, 0);
    add_builtin(global_scope, "__cplus_realloc_", TYPE_VOID, 1, 2, TYPE_VOID, 1, TYPE_INT, 0);

    add_builtin(global_scope, "__cplus_random_", TYPE_INT, 0, 0);
    add_builtin(global_scope, "__cplus_seed_", TYPE_VOID, 0, 1, TYPE_INT, 0);
    add_builtin(global_scope, "__cplus_sqrt_", TYPE_FLOAT, 0, 1, TYPE_FLOAT, 0);
    add_builtin(global_scope, "__cplus_pow_", TYPE_FLOAT, 0, 2, TYPE_FLOAT, 0, TYPE_FLOAT, 0);

    add_builtin(global_scope, "__cplus_time_", TYPE_INT, 0, 0);
    add_builtin(global_scope, "__cplus_system_", TYPE_INT, 0, 1, TYPE_STRING, 0);
    add_builtin(global_scope, "__cplus_panic_", TYPE_VOID, 0, 1, TYPE_STRING, 0);
}
