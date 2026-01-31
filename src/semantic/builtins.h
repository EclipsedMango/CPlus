#ifndef C__BUILTINS_H
#define C__BUILTINS_H

#include "../ast/ast.h"
#include "scope.h"

void register_builtins(Scope *global_scope);
Symbol* create_print_func();

#endif //C__BUILTINS_H