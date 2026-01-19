#include "common.h"

#include <stdlib.h>
#include <string.h>

Vector create_vector(const int capacity, const int element_size) {
    return (Vector){.capacity = capacity, .length = 0, .element_size = element_size, .elements = malloc(element_size * capacity)};
}

void vector_destroy(Vector* vector) {
    free(vector->elements);
    vector->elements = NULL;
    vector->capacity = 0;
    vector->length = 0;
}

void* vector_get(const Vector* vector, const int index) {
    if (index < 0 || index >= vector->length) {
        return NULL;
    }

    return (char*)vector->elements + index * vector->element_size;
}

void vector_set(const Vector* vector, const int index, const void* element) {
    if (index < 0 || index >= vector->length) {
        return;
    }

    void* dest = (char*)vector->elements + index * vector->element_size;
    memcpy(dest, element, vector->element_size);
}

void vector_push(Vector* vector, const void* element) {
    if (vector->length >= vector->capacity) {
        const int new_capacity = vector->capacity == 0 ? 1 : vector->capacity * 2;
        void* new_elements = realloc(vector->elements, new_capacity * vector->element_size);

        if (!new_elements) {
            return;
        }

        vector->elements = new_elements;
        vector->capacity = new_capacity;
    }

    void* dest = (char*)vector->elements + vector->length * vector->element_size;
    memcpy(dest, element, vector->element_size);
    vector->length++;
}

void vector_pop(Vector* vector) {
    if (vector->length > 0) {
        vector->length--;
    }
}



