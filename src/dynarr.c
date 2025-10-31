#include "dynarr.h"

#include <stdlib.h>
#include <string.h>

DynArr dynarr_create(size_t elem_size) {
    return (DynArr) { .c_arr = NULL, .elem_size = elem_size, .length = 0, .capacity = 0 };
}

void dynarr_destroy(DynArr* arr) {
    free(arr->c_arr);
    arr->c_arr = NULL;
    arr->length = 0;
    arr->capacity = 0;
}

void* dynarr_get(DynArr* arr, size_t i) {
    return i < arr->length ? (char*)arr->c_arr + i * arr->elem_size : NULL;
}

bool dynarr_append(DynArr* arr, void* elem) {
    if (arr->length >= arr->capacity) {
        size_t cap = arr->capacity ? arr->capacity * 2 : 1;

        void* new = realloc(arr->c_arr, cap * arr->elem_size);
        if (new == NULL) return true;

        arr->capacity = cap;
        arr->c_arr = new;
    }

    memcpy((char*)arr->c_arr + arr->length++ * arr->elem_size, elem, arr->elem_size);
    return false;
}
