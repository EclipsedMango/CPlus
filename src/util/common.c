#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int max(const int a, const int b) {
    return a > b ? a : b;
}

// DEPRECATED: remove and replace with diagnostics plz do sooner rather than later
void report_error(const SourceLocation loc, const char *fmt, ...) {
    fprintf(stderr, "\033[1;31m%s:%d:%d: error:\033[0m ",
            loc.filename ? loc.filename : "<unknown>",
            loc.line,
            loc.column
    );

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    exit(1);
}