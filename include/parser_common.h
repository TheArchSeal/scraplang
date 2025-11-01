#pragma once

#include "memutils.h"
#include "parser.h"

// initial precedence for parse_expr
#define MAX_PRECEDENCE 12

TypeSpec parse_type_spec(const Token** it);
Expr parse_expr(const Token** it, size_t precedence);
Stmt parse_stmt(const Token** it);
Stmt parse_block(const Token** it);

void unexpected_token(Token token);
bool consume_expected_token(const Token** it, TokenEnum type);

bool is_expr(const Token* const* it);
bool is_statement(const Token* const* it);
bool is_lambda(const Token* const* it);

bool parse_params(
    const Token** it, size_t* len_dst, size_t* opt_dst, Token** names_dst, TypeSpec** types_dst,
    Expr** defs_dst
);
bool parse_args(const Token** it, size_t* len_dst, Expr** vals_dst);

void free_spec(TypeSpec spec);
void free_spec_arrn(TypeSpec* arr, size_t n);
void free_spec_dynarr(DynArr* arr);
void free_expr(Expr expr);
void free_expr_arrn(Expr* arr, size_t n);
void free_expr_dynarr(DynArr* arr);
void free_stmt(Stmt stmt);
void free_stmt_arrn(Stmt* arr, size_t n);
void free_stmt_dynarr(DynArr* arr);
