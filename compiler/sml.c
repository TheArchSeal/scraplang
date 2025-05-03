#include "readfile.h"
#include "tokenizer.h"
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    char* program = readfile("./test.sml");
    if (program == NULL) return EXIT_FAILURE;
    Token* tokens = tokenize(program, 4);
    free(program);
    if (tokens == NULL) return EXIT_FAILURE;

    for (Token* it = tokens; it->type != EOF_TOKEN; it++) {
        printf("%s\n", it->str);
    }

    free_token_arr(&tokens);
    return EXIT_SUCCESS;
}
