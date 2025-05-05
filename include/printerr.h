#pragma once

#include <stddef.h>

typedef struct ErrorData ErrorData;

struct ErrorData {
    size_t line, col;
};

extern const char* error_filename;
extern size_t error_supress;

int print_syntax_error(ErrorData err, const char* format, ...);
int print_malloc_error(void);
int print_read_error(void);
