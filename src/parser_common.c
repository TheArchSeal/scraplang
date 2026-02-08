#include "parser_common.h"

#include <stdlib.h>

#include "printerr.h"

// Write error message to stderr.
void unexpected_token(Token token) {
    error_line = token.line;
    error_col = token.col;
    switch (token.type) {
        case ERROR_TOKEN: syntax_error("unexpected error\n"); return;
        case EOF_TOKEN:   syntax_error("unexpected end of file\n"); return;
        case CHR_LITERAL:
        case STR_LITERAL: syntax_error("unexpected token %s\n", token.str); return;
        default:          syntax_error("unexpected token '%s'\n", token.str); return;
    }
}

bool consume_expected_token(const Token** it, TokenEnum type) {
    if ((*it)->type != type) {
        unexpected_token(**it);
        return true;
    }

    (*it)++;
    return false;
}

// Checks the next token to see if it could be an expression.
// Preserves the it position.
bool is_expr(const Token* const* it) {
    switch ((*it)->type) {
        case INT_LITERAL:
        case CHR_LITERAL:
        case STR_LITERAL:
        case IDENTIFIER:
        case PLUS:
        case DPLUS:
        case MINUS:
        case DMINUS:
        case TILDE:
        case EXCLMARK:
        case STAR:
        case AND:
        case LBRACKET:
        case LPAREN:      return true;

        default: return false;
    }
}

// Checks the next token to see if it could be a statement.
// Preserves the it position.
bool is_statement(const Token* const* it) {
    switch ((*it)->type) {
        case SEMICOLON:
        case VAR_TOKEN:
        case CONST_TOKEN:
        case TYPEDEF_TOKEN:
        case IF_TOKEN:
        case SWITCH_TOKEN:
        case WHILE_TOKEN:
        case DO_TOKEN:
        case FOR_TOKEN:
        case FN_TOKEN:
        case STRUCT_TOKEN:
        case ENUM_TOKEN:
        case RETURN_TOKEN:
        case BREAK_TOKEN:
        case CONTINUE_TOKEN: return true;

        default: return is_expr(it);
    }
}

// Looks ahead and checks whether the token after the
// matching closing parenthesis is a double arrow.
// Preserves the it position.
bool is_lambda(const Token* const* it) {
    const Token* i = *it;

    if (i->type != LPAREN) return false;
    i++;
    size_t level = 1;

    while (level > 0) {
        switch (i->type) {
            case EOF_TOKEN: return false;
            case LPAREN:    level++; break;
            case RPAREN:    level--; break;
            default:        break;
        }
        i++;
    }

    return i->type == DARROW;
}

// Parse parameter list without surrounding parentheses.
// Results are stored in dst parameters unless they are NULL.
// Returns whether an error occurred.
bool parse_params(
    const Token** it, size_t* len_dst, size_t* opt_dst, Token** names_dst, Spec** types_dst,
    Expr** defs_dst
) {
    // x: a, y = 1

    Spec spec;
    Expr def;

    DynArr name_array = dynarr_create(sizeof(Token));
    DynArr type_array = dynarr_create(sizeof(Spec));
    DynArr def_array = dynarr_create(sizeof(Expr));

    size_t optional = 0;
    if ((*it)->type == IDENTIFIER) {
        for (;;) {
            // next parameter name
            Token name = **it;
            if (consume_expected_token(it, IDENTIFIER)) goto err_free_arrs;

            // optional parameter type specifier
            spec = (Spec) { .type = INFERRED_SPEC, .line = name.line, .col = name.col };
            if ((*it)->type == COLON) {
                (*it)++;

                spec = parse_spec(it);
                if (spec.type == ERROR_SPEC) goto err_free_arrs;
            }

            // optional default parameter
            def = (Expr) { .type = NO_EXPR, .line = name.line, .col = name.col };
            if ((*it)->type == EQ_TOKEN) {
                (*it)++;

                def = parse_expr(it, MAX_PRECEDENCE);
                if (def.type == ERROR_EXPR) goto err_free_spec;

                optional++;
            } else if (optional) {
                error_line = name.line;
                error_col = name.col;
                syntax_error("non-optional parameter after optional parameter\n");
                goto err_free_spec;
            }

            // push to arrays
            if (dynarr_append(&name_array, &name) || dynarr_append(&type_array, &spec) ||
                dynarr_append(&def_array, &def))
            {
                goto err_free_def;
            }

            // comma or end of list
            if ((*it)->type == COMMA) (*it)++;
            else break;
        }
    }

    if (len_dst) *len_dst = name_array.length;
    if (opt_dst) *opt_dst = optional;
    if (names_dst) *names_dst = name_array.c_arr;
    if (types_dst) *types_dst = type_array.c_arr;
    if (defs_dst) *defs_dst = def_array.c_arr;

    return false;
err_free_def:
    free_expr(def);
err_free_spec:
    free_spec(spec);
err_free_arrs:
    dynarr_destroy(&name_array);
    free_spec_dynarr(&type_array);
    free_expr_dynarr(&def_array);
    return true;
}

