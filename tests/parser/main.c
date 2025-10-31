#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "printerr.h"
#include "readfile.h"
#include "tokenizer.h"

void print_indent(size_t depth) {
    for (size_t i = 0; i < depth; i++) printf("    ");
}

void print_spec(TypeSpec spec, size_t depth) {
    print_indent(depth);
    printf("type (%d):%zu:%zu", spec.type, spec.line, spec.col);

    switch (spec.type) {
        case ERROR_SPEC:    printf(" (error)\n"); break;
        case INFERRED_SPEC: printf(" (inferred)\n"); break;
        case GROUPED_SPEC:
            printf(" ()\n");
            print_spec(*spec.data.group, depth + 1);
            break;
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
            printf(" (%zu?)=>\n", spec.data.fun.optc);
            for (size_t i = 0; i < spec.data.fun.paramc; i++) {
                print_spec(spec.data.fun.paramt[i], depth + 1);
            }
            print_spec(*spec.data.fun.ret, depth + 1);
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
            print_expr(*expr.data.group, depth + 1);
            break;
        case ATOMIC_EXPR: printf(" %s\n", expr.data.atom.str); break;
        case ARR_EXPR:
            printf(" []\n");
            for (size_t i = 0; i < expr.data.arr.len; i++) {
                print_expr(expr.data.arr.items[i], depth + 1);
            }
            break;
        case LAMBDA_EXPR:
            printf(" ()=>\n");
            for (size_t i = 0; i < expr.data.lambda.paramc; i++) {
                print_indent(depth + 1);
                printf(
                    "param   :%zu:%zu %s\n", expr.data.lambda.paramv[i].line,
                    expr.data.lambda.paramv[i].col, expr.data.lambda.paramv[i].str
                );
                print_spec(expr.data.lambda.paramt[i], depth + 1);
                print_expr(expr.data.lambda.paramd[i], depth + 1);
            }
            print_expr(*expr.data.lambda.expr, depth + 1);
            break;
        case UNOP_EXPR:
            printf(" (%d)%s\n", expr.data.op.type, expr.data.op.token.str);
            print_expr(*expr.data.op.first, depth + 1);
            break;
        case BINOP_EXPR:
            printf(" (%d)%s\n", expr.data.op.type, expr.data.op.token.str);
            print_expr(*expr.data.op.first, depth + 1);
            print_expr(*expr.data.op.second, depth + 1);
            break;
        case TERNOP_EXPR:
            printf(" (%d)%s\n", expr.data.op.type, expr.data.op.token.str);
            print_expr(*expr.data.op.first, depth + 1);
            print_expr(*expr.data.op.second, depth + 1);
            print_expr(*expr.data.op.third, depth + 1);
            break;
        case SUBSRIPT_EXPR:
            printf(" []\n");
            print_expr(*expr.data.subscript.arr, depth + 1);
            print_expr(*expr.data.subscript.idx, depth + 1);
            break;
        case CALL_EXPR:
            printf(" ()\n");
            print_expr(*expr.data.call.fun, depth + 1);
            for (size_t i = 0; i < expr.data.call.argc; i++) {
                print_expr(expr.data.call.argv[i], depth + 1);
            }
            break;
        case CONSTRUCTOR_EXPR:
            printf(" {}\n");
            print_expr(*expr.data.call.fun, depth + 1);
            for (size_t i = 0; i < expr.data.call.argc; i++) {
                print_expr(expr.data.call.argv[i], depth + 1);
            }
            break;
        case ACCESS_EXPR:
            printf(" .%s\n", expr.data.access.memeber.str);
            print_expr(*expr.data.access.obj, depth + 1);
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
            for (size_t i = 0; i < stmt.data.block.len; i++) {
                print_stmt(stmt.data.block.stmts[i], depth + 1);
            }
            break;
        case EXPR_STMT:
            printf(" ;\n");
            print_expr(stmt.data.expr, depth + 1);
            break;
        case DECL:
            printf(" %s %s\n", stmt.data.decl.mutable ? "var" : "const", stmt.data.decl.name.str);
            print_expr(stmt.data.decl.val, depth + 1);
            print_spec(stmt.data.decl.spec, depth + 1);
            break;
        case TYPEDEF:
            printf(" type %s\n", stmt.data.type.name.str);
            print_spec(stmt.data.type.val, depth + 1);
            break;
        case IFELSE_STMT:
            printf(" if%s\n", stmt.data.ifelse.on_false ? " else" : "");
            print_expr(stmt.data.ifelse.condition, depth + 1);
            print_stmt(*stmt.data.ifelse.on_true, depth + 1);
            if (stmt.data.ifelse.on_false) {
                print_stmt(*stmt.data.ifelse.on_false, depth + 1);
            }
            break;
        case SWITCH_STMT:
            printf(" switch\n");
            print_expr(stmt.data.switchcase.expr, depth + 1);
            for (size_t i = 0; i < stmt.data.switchcase.casec; i++) {
                if (i == stmt.data.switchcase.defaulti) {
                    print_indent(depth + 1);
                    printf("default\n");
                } else {
                    print_expr(stmt.data.switchcase.casev[i], depth + 1);
                }
                print_stmt(stmt.data.switchcase.branchv[i], depth + 1);
            }
            break;
        case WHILE_STMT:
            printf(" while\n");
            print_expr(stmt.data.whileloop.condition, depth + 1);
            print_stmt(*stmt.data.whileloop.body, depth + 1);
            break;
        case DOWHILE_STMT:
            printf(" do while\n");
            print_expr(stmt.data.whileloop.condition, depth + 1);
            print_stmt(*stmt.data.whileloop.body, depth + 1);
            break;
        case FOR_STMT:
            printf(" for\n");
            print_stmt(*stmt.data.forloop.init, depth + 1);
            print_expr(stmt.data.forloop.condition, depth + 1);
            print_expr(stmt.data.forloop.expr, depth + 1);
            print_stmt(*stmt.data.forloop.body, depth + 1);
            break;
        case FUNCTION_STMT:
            printf(" fn %s\n", stmt.data.fun.name.str);
            for (size_t i = 0; i < stmt.data.fun.paramc; i++) {
                print_indent(depth + 1);
                printf(
                    "param   :%zu:%zu %s\n", stmt.data.fun.paramv[i].line,
                    stmt.data.fun.paramv[i].col, stmt.data.fun.paramv[i].str
                );
                print_spec(stmt.data.fun.paramt[i], depth + 1);
                print_expr(stmt.data.fun.paramd[i], depth + 1);
            }
            print_spec(stmt.data.fun.ret, depth + 1);
            print_stmt(*stmt.data.fun.body, depth + 1);
            break;
        case STRUCT_STMT:
            printf(" struct %s\n", stmt.data.structdef.name.str);
            for (size_t i = 0; i < stmt.data.structdef.paramc; i++) {
                print_indent(depth + 1);
                printf(
                    "member  :%zu:%zu %s\n", stmt.data.structdef.paramv[i].line,
                    stmt.data.structdef.paramv[i].col, stmt.data.structdef.paramv[i].str
                );
                print_spec(stmt.data.structdef.paramt[i], depth + 1);
                print_expr(stmt.data.structdef.paramd[i], depth + 1);
            }
            break;
        case ENUM_STMT:
            printf(" enum %s\n", stmt.data.enumdef.name.str);
            for (size_t i = 0; i < stmt.data.enumdef.len; i++) {
                print_indent(depth + 1);
                printf(
                    "value   :%zu:%zu %s\n", stmt.data.enumdef.items[i].line,
                    stmt.data.enumdef.items[i].col, stmt.data.enumdef.items[i].str
                );
            }
            break;
        case RETURN_STMT:
            printf(" return\n");
            print_expr(stmt.data.expr, depth + 1);
            break;
        case BREAK_STMT:    printf(" break\n"); break;
        case CONTINUE_STMT: printf(" continue\n"); break;
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
