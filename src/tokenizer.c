#include "tokenizer.h"
#include "printerr.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

bool is_lower(char chr) {
    return 'a' <= chr && chr <= 'z';
}

bool is_upper(char chr) {
    return 'A' <= chr && chr <= 'Z';
}

bool is_alpha(char chr) {
    return is_lower(chr) || is_upper(chr);
}

bool is_num(char chr) {
    return '0' <= chr && chr <= '9';
}

bool is_hex(char chr) {
    return is_num(chr)
        || ('a' <= chr && chr <= 'f')
        || ('A' <= chr && chr <= 'F');
}

bool is_alphanum(char chr) {
    return is_alpha(chr) || is_num(chr);
}

bool is_int(const char* str, size_t len) {
    if (len == 0) return false;
    if (!is_num(str[0])) return false;
    for (size_t i = 1; i < len; i++) {
        if (!(is_alphanum(str[i]) || str[i] == '_')) {
            return false;
        }
    }
    return true;
}

bool is_var(const char* str, size_t len) {
    if (len == 0) return false;
    if (!(is_alpha(str[0]) || str[0] == '_')) return false;
    for (size_t i = 1; i < len; i++) {
        if (!(is_alphanum(str[i]) || str[i] == '_')) {
            return false;
        }
    }
    return true;
}

TokenEnum get_keyword_type(const char* str, size_t len) {
    if (strncmp(str, "var", len) == 0) return VAR;
    if (strncmp(str, "const", len) == 0) return CONST;
    if (strncmp(str, "fn", len) == 0) return FN;
    if (strncmp(str, "wire", len) == 0) return WIRE;
    if (strncmp(str, "part", len) == 0) return PART;
    if (strncmp(str, "primitive", len) == 0) return PRIMITIVE;
    if (strncmp(str, "struct", len) == 0) return STRUCT;
    if (strncmp(str, "enum", len) == 0) return ENUM;
    if (strncmp(str, "if", len) == 0) return IF;
    if (strncmp(str, "else", len) == 0) return ELSE;
    if (strncmp(str, "switch", len) == 0) return SWITCH;
    if (strncmp(str, "case", len) == 0) return CASE;
    if (strncmp(str, "default", len) == 0) return DEFAULT;
    if (strncmp(str, "while", len) == 0) return WHILE;
    if (strncmp(str, "do", len) == 0) return DO;
    if (strncmp(str, "for", len) == 0) return FOR;
    if (strncmp(str, "null", len) == 0) return NULL_TOKEN;
    if (strncmp(str, "type", len) == 0) return TYPE_TOKEN;
    if (strncmp(str, "i8", len) == 0) return I8_TOKEN;
    if (strncmp(str, "i16", len) == 0) return I16_TOKEN;
    if (strncmp(str, "i32", len) == 0) return I32_TOKEN;
    if (strncmp(str, "i64", len) == 0) return I64_TOKEN;
    if (strncmp(str, "u8", len) == 0) return U8_TOKEN;
    if (strncmp(str, "u16", len) == 0) return U16_TOKEN;
    if (strncmp(str, "u32", len) == 0) return U32_TOKEN;
    if (strncmp(str, "u64", len) == 0) return U64_TOKEN;
    return ERROR_TOKEN;
}

TokenEnum get_symbol_type(const char* str, size_t len) {
    if (strncmp(str, "(", len) == 0) return LPAREN;
    if (strncmp(str, ")", len) == 0) return RPAREN;
    if (strncmp(str, "[", len) == 0) return LBRACKET;
    if (strncmp(str, "]", len) == 0) return RBRACKET;
    if (strncmp(str, "{", len) == 0) return LBRACE;
    if (strncmp(str, "}", len) == 0) return RBRACE;
    if (strncmp(str, "+", len) == 0) return PLUS;
    if (strncmp(str, "++", len) == 0) return DPLUS;
    if (strncmp(str, "-", len) == 0) return MINUS;
    if (strncmp(str, "--", len) == 0) return DMINUS;
    if (strncmp(str, "*", len) == 0) return STAR;
    if (strncmp(str, "/", len) == 0) return SLASH;
    if (strncmp(str, "%", len) == 0) return PERCENT;
    if (strncmp(str, "|", len) == 0) return PIPE;
    if (strncmp(str, "||", len) == 0) return DPIPE;
    if (strncmp(str, "&", len) == 0) return AND;
    if (strncmp(str, "&&", len) == 0) return DAND;
    if (strncmp(str, "^", len) == 0) return CARET;
    if (strncmp(str, "~", len) == 0) return TILDE;
    if (strncmp(str, "!", len) == 0) return EXCLMARK;
    if (strncmp(str, "?", len) == 0) return QMARK;
    if (strncmp(str, "=", len) == 0) return EQ;
    if (strncmp(str, "==", len) == 0) return DEQ;
    if (strncmp(str, "<", len) == 0) return LT;
    if (strncmp(str, "<<", len) == 0) return DLT;
    if (strncmp(str, "<=", len) == 0) return LEQ;
    if (strncmp(str, ">", len) == 0) return GT;
    if (strncmp(str, ">>", len) == 0) return DGT;
    if (strncmp(str, ">=", len) == 0) return GEQ;
    if (strncmp(str, "+=", len) == 0) return PLUSEQ;
    if (strncmp(str, "-=", len) == 0) return MINUSEQ;
    if (strncmp(str, "*=", len) == 0) return STAREQ;
    if (strncmp(str, "/=", len) == 0) return SLASHEQ;
    if (strncmp(str, "%=", len) == 0) return PERCENTEQ;
    if (strncmp(str, "|=", len) == 0) return PIPEEQ;
    if (strncmp(str, "&=", len) == 0) return ANDEQ;
    if (strncmp(str, "^=", len) == 0) return CARETEQ;
    if (strncmp(str, "<<=", len) == 0) return DLTEQ;
    if (strncmp(str, ">>=", len) == 0) return DGTEQ;
    if (strncmp(str, "->", len) == 0) return ARROW;
    if (strncmp(str, "=>", len) == 0) return DARROW;
    if (strncmp(str, ".", len) == 0) return DOT;
    if (strncmp(str, ",", len) == 0) return COMMA;
    if (strncmp(str, ":", len) == 0) return COLON;
    if (strncmp(str, "::", len) == 0) return DCOLON;
    if (strncmp(str, ";", len) == 0) return SEMICOLON;
    return ERROR_TOKEN;
}

