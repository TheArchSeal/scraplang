#pragma once

#include <stddef.h>
#include <inttypes.h>

typedef uint64_t literal_t;
#define PRIliteral PRIu64

typedef struct Token Token;
typedef enum TokenEnum TokenEnum;
typedef union TokenData TokenData;

enum TokenEnum {
    ERROR_TOKEN,
    EOF_TOKEN,

    INT_LITERAL,
    CHR_LITERAL,
    STR_LITERAL,
    VAR_NAME,

    LPAREN,     // (
    RPAREN,     // )
    LBRACKET,   // [
    RBRACKET,   // ]
    LBRACE,     // {
    RBRACE,     // }

    PLUS,       // +
    DPLUS,      // ++
    MINUS,      // -
    DMINUS,     // --
    STAR,       // *
    SLASH,      // /
    PERCENT,    // %

    PIPE,       // |
    DPIPE,      // ||
    AND,        // &
    DAND,       // &&
    CARET,      // ^
    TILDE,      // ~
    EXCLMARK,   // !
    QMARK,      // ?

    EQ,         // =
    DEQ,        // ==
    LT,         // <
    DLT,        // <<
    LEQ,        // <=
    GT,         // >
    DGT,        // >>
    GEQ,        // >=

    PLUSEQ,     // +=
    MINUSEQ,    // -=
    STAREQ,     // *=
    SLASHEQ,    // /=
    PERCENTEQ,  // %=
    PIPEEQ,     // |=
    ANDEQ,      // &=
    CARETEQ,    // ^=
    DLTEQ,      // <<=
    DGTEQ,      // >>=

    ARROW,      // ->
    DARROW,     // =>

    DOT,        // .
    COMMA,      // ,
    COLON,      // :
    DCOLON,     // ::
    SEMICOLON,  // ;

    VAR,
    CONST,
    FN,
    WIRE,
    PART,
    PRIMITIVE,
    STRUCT,
    ENUM,

    IF,
    ELSE,
    SWITCH,
    CASE,
    DEFAULT,
    WHILE,
    DO,
    FOR,

    NULL_TOKEN,
    TYPE_TOKEN,
    I8_TOKEN,
    I16_TOKEN,
    I32_TOKEN,
    I64_TOKEN,
    U8_TOKEN,
    U16_TOKEN,
    U32_TOKEN,
    U64_TOKEN,
};

union TokenData {
    literal_t int_literal;
    char chr_literal;
    char* str_literal;
    char* var_name;
};

struct Token {
    TokenEnum type;
    char* str;
    size_t line, col;
    TokenData data;
};

void free_token_arr(Token* arr);
void free_token_arrn(Token* arr, size_t n);

Token* tokenize(const char* program, size_t tabsize, const char* filename);
