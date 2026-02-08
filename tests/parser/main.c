#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "printerr.h"
#include "readfile.h"
#include "tokenizer.h"

void print_indent(size_t depth) {
    for (size_t i = 0; i < depth; i++) printf("    ");
}

void print_spec(Spec spec, size_t depth) {
    print_indent(depth);
    printf("type (%d):%zu:%zu", spec.type, spec.line, spec.col);

    switch (spec.type) {
        case ERROR_SPEC:    printf(" (error)\n"); break;
        case INFERRED_SPEC: printf(" (inferred)\n"); break;
        case GROUPED_SPEC:
            printf(" ()\n");
            print_spec(*spec.group, depth + 1);
            break;
        case ATOMIC_SPEC: printf(" %s\n", spec.atom.str); break;
        case ARR_SPEC:
            printf(" %s[]\n", spec.ptr.mutable ? "" : "const");
            print_spec(*spec.ptr.spec, depth + 1);
            break;
        case PTR_SPEC:
            printf(" %s*\n", spec.ptr.mutable ? "" : "const");
            print_spec(*spec.ptr.spec, depth + 1);
            break;
        case FUN_SPEC:
            printf(" (%zu?)=>\n", spec.fun.optc);
            for (size_t i = 0; i < spec.fun.paramc; i++) {
                print_spec(spec.fun.paramt[i], depth + 1);
            }
            print_spec(*spec.fun.ret, depth + 1);
            break;
    }
}

void print_expr(Expr expr, size_t depth) {
    print_indent(depth);
    printf("expr (%d):%zu:%zu", expr.type, expr.line, expr.col);

    switch (expr.type) {
        case ERROR_EXPR: printf(" (error)\n"); break;
        case NO_EXPR:    printf(" (empty)\n"); break;
        case GROUPED_EXPR:
            printf(" ()\n");
            print_expr(*expr.group, depth + 1);
            break;
        case ATOMIC_EXPR: printf(" %s\n", expr.atom.str); break;
        case ARR_EXPR:
            printf(" []\n");
            for (size_t i = 0; i < expr.arr.len; i++) {
                print_expr(expr.arr.items[i], depth + 1);
            }
            break;
        case LAMBDA_EXPR:
            printf(" ()=>\n");
            for (size_t i = 0; i < expr.lambda.paramc; i++) {
                print_indent(depth + 1);
                printf(
                    "param   :%zu:%zu %s\n", expr.lambda.paramv[i].line, expr.lambda.paramv[i].col,
                    expr.lambda.paramv[i].str
                );
                print_spec(expr.lambda.paramt[i], depth + 1);
                print_expr(expr.lambda.paramd[i], depth + 1);
            }
            print_expr(*expr.lambda.expr, depth + 1);
            break;
        case UNOP_EXPR:
            printf(" (%d)%s\n", expr.op.type, expr.op.token.str);
            print_expr(*expr.op.first, depth + 1);
            break;
        case BINOP_EXPR:
            printf(" (%d)%s\n", expr.op.type, expr.op.token.str);
            print_expr(*expr.op.first, depth + 1);
            print_expr(*expr.op.second, depth + 1);
            break;
        case TERNOP_EXPR:
            printf(" (%d)%s\n", expr.op.type, expr.op.token.str);
            print_expr(*expr.op.first, depth + 1);
            print_expr(*expr.op.second, depth + 1);
            print_expr(*expr.op.third, depth + 1);
            break;
        case SUBSRIPT_EXPR:
            printf(" []\n");
            print_expr(*expr.subscript.arr, depth + 1);
            print_expr(*expr.subscript.idx, depth + 1);
            break;
        case CALL_EXPR:
            printf(" ()\n");
            print_expr(*expr.call.fun, depth + 1);
            for (size_t i = 0; i < expr.call.argc; i++) {
                print_expr(expr.call.argv[i], depth + 1);
            }
            break;
        case CONSTRUCTOR_EXPR:
            printf(" {}\n");
            print_expr(*expr.call.fun, depth + 1);
            for (size_t i = 0; i < expr.call.argc; i++) {
                print_expr(expr.call.argv[i], depth + 1);
            }
            break;
        case ACCESS_EXPR:
            printf(" .%s\n", expr.access.memeber.str);
            print_expr(*expr.access.obj, depth + 1);
            break;
    }
}

