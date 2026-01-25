#ifndef C__DIAGNOSTICS_H
#define C__DIAGNOSTICS_H

#include "common.h"
#include <stdarg.h>
#include <stdbool.h>

typedef enum {
    DIAG_ERROR,
    DIAG_WARNING,
    DIAG_NOTE,
    DIAG_INFO
} DiagnosticLevel;

typedef struct {
    DiagnosticLevel level;
    SourceLocation location;
    char *message;
} Diagnostic;

typedef struct {
    Diagnostic *diagnostics;
    int count;
    int capacity;
    int error_count;
    int warning_count;
} DiagnosticEngine;

DiagnosticEngine* diag_create(void);
void diag_destroy(DiagnosticEngine *engine);

void diag_report(DiagnosticEngine *engine, DiagnosticLevel level, SourceLocation loc, const char *fmt, ...);

// print all diagnostics to stderr
void diag_print_all(DiagnosticEngine *engine);

// query diagnostic state
bool diag_has_errors(const DiagnosticEngine *engine);
bool diag_has_warnings(const DiagnosticEngine *engine);
int diag_get_error_count(const DiagnosticEngine *engine);
int diag_get_warning_count(const DiagnosticEngine *engine);

void diag_clear(DiagnosticEngine *engine);

#define diag_error(engine, loc, ...) \
diag_report(engine, DIAG_ERROR, loc, __VA_ARGS__)

#define diag_warning(engine, loc, ...) \
diag_report(engine, DIAG_WARNING, loc, __VA_ARGS__)

#define diag_note(engine, loc, ...) \
diag_report(engine, DIAG_NOTE, loc, __VA_ARGS__)

#define diag_info(engine, loc, ...) \
diag_report(engine, DIAG_INFO, loc, __VA_ARGS__)

#endif //C__DIAGNOSTICS_H