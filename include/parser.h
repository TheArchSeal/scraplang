#pragma once

#include "tokenizer.h"
#include <stdbool.h>

typedef struct TypeSpec TypeSpec;
typedef enum TypeSpecEnum TypeSpecEnum;
typedef union TypeSpecData TypeSpecData;
typedef struct PtrTypeSpecData PtrTypeSpecData;
typedef struct FunTypeSpecData FunTypeSpecData;

typedef struct Expr Expr;
typedef enum ExprEnum ExprEnum;
typedef enum OpEnum OpEnum;
typedef union ExprData ExprData;
typedef struct ArrExprData ArrExprData;
typedef struct LambdaExprData LambdaExprData;
typedef struct OpExprData OpExprData;

typedef struct Stmt Stmt;
typedef enum StmtEnum StmtEnum;
typedef union StmtData StmtData;
typedef struct BlockStmtData BlockStmtData;

typedef Stmt AST;

enum TypeSpecEnum {
    ERROR_SPEC,
    INFERRED_SPEC,
    ATOMIC_SPEC,
    ARR_SPEC,
    PTR_SPEC,
    FUN_SPEC,
};

struct PtrTypeSpecData {
    TypeSpec* item_type;
    bool mutable;
};

struct FunTypeSpecData {
    size_t paramc, optc;
    TypeSpec* paramt;
    TypeSpec* ret;
};

union TypeSpecData {
    PtrTypeSpecData ptr;
    FunTypeSpecData fun;
};

struct TypeSpec {
    TypeSpecEnum type;
    Token token;
    TypeSpecData data;
};

enum ExprEnum {
    ERROR_EXPR,

    LITERAL_EXPR,
    VAR_EXPR,

    ARR_EXPR,
    LAMBDA_EXPR,

    UNOP_EXPR,
    BINOP_EXPR,
    TERNOP_EXPR,
};

enum OpEnum {
    ERROR_OP,

    POSTFIX_INC,
    POSTFIX_DEC,

    PREFIX_INC,
    PREFIX_DEC,
    UNARY_PLUS,
    UNARY_MINUS,
    LOGICAL_NOT,
    BINARY_NOT,
    DEREFERENCE,
    ADDRESS_OF,

    MULTIPLICATION,
    DIVISION,
    MODULO,
    ADDITION,
    SUBTRACTION,
    LEFT_SHIFT,
    RIGHT_SHIFT,

    BITWISE_AND,
    BITWISE_XOR,
    BITWISE_OR,

    LESS_THAN,
    LESS_OR_EQUAL,
    GREATER_THAN,
    GREATER_OR_EQUAL,
    EQUAL,
    NOT_EQUAL,

    LOGICAL_AND,
    LOGICAL_OR,

    TERNARY,

    ASSIGNMENT,

    COMMA_OP,
};

struct ArrExprData {
    size_t len;
    Expr* items;
};

struct LambdaExprData {
    size_t paramc;
    Token* paramv;
    TypeSpec* paramt;
    Expr* expr;
    TypeSpec* ret;
};

struct OpExprData {
    OpEnum type;
    Expr* first;
    Expr* second;
    Expr* third;
};

union ExprData {
    ArrExprData arr;
    LambdaExprData lambda;
    OpExprData op;
};

struct Expr {
    ExprEnum type;
    Token token;
    ExprData data;
};

enum StmtEnum {
    ERROR_STMT,

    NOP,
    BLOCK,
    EXPR_STMT,
};

struct BlockStmtData {
    size_t len;
    Stmt* stmts;
};

union StmtData {
    BlockStmtData block;
    Expr expr;
};

struct Stmt {
    StmtEnum type;
    Token token;
    StmtData data;
};

AST* parse(const Token* program);
void free_ast_p(AST* ast);
