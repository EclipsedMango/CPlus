#ifndef C__STRING_BUILDER_H
#define C__STRING_BUILDER_H

#include <stddef.h>

typedef struct StringBuilder StringBuilder;

StringBuilder* sb_create(size_t initial_capacity);
void sb_destroy(StringBuilder *sb);

void sb_append(StringBuilder *sb, const char *str);
void sb_append_char(StringBuilder *sb, char c);
void sb_append_int(StringBuilder *sb, int value);

char* sb_to_string(StringBuilder *sb);
void sb_clear(StringBuilder *sb);

#endif //C__STRING_BUILDER_H