#include "readfile.h"
#include "tokenizer.h"
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    const char* filename = "./test.sml";
    char* program = readfile(filename);
    Token* tokens = tokenize(program, 8, filename);
    free(program);
    if (tokens == NULL) return EXIT_FAILURE;

    for (Token* it = tokens; it->type != EOF_TOKEN; it++) {
        printf("(%d)\t%s", it->type, it->str);
        switch (it->type) {
            case INT_LITERAL: printf("\t%"PRIliteral"\n", it->data.int_literal); break;
            case CHR_LITERAL: printf("\t%c\n", it->data.chr_literal); break;
            case STR_LITERAL: printf("\t%s\n", it->data.str_literal); break;
            case VAR_NAME: printf("\t%s\n", it->data.var_name); break;
            default: printf("\n"); break;
        }
    }

    free_token_arr(tokens);
    return EXIT_SUCCESS;
}
