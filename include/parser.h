#pragma once

#include "tokenizer.h"
#include <stdbool.h>

typedef struct TypeSpec TypeSpec;
typedef struct Expr Expr;
typedef struct Stmt Stmt;
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
    TypeSpec* spec;
    bool mutable;
};

struct FunTypeSpecData {
    size_t paramc, optc;
    TypeSpec* paramt;
    TypeSpec* ret;
};

typedef struct PtrTypeSpecData PtrTypeSpecData;
typedef struct FunTypeSpecData FunTypeSpecData;

union TypeSpecData {
    Token atom;
    PtrTypeSpecData ptr;
    FunTypeSpecData fun;
};

typedef enum TypeSpecEnum TypeSpecEnum;
typedef union TypeSpecData TypeSpecData;

struct TypeSpec {
    TypeSpecEnum type;
    size_t line, col;
    TypeSpecData data;
};

enum ExprEnum {
    ERROR_EXPR,
    NO_EXPR,

    ATOMIC_EXPR,
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
    TypeSpec ret;
};

typedef enum OpEnum OpEnum;
typedef struct ArrExprData ArrExprData;
typedef struct LambdaExprData LambdaExprData;
typedef struct OpExprData OpExprData;

struct OpExprData {
    OpEnum type;
    Token token;
    Expr* first;
    Expr* second;
    Expr* third;
};

union ExprData {
    Token atom;
    ArrExprData arr;
    LambdaExprData lambda;
    OpExprData op;
};

typedef enum ExprEnum ExprEnum;
typedef union ExprData ExprData;

struct Expr {
    ExprEnum type;
    size_t line, col;
    ExprData data;
};

enum StmtEnum {
    ERROR_STMT,

    NOP,
    BLOCK,
    EXPR_STMT,
    DECL,
    TYPEDEF,

    IFELSE_STMT,
    SWITCH_STMT,
    WHILE_STMT,
    DOWHILE_STMT,
    FOR_STMT,
};

struct BlockStmtData {
    size_t len;
    Stmt* stmts;
};

struct DeclData {
    Token name;
    Expr val;
    TypeSpec spec;
    bool mutable;
};

struct TypedefData {
    Token name;
    TypeSpec val;
};

struct IfElseData {
    Expr condition;
    Stmt* on_true;
    Stmt* on_false;
};

struct SwitchData {
    Expr expr;
    size_t casec;
    Expr* casev;
    Stmt* branchv;
    size_t defaulti;
};

struct WhileData {
    Expr condition;
    Stmt* body;
};

struct ForData {
    Stmt* init;
    Expr condition;
    Expr expr;
    Stmt* body;
};

typedef struct BlockStmtData BlockStmtData;
typedef struct DeclData DeclData;
typedef struct TypedefData TypedefData;
typedef struct IfElseData IfElseData;
typedef struct SwitchData SwitchData;
typedef struct WhileData WhileData;
typedef struct ForData ForData;

union StmtData {
    BlockStmtData block;
    Expr expr;
    DeclData decl;
    TypedefData type;
    IfElseData ifelse;
    SwitchData switchcase;
    WhileData whileloop;
    ForData forloop;
};

typedef enum StmtEnum StmtEnum;
typedef union StmtData StmtData;

struct Stmt {
    StmtEnum type;
    size_t line, col;
    StmtData data;
};

AST* parse(const Token* program);
void free_ast_p(AST* ast);
