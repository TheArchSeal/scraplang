#include "readfile.h"
#include "printerr.h"
#include <stdio.h>
#include <stdlib.h>

// Read entire file to string.
// Returns NULL on error.
char* readfile(const char* filename) {
    if (filename == NULL) return NULL;

    // open file
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        print_read_err(filename);
        return NULL;
    }

    // get file size
    int failed = fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    if (failed || fseek(fp, 0, SEEK_SET)) {
        print_read_err(filename);
        fclose(fp);
        return NULL;
    }

    // allocate string
    char* content = malloc(size + 1);
    if (content == NULL) {
        print_malloc_err();
        fclose(fp);
        return NULL;
    }

    // read file
    if (size > fread(content, 1, size, fp)) {
        print_read_err(filename);
        fclose(fp);
        free(content);
        return NULL;
    }

    // close and add null terminator
    fclose(fp);
    content[size] = '\0';
    return content;
}