// Parse argument list without surrounding parentheses.
// Results are stored in dst parameters unless they are NULL.
// Returns whether an error occurred.
bool parse_args(const Token** it, size_t* len_dst, Expr** vals_dst) {
    // x, y, z

    Expr item;

    DynArr array = dynarr_create(sizeof(Expr));
    if (is_expr(it)) {
        for (;;) {
            // next argument
            item = parse_expr(it, MAX_PRECEDENCE);
            if (item.type == ERROR_EXPR) goto err_free_arr;

            if (dynarr_append(&array, &item)) goto err_free_item;

            // comma or end of list
            if ((*it)->type == COMMA) (*it)++;
            else break;
        }
    }

    if (len_dst) *len_dst = array.length;
    if (vals_dst) *vals_dst = array.c_arr;

    return false;
err_free_item:
    free_expr(item);
err_free_arr:
    free_expr_dynarr(&array);
    return true;
}

// Parse EOF_TOKEN terminated token array.
// Result is not tagged.
// Returns NULL if an error occurred.
AST* parse(const Token* program) {
    if (program == NULL) goto err;

    const Token** it = &program;
    Stmt stmt = parse_block(it);
    if (stmt.type == ERROR_STMT) goto err;

    if (consume_expected_token(it, EOF_TOKEN)) goto err_free_stmt;

    Stmt* ast = MALLOC_STRUCT(stmt);
    if (ast == NULL) goto err_free_stmt;

    return ast;
err_free_stmt:
    free_stmt(stmt);
err:
    return NULL;
}

// Free all data inside type specifier.
void free_spec(Spec spec) {
    switch (spec.type) {
        case ERROR_SPEC:    break;
        case INFERRED_SPEC: break;

        case GROUPED_SPEC:
            free_spec(*spec.group);
            free(spec.group);
            break;
        case ATOMIC_SPEC: break;
        case ARR_SPEC:
        case PTR_SPEC:
            free_spec(*spec.ptr.spec);
            free(spec.ptr.spec);
            break;
        case FUN_SPEC:
            free_spec_arrn(spec.fun.paramt, spec.fun.paramc);
            free_spec(*spec.fun.ret);
            free(spec.fun.ret);
            break;
    }
}

// Free type specifier array of length n all all data inside it.
void free_spec_arrn(Spec* arr, size_t n) {
    for (size_t i = 0; i < n; i++) free_spec(arr[i]);
    free(arr);
}

// Free dynamic type specifier array all data inside it.
void free_spec_dynarr(DynArr* arr) {
    for (size_t i = 0; i < arr->length; i++) free_spec(*(Spec*)dynarr_get(arr, i));
    dynarr_destroy(arr);
}

// Free all data inside expression.
void free_expr(Expr expr) {
    switch (expr.type) {
        case ERROR_EXPR: break;
        case NO_EXPR:    break;

        case GROUPED_EXPR:
            free_expr(*expr.group);
            free(expr.group);
            break;
        case ATOMIC_EXPR: break;
        case ARR_EXPR:    free_expr_arrn(expr.arr.items, expr.arr.len); break;
        case LAMBDA_EXPR:
            free(expr.lambda.paramv);
            free_spec_arrn(expr.lambda.paramt, expr.lambda.paramc);
            free_expr_arrn(expr.lambda.paramd, expr.lambda.paramc);
            free_expr(*expr.lambda.expr);
            free(expr.lambda.expr);
            break;

        case UNOP_EXPR:
            free_expr(*expr.op.first);
            free(expr.op.first);
            break;
        case BINOP_EXPR:
            free_expr(*expr.op.first);
            free(expr.op.first);
            free_expr(*expr.op.second);
            free(expr.op.second);
            break;
        case TERNOP_EXPR:
            free_expr(*expr.op.first);
            free(expr.op.first);
            free_expr(*expr.op.second);
            free(expr.op.second);
            free_expr(*expr.op.third);
            free(expr.op.third);
            break;

        case SUBSRIPT_EXPR:
            free_expr(*expr.subscript.arr);
            free(expr.subscript.arr);
            free_expr(*expr.subscript.idx);
            free(expr.subscript.idx);
            break;
        case CALL_EXPR:
        case CONSTRUCTOR_EXPR:
            free_expr(*expr.call.fun);
            free(expr.call.fun);
            free_expr_arrn(expr.call.argv, expr.call.argc);
            break;
        case ACCESS_EXPR:
            free_expr(*expr.access.obj);
            free(expr.access.obj);
            break;
    }
}

