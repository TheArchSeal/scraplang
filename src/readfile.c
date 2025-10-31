#include "readfile.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "printerr.h"

// Read entire file to string.
// Returns NULL on error.
char* readfile(const char* filename) {
    if (filename == NULL) return NULL;

    // open file
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        fread_error();
        return NULL;
    }

    // get file size
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (errno) {
        fread_error();
        fclose(fp);
        return NULL;
    }

    // allocate string
    char* content = malloc(size + 1);
    if (content == NULL) {
        malloc_error();
        fclose(fp);
        return NULL;
    }

    // read file
    if (size > fread(content, 1, size, fp)) {
        fread_error();
        fclose(fp);
        free(content);
        return NULL;
    }

    // close and add null terminator
    fclose(fp);
    content[size] = '\0';
    return content;
}
