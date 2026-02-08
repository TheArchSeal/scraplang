#pragma once

#include <stdbool.h>

#include "tokenizer.h"

struct Type;
typedef struct Type Type;

typedef struct Spec Spec;
typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef Stmt AST;

enum SpecEnum {
    ERROR_SPEC,
    INFERRED_SPEC,

    GROUPED_SPEC,
    ATOMIC_SPEC,
    ARR_SPEC,
    PTR_SPEC,
    FUN_SPEC,
};

struct PtrSpecData {
    Spec* spec;
    bool mutable;
};

struct FunSpecData {
    size_t paramc, optc;
    Spec* paramt;
    Spec* ret;
};

typedef struct PtrSpecData PtrSpecData;
typedef struct FunSpecData FunSpecData;

typedef enum SpecEnum SpecEnum;

struct Spec {
    SpecEnum type;
    size_t line, col;

    union {
        Spec* group;
        Token atom;
        PtrSpecData ptr;
        FunSpecData fun;
    };
};

enum ExprEnum {
    ERROR_EXPR,
    NO_EXPR,

    GROUPED_EXPR,
    ATOMIC_EXPR,
    ARR_EXPR,
    LAMBDA_EXPR,

    UNOP_EXPR,
    BINOP_EXPR,
    TERNOP_EXPR,

    SUBSRIPT_EXPR,
    CALL_EXPR,
    CONSTRUCTOR_EXPR,
    ACCESS_EXPR,
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
    size_t paramc, optc;
    Token* paramv;
    Spec* paramt;
    Expr* paramd;
    Expr* expr;
};

struct SubscriptData {
    Expr* arr;
    Expr* idx;
};

struct CallData {
    Expr* fun;
    size_t argc;
    Expr* argv;
};

struct AccessData {
    Expr* obj;
    Token memeber;
};

typedef enum OpEnum OpEnum;

struct OpExprData {
    OpEnum type;
    Token token;
    Expr* first;
    Expr* second;
    Expr* third;
};

typedef struct ArrExprData ArrExprData;
typedef struct LambdaExprData LambdaExprData;
typedef struct OpExprData OpExprData;
typedef struct SubscriptData SubscriptData;
typedef struct CallData CallData;
typedef struct ConstructorData ConstructorData;
typedef struct AccessData AccessData;

typedef enum ExprEnum ExprEnum;

struct Expr {
    ExprEnum type;
    size_t line, col;

    union {
        Expr* group;
        Token atom;
        ArrExprData arr;
        LambdaExprData lambda;
        OpExprData op;
        SubscriptData subscript;
        CallData call;
        AccessData access;
    };

    Type* annotation;
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

    FUNCTION_STMT,
    STRUCT_STMT,
    ENUM_STMT,

    RETURN_STMT,
    BREAK_STMT,
    CONTINUE_STMT,
};

struct BlockStmtData {
    size_t len;
    Stmt* stmts;
};

struct DeclData {
    Token name;
    Expr val;
    Spec spec;
    bool mutable;
};

struct TypedefData {
    Token name;
    Spec val;
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

struct FunData {
    Token name;
    size_t paramc, optc;
    Token* paramv;
    Spec* paramt;
    Expr* paramd;
    Spec ret;
    Stmt* body;
};

struct StructData {
    Token name;
    size_t paramc, optc;
    Token* paramv;
    Spec* paramt;
    Expr* paramd;
};

struct EnumData {
    Token name;
    size_t len;
    Token* items;
};

typedef struct BlockStmtData BlockStmtData;
typedef struct DeclData DeclData;
typedef struct TypedefData TypedefData;
typedef struct IfElseData IfElseData;
typedef struct SwitchData SwitchData;
typedef struct WhileData WhileData;
typedef struct ForData ForData;
typedef struct FunData FunData;
typedef struct StructData StructData;
typedef struct EnumData EnumData;

typedef enum StmtEnum StmtEnum;

struct Stmt {
    StmtEnum type;
    size_t line, col;

    union {
        BlockStmtData block;
        Expr expr;
        DeclData decl;
        TypedefData typedefdata;
        IfElseData ifelse;
        SwitchData switchcase;
        WhileData whileloop;
        ForData forloop;
        FunData fun;
        StructData structdef;
        EnumData enumdef;
    };
};

AST* parse(const Token* program);
void free_ast_p(AST* ast);
