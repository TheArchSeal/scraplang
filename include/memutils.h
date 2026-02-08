#pragma once

#include <stdbool.h>
#include <stddef.h>

char* strndup(const char* str, size_t n);

void* malloc_struct(void* elem, size_t size);
#define MALLOC_STRUCT(elem) malloc_struct(&(elem), sizeof(elem))

typedef struct DynArr DynArr;
struct DynArr {
    void* c_arr;
    size_t elem_size;
    size_t length;
    size_t capacity;
};

DynArr dynarr_create(size_t elem_size);
void dynarr_destroy(DynArr* arr);

void* dynarr_get(DynArr* arr, size_t i);
bool dynarr_append(DynArr* arr, void* elem);
