#pragma once

#include "memutils.h"
#include "typechecker.h"

enum SymbolEnum {
    ERROR_SYMBOL,

    ANNOTATED_SYMBOL,
    STRUCT_SYMBOL,
    ENUM_SYMBOL,
    TYPEDEF_SYMBOL
};

struct StructSymbolData {
    size_t paramc, optc;
    char** paramv;
    Annot* paramt;
};
struct EnumSymbolData {
    size_t len;
    const char** items;
};

typedef struct StructSymbolData StructSymbolData;
typedef struct EnumSymbolData EnumSymbolData;

typedef enum SymbolEnum SymbolEnum;

struct Symbol {
    SymbolEnum type;
    const char* identifier;
    size_t line, col;

    union {
        Annot annotation;
        StructSymbolData structdata;
        EnumSymbolData enumdata;
        Symbol* typedefdata;
    };
};

typedef struct SymbolTable SymbolTable;
struct SymbolTable {
    SymbolTable* parent;
    size_t childc;
    SymbolTable* childv;

    size_t len;
    Symbol* symbols;
};

void free_symbol_table_p(SymbolTable* ast);
