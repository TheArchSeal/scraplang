#include "readfile.h"
#include <stdio.h>
#include <stdlib.h>

char* readfile(const char* filename) {
    if (filename == NULL) return NULL;

    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: Failed to open file '%s'\n", filename);
        return NULL;
    }

    int err = fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    if (err || fseek(fp, 0, SEEK_SET)) {
        fprintf(stderr, "Error: Failed to seek file '%s'\n", filename);
        fclose(fp);
        return NULL;
    }

    char* content = malloc(size + 1);
    if (content == NULL) {
        fprintf(stderr, "Error: Memory allocation failed when reading file '%s'\n", filename);
        fclose(fp);
        return NULL;
    }
    if (size > fread(content, 1, size, fp)) {
        fprintf(stderr, "Error: Failed to read file '%s'\n", filename);
        fclose(fp);
        free(content);
        return NULL;
    }

    fclose(fp);
    content[size] = '\0';
    return content;
}