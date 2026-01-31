#ifndef C__MAP_H
#define C__MAP_H
#include <stdbool.h>

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

#endif //C__MAP_H