// Free expression array of length n all all data inside it.
void free_expr_arrn(Expr* arr, size_t n) {
    for (size_t i = 0; i < n; i++) free_expr(arr[i]);
    free(arr);
}

// Free dynamic expression array all data inside it.
void free_expr_dynarr(DynArr* arr) {
    for (size_t i = 0; i < arr->length; i++) free_expr(*(Expr*)dynarr_get(arr, i));
    dynarr_destroy(arr);
}

// Free all data inside statement.
void free_stmt(Stmt stmt) {
    switch (stmt.type) {
        case ERROR_STMT: break;
        case NOP:        break;

        case BLOCK:     free_stmt_arrn(stmt.block.stmts, stmt.block.len); break;
        case EXPR_STMT: free_expr(stmt.expr); break;
        case DECL:
            free_expr(stmt.decl.val);
            free_spec(stmt.decl.spec);
            break;
        case TYPEDEF: free_spec(stmt.typedefdata.val); break;

        case IFELSE_STMT:
            free_expr(stmt.ifelse.condition);
            free_stmt(*stmt.ifelse.on_true);
            free(stmt.ifelse.on_true);
            if (stmt.ifelse.on_false) {
                free_stmt(*stmt.ifelse.on_false);
                free(stmt.ifelse.on_false);
            }
            break;
        case SWITCH_STMT:
            free_expr(stmt.switchcase.expr);
            free_expr_arrn(stmt.switchcase.casev, stmt.switchcase.casec);
            free_stmt_arrn(stmt.switchcase.branchv, stmt.switchcase.casec);
            break;
        case WHILE_STMT:
        case DOWHILE_STMT:
            free_expr(stmt.whileloop.condition);
            free_stmt(*stmt.whileloop.body);
            free(stmt.whileloop.body);
            break;
        case FOR_STMT:
            free_stmt(*stmt.forloop.init);
            free(stmt.forloop.init);
            free_expr(stmt.forloop.condition);
            free_expr(stmt.forloop.expr);
            free_stmt(*stmt.forloop.body);
            free(stmt.forloop.body);
            break;

        case FUNCTION_STMT:
            free(stmt.fun.paramv);
            free_spec_arrn(stmt.fun.paramt, stmt.fun.paramc);
            free_expr_arrn(stmt.fun.paramd, stmt.fun.paramc);
            free_spec(stmt.fun.ret);
            free_stmt(*stmt.fun.body);
            free(stmt.fun.body);
            break;
        case STRUCT_STMT:
            free(stmt.structdef.paramv);
            free_spec_arrn(stmt.structdef.paramt, stmt.structdef.paramc);
            free_expr_arrn(stmt.structdef.paramd, stmt.structdef.paramc);
            break;
        case ENUM_STMT: free(stmt.enumdef.items); break;

        case RETURN_STMT:   free_expr(stmt.expr); break;
        case BREAK_STMT:    break;
        case CONTINUE_STMT: break;
    }
}

// Free statement array of length n all all data inside it.
void free_stmt_arrn(Stmt* arr, size_t n) {
    for (size_t i = 0; i < n; i++) free_stmt(arr[i]);
    free(arr);
}

// Free dynamic statement array all all data inside it.
void free_stmt_dynarr(DynArr* arr) {
    for (size_t i = 0; i < arr->length; i++) free_stmt(*(Stmt*)dynarr_get(arr, i));
    dynarr_destroy(arr);
}

// Free non-tagged abstract syntax tree token and all data inside it.
void free_ast_p(AST* ast) {
    if (ast == NULL) return;
    free_stmt(*ast);
    free(ast);
}
