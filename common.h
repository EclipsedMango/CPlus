#ifndef C__COMMON_H
#define C__COMMON_H

#include <stdbool.h>

typedef struct SourceLocation {
    int line;
    int column;
    const char *filename;
} SourceLocation;

int max(int a, int b);

// VECTOR IMPL

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

// MAP IMPL

typedef struct Entry {
    char *key;
    int value;
    struct Entry *next;
} Entry;

typedef struct Map {
    int size;
    int capacity;
    Entry **buckets;
} Map;

Map create_map(int capacity);
void map_destroy(Map *map);

int map_get(const Map *map, const char *key);  // -1 on fail
bool map_contains(const Map *map, const char *key);
bool map_add(Map *map, const char *key, int value); /* returns true on success */
bool map_remove(Map *map, const char *key);

// ... is representation of variadic arguments, meaning accepts arbitrary number of params after defined ones
// exits the program after reporting the error
void report_error(SourceLocation loc, const char *fmt, ...);

#endif //C__COMMON_H