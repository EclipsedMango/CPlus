
#include "preprocessor.h"
#include "../util/string_builder.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// internal state
struct Preprocessor {
    const char *filename;
    DiagnosticEngine *diagnostics;
    MacroTable *macros;

    const char *current;
    int line;
    int column;

    Vector expanding_macros;
};

static char* process_line(Preprocessor *prep, const char *line);
static bool parse_define_directive(Preprocessor *prep, const char *line);
static char* expand_macros(Preprocessor *prep, const char *text);
static char* expand_function_macro(Preprocessor *prep, Macro *macro, const char *args_start);
static bool is_macro_expanding(Preprocessor *prep, const char *name);

Preprocessor* preprocessor_create(const char *filename, DiagnosticEngine *diag) {
    Preprocessor *prep = malloc(sizeof(Preprocessor));
    if (!prep) return NULL;

    prep->filename = filename;
    prep->diagnostics = diag;
    prep->macros = macro_table_create();
    prep->line = 1;
    prep->column = 1;
    prep->expanding_macros = create_vector(8, sizeof(char*));

    return prep;
}

void preprocessor_destroy(Preprocessor *prep) {
    if (!prep) return;

    macro_table_destroy(prep->macros);
    vector_destroy(&prep->expanding_macros);
    free(prep);
}

char* preprocessor_process_file(Preprocessor *prep, FILE *file) {
    if (!prep || !file) return NULL;

    fseek(file, 0, SEEK_END);
    const long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(size + 1);
    fread(content, 1, size, file);
    content[size] = '\0';

    char *result = preprocessor_process(prep, content);
    free(content);
    return result;
}

char* preprocessor_process(Preprocessor *prep, const char *input) {
    if (!prep || !input) return NULL;

    StringBuilder *sb = sb_create(1024);
    prep->current = input;
    prep->line = 1;
    prep->column = 1;

    const char *line_start = input;
    const char *p = input;

    while (*p) {
        if (*p == '\n') {
            const size_t line_len = p - line_start;
            char *line = strndup(line_start, line_len);

            char *processed = process_line(prep, line);
            if (processed) {
                sb_append(sb, processed);
                sb_append(sb, "\n");
                free(processed);
            }

            free(line);

            line_start = p + 1;
            prep->line++;
            prep->column = 1;
        }
        p++;
    }

    if (line_start < p) {
        char *line = strdup(line_start);
        char *processed = process_line(prep, line);
        if (processed) {
            sb_append(sb, processed);
            free(processed);
        }
        free(line);
    }

    return sb_to_string(sb);
}

static char* process_line(Preprocessor *prep, const char *line) {
    while (isspace(*line)) line++;

    if (*line == '#') {
        line++; // skip the '#'
        while (isspace(*line)) line++;

        if (strncmp(line, "define", 6) == 0) {
            parse_define_directive(prep, line + 6);
            return NULL;
        }
        if (strncmp(line, "undef", 5) == 0) {
            // TODO: Parse undef
            return NULL;
        }
        // ddd more directives such as #ifdef, #include, etc.
    }

    return expand_macros(prep, line);
}

static bool parse_define_directive(Preprocessor *prep, const char *line) {
    while (isspace(*line)) line++;

    const char *name_start = line;
    while (isalnum(*line) || *line == '_') line++;

    if (line == name_start) {
        diag_error(prep->diagnostics, (SourceLocation){prep->line, prep->column, prep->filename}, "Expected macro name after #define");
        return false;
    }

    const size_t name_len = line - name_start;
    char *name = strndup(name_start, name_len);

    if (*line == '(') {
        line++;

        Vector params = create_vector(4, sizeof(char*));

        // params
        while (*line && *line != ')') {
            while (isspace(*line)) line++;

            const char *param_start = line;
            while (isalnum(*line) || *line == '_') line++;

            if (line > param_start) {
                const size_t param_len = line - param_start;
                char *param = strndup(param_start, param_len);
                vector_push(&params, &param);
            }

            while (isspace(*line)) line++;

            if (*line == ',') {
                line++;
            } else if (*line != ')') {
                diag_error(prep->diagnostics, (SourceLocation){prep->line, prep->column, prep->filename}, "Expected ',' or ')' in macro parameter list");
                free(name);
                vector_destroy(&params);
                return false;
            }
        }

        if (*line != ')') {
            diag_error(prep->diagnostics, (SourceLocation){prep->line, prep->column, prep->filename}, "Expected ')' after macro parameters");
            free(name);
            vector_destroy(&params);
            return false;
        }

        line++;
        while (isspace(*line)) line++;

        const char *replacement = line;

        Macro *macro = macro_create_function(
            name,
            params.elements,
            params.length,
            replacement,
            (SourceLocation){prep->line, prep->column, prep->filename}
        );

        macro_define(prep->macros, macro);
        free(macro);
        free(name);

        for (int i = 0; i < params.length; i++) {
            char **param = vector_get(&params, i);
            free(*param);
        }

        vector_destroy(&params);

        return true;
    }

    while (isspace(*line)) line++;

    const char *replacement = line;

    Macro *macro = macro_create_object(
        name,
        replacement,
        (SourceLocation){prep->line, prep->column, prep->filename}
    );

    macro_define(prep->macros, macro);
    free(macro);
    free(name);

    return true;
}

