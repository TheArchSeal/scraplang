#pragma once

#include <stddef.h>
#include <stdbool.h>

extern const char* error_filename;
extern size_t error_line, error_col;

void syntax_error(const char* format, ...);
void type_error(const char* format, ...);
void malloc_error(void);
void fread_error(void);
