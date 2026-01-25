#include "typecheck.h"
#include "../ast/ast.h"

const char* type_to_string(TypeKind type) {
    switch (type) {
        case TYPE_INT: return "int";
        case TYPE_LONG: return "long";
        case TYPE_CHAR: return "char";
        case TYPE_FLOAT: return "float";
        case TYPE_DOUBLE: return "double";
        case TYPE_STRING: return "string";
        case TYPE_BOOLEAN: return "bool";
        case TYPE_VOID: return "void";
        default: return "unknown";
    }
}

bool is_numeric_type(const TypeKind type) {
    return type == TYPE_INT || type == TYPE_LONG || type == TYPE_FLOAT || type == TYPE_DOUBLE || type == TYPE_CHAR;
}

bool is_integer_type(const TypeKind type) {
    return type == TYPE_INT || type == TYPE_LONG || type == TYPE_CHAR;
}

bool is_floating_type(const TypeKind type) {
    return type == TYPE_FLOAT || type == TYPE_DOUBLE;
}

bool types_compatible(const TypeKind target, const TypeKind source) {
    if (target == source) return true;

    if (is_numeric_type(target) && is_numeric_type(source)) {
        return true;
    }

    return false;
}

bool types_compatible_with_pointers(const TypeKind target_type, const int target_ptr_level, const TypeKind source_type, const int source_ptr_level) {
    // string type is compatible with char*
    if (target_type == TYPE_STRING && source_type == TYPE_CHAR && source_ptr_level == 1) {
        return true;
    }
    if (target_type == TYPE_CHAR && target_ptr_level == 1 && source_type == TYPE_STRING) {
        return true;
    }

    // generic pointer
    if (target_ptr_level > 0 && source_ptr_level > 0) {
        if (target_type == TYPE_VOID || source_type == TYPE_VOID) {
            return true;
        }
    }

    // int to pointer conversion
    if (target_type == TYPE_INT && source_ptr_level > 0) {
        return true;
    }
    if (source_type == TYPE_INT && target_ptr_level > 0) {
        return true;
    }

    // pointer levels must match and types must be compatible
    if (target_ptr_level != source_ptr_level) {
        return false;
    }

    return types_compatible(target_type, source_type);
}

bool is_arithmetic_op(const BinaryOp op) {
    return op == BIN_ADD || op == BIN_SUB || op == BIN_MUL || op == BIN_DIV || op == BIN_MOD;
}

bool is_comparison_op(const BinaryOp op) {
    return op == BIN_EQUAL || op == BIN_GREATER || op == BIN_LESS || op == BIN_GREATER_EQ || op == BIN_LESS_EQ || op == BIN_NOT_EQUAL;
}

bool is_logical_op(const BinaryOp op) {
    return op == BIN_LOGICAL_AND || op == BIN_LOGICAL_OR;
}

bool is_assignment_op(const BinaryOp op) {
    return op == BIN_ASSIGN;
}