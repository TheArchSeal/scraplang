#pragma once

#include <inttypes.h>
#include <stddef.h>

typedef int64_t literal_t;
#define PRIliteral PRIi64
#define LITERAL_TYPE I64_TYPE

typedef struct Token Token;

enum TokenEnum {
    ERROR_TOKEN,
    EOF_TOKEN,

    INT_LITERAL,
    CHR_LITERAL,
    STR_LITERAL,
    VAR_NAME,

    LPAREN,    // (
    RPAREN,    // )
    LBRACKET,  // [
    RBRACKET,  // ]
    LBRACE,    // {
    RBRACE,    // }

    PLUS,     // +
    DPLUS,    // ++
    MINUS,    // -
    DMINUS,   // --
    STAR,     // *
    SLASH,    // /
    PERCENT,  // %

    PIPE,      // |
    DPIPE,     // ||
    AND,       // &
    DAND,      // &&
    CARET,     // ^
    TILDE,     // ~
    EXCLMARK,  // !
    QMARK,     // ?

    EQ_TOKEN,   // =
    DEQ_TOKEN,  // ==
    NEQ_TOKEN,  // !=
    LT_TOKEN,   // <
    DLT_TOKEN,  // <<
    LEQ_TOKEN,  // <=
    GT_TOKEN,   // >
    DGT_TOKEN,  // >>
    GEQ_TOKEN,  // >=

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

    ARROW,   // ->
    DARROW,  // =>

    DOT,        // .
    COMMA,      // ,
    COLON,      // :
    SEMICOLON,  // ;

    VAR_TOKEN,
    CONST_TOKEN,
    FN_TOKEN,
    WIRE_TOKEN,
    PART_TOKEN,
    PRIMITIVE_TOKEN,
    STRUCT_TOKEN,
    ENUM_TOKEN,

    IF_TOKEN,
    ELSE_TOKEN,
    SWITCH_TOKEN,
    CASE_TOKEN,
    DEFAULT_TOKEN,
    WHILE_TOKEN,
    DO_TOKEN,
    FOR_TOKEN,

    RETURN_TOKEN,
    BREAK_TOKEN,
    CONTINUE_TOKEN,

    TYPE_TOKEN,
    VOID_TOKEN,
    BOOL_TOKEN,
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

typedef enum TokenEnum TokenEnum;
typedef union TokenData TokenData;

struct Token {
    TokenEnum type;
    char* str;
    size_t line, col;
    TokenData data;
};

Token* tokenize(const char* program, size_t tabsize);
void free_token_arr(Token* arr);
