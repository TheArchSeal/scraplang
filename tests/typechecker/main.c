#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "printerr.h"
#include "readfile.h"
#include "tokenizer.h"
#include "typechecker.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "error: wrong number of command-line arguments\n");
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    error_filename = filename;

    char* program = readfile(filename);
    Token* tokens = tokenize(program, 4);
    AST* ast = parse(tokens);
    if (typecheck(ast)) {
        free(program);
        free_token_arr(tokens);
        free_ast_p(ast);
        return EXIT_FAILURE;
    }

    free(program);
    free_token_arr(tokens);
    free_typed_ast_p(ast);
    return EXIT_SUCCESS;
}
