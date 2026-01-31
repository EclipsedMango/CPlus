
#include <stdio.h>
#include "runtime.h"

#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// input and conversion
char* __cplus_input_() {
    size_t size = 128;
    size_t len = 0;
    char *buf = malloc(size);
    if (!buf) return NULL;

    int c;
    while ((c = getchar()) != EOF && c != '\n') {
        if (len + 1 >= size) {
            size *= 2;
            char *tmp = realloc(buf, size);
            if (!tmp) {
                free(buf);
                return NULL;
            }
            buf = tmp;
        }
        buf[len++] = (char)c;
    }

    if (c == EOF && len == 0) {
        free(buf);
        return NULL;
    }

    buf[len] = '\0';
    return buf;
}

int __cplus_to_int_(char* s) {
    if (!s) return 0;

    char *end;
    errno = 0;

    const long val = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') {
        return 0;
    }

    return (int)val;
}

float __cplus_to_float_(char* s) {
    if (!s) return 0.0f;

    char *end;
    errno = 0;

    const float val = strtof(s, &end);
    if (errno != 0 || end == s) {
        return 0.0f;
    }

    return val;
}

char* __cplus_int_to_string_(const int i) {
    char *buf = malloc(32);
    if (!buf) return NULL;
    snprintf(buf, 32, "%d", i);
    return buf;
}

char* __cplus_float_to_string_(const float f) {
    const int len = snprintf(NULL, 0, "%f", f);
    if (len < 0) return NULL;

    char *buf = malloc(len + 1);
    if (!buf) return NULL;

    snprintf(buf, len + 1, "%f", f);
    return buf;
}

// string manipulation
void __cplus_print_(char* msg) {
    printf("%s", msg);
}

char* __cplus_str_concat(const char *s1, const char *s2) {
    const size_t len1 = strlen(s1);
    const size_t len2 = strlen(s2);

    char *out = malloc(len1 + len2 + 1);
    if (!out) return NULL;

    memcpy(out, s1, len1);
    memcpy(out + len1, s2, len2 + 1);
    return out;
}

bool __cplus_strcmp_(const char *s1, const char *s2) {
    return strcmp(s1, s2) == 0;
}

char* __cplus_substr_(const char *s1, const int start, int len) {
    size_t slen = strlen(s1);
    if (start >= slen) return strdup("");

    if (start + len > slen) {
        len = slen - start;
    }

    char *out = malloc(len + 1);
    memcpy(out, s1 + start, len);
    out[len] = '\0';
    return out;
}

char __cplus_char_at_(const char *s1, const int index) {
    if (index < 0) {
        fprintf(stderr, "char_at: index out of bounds: %d\n", index);
        abort();
    }

    if (index >= strlen(s1)) {
        fprintf(stderr, "char_at: index out of bounds: %d\n", index);
        abort();
    }

    return s1[index];
}

// memory and args
void __cplus_memcpy_(void* dest, const void* src, const int n) {
    memcpy(dest, src, n);
}

void __cplus_memset_(void* ptr, const int val, const int n) {
    memset(ptr, val, n);
}

void* __cplus_realloc_(void* ptr, const int size) {
    return realloc(ptr, size);
}

// math and randomness
int __cplus_random_() {
    return rand();
}

void __cplus_seed_(const int s) {
    srand(s);
}

float __cplus_sqrt_(const float f) {
    return sqrt(f);
}

float __cplus_pow_(const float base, const float exp) {
    return pow(base, exp);
}

// system util
int __cplus_time_() {
    return (int)time(NULL);
}

int __cplus_system_(const char* cmd) {
    return system(cmd);
}

void __cplus_panic_(char* cmd) {
    fprintf(stderr, "panic: %s\n", cmd);
    abort();
}