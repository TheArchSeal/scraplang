#include "tokenizer.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_CAPACITY 64
#define MAX_TOKEN_LENGTH 255
#define MALLOC_FAILED_MSG "Error: Memory allocation failed in tokenization\n"

bool is_alpha(char chr) {
    return ('a' <= chr && chr <= 'z')
        || ('A' <= chr && chr <= 'Z');
}

bool is_num(char chr) {
    return '0' <= chr && chr <= '9';
}

bool is_alphanum(char chr) {
    return is_alpha(chr) || is_num(chr);
}

bool is_int(const char* str) {
    if (str == NULL) return false;
    if (!is_num(*str)) return false;
    while (*++str) if (!is_alphanum(*str)) return false;
    return true;
}

bool is_var(const char* str) {
    if (str == NULL) return false;
    if (!is_alpha(*str)) return false;
    while (*++str) if (!is_alphanum(*str)) return false;
    return true;
}

TokenEnum get_keyword_type(const char* str) {
    if (strcmp(str, "var") == 0) return VAR;
    if (strcmp(str, "const") == 0) return CONST;
    if (strcmp(str, "fn") == 0) return FN;
    if (strcmp(str, "wire") == 0) return WIRE;
    if (strcmp(str, "part") == 0) return PART;
    if (strcmp(str, "primitive") == 0) return PRIMITIVE;
    if (strcmp(str, "struct") == 0) return STRUCT;
    if (strcmp(str, "enum") == 0) return ENUM;
    if (strcmp(str, "if") == 0) return IF;
    if (strcmp(str, "else") == 0) return ELSE;
    if (strcmp(str, "switch") == 0) return SWITCH;
    if (strcmp(str, "case") == 0) return CASE;
    if (strcmp(str, "default") == 0) return DEFAULT;
    if (strcmp(str, "while") == 0) return WHILE;
    if (strcmp(str, "do") == 0) return DO;
    if (strcmp(str, "for") == 0) return FOR;
    if (strcmp(str, "null") == 0) return NULL_TOKEN;
    if (strcmp(str, "type") == 0) return TYPE_TOKEN;
    if (strcmp(str, "i8") == 0) return I8_TOKEN;
    if (strcmp(str, "i16") == 0) return I16_TOKEN;
    if (strcmp(str, "i32") == 0) return I32_TOKEN;
    if (strcmp(str, "i64") == 0) return I64_TOKEN;
    if (strcmp(str, "u8") == 0) return U8_TOKEN;
    if (strcmp(str, "u16") == 0) return U16_TOKEN;
    if (strcmp(str, "u32") == 0) return U32_TOKEN;
    if (strcmp(str, "u64") == 0) return U64_TOKEN;
    return ERROR_TOKEN;
}

TokenEnum get_symbol_type(const char* str) {
    if (strcmp(str, "(") == 0) return LPAREN;
    if (strcmp(str, ")") == 0) return RPAREN;
    if (strcmp(str, "[") == 0) return LBRACKET;
    if (strcmp(str, "]") == 0) return RBRACKET;
    if (strcmp(str, "{") == 0) return LBRACE;
    if (strcmp(str, "}") == 0) return RBRACE;
    if (strcmp(str, "+") == 0) return PLUS;
    if (strcmp(str, "++") == 0) return DPLUS;
    if (strcmp(str, "-") == 0) return MINUS;
    if (strcmp(str, "--") == 0) return DMINUS;
    if (strcmp(str, "*") == 0) return STAR;
    if (strcmp(str, "/") == 0) return SLASH;
    if (strcmp(str, "%") == 0) return PERCENT;
    if (strcmp(str, "|") == 0) return PIPE;
    if (strcmp(str, "||") == 0) return DPIPE;
    if (strcmp(str, "&") == 0) return AND;
    if (strcmp(str, "&&") == 0) return DAND;
    if (strcmp(str, "^") == 0) return CARET;
    if (strcmp(str, "~") == 0) return TILDE;
    if (strcmp(str, "!") == 0) return EXCLMARK;
    if (strcmp(str, "?") == 0) return QMARK;
    if (strcmp(str, "=") == 0) return EQ;
    if (strcmp(str, "==") == 0) return DEQ;
    if (strcmp(str, "<") == 0) return LT;
    if (strcmp(str, "<<") == 0) return DLT;
    if (strcmp(str, "<=") == 0) return LEQ;
    if (strcmp(str, ">") == 0) return GT;
    if (strcmp(str, ">>") == 0) return DGT;
    if (strcmp(str, ">=") == 0) return GEQ;
    if (strcmp(str, "+=") == 0) return PLUSEQ;
    if (strcmp(str, "-=") == 0) return MINUSEQ;
    if (strcmp(str, "*=") == 0) return STAREQ;
    if (strcmp(str, "/=") == 0) return SLASHEQ;
    if (strcmp(str, "%=") == 0) return PERCENTEQ;
    if (strcmp(str, "|=") == 0) return PIPEEQ;
    if (strcmp(str, "&=") == 0) return ANDEQ;
    if (strcmp(str, "^=") == 0) return CARETEQ;
    if (strcmp(str, "<<=") == 0) return DLTEQ;
    if (strcmp(str, ">>=") == 0) return DGTEQ;
    if (strcmp(str, "->") == 0) return ARROW;
    if (strcmp(str, "=>") == 0) return DARROW;
    if (strcmp(str, ".") == 0) return DOT;
    if (strcmp(str, ",") == 0) return COMMA;
    if (strcmp(str, ":") == 0) return COLON;
    if (strcmp(str, "::") == 0) return DCOLON;
    if (strcmp(str, ";") == 0) return SEMICOLON;
    return ERROR_TOKEN;
}

