#include "string_builder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct StringBuilder {
    char *buffer;
    size_t length;
    size_t capacity;
};

StringBuilder* sb_create(size_t initial_capacity) {
    if (initial_capacity < 16) {
        initial_capacity = 16;
    }

    StringBuilder *sb = malloc(sizeof(StringBuilder));
    if (!sb) return NULL;

    sb->buffer = malloc(initial_capacity);
    if (!sb->buffer) {
        free(sb);
        return NULL;
    }

    sb->length = 0;
    sb->capacity = initial_capacity;
    sb->buffer[0] = '\0';

    return sb;
}

void sb_destroy(StringBuilder *sb) {
    if (!sb) return;

    free(sb->buffer);
    free(sb);
}

void sb_append(StringBuilder *sb, const char *str) {
    if (!sb || !str) return;

    const size_t str_len = strlen(str);
    const size_t needed = sb->length + str_len + 1;

    if (needed > sb->capacity) {
        size_t new_capacity = sb->capacity;
        while (new_capacity < needed) {
            new_capacity *= 2;
        }

        char *new_buffer = realloc(sb->buffer, new_capacity);
        if (!new_buffer) return;

        sb->buffer = new_buffer;
        sb->capacity = new_capacity;
    }

    memcpy(sb->buffer + sb->length, str, str_len);
    sb->length += str_len;
    sb->buffer[sb->length] = '\0';
}

void sb_append_char(StringBuilder *sb, char c) {
    if (!sb) return;

    const size_t needed = sb->length + 2;

    if (needed > sb->capacity) {
        const size_t new_capacity = sb->capacity * 2;
        char *new_buffer = realloc(sb->buffer, new_capacity);
        if (!new_buffer) return;

        sb->buffer = new_buffer;
        sb->capacity = new_capacity;
    }

    sb->buffer[sb->length] = c;
    sb->length++;
    sb->buffer[sb->length] = '\0';
}

void sb_append_int(StringBuilder *sb, int value) {
    if (!sb) return;

    char temp[12];
    snprintf(temp, sizeof(temp), "%d", value);
    sb_append(sb, temp);
}

char* sb_to_string(StringBuilder *sb) {
    if (!sb) return NULL;

    char *result = malloc(sb->length + 1);
    if (!result) return NULL;

    memcpy(result, sb->buffer, sb->length);
    result[sb->length] = '\0';

    return result;
}

void sb_clear(StringBuilder *sb) {
    if (!sb) return;

    sb->length = 0;
    sb->buffer[0] = '\0';
}