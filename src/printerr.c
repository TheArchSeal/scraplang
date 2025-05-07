#include "printerr.h"
#include <stdio.h>
#include <stdarg.h>

const char* error_filename = NULL;
size_t error_suppress = 0;
bool error_indicator = false;

// Write error data and formatted output to stderr.
// Can be suppressed.
void syntax_error(ErrorData err, const char* format, ...) {
    if (error_suppress) return;

    va_list args;
    va_start(args, format);
    fprintf(stderr, "%s:%zu:%zu: syntax error: ", error_filename, err.line, err.col);
    vfprintf(stderr, format, args);
    error_indicator = true;
}

// Write error message to stderr.
void malloc_error(void) {
    fprintf(stderr, "error: memory allocation failed\n");
    error_indicator = true;
}

// Write error data and message to stderr.
void fread_error(void) {
    fprintf(stderr, "%s: error: cannot read file\n", error_filename);
    error_indicator = true;
}
