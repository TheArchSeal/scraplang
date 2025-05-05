#include "printerr.h"
#include <stdio.h>
#include <stdarg.h>

const char* error_filename = NULL;
size_t error_supress = 0;

// Write error data and formatted output to stderr.
int print_syntax_error(ErrorData err, const char* format, ...) {
    if (error_supress) return 0;

    va_list args;
    va_start(args, format);
    int a = fprintf(stderr, "%s:%zu:%zu: syntax error: ", error_filename, err.line, err.col);
    int b = vfprintf(stderr, format, args);
    return a + b;
}

// Write error message to stderr.
int print_malloc_error(void) {
    if (error_supress) return 0;

    return fprintf(stderr, "error: memory allocation failed\n");
}

// Write error data and message to stderr.
int print_read_error(void) {
    if (error_supress) return 0;

    return fprintf(stderr, "%s: error: cannot read file\n", error_filename);
}
