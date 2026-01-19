#ifndef C__COMMON_H
#define C__COMMON_H

typedef struct SourceLocation {
    int line;
    int column;
    const char *filename;
} SourceLocation;

typedef struct Vector {
    int length;
    int capacity;
    int element_size;
    void* elements;
} Vector;

Vector create_vector(int capacity, int element_size);
void vector_destroy(Vector* vector);

void* vector_get(const Vector* vector, int index);
void vector_set(const Vector* vector, int index, const void* element);

void vector_push(Vector* vector, const void* element);
void vector_pop(Vector* vector);

// ... is representation of variadic arguments, meaning accepts arbitrary number of params after defined ones
// exits the program after reporting the error
void report_error(SourceLocation loc, const char *fmt, ...);

#endif //C__COMMON_H