TokenEnum get_token_type(const char* str, size_t len) {
    TokenEnum token = get_keyword_type(str, len);
    if (token != ERROR_TOKEN) return token;
    token = get_symbol_type(str, len);
    if (token != ERROR_TOKEN) return token;
    if (is_int(str, len)) return INT_LITERAL;
    if (is_var(str, len)) return VAR_NAME;
    return ERROR_TOKEN;
}

literal_t parse_digit(char chr) {
    if (is_num(chr)) return chr - '0';
    if (is_lower(chr)) return chr - 'a' + 10;
    if (is_upper(chr)) return chr - 'A' + 10;
    return 0;
}

bool parse_int(literal_t* dst, const char* src, ErrorData err) {
    if (src == NULL) return true;

    literal_t base = 10;
    const char* it = src;

    if (strncmp(it, "0x", 2) == 0) {
        base = 16;
        it += 2;
    } else if (strncmp(it, "0b", 2) == 0) {
        base = 2;
        it += 2;
    }

    literal_t n = 0;
    for (; *it != '\0'; it++) {
        if (*it == '_') continue;
        literal_t d = parse_digit(*it);
        if (is_alphanum(*it) && d < base) {
            n = n * base + d;
        } else {
            printerr(err, "invalid digit '%c' in integer literal '%s'\n", *it, src);
            return true;
        }
    }

    if (dst) *dst = n;
    return false;
}

const char* literal_name(char quote) {
    switch (quote) {
        case '\'': return "character";
        case '\"': return "string";
        default: return NULL;
    }
}

bool parse_str(char** dst, size_t* dst_len, const char* src, size_t src_len, ErrorData err) {
    if (src == NULL) return true;

    char* str = malloc(src_len); // parsed string is never larger
    if (str == NULL) {
        print_malloc_err();
        return false;
    }

    size_t i = 0;
    bool escaping = false;
    const char* it = src;
    char quote = *it;

    while (*++it != quote || escaping) {
        if (escaping) {
            switch (*it) {
                case '\\': str[i++] = '\\'; break;
                case '\'': str[i++] = '\''; break;
                case '\"': str[i++] = '\"'; break;
                case 'n':  str[i++] = '\n'; break;
                case 'r':  str[i++] = '\r'; break;
                case 't':  str[i++] = '\t'; break;
                case 'v':  str[i++] = '\v'; break;
                case '0':  str[i++] = '\0'; break;

                case 'x':
                    char hi = *++it;
                    if (hi == '\0') {
                        printerr(err,
                            "invalid escape sequence '\\x' in %s literal %s\n",
                            literal_name(quote), src
                        );
                        free(str);
                        return true;
                    }
                    char lo = *++it;
                    if (hi == '\0') {
                        printerr(err,
                            "invalid escape sequence '\\x%c' in %s literal %s\n",
                            hi, literal_name(quote), src
                        );
                        free(str);
                        return true;
                    }

                    literal_t hid = parse_digit(hi);
                    literal_t lod = parse_digit(lo);

                    if (is_alphanum(hi) && is_alphanum(lo) && hid < 16 && lod < 16) {
                        str[i++] = hid * 16 + lod;
                    } else {
                        printerr(err,
                            "invalid escape sequence '\\x%c%c' in %s literal %s\n",
                            hi, lo, literal_name(quote), src
                        );
                        free(str);
                        return true;
                    }
                    break;

                default:
                    printerr(err,
                        "invalid escape sequence '\\%c' in %s literal %s\n",
                        *it, literal_name(quote), src
                    );
                    free(str);
                    return true;
            }

            escaping = false;
        } else if (*it == '\\') {
            escaping = true;
        } else {
            str[i++] = *it;
        }
    }

    str[i] = '\0';
    if (dst) *dst = str;
    if (dst_len) *dst_len = i;
    return false;
}

