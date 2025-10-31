#include <stdio.h>
#include <stdlib.h>

#include "printerr.h"
#include "readfile.h"
#include "tokenizer.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "error: wrong number of command-line arguments\n");
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    error_filename = filename;

    char* program = readfile(filename);
    Token* tokens = tokenize(program, 4);
    if (tokens == NULL) {
        free(program);
        free_token_arr(tokens);
        return EXIT_FAILURE;
    }

    for (Token* it = tokens; it->type != EOF_TOKEN; it++) {
        printf("(%d):%zu:%zu %s", it->type, it->line, it->col, it->str);
        switch (it->type) {
            case INT_LITERAL: printf(" %" PRIliteral "\n", it->data.int_literal); break;
            case CHR_LITERAL: printf(" %c\n", it->data.chr_literal); break;
            case STR_LITERAL: printf(" %s\n", it->data.str_literal); break;
            case VAR_NAME:    printf(" %s\n", it->data.var_name); break;
            default:          printf("\n"); break;
        }
    }

    free(program);
    free_token_arr(tokens);
    return EXIT_SUCCESS;
}
