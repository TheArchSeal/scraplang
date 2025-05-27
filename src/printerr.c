#include "printerr.h"
#include <stdio.h>
#include <stdarg.h>

const char* error_filename = NULL;
size_t error_line = 0, error_col = 0;

// Write error and formatted output to stderr.
// Can be suppressed.
void syntax_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "%s:%zu:%zu: syntax error: ", error_filename, error_line, error_col);
    vfprintf(stderr, format, args);
}

// Write error message to stderr.
void malloc_error(void) {
    fprintf(stderr, "error: memory allocation failed\n");
}

// Write error message to stderr.
void fread_error(void) {
    fprintf(stderr, "%s: error: cannot read file\n", error_filename);
}
