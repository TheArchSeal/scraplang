#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

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
