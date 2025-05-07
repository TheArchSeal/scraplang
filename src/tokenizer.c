#include "tokenizer.h"
#include "printerr.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Check whether chr is a lowercase letter.
bool is_lower(char chr) {
    return 'a' <= chr && chr <= 'z';
}

// Check whether chr is an uppercase letter.
bool is_upper(char chr) {
    return 'A' <= chr && chr <= 'Z';
}

// Check whether chr is a letter.
bool is_alpha(char chr) {
    return is_lower(chr) || is_upper(chr);
}

// Check whether chr is a decimal digit.
bool is_num(char chr) {
    return '0' <= chr && chr <= '9';
}

// Check whether chr is a hexadecimal digit.
bool is_hex(char chr) {
    return is_num(chr)
        || ('a' <= chr && chr <= 'f')
        || ('A' <= chr && chr <= 'F');
}

// Check whether chr is a letter or digit.
bool is_alphanum(char chr) {
    return is_alpha(chr) || is_num(chr);
}

// Check whether str of length len is an integer literal.
bool is_int(const char* str, size_t len) {
    // [0-9][_0-9a-zA-Z]*
    if (len == 0) return false;
    if (!is_num(str[0])) return false;
    for (size_t i = 1; i < len; i++) {
        if (!(is_alphanum(str[i]) || str[i] == '_')) {
            return false;
        }
    }
    return true;
}

// Check whether str of length len is a variable name.
bool is_var(const char* str, size_t len) {
    // [_a-zA-Z][_0-9a-zA-Z]*
    if (len == 0) return false;
    if (!(is_alpha(str[0]) || str[0] == '_')) return false;
    for (size_t i = 1; i < len; i++) {
        if (!(is_alphanum(str[i]) || str[i] == '_')) {
            return false;
        }
    }
    return true;
}

