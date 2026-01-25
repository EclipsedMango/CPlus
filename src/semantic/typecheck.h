#ifndef C__SEMANTIC_TYPECHECK_H
#define C__SEMANTIC_TYPECHECK_H

#include "../ast/ast.h"
#include <stdbool.h>

const char* type_to_string(TypeKind type);
bool is_numeric_type(TypeKind type);
bool is_integer_type(TypeKind type);
bool is_floating_type(TypeKind type);

bool types_compatible(TypeKind target, TypeKind source);
bool types_compatible_with_pointers(TypeKind target_type, int target_ptr_level, TypeKind source_type, int source_ptr_level);

bool is_arithmetic_op(BinaryOp op);
bool is_comparison_op(BinaryOp op);
bool is_logical_op(BinaryOp op);
bool is_assignment_op(BinaryOp op);

#endif //C__SEMANTIC_TYPECHECK_H