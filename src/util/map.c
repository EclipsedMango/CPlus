#include "map.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* djb2 hash */
static unsigned long map_hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

Map create_map(int capacity) {
    if (capacity <= 0) capacity = 16;
    Entry **buckets = calloc((size_t)capacity, sizeof(Entry *));
    return (Map){.size = 0, .capacity = capacity, .buckets = buckets};
}

static void free_chain(Entry *e) {
    while (e) {
        Entry *next = e->next;
        free(e->key);
        free(e);
        e = next;
    }
}

void map_destroy(Map *map) {
    if (!map) return;
    for (int i = 0; i < map->capacity; ++i) {
        free_chain(map->buckets[i]);
    }
    free(map->buckets);
    map->buckets = NULL;
    map->capacity = 0;
    map->size = 0;
}

static bool map_rehash(Map *map, int new_capacity) {
    Entry **new_buckets = calloc((size_t)new_capacity, sizeof(Entry *));
    if (!new_buckets) return false;

    for (int i = 0; i < map->capacity; ++i) {
        Entry *e = map->buckets[i];
        while (e) {
            Entry *next = e->next;
            unsigned long h = map_hash(e->key);
            int idx = (int)(h % (unsigned long)new_capacity);
            e->next = new_buckets[idx];
            new_buckets[idx] = e;
            e = next;
        }
    }

    free(map->buckets);
    map->buckets = new_buckets;
    map->capacity = new_capacity;
    return true;
}

int map_get(const Map *map, const char *key) {
    if (!map || !key || map->capacity == 0) return -1;
    unsigned long h = map_hash(key);
    int idx = (int)(h % (unsigned long)map->capacity);
    Entry *e = map->buckets[idx];
    while (e) {
        if (strcmp(e->key, key) == 0) return e->value;
        e = e->next;
    }
    return -1;
}

bool map_contains(const Map *map, const char *key) {
    return map_get(map, key) != -1;
}

bool map_add(Map *map, const char *key, int value) {
    if (!map || !key) return false;

    /* resize if load factor > 0.75 */
    if (map->size + 1 > (int)(map->capacity * 0.75)) {
        if (!map_rehash(map, map->capacity * 2)) return false;
    }

    unsigned long h = map_hash(key);
    int idx = (int)(h % (unsigned long)map->capacity);
    Entry *e = map->buckets[idx];
    while (e) {
        if (strcmp(e->key, key) == 0) {
            /* replace value */
            e->value = value;
            return true;
        }
        e = e->next;
    }

    /* insert new entry at head */
    Entry *ne = malloc(sizeof(Entry));
    if (!ne) return false;
    char *kcopy = strdup(key);
    if (!kcopy) {
        free(ne);
        return false;
    }
    ne->key = kcopy;
    ne->value = value;
    ne->next = map->buckets[idx];
    map->buckets[idx] = ne;
    map->size++;
    return true;
}

bool map_remove(Map *map, const char *key) {
    if (!map || !key) return false;
    unsigned long h = map_hash(key);
    int idx = (int)(h % (unsigned long)map->capacity);
    Entry *e = map->buckets[idx];
    Entry *prev = NULL;
    while (e) {
        if (strcmp(e->key, key) == 0) {
            if (prev) prev->next = e->next;
            else map->buckets[idx] = e->next;
            free(e->key);
            free(e);
            map->size--;
            return true;
        }
        prev = e;
        e = e->next;
    }
    return false;
}