static char* expand_macros(Preprocessor *prep, const char *text) {
    StringBuilder *sb = sb_create(256);
    const char *p = text;

    while (*p) {
        if (!isalpha(*p) && *p != '_') {
            sb_append_char(sb, *p);
            p++;
            continue;
        }

        const char *id_start = p;
        while (isalnum(*p) || *p == '_') p++;

        const size_t id_len = p - id_start;
        char *identifier = strndup(id_start, id_len);

        Macro *macro = macro_lookup(prep->macros, identifier);

        if (macro && !is_macro_expanding(prep, identifier)) {
            if (macro->is_function_like) {
                const char *after_id = p;
                while (isspace(*after_id)) after_id++;

                if (*after_id == '(') {
                    char *expanded = expand_function_macro(prep, macro, after_id);
                    sb_append(sb, expanded);
                    free(expanded);

                    int paren_depth = 0;
                    p = after_id;
                    do {
                        if (*p == '(') paren_depth++;
                        if (*p == ')') paren_depth--;
                        p++;
                    } while (paren_depth > 0 && *p);

                    free(identifier);
                    continue;
                }

                sb_append(sb, identifier);

            } else {
                vector_push(&prep->expanding_macros, &identifier);

                char *expanded = expand_macros(prep, macro->replacement);
                sb_append(sb, expanded);
                free(expanded);

                vector_pop(&prep->expanding_macros);
            }
        } else {
            sb_append(sb, identifier);
        }

        free(identifier);
    }

    return sb_to_string(sb);
}

static char* expand_function_macro(Preprocessor *prep, Macro *macro, const char *args_start) {
    Vector args = create_vector(macro->param_count, sizeof(char*));

    const char *p = args_start + 1;
    int paren_depth = 1;
    const char *arg_start = p;

    while (*p && paren_depth > 0) {
        if (*p == '(') {
            paren_depth++;
        } else if (*p == ')') {
            paren_depth--;
            if (paren_depth == 0) {
                if (p > arg_start) {
                    const size_t arg_len = p - arg_start;
                    char *arg = strndup(arg_start, arg_len);

                    const char *trimmed = arg;
                    while (isspace(*trimmed) && *trimmed) trimmed++;
                    char *arg_copy = strdup(trimmed);
                    free(arg);

                    vector_push(&args, &arg_copy);
                }
            }
        } else if (*p == ',' && paren_depth == 1) {
            const size_t arg_len = p - arg_start;
            char *arg = strndup(arg_start, arg_len);

            const char *trimmed = arg;
            while (isspace(*trimmed) && *trimmed) trimmed++;
            char *arg_copy = strdup(trimmed);
            free(arg);

            vector_push(&args, &arg_copy);
            arg_start = p + 1;
        }
        p++;
    }

    if (args.length != macro->param_count) {
        diag_error(prep->diagnostics, (SourceLocation){prep->line, prep->column, prep->filename}, "Macro '%s' expects %d arguments, got %d", macro->name, macro->param_count, args.length);
        vector_destroy(&args);
        return strdup("");
    }

    StringBuilder *sb = sb_create(256);
    const char *repl = macro->replacement;

    while (*repl) {
        bool found_param = false;

        for (int i = 0; i < macro->param_count; i++) {
            char **param_ptr = vector_get(&macro->params, i);
            const size_t param_len = strlen(*param_ptr);

            if (strncmp(repl, *param_ptr, param_len) == 0) {
                const char next = repl[param_len];
                if (!isalnum(next) && next != '_') {
                    char **arg_ptr = vector_get(&args, i);
                    sb_append(sb, *arg_ptr);
                    repl += param_len;
                    found_param = true;
                    break;
                }
            }
        }

        if (!found_param) {
            sb_append_char(sb, *repl);
            repl++;
        }
    }

    for (int i = 0; i < args.length; i++) {
        char **arg = vector_get(&args, i);
        free(*arg);
    }
    vector_destroy(&args);

    char *result = sb_to_string(sb);
    char *final = expand_macros(prep, result);
    free(result);

    return final;
}

static bool is_macro_expanding(Preprocessor *prep, const char *name) {
    for (int i = 0; i < prep->expanding_macros.length; i++) {
        char **expanding = vector_get(&prep->expanding_macros, i);
        if (strcmp(*expanding, name) == 0) {
            return true;
        }
    }

    return false;
}