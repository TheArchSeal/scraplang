#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct ErrorData ErrorData;

struct ErrorData {
    size_t line, col;
};

extern const char* error_filename;
extern size_t error_suppress;
extern bool error_indicator;

void syntax_error(ErrorData err, const char* format, ...);
void malloc_error(void);
void fread_error(void);
