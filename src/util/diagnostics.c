#include "diagnostics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_WHITE   "\033[1;37m"
#define COLOR_BOLD    "\033[1m"

DiagnosticEngine* diag_create(void) {
    DiagnosticEngine *engine = malloc(sizeof(DiagnosticEngine));
    if (!engine) return NULL;

    engine->diagnostics = malloc(sizeof(Diagnostic) * 16);
    if (!engine->diagnostics) {
        free(engine);
        return NULL;
    }

    engine->count = 0;
    engine->capacity = 16;
    engine->error_count = 0;
    engine->warning_count = 0;

    return engine;
}

void diag_destroy(DiagnosticEngine *engine) {
    if (!engine) return;

    for (int i = 0; i < engine->count; i++) {
        free(engine->diagnostics[i].message);
    }

    free(engine->diagnostics);
    free(engine);
}

void diag_report(DiagnosticEngine *engine, const DiagnosticLevel level, const SourceLocation loc, const char *fmt, ...) {
    if (!engine) return;

    // expand array if needed
    if (engine->count >= engine->capacity) {
        engine->capacity *= 2;
        Diagnostic *new_diags = realloc(engine->diagnostics, sizeof(Diagnostic) * engine->capacity);
        if (!new_diags) {
            fprintf(stderr, "Fatal: Out of memory while reporting diagnostic\n");
            return;
        }

        engine->diagnostics = new_diags;
    }

    va_list args;
    va_start(args, fmt);

    va_list args_copy;
    va_copy(args_copy, args);
    const int size = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    char *message = malloc(size + 1);
    if (!message) {
        fprintf(stderr, "Fatal: Out of memory while formatting diagnostic\n");
        va_end(args);
        return;
    }

    vsnprintf(message, size + 1, fmt, args);
    va_end(args);

    const Diagnostic diag = {
        .level = level,
        .location = loc,
        .message = message
    };

    engine->diagnostics[engine->count++] = diag;

    if (level == DIAG_ERROR) {
        engine->error_count++;
    } else if (level == DIAG_WARNING) {
        engine->warning_count++;
    }
}

void diag_print_all(DiagnosticEngine *engine) {
    if (!engine) return;

    for (int i = 0; i < engine->count; i++) {
        const Diagnostic *d = &engine->diagnostics[i];

        const char *level_str;
        const char *color;

        switch (d->level) {
            case DIAG_ERROR: {
                level_str = "error";
                color = COLOR_RED;
                break;
            }
            case DIAG_WARNING: {
                level_str = "warning";
                color = COLOR_YELLOW;
                break;
            }
            case DIAG_NOTE: {
                level_str = "note";
                color = COLOR_CYAN;
                break;
            }
            case DIAG_INFO: {
                level_str = "info";
                color = COLOR_WHITE;
                break;
            }
            default: {
                level_str = "unknown";
                color = COLOR_RESET;
                break;
            }
        }

        // format: file:line:column: level: message
        fprintf(stderr, "%s%s:%d:%d: %s%s%s: %s%s\n",
                COLOR_BOLD,
                d->location.filename ? d->location.filename : "<unknown>",
                d->location.line,
                d->location.column,
                color, level_str,
                COLOR_RESET,
                d->message,
                COLOR_RESET
        );
    }

    // print summary if there are errors or warnings
    if (engine->error_count > 0 || engine->warning_count > 0) {
        fprintf(stderr, "\n");
        if (engine->error_count > 0) {
            fprintf(stderr, "%s%d error%s generated%s\n",
                    COLOR_RED,
                    engine->error_count,
                    engine->error_count == 1 ? "" : "s",
                    COLOR_RESET
            );
        }
        if (engine->warning_count > 0) {
            fprintf(stderr, "%s%d warning%s generated%s\n",
                    COLOR_YELLOW,
                    engine->warning_count,
                    engine->warning_count == 1 ? "" : "s",
                    COLOR_RESET
            );
        }
    }
}

bool diag_has_errors(const DiagnosticEngine *engine) {
    return engine && engine->error_count > 0;
}

bool diag_has_warnings(const DiagnosticEngine *engine) {
    return engine && engine->warning_count > 0;
}

int diag_get_error_count(const DiagnosticEngine *engine) {
    return engine ? engine->error_count : 0;
}

int diag_get_warning_count(const DiagnosticEngine *engine) {
    return engine ? engine->warning_count : 0;
}

void diag_clear(DiagnosticEngine *engine) {
    if (!engine) return;

    for (int i = 0; i < engine->count; i++) {
        free(engine->diagnostics[i].message);
    }

    engine->count = 0;
    engine->error_count = 0;
    engine->warning_count = 0;
}