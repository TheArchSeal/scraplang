#include "readfile.h"
#include "printerr.h"
#include <stdio.h>
#include <stdlib.h>

char* readfile(const char* filename) {
    if (filename == NULL) return NULL;

    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        print_read_err(filename);
        return NULL;
    }

    int failed = fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    if (failed || fseek(fp, 0, SEEK_SET)) {
        print_read_err(filename);
        fclose(fp);
        return NULL;
    }

    char* content = malloc(size + 1);
    if (content == NULL) {
        print_malloc_err();
        fclose(fp);
        return NULL;
    }
    if (size > fread(content, 1, size, fp)) {
        print_read_err(filename);
        fclose(fp);
        free(content);
        return NULL;
    }

    fclose(fp);
    content[size] = '\0';
    return content;
}