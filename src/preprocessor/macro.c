
#include "macro.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

MacroTable* macro_table_create(void) {
    MacroTable *table = malloc(sizeof(MacroTable));
    if (!table) return NULL;

    table->macros = create_map(64);
    table->definitions = malloc(sizeof(Macro) * 16);
    table->count = 0;
    table->capacity = 16;

    return table;
}

void macro_table_destroy(MacroTable *table) {
    if (!table) return;

    for (int i = 0; i < table->count; i++) {
        macro_destroy(&table->definitions[i]);
    }

    free(table->definitions);
    map_destroy(&table->macros);
    free(table);
}

void macro_define(MacroTable *table, Macro *macro) {
    if (!table || !macro) return;

    if (macro_is_defined(table, macro->name)) {
        const int index = map_get(&table->macros, macro->name);
        if (index >= 0) {
            macro_destroy(&table->definitions[index]);

            Macro new_macro = {
                .name = strdup(macro->name),
                .replacement = strdup(macro->replacement),
                .is_function_like = macro->is_function_like,
                .param_count = macro->param_count,
                .location = macro->location
            };

            if (macro->is_function_like && macro->param_count > 0) {
                new_macro.params = create_vector(macro->param_count, sizeof(char*));
                for (int i = 0; i < macro->param_count; i++) {
                    char **param_ptr = (char**)vector_get(&macro->params, i);
                    char *param_copy = strdup(*param_ptr);
                    vector_push(&new_macro.params, &param_copy);
                }
            } else {
                memset(&new_macro.params, 0, sizeof(Vector));
            }

            table->definitions[index] = new_macro;
        }

        return;
    }

    if (table->count >= table->capacity) {
        table->capacity *= 2;
        table->definitions = realloc(table->definitions, sizeof(Macro) * table->capacity);
    }

    Macro new_macro = {
        .name = strdup(macro->name),
        .replacement = strdup(macro->replacement),
        .is_function_like = macro->is_function_like,
        .param_count = macro->param_count,
        .location = macro->location
    };

    if (macro->is_function_like && macro->param_count > 0) {
        new_macro.params = create_vector(macro->param_count, sizeof(char*));
        for (int i = 0; i < macro->param_count; i++) {
            char **param_ptr = (char**)vector_get(&macro->params, i);
            char *param_copy = strdup(*param_ptr);
            vector_push(&new_macro.params, &param_copy);
        }
    } else {
        memset(&new_macro.params, 0, sizeof(Vector));
    }

    table->definitions[table->count] = new_macro;
    map_add(&table->macros, new_macro.name, table->count);
    table->count++;
}

void macro_undefine(MacroTable *table, const char *name) {
    if (!table || !name) return;

    const int index = map_get(&table->macros, name);
    if (index >= 0) {
        macro_destroy(&table->definitions[index]);
        map_remove(&table->macros, name);
        table->definitions[index].name = NULL;
    }
}

Macro* macro_lookup(const MacroTable *table, const char *name) {
    if (!table || !name) return NULL;

    const int index = map_get(&table->macros, name);
    if (index < 0) return NULL;

    Macro* macro = &table->definitions[index];
    if (macro->name == NULL) return NULL;

    return macro;
}

bool macro_is_defined(const MacroTable *table, const char *name) {
    return macro_lookup(table, name) != NULL;
}

Macro* macro_create_object(const char *name, const char *replacement, const SourceLocation loc) {
    Macro *macro = malloc(sizeof(Macro));

    macro->name = strdup(name);
    macro->replacement = strdup(replacement);
    macro->params = create_vector(0, sizeof(char*));
    macro->is_function_like = false;
    macro->param_count = 0;

    macro->location = loc;
    return macro;
}

Macro* macro_create_function(const char *name, char **params, const int param_count, const char *replacement, const SourceLocation loc) {
    Macro *macro = malloc(sizeof(Macro));
    macro->name = strdup(name);
    macro->replacement = strdup(replacement);

    macro->is_function_like = true;
    macro->param_count = param_count;

    if (param_count > 0) {
        macro->params = create_vector(param_count, sizeof(char*));
        for (int i = 0; i < param_count; i++) {
            char *param_copy = strdup(params[i]);
            vector_push(&macro->params, &param_copy);
        }
    } else {
        macro->params = create_vector(param_count, sizeof(char*));
    }

    macro->location = loc;

    return macro;
}

void macro_destroy(Macro *macro) {
    if (!macro) return;

    free(macro->name);

    if (macro->params.capacity > 0) {
        for (int i = 0; i < macro->param_count; i++) {
            char **param_ptr = vector_get(&macro->params, i);
            free(*param_ptr);
        }

        vector_destroy(&macro->params);
    }

    free(macro->replacement);
}