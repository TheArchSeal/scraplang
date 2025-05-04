#pragma once

#include <stddef.h>

typedef struct ErrorData ErrorData;

struct ErrorData {
    const char* filename;
    size_t line, col;
};

int printerr(ErrorData err, const char* format, ...);
int print_malloc_err(void);
int print_read_err(const char* filename);
