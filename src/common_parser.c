#include "common_parser.h"

#include <stdlib.h>
#include <string.h>

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

// Checks the next token to see if it could be an expression.
// Preserves the it position.
bool is_expr(const Token* const* it) {
    switch ((*it)->type) {
        case INT_LITERAL:
        case CHR_LITERAL:
        case STR_LITERAL:
        case VAR_NAME:
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
        case TYPE_TOKEN:
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
    const Token** it, size_t* len_dst, size_t* opt_dst, Token** names_dst, TypeSpec** types_dst,
    Expr** defs_dst
) {
    // x: a, y = 1

    DynArr name_array = dynarr_create(sizeof(Token));
    DynArr type_array = dynarr_create(sizeof(TypeSpec));
    DynArr def_array = dynarr_create(sizeof(Expr));

    size_t optional = 0;
    if ((*it)->type == VAR_NAME) {
        for (;;) {
            // next parameter name
            Token name = **it;
            if (name.type != VAR_NAME) {
                unexpected_token(name);
                dynarr_destroy(&name_array);
                free_spec_dynarr(&type_array);
                free_expr_dynarr(&def_array);
                return true;
            }
            (*it)++;

            // optional parameter type specifier
            TypeSpec spec = { INFERRED_SPEC, name.line, name.col, {} };
            if ((*it)->type == COLON) {
                (*it)++;

                spec = parse_type_spec(it);
                if (spec.type == ERROR_SPEC) {
                    dynarr_destroy(&name_array);
                    free_spec_dynarr(&type_array);
                    free_expr_dynarr(&def_array);
                    return true;
                }
            }

            // optional default parameter
            Expr def = { NO_EXPR, name.line, name.col, {}, NULL };
            if ((*it)->type == EQ_TOKEN) {
                (*it)++;

                def = parse_expr(it, MAX_PRECEDENCE);
                if (def.type == ERROR_EXPR) {
                    free_spec(spec);
                    dynarr_destroy(&name_array);
                    free_spec_dynarr(&type_array);
                    free_expr_dynarr(&def_array);
                    return true;
                }

                optional++;
            } else if (optional) {
                error_line = name.line;
                error_col = name.col;
                syntax_error("non-optional parameter after optional parameter\n");
                free_spec(spec);
                free_expr(def);
                dynarr_destroy(&name_array);
                free_spec_dynarr(&type_array);
                free_expr_dynarr(&def_array);
                return true;
            }

            // push to arrays
            if (dynarr_append(&name_array, &name) || dynarr_append(&type_array, &spec) ||
                dynarr_append(&def_array, &def))
            {
                malloc_error();
                free_spec(spec);
                free_expr(def);
                free_expr(def);
                dynarr_destroy(&name_array);
                free_spec_dynarr(&type_array);
                free_expr_dynarr(&def_array);
                return true;
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
}

// Parse argument list without surrounding parentheses.
// Results are stored in dst parameters unless they are NULL.
// Returns whether an error occurred.
bool parse_args(const Token** it, size_t* len_dst, Expr** vals_dst) {
    // x, y, z

    DynArr array = dynarr_create(sizeof(Expr));
    if (is_expr(it)) {
        for (;;) {
            // next argument
            Expr item = parse_expr(it, MAX_PRECEDENCE);
            if (item.type == ERROR_EXPR) {
                free_expr_dynarr(&array);
                return true;
            }

            if (dynarr_append(&array, &item)) {
                malloc_error();
                free_expr(item);
                free_expr_dynarr(&array);
                return true;
            }

            // comma or end of list
            if ((*it)->type == COMMA) (*it)++;
            else break;
        }
    }

    if (len_dst) *len_dst = array.length;
    if (vals_dst) *vals_dst = array.c_arr;
    return false;
}

// Parse EOF_TOKEN terminated token array.
// Result is not tagged.
// Returns NULL if an error occurred.
AST* parse(const Token* program) {
    if (program == NULL) return NULL;

    const Token** it = &program;
    Stmt stmt = parse_block(it);
    if (stmt.type == ERROR_STMT) return NULL;

    if ((*it)->type != EOF_TOKEN) {
        unexpected_token(**it);
        free_stmt(stmt);
        return NULL;
    }

    Stmt* ast = malloc(sizeof(Stmt));
    if (ast == NULL) {
        malloc_error();
        free_stmt(stmt);
        return NULL;
    }
    memcpy(ast, &stmt, sizeof(Stmt));
    return ast;
}

// Free all data inside type specifier.
void free_spec(TypeSpec spec) {
    switch (spec.type) {
        case ERROR_SPEC:    break;
        case INFERRED_SPEC: break;

        case GROUPED_SPEC:
            free_spec(*spec.data.group);
            free(spec.data.group);
            break;
        case ATOMIC_SPEC: break;
        case ARR_SPEC:
        case PTR_SPEC:
            free_spec(*spec.data.ptr.spec);
            free(spec.data.ptr.spec);
            break;
        case FUN_SPEC:
            free_spec_arrn(spec.data.fun.paramt, spec.data.fun.paramc);
            free_spec(*spec.data.fun.ret);
            free(spec.data.fun.ret);
            break;
    }
}

// Free type specifier array of length n all all data inside it.
void free_spec_arrn(TypeSpec* arr, size_t n) {
    for (size_t i = 0; i < n; i++) free_spec(arr[i]);
    free(arr);
}

// Free dynamic type specifier array all data inside it.
void free_spec_dynarr(DynArr* arr) {
    for (size_t i = 0; i < arr->length; i++) free_spec(*(TypeSpec*)dynarr_get(arr, i));
    dynarr_destroy(arr);
}

// Free all data inside expression.
void free_expr(Expr expr) {
    switch (expr.type) {
        case ERROR_EXPR: break;
        case NO_EXPR:    break;

        case GROUPED_EXPR:
            free_expr(*expr.data.group);
            free(expr.data.group);
            break;
        case ATOMIC_EXPR: break;
        case ARR_EXPR:    free_expr_arrn(expr.data.arr.items, expr.data.arr.len); break;
        case LAMBDA_EXPR:
            free(expr.data.lambda.paramv);
            free_spec_arrn(expr.data.lambda.paramt, expr.data.lambda.paramc);
            free_expr_arrn(expr.data.lambda.paramd, expr.data.lambda.paramc);
            free_expr(*expr.data.lambda.expr);
            free(expr.data.lambda.expr);
            break;

        case UNOP_EXPR:
            free_expr(*expr.data.op.first);
            free(expr.data.op.first);
            break;
        case BINOP_EXPR:
            free_expr(*expr.data.op.first);
            free(expr.data.op.first);
            free_expr(*expr.data.op.second);
            free(expr.data.op.second);
            break;
        case TERNOP_EXPR:
            free_expr(*expr.data.op.first);
            free(expr.data.op.first);
            free_expr(*expr.data.op.second);
            free(expr.data.op.second);
            free_expr(*expr.data.op.third);
            free(expr.data.op.third);
            break;

        case SUBSRIPT_EXPR:
            free_expr(*expr.data.subscript.arr);
            free(expr.data.subscript.arr);
            free_expr(*expr.data.subscript.idx);
            free(expr.data.subscript.idx);
            break;
        case CALL_EXPR:
        case CONSTRUCTOR_EXPR:
            free_expr(*expr.data.call.fun);
            free(expr.data.call.fun);
            free_expr_arrn(expr.data.call.argv, expr.data.call.argc);
            break;
        case ACCESS_EXPR:
            free_expr(*expr.data.access.obj);
            free(expr.data.access.obj);
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

        case BLOCK:     free_stmt_arrn(stmt.data.block.stmts, stmt.data.block.len); break;
        case EXPR_STMT: free_expr(stmt.data.expr); break;
        case DECL:
            free_expr(stmt.data.decl.val);
            free_spec(stmt.data.decl.spec);
            break;
        case TYPEDEF: free_spec(stmt.data.type.val); break;

        case IFELSE_STMT:
            free_expr(stmt.data.ifelse.condition);
            free_stmt(*stmt.data.ifelse.on_true);
            free(stmt.data.ifelse.on_true);
            if (stmt.data.ifelse.on_false) {
                free_stmt(*stmt.data.ifelse.on_false);
                free(stmt.data.ifelse.on_false);
            }
            break;
        case SWITCH_STMT:
            free_expr(stmt.data.switchcase.expr);
            free_expr_arrn(stmt.data.switchcase.casev, stmt.data.switchcase.casec);
            free_stmt_arrn(stmt.data.switchcase.branchv, stmt.data.switchcase.casec);
            break;
        case WHILE_STMT:
        case DOWHILE_STMT:
            free_expr(stmt.data.whileloop.condition);
            free_stmt(*stmt.data.whileloop.body);
            free(stmt.data.whileloop.body);
            break;
        case FOR_STMT:
            free_stmt(*stmt.data.forloop.init);
            free(stmt.data.forloop.init);
            free_expr(stmt.data.forloop.condition);
            free_expr(stmt.data.forloop.expr);
            free_stmt(*stmt.data.forloop.body);
            free(stmt.data.forloop.body);
            break;

        case FUNCTION_STMT:
            free(stmt.data.fun.paramv);
            free_spec_arrn(stmt.data.fun.paramt, stmt.data.fun.paramc);
            free_expr_arrn(stmt.data.fun.paramd, stmt.data.fun.paramc);
            free_spec(stmt.data.fun.ret);
            free_stmt(*stmt.data.fun.body);
            free(stmt.data.fun.body);
            break;
        case STRUCT_STMT:
            free(stmt.data.structdef.paramv);
            free_spec_arrn(stmt.data.structdef.paramt, stmt.data.structdef.paramc);
            free_expr_arrn(stmt.data.structdef.paramd, stmt.data.structdef.paramc);
            break;
        case ENUM_STMT: free(stmt.data.enumdef.items); break;

        case RETURN_STMT:   free_expr(stmt.data.expr); break;
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
