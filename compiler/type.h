#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct Type Type;
typedef enum TypeEnum TypeEnum;
typedef union TypeData TypeData;

typedef struct FunctionType FunctionType;
typedef struct PointerType PointerType;
typedef struct ArrayType ArrayType;
typedef struct TupleType TupleType;
typedef struct StructType StructType;
typedef struct EnumType EnumType;

enum TypeEnum {
    NULL_TYPE,
    TYPE_TYPE,

    I8_TYPE,
    I16_TYPE,
    I32_TYPE,
    I64_TYPE,
    U8_TYPE,
    U16_TYPE,
    U32_TYPE,
    U64_TYPE,

    FUNCTION_TYPE,
    POINTER_TYPE,
    ARRAY_TYPE,
    TUPLE_TYPE,
    STRUCT_TYPE,
};

struct Type {
    TypeEnum type;
    TypeData* data;
};

struct FunctionType {
    size_t param_count;
    char** param_names;
    Type* param_types;
    bool* param_opt;
    bool variadic;
};

struct PointerType {
    Type type;
    bool constant;
};

struct ArrayType {
    bool fixed_length;
    size_t length;
    Type type;
    bool constant;
};

struct TupleType {
    size_t length;
    Type* types;
};

struct StructType {
    size_t member_count;
    char** member_names;
    Type* member_types;
};

struct EnumType {
    size_t value_count;
    char** value_names;
};

union TypeData {
    FunctionType function_d;
    PointerType pointer_d;
    ArrayType array_d;
    TupleType tuple_d;
    StructType struct_d;
    EnumType enum_d;
    Type type_d;
};
