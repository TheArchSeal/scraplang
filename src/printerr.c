#include "printerr.h"
#include <stdio.h>
#include <stdarg.h>

// Write error data and formatted output to stderr.
int printerr(ErrorData err, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int a = fprintf(stderr, "%s:%zu:%zu: error: ", err.filename, err.line, err.col);
    int b = vfprintf(stderr, format, args);
    return a + b;
}

// Write error message to stderr.
int print_malloc_err(void) {
    return fprintf(stderr, "error: memory allocation failed\n");
}

// Write error data and message to stderr.
int print_read_err(const char* filename) {
    return fprintf(stderr, "%s: error: cannot read file\n", filename);
}