// Find the token type of the keyword str of length len.
// Returns ERROR_TOKEN if it is not a keyword.
TokenEnum get_keyword_type(const char* str, size_t len) {
    if (strncmp(str, "var", len) == 0) return VAR_TOKEN;
    if (strncmp(str, "const", len) == 0) return CONST_TOKEN;
    if (strncmp(str, "fn", len) == 0) return FN_TOKEN;
    if (strncmp(str, "wire", len) == 0) return WIRE_TOKEN;
    if (strncmp(str, "part", len) == 0) return PART_TOKEN;
    if (strncmp(str, "primitive", len) == 0) return PRIMITIVE_TOKEN;
    if (strncmp(str, "struct", len) == 0) return STRUCT_TOKEN;
    if (strncmp(str, "enum", len) == 0) return ENUM_TOKEN;
    if (strncmp(str, "if", len) == 0) return IF_TOKEN;
    if (strncmp(str, "else", len) == 0) return ELSE_TOKEN;
    if (strncmp(str, "switch", len) == 0) return SWITCH_TOKEN;
    if (strncmp(str, "case", len) == 0) return CASE_TOKEN;
    if (strncmp(str, "default", len) == 0) return DEFAULT_TOKEN;
    if (strncmp(str, "while", len) == 0) return WHILE_TOKEN;
    if (strncmp(str, "do", len) == 0) return DO_TOKEN;
    if (strncmp(str, "for", len) == 0) return FOR_TOKEN;
    if (strncmp(str, "type", len) == 0) return TYPE_TOKEN;
    if (strncmp(str, "void", len) == 0) return VOID_TOKEN;
    if (strncmp(str, "bool", len) == 0) return BOOL_TOKEN;
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

// Find the token type of the symbol str of length len.
// Returns ERROR_TOKEN if it is not a symbol.
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
    if (strncmp(str, "=", len) == 0) return EQ_TOKEN;
    if (strncmp(str, "==", len) == 0) return DEQ_TOKEN;
    if (strncmp(str, "!=", len) == 0) return NEQ_TOKEN;
    if (strncmp(str, "<", len) == 0) return LT_TOKEN;
    if (strncmp(str, "<<", len) == 0) return DLT_TOKEN;
    if (strncmp(str, "<=", len) == 0) return LEQ_TOKEN;
    if (strncmp(str, ">", len) == 0) return GT_TOKEN;
    if (strncmp(str, ">>", len) == 0) return DGT_TOKEN;
    if (strncmp(str, ">=", len) == 0) return GEQ_TOKEN;
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

// Find the token type of str of length len.
// Returns ERROR_TOKEN if it is not a token.
TokenEnum get_token_type(const char* str, size_t len) {
    TokenEnum token = get_keyword_type(str, len);
    if (token != ERROR_TOKEN) return token;
    token = get_symbol_type(str, len);
    if (token != ERROR_TOKEN) return token;
    if (is_int(str, len)) return INT_LITERAL;
    if (is_var(str, len)) return VAR_NAME;
    return ERROR_TOKEN;
}

// Find the value of the digit chr.
literal_t parse_digit(char chr) {
    if (is_num(chr)) return chr - '0';
    if (is_lower(chr)) return chr - 'a' + 10;
    if (is_upper(chr)) return chr - 'A' + 10;
    return 0;
}

// Find the value of the integer literal src.
// Result is stored in dst unless dst is NULL.
// Returns whether an error occurred.
bool parse_int(literal_t* dst, const char* src, ErrorData err) {
    if (src == NULL) return true;
    const char* it = src;

    // find the base of the integer
    literal_t base = 10;
    if (strncmp(it, "0x", 2) == 0) {
        base = 16;
        it += 2;
    } else if (strncmp(it, "0b", 2) == 0) {
        base = 2;
        it += 2;
    }

    literal_t n = 0;
    for (; *it != '\0'; it++) {
        // underscores do nothing
        if (*it == '_') continue;
        literal_t d = parse_digit(*it);
        // check that digit is valid in base
        if (is_alphanum(*it) && d < base) {
            n = n * base + d;
        } else {
            print_syntax_error(err, "invalid digit '%c' in integer literal '%s'\n", *it, src);
            return true;
        }
    }

    if (dst) *dst = n;
    return false;
}

// Find the name of the literal type based on the quote character.
const char* literal_name(char quote) {
    switch (quote) {
        case '\'': return "character";
        case '\"': return "string";
        default: return NULL;
    }
}

// Find the value of the string literal src of length src_len.
// The string literal must begin and end with a quote character.
// Result is stored in dst unless dst is NULL.
// Result length is stored in dst_len unless dst_len is NULL.
// Returns whether an error occurred.
bool parse_str(char** dst, size_t* dst_len, const char* src, size_t src_len, ErrorData err) {
    if (src == NULL) return true;

    // parsed string is never longer
    char* str = malloc(src_len);
    if (str == NULL) {
        print_malloc_error();
        return false;
    }

    size_t i = 0;
    bool escaping = false;
    const char* it = src;
    char quote = *it;

    while (*++it != quote || escaping) {
        if (escaping) {
            switch (*it) {
                // simple escape sequences
                case '\\': str[i++] = '\\'; break;
                case '\'': str[i++] = '\''; break;
                case '\"': str[i++] = '\"'; break;
                case 'n':  str[i++] = '\n'; break;
                case 'r':  str[i++] = '\r'; break;
                case 't':  str[i++] = '\t'; break;
                case 'v':  str[i++] = '\v'; break;
                case '0':  str[i++] = '\0'; break;

                case 'x': // numeric escape sequence
                    // left digit
                    char hi = *++it;
                    if (hi == '\0') {
                        print_syntax_error(err,
                            "invalid escape sequence '\\x' in %s literal %s\n",
                            literal_name(quote), src
                        );
                        free(str);
                        return true;
                    }

                    // right digit
                    char lo = *++it;
                    if (hi == '\0') {
                        print_syntax_error(err,
                            "invalid escape sequence '\\x%c' in %s literal %s\n",
                            hi, literal_name(quote), src
                        );
                        free(str);
                        return true;
                    }

                    // check that digits are valid hex
                    if (is_hex(hi) && is_hex(lo)) {
                        str[i++] = parse_digit(hi) * 16 + parse_digit(lo);
                    } else {
                        print_syntax_error(err,
                            "invalid escape sequence '\\x%c%c' in %s literal %s\n",
                            hi, lo, literal_name(quote), src
                        );
                        free(str);
                        return true;
                    }
                    break;

                default: // invalid escape sequence
                    print_syntax_error(err,
                        "invalid escape sequence '\\%c' in %s literal %s\n",
                        *it, literal_name(quote), src
                    );
                    free(str);
                    return true;
            }

            escaping = false;
        } else if (*it == '\\') {
            // begin escape sequence
            escaping = true;
        } else {
            // push regular character
            str[i++] = *it;
        }
    }

    // add null terminator
    str[i] = '\0';
    if (dst) *dst = str;
    if (dst_len) *dst_len = i;
    return false;
}

// Find the value of the character literal src of length src_len.
// The character literal must begin and end with a quote character.
// Result is stored in dst unless dst is NULL.
// Returns whether an error occurred.
bool parse_chr(char* dst, const char* src, size_t src_len, ErrorData err) {
    // parse like string literal
    char* str;
    size_t len;
    bool failed = parse_str(&str, &len, src, src_len, err);
    if (failed) return true;

    // check that length is 1
    if (len < 1) {
        print_syntax_error(err, "empty character literal %s\n", src);
        free(str);
        return true;
    }
    if (len > 1) {
        print_syntax_error(err, "multiple characters in character literal %s\n", src);
        free(str);
        return true;
    }

    // get first character
    *dst = *str;
    return false;
}

// Tokenize program assuming a tab width of tabsize.
// Result is terminated by an EOF_TOKEN.
// Returns NULL if an error occurred.
Token* tokenize(const char* program, size_t tabsize) {
    if (program == NULL) return NULL;

    // initialize array
    size_t length = 0, capacity = 1;
    Token* array = malloc(sizeof(Token) * capacity);
    if (array == NULL) {
        print_malloc_error();
        return NULL;
    }

    // initialize position
    size_t line = 1, col = 1;

    // initialize current token
    const char* tokenpos = program;
    size_t tokenlen = 0;
    TokenEnum tokentype = ERROR_TOKEN;
    size_t tokenline = line, tokencol = col;

    char chr;
    do {
        chr = *program++; // pop a character
        bool push_token = false; // whether current token is finished

        switch (chr) {
            // end token on whitespace or EOF
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
            case '\r':
                col = 1;
                push_token = true;
                break;
            case '\n':
                line++;
                col = 1;
                push_token = true;
                break;

            case '#': // comment
                // skip until end of line or file
                do {
                    chr = *program++;
                    col++;
                } while (chr != '\0' && chr != '\n');
                chr = *program--; // don't skip the end of line or file
                push_token = true;
                break;

            case '\'': // character and string literals
            case '\"':
                // end previous token first
                if (tokenlen) {
                    chr = *program--;
                    push_token = true;
                } else {
                    // parse entire string here
                    bool escaping = false;
                    char quote = chr;
                    do {
                        escaping = !escaping && chr == '\\';
                        chr = *program++;
                        tokenlen++;
                        col++;

                        // hit end of line or file before closing string
                        if (chr == '\0' || (!escaping && chr == '\n')) {
                            print_syntax_error((ErrorData) { tokenline, tokencol },
                                "missing terminating %c character in %s literal %.*s\n",
                                quote, literal_name(quote), tokenlen, tokenpos
                            );
                            free_token_arrn(array, length);
                            return NULL;
                        }
                    } while (chr != quote || escaping);

                    // check if it was a character or string by its quotes
                    tokentype = quote == '\'' ? CHR_LITERAL : STR_LITERAL;
                    tokenlen++;
                    col++;
                    push_token = true;
                }
                break;

            default:
                // check if current token would become invalid by adding next character
                TokenEnum next_type = get_token_type(tokenpos, tokenlen + 1);
                if (tokentype != ERROR_TOKEN && next_type == ERROR_TOKEN) {
                    // end previous one token if it does
                    push_token = true;
                    program--;
                } else {
                    // add character to current token otherwise
                    tokentype = next_type;
                    tokenlen++;
                    col++;
                }
                break;
        }

        // add current token to array and clear it
        if (push_token) {
            // skip empty tokens
            if (tokenlen > 0) {
                ErrorData err = { tokenline, tokencol };

                // check that token is valid
                if (tokentype == ERROR_TOKEN) {
                    print_syntax_error(err, "invalid token '%.*s'\n", tokenlen, tokenpos);
                    free_token_arrn(array, length);
                    return NULL;
                }

                // expand array when out of capacity
                if (length + 1 >= capacity) {
                    capacity *= 2;
                    Token* new_array = realloc(array, sizeof(Token) * capacity);
                    if (new_array == NULL) {
                        print_malloc_error();
                        free_token_arrn(array, length);
                        return NULL;
                    }
                    array = new_array;
                }

                // copy token to new string
                char* str = malloc(tokenlen + 1);
                if (str == NULL) {
                    print_malloc_error();
                    free_token_arrn(array, length);
                    return NULL;
                }
                strncpy(str, tokenpos, tokenlen);
                str[tokenlen] = '\0';

                // add necessary data based on type
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
                        // variable name is same as token string
                        data.var_name = str;
                        break;
                    default:
                        break;
                }

                // push token
                array[length++] = (Token){
                    .type = tokentype,
                    .str = str,
                    .line = tokenline,
                    .col = tokencol,
                    .data = data,
                };

                // clear token
                tokentype = ERROR_TOKEN;
                tokenlen = 0;
            }

            // set new start position
            tokenpos = program;
            tokenline = line;
            tokenline = col;
        }

    } while (chr);

    // add EOF terminator
    array[length] = (Token){
        .type = EOF_TOKEN,
        .str = NULL,
        .line = line,
        .col = col,
    };

    return array;
}

// Free all data inside token.
void free_token(Token token) {
    switch (token.type) {
        case ERROR_TOKEN:
        case EOF_TOKEN:
            break;

        case STR_LITERAL:
            free(token.str);
            free(token.data.str_literal);
            break;

        default:
            free(token.str);
            break;
    }
}

// Free token array of length n and all token data inside it.
void free_token_arrn(Token* arr, size_t n) {
    if (arr == NULL) return;
    for (size_t i = 0; i < n; i++) free_token(arr[i]);
    free(arr);
}

// Free EOF terminated token array and all token data inside it.
void free_token_arr(Token* arr) {
    if (arr == NULL) return;
    for (Token* it = arr; it->type != EOF_TOKEN; it++) free_token(*it);
    free(arr);
}