TokenEnum get_token_type(const char* str) {
    TokenEnum token = get_keyword_type(str);
    if (token != ERROR_TOKEN) return token;
    token = get_symbol_type(str);
    if (token != ERROR_TOKEN) return token;
    if (is_int(str)) return INT_LITERAL;
    if (is_var(str)) return VAR_NAME;
    return ERROR_TOKEN;
}

Token* tokenize(const char* program, size_t tabsize) {
    if (program == NULL) return NULL;

    size_t length = 0, capacity = INITIAL_CAPACITY;
    Token* array = malloc(sizeof(Token) * capacity);
    if (array == NULL) {
        fprintf(stderr, MALLOC_FAILED_MSG);
        return NULL;
    }

    TokenEnum type = ERROR_TOKEN;
    size_t tokenlen = 0;
    char token[MAX_TOKEN_LENGTH + 1] = { '\0' }; // extra space for null terminator

    size_t line = 1, col = 1;
    size_t start_line = line, start_col = col;

    char chr;
    do {
        chr = *program++;
        bool push_token = false;

        switch (chr) {
            case '\0':
                push_token = true;
                break;
            case ' ':
                col++;
                push_token = true;
                break;
            case '\t':
                // next multiple of tabsize + 1
                col += tabsize - (col - 1) % tabsize;
                push_token = true;
                break;
            case '\n':
                line++;
                col = 1;
                push_token = true;
                break;

            case '#': // comment
                // skip untill end of line or file
                do {
                    chr = *program++;
                    col++;
                } while (chr != '\0' && chr != '\n');
                chr = *program--; // read end of line or file as usual
                push_token = true;
                break;

            default:
                if (tokenlen < MAX_TOKEN_LENGTH) {
                    token[tokenlen] = chr;
                    TokenEnum new_type = get_token_type(token);
                    if (type != ERROR_TOKEN && new_type == ERROR_TOKEN) {
                        token[tokenlen] = '\0';
                        push_token = true;
                        program--;
                    } else {
                        type = new_type;
                        tokenlen++;
                        col++;
                    }
                } else {
                    push_token = true;
                    program--;
                }
                break;
        }

        if (push_token) {
            if (tokenlen > 0) {

                if (type == ERROR_TOKEN) {
                    fprintf(stderr,
                        "Error: Unrecognized token '%s' at line %zu, character %zu.\n",
                        token, start_line, start_col
                    );
                    free(array);
                    return NULL;
                }

                // expand array when out of capacity
                if (length + 1 >= capacity) { // make sure EOF will still fit
                    capacity *= 2;
                    Token* new_array = realloc(array, sizeof(Token) * capacity);
                    if (new_array == NULL) {
                        fprintf(stderr, MALLOC_FAILED_MSG);
                        free(array);
                        return NULL;
                    }
                    array = new_array;
                }

                char* str = malloc(tokenlen + 1); // extra space for null terminator
                if (str == NULL) {
                    fprintf(stderr, MALLOC_FAILED_MSG);
                    free(array);
                    return NULL;
                }
                strcpy(str, token);

                TokenData data;
                switch (type) {
                    case INT_LITERAL:
                    case CHR_LITERAL:
                    case STR_LITERAL:
                        break;
                    case VAR_NAME:
                        data.var_name = str;
                        break;
                    default:
                        break;
                }

                array[length++] = (Token){
                    .type = type,
                    .str = str,
                    .line = start_line,
                    .col = start_col,
                    .data = data,
                };

                type = ERROR_TOKEN;
                memset(token, '\0', tokenlen);
                tokenlen = 0;
            }

            start_line = line;
            start_col = col;
        }

    } while (chr);

    array[length++] = (Token){
        .type = EOF_TOKEN,
        .str = NULL,
        .line = line,
        .col = col,
    };

    return array;
}

void free_token_arr(Token** arr) {
    if (arr == NULL || *arr == NULL) return;
    for (Token* it = *arr; it->type != EOF_TOKEN; it++) {
        if (it->type == STR_LITERAL) {
            free(it->data.str_literal);
        }
        free(it->str);
    }
    free(*arr);
}
