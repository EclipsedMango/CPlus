#ifndef C__BUILTINS_H
#define C__BUILTINS_H

#include "../ast/ast.h"
#include "scope.h"

void register_builtins(Scope *global_scope);

#endif //C__BUILTINS_H