#pragma once

#include "parser.h"

typedef struct Symbol Symbol;
typedef struct SymbolTable SymbolTable;

enum AnnotEnum {
    ERROR_ANNOT,
    UNDEFINED_ANNOT,

    VOID_ANNOT,
    BOOL_ANNOT,
    I8_ANNOT,
    I16_ANNOT,
    I32_ANNOT,
    I64_ANNOT,
    U8_ANNOT,
    U16_ANNOT,
    U32_ANNOT,
    U64_ANNOT,

    ARR_ANNOT,
    PTR_ANNOT,
    FUN_ANNOT,
    STRUCT_ANNOT,
    ENUM_ANNOT,
    TYPEDEF_ANNOT,
};

struct PtrAnnotData {
    Annot* type;
    bool mutable;
};

struct FunAnnotData {
    size_t paramc, optc;
    Annot* paramt;
    Annot* ret;
};

typedef struct PtrAnnotData PtrAnnotData;
typedef struct FunAnnotData FunAnnotData;

typedef enum AnnotEnum AnnotEnum;

struct Annot {
    AnnotEnum type;
    bool lvalue;
    bool mutable;

    union {
        Token atom;
        PtrAnnotData ptr;
        FunAnnotData fun;
        Symbol* typedefdata;
    };
};

SymbolTable* typecheck(AST* ast);
void free_typed_ast_p(AST* ast);
