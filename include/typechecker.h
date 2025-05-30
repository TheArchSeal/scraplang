#pragma once

#include "parser.h"

enum TypeEnum {
    ERROR_TYPE,
    UNDEFINED_TYPE,

    VOID_TYPE,
    BOOL_TYPE,
    I8_TYPE,
    I16_TYPE,
    I32_TYPE,
    I64_TYPE,
    U8_TYPE,
    U16_TYPE,
    U32_TYPE,
    U64_TYPE,

    ARR_TYPE,
    PTR_TYPE,
    FUN_TYPE,
    STRUCT_TYPE,
    ENUM_TYPE,
    ENUM_ITEM_TYPE,
    TYPEDEF_TYPE,
};

struct PtrTypeData {
    Type* type;
    bool mutable;
};

struct FunTypeData {
    size_t paramc, optc;
    Type* paramt;
    Type* ret;
};

struct StructTypeData {
    size_t id;
    char* name;
    size_t paramc, optc;
    char** paramv;
    Type* paramt;
};

struct EnumTypeData {
    size_t id;
    char* name;
    size_t len;
    char** items;
};

struct EnumItemData {
    size_t id;
    char* name;
    char* item;
};

struct TypedefTypeData {
    size_t id;
    char* name;
    Type* type;
};

typedef struct PtrTypeData PtrTypeData;
typedef struct FunTypeData FunTypeData;
typedef struct StructTypeData StructTypeData;
typedef struct EnumTypeData EnumTypeData;
typedef struct EnumItemData EnumItemData;
typedef struct TypedefTypeData TypedefTypeData;

union TypeData {
    Token atom;
    PtrTypeData ptr;
    FunTypeData fun;
    StructTypeData structtype;
    EnumTypeData enumtype;
    EnumItemData enumitem;
    TypedefTypeData typedeftype;
};

typedef enum TypeEnum TypeEnum;
typedef union TypeData TypeData;

struct Type {
    TypeEnum type;
    bool lvalue;
    bool mutable;
    TypeData data;
};

bool typecheck(AST* ast);
void free_typed_ast_p(AST* ast);
Type clone_type(Type type);