bool parse_chr(char* dst, const char* src, size_t src_len, ErrorData err) {
    char* str;
    size_t len;
    bool failed = parse_str(&str, &len, src, src_len, err);
    if (failed) return true;

    if (len < 1) {
        printerr(err, "empty character literal %s\n", src);
        free(str);
        return true;
    }
    if (len > 1) {
        printerr(err, "multiple characters in character literal %s\n", src);
        free(str);
        return true;
    }

    *dst = *str;
    return false;
}

Token* tokenize(const char* program, size_t tabsize, const char* filename) {
    if (program == NULL) return NULL;

    size_t length = 0, capacity = 1;
    Token* array = malloc(sizeof(Token) * capacity);
    if (array == NULL) {
        print_malloc_err();
        return NULL;
    }

    size_t line = 1, col = 1;

    const char* tokenpos = program;
    size_t tokenlen = 0;
    TokenEnum tokentype = ERROR_TOKEN;
    size_t tokenline = line, tokencol = col;

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

            case '\'':
            case '\"':
                if (tokenlen) {
                    chr = *program--;
                    push_token = true;
                } else {
                    bool escaping = false;
                    char quote = chr;
                    do {
                        escaping = !escaping && chr == '\\';
                        chr = *program++;
                        tokenlen++;
                        col++;

                        if (chr == '\0' || (!escaping && chr == '\n')) {
                            printerr((ErrorData) { filename, tokenline, tokencol },
                                "missing terminating %c character in %s literal %.*s\n",
                                quote, literal_name(quote), tokenlen, tokenpos
                            );
                            free_token_arrn(array, length);
                            return NULL;
                        }
                    } while (chr != quote || escaping);

                    tokentype = quote == '\'' ? CHR_LITERAL : STR_LITERAL;
                    tokenlen++;
                    col++;
                    push_token = true;
                }
                break;

            default:
                TokenEnum next_type = get_token_type(tokenpos, tokenlen + 1);
                if (tokentype != ERROR_TOKEN && next_type == ERROR_TOKEN) {
                    push_token = true;
                    program--;
                } else {
                    tokentype = next_type;
                    tokenlen++;
                    col++;
                }
                break;
        }

        if (push_token) {
            if (tokenlen > 0) {
                ErrorData err = { filename, tokenline, tokencol };

                if (tokentype == ERROR_TOKEN) {
                    printerr(err, "unrecognized token '%.*s'\n", tokenlen, tokenpos);
                    free_token_arrn(array, length);
                    return NULL;
                }

                // expand array when out of capacity
                if (length + 1 >= capacity) { // make sure EOF will still fit
                    capacity *= 2;
                    Token* new_array = realloc(array, sizeof(Token) * capacity);
                    if (new_array == NULL) {
                        print_malloc_err();
                        free_token_arrn(array, length);
                        return NULL;
                    }
                    array = new_array;
                }

                char* str = malloc(tokenlen + 1); // extra space for null terminator
                if (str == NULL) {
                    print_malloc_err();
                    free_token_arrn(array, length);
                    return NULL;
                }
                strncpy(str, tokenpos, tokenlen);
                str[tokenlen] = '\0';

                TokenData data;
                switch (tokentype) {
                    case INT_LITERAL:
                        if (parse_int(&data.int_literal, str, err)) {
                            free_token_arrn(array, length);
                            free(str);
                            return NULL;
                        }
                        break;
                    case CHR_LITERAL:
                        if (parse_chr(&data.chr_literal, str, tokenlen, err)) {
                            free_token_arrn(array, length);
                            free(str);
                            return NULL;
                        }
                        break;
                    case STR_LITERAL:
                        if (parse_str(&data.str_literal, NULL, str, tokenlen, err)) {
                            free_token_arrn(array, length);
                            free(str);
                            return NULL;
                        }
                        break;
                    case VAR_NAME:
                        data.var_name = str;
                        break;
                    default:
                        break;
                }

                array[length++] = (Token){
                    .type = tokentype,
                    .str = str,
                    .line = tokenline,
                    .col = tokencol,
                    .data = data,
                };

                tokentype = ERROR_TOKEN;
                tokenlen = 0;
            }

            tokenpos = program;
            tokenline = line;
            tokenline = col;
        }

    } while (chr);

    array[length] = (Token){
        .type = EOF_TOKEN,
        .str = NULL,
        .line = line,
        .col = col,
    };

    return array;
}

void free_token_arr(Token* arr) {
    if (arr == NULL) return;
    for (Token* it = arr; it->type != EOF_TOKEN; it++) {
        if (it->type == STR_LITERAL) {
            free(it->data.str_literal);
        }
        free(it->str);
    }
    free(arr);
}

void free_token_arrn(Token* arr, size_t n) {
    if (arr == NULL) return;
    for (size_t i = 0; i < n; i++) {
        if (arr[i].type == STR_LITERAL) {
            free(arr[i].data.str_literal);
        }
        if (arr[i].type != NULL_TOKEN) {
            free(arr[i].str);
        }
    }
    free(arr);
}
