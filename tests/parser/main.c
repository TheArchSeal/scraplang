#include "printerr.h"
#include "readfile.h"
#include "tokenizer.h"
#include "parser.h"
#include <stdlib.h>
#include <stdio.h>

void print_spec(TypeSpec spec, size_t depth) {
    for (size_t i = 0; i < depth; i++) printf("    ");
    printf("type (%d):%zu:%zu", spec.type, spec.line, spec.col);

    switch (spec.type) {
        case ERROR_SPEC: printf(" (error)\n"); break;
        case INFERRED_SPEC: printf(" (inferred)\n"); break;
        case ATOMIC_SPEC: printf(" %s\n", spec.data.atom.str); break;
        case ARR_SPEC:
            printf(" %s[]\n", spec.data.ptr.mutable ? "" : "const");
            print_spec(*spec.data.ptr.spec, depth + 1);
            break;
        case PTR_SPEC:
            printf(" %s*\n", spec.data.ptr.mutable ? "" : "const");
            print_spec(*spec.data.ptr.spec, depth + 1);
            break;
        case FUN_SPEC:
            printf(" (...%zu?)=>...\n", spec.data.fun.optc);
            for (size_t i = 0; i < spec.data.fun.paramc; i++) {
                print_spec(spec.data.fun.paramt[i], depth + 1);
            }
            print_spec(*spec.data.fun.ret, depth + 1);
            break;
    }
}

void print_expr(Expr expr, size_t depth) {
    for (size_t i = 0; i < depth; i++) printf("    ");
    printf("expr (%d):%zu:%zu", expr.type, expr.line, expr.col);

    switch (expr.type) {
        case ERROR_EXPR: printf(" (error)\n"); break;
        case ATOMIC_EXPR: printf(" %s\n", expr.data.atom.str); break;
        case ARR_EXPR:
            printf(" [...]\n");
            for (size_t i = 0; i < expr.data.arr.len; i++) {
                print_expr(expr.data.arr.items[i], depth + 1);
            }
            break;
        case LAMBDA_EXPR:
            printf(" (...)=>...\n");
            for (size_t i = 0; i < expr.data.lambda.paramc; i++) {
                for (size_t i = 0; i < depth + 1; i++) printf("    ");
                printf("param   :%zu:%zu %s\n",
                    expr.data.lambda.paramv[i].line,
                    expr.data.lambda.paramv[i].col,
                    expr.data.lambda.paramv[i].str
                );
                print_spec(expr.data.lambda.paramt[i], depth + 1);
            }
            print_expr(*expr.data.lambda.expr, depth + 1);
            print_spec(*expr.data.lambda.ret, depth + 1);
            break;
        case UNOP_EXPR:
            printf(" %s\n", expr.data.op.token.str);
            print_expr(*expr.data.op.first, depth + 1);
            break;
        case BINOP_EXPR:
            printf(" %s\n", expr.data.op.token.str);
            print_expr(*expr.data.op.first, depth + 1);
            print_expr(*expr.data.op.second, depth + 1);
            break;
        case TERNOP_EXPR:
            printf(" %s\n", expr.data.op.token.str);
            print_expr(*expr.data.op.first, depth + 1);
            print_expr(*expr.data.op.second, depth + 1);
            print_expr(*expr.data.op.third, depth + 1);
            break;
    }
}

void print_stmt(Stmt stmt, size_t depth) {
    for (size_t i = 0; i < depth; i++) printf("    ");
    printf("stmt (%d):%zu:%zu", stmt.type, stmt.line, stmt.col);

    switch (stmt.type) {
        case ERROR_STMT: printf(" (error)\n"); break;
        case NOP: printf(" ;\n"); break;
        case BLOCK:
            printf(" {...}\n");
            for (size_t i = 0; i < stmt.data.block.len; i++) {
                print_stmt(stmt.data.block.stmts[i], depth + 1);
            }
            break;
        case EXPR_STMT:
            printf(" ...;\n");
            print_expr(stmt.data.expr, depth + 1);
            break;
        case DECL:
            printf(" %s %s=...;\n", stmt.data.decl.mutable ? "var" : "const", stmt.data.decl.name.str);
            print_expr(stmt.data.decl.val, depth + 1);
            print_spec(stmt.data.decl.spec, depth + 1);
            break;
    }
}

void print_ast_p(AST* ast) {
    for (size_t i = 0; i < ast->data.block.len; i++) {
        print_stmt(ast->data.block.stmts[i], 0);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "error: wrong number of command-line arguments\n");
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    error_filename = filename;

    char* program = readfile(filename);
    Token* tokens = tokenize(program, 4);
    AST* ast = parse(tokens);
    if (ast == NULL) {
        free(program);
        free_token_arr(tokens);
        free_ast_p(ast);
        return EXIT_FAILURE;
    }

    print_ast_p(ast);

    free(program);
    free_token_arr(tokens);
    free_ast_p(ast);
    return EXIT_SUCCESS;
}