void print_stmt(Stmt stmt, size_t depth) {
    print_indent(depth);
    printf("stmt (%d):%zu:%zu", stmt.type, stmt.line, stmt.col);

    switch (stmt.type) {
        case ERROR_STMT: printf(" (error)\n"); break;
        case NOP:        printf(" (nop)\n"); break;
        case BLOCK:
            printf(" {}\n");
            for (size_t i = 0; i < stmt.block.len; i++) {
                print_stmt(stmt.block.stmts[i], depth + 1);
            }
            break;
        case EXPR_STMT:
            printf(" ;\n");
            print_expr(stmt.expr, depth + 1);
            break;
        case DECL:
            printf(" %s %s\n", stmt.decl.mutable ? "var" : "const", stmt.decl.name.str);
            print_expr(stmt.decl.val, depth + 1);
            print_spec(stmt.decl.spec, depth + 1);
            break;
        case TYPEDEF:
            printf(" type %s\n", stmt.typedefdata.name.str);
            print_spec(stmt.typedefdata.val, depth + 1);
            break;
        case IFELSE_STMT:
            printf(" if%s\n", stmt.ifelse.on_false ? " else" : "");
            print_expr(stmt.ifelse.condition, depth + 1);
            print_stmt(*stmt.ifelse.on_true, depth + 1);
            if (stmt.ifelse.on_false) {
                print_stmt(*stmt.ifelse.on_false, depth + 1);
            }
            break;
        case SWITCH_STMT:
            printf(" switch\n");
            print_expr(stmt.switchcase.expr, depth + 1);
            for (size_t i = 0; i < stmt.switchcase.casec; i++) {
                if (i == stmt.switchcase.defaulti) {
                    print_indent(depth + 1);
                    printf("default\n");
                } else {
                    print_expr(stmt.switchcase.casev[i], depth + 1);
                }
                print_stmt(stmt.switchcase.branchv[i], depth + 1);
            }
            break;
        case WHILE_STMT:
            printf(" while\n");
            print_expr(stmt.whileloop.condition, depth + 1);
            print_stmt(*stmt.whileloop.body, depth + 1);
            break;
        case DOWHILE_STMT:
            printf(" do while\n");
            print_expr(stmt.whileloop.condition, depth + 1);
            print_stmt(*stmt.whileloop.body, depth + 1);
            break;
        case FOR_STMT:
            printf(" for\n");
            print_stmt(*stmt.forloop.init, depth + 1);
            print_expr(stmt.forloop.condition, depth + 1);
            print_expr(stmt.forloop.expr, depth + 1);
            print_stmt(*stmt.forloop.body, depth + 1);
            break;
        case FUNCTION_STMT:
            printf(" fn %s\n", stmt.fun.name.str);
            for (size_t i = 0; i < stmt.fun.paramc; i++) {
                print_indent(depth + 1);
                printf(
                    "param   :%zu:%zu %s\n", stmt.fun.paramv[i].line, stmt.fun.paramv[i].col,
                    stmt.fun.paramv[i].str
                );
                print_spec(stmt.fun.paramt[i], depth + 1);
                print_expr(stmt.fun.paramd[i], depth + 1);
            }
            print_spec(stmt.fun.ret, depth + 1);
            print_stmt(*stmt.fun.body, depth + 1);
            break;
        case STRUCT_STMT:
            printf(" struct %s\n", stmt.structdef.name.str);
            for (size_t i = 0; i < stmt.structdef.paramc; i++) {
                print_indent(depth + 1);
                printf(
                    "member  :%zu:%zu %s\n", stmt.structdef.paramv[i].line,
                    stmt.structdef.paramv[i].col, stmt.structdef.paramv[i].str
                );
                print_spec(stmt.structdef.paramt[i], depth + 1);
                print_expr(stmt.structdef.paramd[i], depth + 1);
            }
            break;
        case ENUM_STMT:
            printf(" enum %s\n", stmt.enumdef.name.str);
            for (size_t i = 0; i < stmt.enumdef.len; i++) {
                print_indent(depth + 1);
                printf(
                    "value   :%zu:%zu %s\n", stmt.enumdef.items[i].line, stmt.enumdef.items[i].col,
                    stmt.enumdef.items[i].str
                );
            }
            break;
        case RETURN_STMT:
            printf(" return\n");
            print_expr(stmt.expr, depth + 1);
            break;
        case BREAK_STMT:    printf(" break\n"); break;
        case CONTINUE_STMT: printf(" continue\n"); break;
    }
}

void print_ast_p(AST* ast) {
    for (size_t i = 0; i < ast->block.len; i++) {
        print_stmt(ast->block.stmts[i], 0);
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
