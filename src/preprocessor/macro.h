#ifndef C__MACRO_H
#define C__MACRO_H

#include <stdbool.h>

#include "../util/common.h"
#include "../util/map.h"
#include "../util/vector.h"

typedef struct Macro {
    char *name;
    char *replacement;

    int is_function_like;
    Vector params;
    int param_count;

    SourceLocation location;
} Macro;

typedef struct MacroTable {
    Map macros;
    Macro *definitions;
    int count;
    int capacity;
} MacroTable;

MacroTable* macro_table_create(void);
void macro_table_destroy(MacroTable *table);

void macro_define(MacroTable *table, Macro *macro);
void macro_undefine(MacroTable *table, const char *name);

Macro* macro_lookup(const MacroTable *table, const char *name);
bool macro_is_defined(const MacroTable *table, const char *name);

Macro* macro_create_object(const char *name, const char *replacement, SourceLocation loc);
Macro* macro_create_function(const char *name, char **params, int param_count, const char *replacement, SourceLocation loc);

void macro_destroy(Macro *macro);

#endif //C__MACRO_H