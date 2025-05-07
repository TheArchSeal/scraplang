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
    NOP,
    BLOCK,
    EXPR_STMT,

    DECL,
    FN_DECL,
    FN_DEF,
    BRANCH,
    SWITCH_STMT,
    WHILE_STMT,
    DO_STMT,
    FOR_STMT,
};

struct Stmt {
    Token token;
    size_t exprc, stmtc;
    Expr* exprv;
    Stmt* stmtv;
};

AST* parse(const Token* program);

void free_spec(TypeSpec spec);
void free_spec_arrn(TypeSpec* arr, size_t n);
void free_expr(Expr expr);
void free_expr_arrn(Expr* arr, size_t n);
void free_stmt(Stmt stmt);
void free_ast(AST* ast);
