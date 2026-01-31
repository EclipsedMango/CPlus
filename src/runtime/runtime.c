
#include <stdio.h>
#include "runtime.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// input and conversion
char* __cplus_input_() {

}

int __cplus_to_int(char* s) {

}

float __cplus_to_float(char* s) {

}

char* __cplus_int_to_string(int i) {

}

// string manipulation
void __cplus_print_(char* msg) {
    printf("%s", msg);
}

char* __cplus_str_concat(char *s1, char *s2) {
    if (!s1 || !s2) return NULL;

    const size_t len1 = strlen(s1);
    const size_t len2 = strlen(s2);

    char *result = malloc(len1 + len2 + 1);

    strcpy(result, s1);
    strcat(result, s2);

    return result;
}

bool __cplus_strcmp_(char *s1, char *s2) {

}

char* __cplus_substr_(char *s1, int start, int len) {

}

char __cplus_char_at_(char *s1, int index) {

}

// memory and args
void __cplus_memcpy_(void* dest, void* src, int n) {

}

void __cplus_memset_(void* ptr, int val, int n) {

}

void* __cplus_realloc_(void* ptr, int size) {

}

// math and randomness
int __cplus_random_() {

}

void __cplus_seed_(int s) {

}

float __cplus_sqrt_(float f) {

}

float __cplus_pow_(float base, float exp) {

}

// system util
int __cplus_time_() {

}

int __cplus_system_(char* cmd) {

}

void __cplus_panic_(char* cmd) {

}