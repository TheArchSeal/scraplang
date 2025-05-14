#include "printerr.h"
#include "readfile.h"
#include "tokenizer.h"
#include "parser.h"
#include <stdlib.h>
#include <stdio.h>

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
    if (tokens == NULL) {
        free(program);
        free_token_arr(tokens);
        free_ast_p(ast);
        return EXIT_FAILURE;
    }

    // for (Token* it = tokens; it->type != EOF_TOKEN; it++) {
    //     printf("(%d)\t%s", it->type, it->str);
    //     switch (it->type) {
    //         case INT_LITERAL: printf("\t%"PRIliteral"\n", it->data.int_literal); break;
    //         case CHR_LITERAL: printf("\t%c\n", it->data.chr_literal); break;
    //         case STR_LITERAL: printf("\t%s\n", it->data.str_literal); break;
    //         case VAR_NAME: printf("\t%s\n", it->data.var_name); break;
    //         default: printf("\n"); break;
    //     }
    // }

    free(program);
    free_token_arr(tokens);
    free_ast_p(ast);
    return EXIT_SUCCESS;
}
