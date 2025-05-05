#include "parser.h"
#include "printerr.h"
#include <stdlib.h>
#include <string.h>

TypeSpec parse_type_spec_mod(const Token** it, TypeSpec base);

Expr parse_postfix(const Token** it, Expr term);
Expr parse_term(const Token** it);
Expr parse_expr(const Token** it);

int print_unexpected_token(Token token) {
    ErrorData err = { token.line, token.col };
    switch (token.type) {
        case ERROR_TOKEN: return print_syntax_error(err,
            "unexpected error\n");
        case EOF_TOKEN: return print_syntax_error(err,
            "unexpected end of file\n");
        case CHR_LITERAL:
        case STR_LITERAL: return print_syntax_error(err,
            "unexpected token %s", token.str);
        default: return print_syntax_error(err,
            "unexpected token '%s'", token.str);
    }
}

TypeSpec parse_fun_spec(const Token** it) {
    TypeSpec err = { ERROR_SPEC };

    Token token = *(*it)++;
    if (token.type != LPAREN) {
        print_unexpected_token(token);
        return err;
    }

    size_t length = 0, capacity = 1;
    TypeSpec* array = malloc(sizeof(TypeSpec) * capacity);

    size_t optional = 0;
    while ((*it)->type != RPAREN) {
        // expand array when out of capacity
        if (length >= capacity) {
            capacity *= 2;
            TypeSpec* new_array = realloc(array, sizeof(TypeSpec) * capacity);
            if (new_array == NULL) {
                print_malloc_error();
                free_spec_arrn(array, length);
                return err;
            }
            array = new_array;
        }

        TypeSpec item = parse_type_spec(it);
        if (item.type == ERROR_SPEC) {
            free_spec_arrn(array, length);
            return item;
        }
        memcpy(&array[length++], &item, sizeof(TypeSpec));

        if ((*it)->type == QMARK) {
            (*it)++;
            optional++;
        } else if (optional) {
            print_syntax_error((ErrorData) { token.line, token.col },
                "non-optional parameter after optional parameter"
            );
            free_spec_arrn(array, length);
            return err;
        }

        if ((*it)->type == COMMA) (*it)++;
        else if ((*it)->type == RPAREN) break;
        else {
            print_unexpected_token(**it);
            free_spec_arrn(array, length);
            return err;
        }
    }
    (*it)++;

    if ((*it)->type != DARROW) {
        print_unexpected_token(**it);
        free_spec_arrn(array, length);
        return err;
    }
    (*it)++;

    TypeSpec item = parse_type_spec(it);
    if (item.type == ERROR_SPEC) {
        free_spec_arrn(array, length);
        return item;
    }

    TypeSpec* ret = malloc(sizeof(TypeSpec));
    if (ret == NULL) {
        print_malloc_error();
        free_spec_arrn(array, length);
        free_spec(item);
        return err;
    }
    memcpy(ret, &item, sizeof(TypeSpec));

    TypeSpec spec;
    spec.type = FUN_SPEC;
    spec.token = token;
    spec.data.fun.paramc = length;
    spec.data.fun.optc = optional;
    spec.data.fun.paramt = array;
    spec.data.fun.ret = ret;
    return spec;
}

TypeSpec handle_type_spec_mod(TypeSpecEnum type, bool mut, const Token** it, TypeSpec base) {
    Token token = *(*it)++;

    TypeSpec* inner = malloc(sizeof(TypeSpec));
    if (inner == NULL) {
        print_malloc_error();
        return (TypeSpec) { ERROR_SPEC };
    }
    memcpy(inner, &base, sizeof(TypeSpec));

    TypeSpec spec;
    spec.type = type;
    spec.token = token;
    spec.data.ptr.item_type = inner;
    spec.data.ptr.mutable = mut;

    TypeSpec next = parse_type_spec_mod(it, spec);
    if (next.type == ERROR_SPEC) {
        free(inner);
        return next;
    }
    return next;
}

TypeSpec parse_type_spec_mod(const Token** it, TypeSpec base) {
    switch ((*it)->type) {
        case LBRACKET: return handle_type_spec_mod(ARR_SPEC, true, it, base);
        case STAR: return handle_type_spec_mod(PTR_SPEC, true, it, base);
        case CONST_TOKEN:
            (*it)++;
            switch ((*it)->type) {
                case LBRACKET: return handle_type_spec_mod(ARR_SPEC, false, it, base);
                case STAR: return handle_type_spec_mod(PTR_SPEC, false, it, base);
                default:
                    print_unexpected_token(**it);
                    return (TypeSpec) { ERROR_SPEC };
            }
        default: return base;
    }
}

TypeSpec parse_type_spec(const Token** it) {
    switch ((*it)->type) {
        case VOID_TOKEN:
        case BOOL_TOKEN:
        case I8_TOKEN:
        case I16_TOKEN:
        case I32_TOKEN:
        case I64_TOKEN:
        case U8_TOKEN:
        case U16_TOKEN:
        case U32_TOKEN:
        case U64_TOKEN:
        case VAR_NAME:
            Token token = *(*it)++;
            TypeSpec spec;
            spec.type = ATOMIC_SPEC;
            spec.token = token;
            return parse_type_spec_mod(it, spec);

        case LPAREN:
            const Token* branch = *it;
            error_supress++;
            TypeSpec spec = parse_fun_spec(it);
            error_supress--;
            if (spec.type == ERROR_SPEC) {
                *it = branch;

                (*it)++;
                spec = parse_type_spec(it);
                if (spec.type == ERROR_SPEC) return spec;

                if ((*it)->type != RPAREN) {
                    print_unexpected_token(**it);
                    free_spec(spec);
                    return (TypeSpec) { ERROR_SPEC };
                }
                (*it)++;
            }
            return parse_type_spec_mod(it, spec);

        default:
            print_unexpected_token(token);
            return (TypeSpec) { ERROR_SPEC };
    }
}

Expr parse_array_literal(const Token** it) {
    Token token = *(*it)++;

    size_t length = 0, capacity = 1;
    Expr* array = malloc(sizeof(Expr) * capacity);

    while ((*it)->type != RBRACKET) {
        // expand array when out of capacity
        if (length >= capacity) {
            capacity *= 2;
            Expr* new_array = realloc(array, sizeof(Expr) * capacity);
            if (new_array == NULL) {
                print_malloc_error();
                free_expr_arrn(array, length);
                return (Expr) { ERROR_EXPR };
            }
            array = new_array;
        }

        Expr item = parse_expr(it);
        if (item.type == ERROR_EXPR) {
            free_expr_arrn(array, length);
            return item;
        }
        memcpy(&array[length++], &item, sizeof(Expr));

        if ((*it)->type == COMMA) (*it)++;
        else if ((*it)->type == RBRACKET) break;
        else {
            print_unexpected_token(**it);
            free_expr_arrn(array, length);
            return (Expr) { ERROR_EXPR };
        }
    }
    (*it)++;

    Expr expr;
    expr.type = ARR_EXPR;
    expr.token = token;
    expr.data.arr.len = length;
    expr.data.arr.items = array;
    return expr;
}

Expr parse_lambda(const Token** it) {
    Expr err = { ERROR_EXPR };

    Token token = *(*it)++;
    if (token.type != LPAREN) {
        print_unexpected_token(token);
        return err;
    }

    size_t length = 0, capacity = 1;
    Token* name_array = malloc(sizeof(Expr) * capacity);
    TypeSpec* spec_array = malloc(sizeof(TypeSpec) * capacity);

    while ((*it)->type != RPAREN) {
        // expand arrays when out of capacity
        if (length >= capacity) {
            capacity *= 2;
            Token* new_name_array = realloc(name_array, sizeof(Token) * capacity);
            TypeSpec* new_spec_array = realloc(name_array, sizeof(TypeSpec) * capacity);
            if (new_name_array == NULL || new_spec_array == NULL) {
                print_malloc_error();
                free_token_arrn(name_array, length);
                free_spec_arrn(spec_array, length);
                return err;
            }
            name_array = new_name_array;
            spec_array = new_spec_array;
        }

        Token name = **it;
        if (name.type != VAR_NAME) {
            print_unexpected_token(**it);
            free_token_arrn(name_array, length);
            free_spec_arrn(spec_array, length);
            return err;
        }
        (*it)++;

        TypeSpec spec = { INFERRED_SPEC };
        if ((*it)->type == COLON) {
            (*it)++;

            spec = parse_type_spec(it);
            if (spec.type == ERROR_SPEC) {
                free_token_arrn(name_array, length);
                free_spec_arrn(spec_array, length);
                return err;
            }
        }

        memcpy(&name_array[length], &name, sizeof(Token));
        memcpy(&spec_array[length], &spec, sizeof(TypeSpec));
        length++;

        if ((*it)->type == COMMA) (*it)++;
        else if ((*it)->type == RPAREN) break;
        else {
            print_unexpected_token(**it);
            free_token_arrn(name_array, length);
            free_spec_arrn(spec_array, length);
            return err;
        }
    }
    (*it)++;

    TypeSpec spec = { INFERRED_SPEC };
    if ((*it)->type == COLON) {
        (*it)++;

        spec = parse_type_spec(it);
        if (spec.type == ERROR_SPEC) {
            free_token_arrn(name_array, length);
            free_spec_arrn(spec_array, length);
            return err;
        }
    }

    TypeSpec* ret = malloc(sizeof(TypeSpec));
    if (ret == NULL) {
        print_malloc_error();
        free_token_arrn(name_array, length);
        free_spec_arrn(spec_array, length);
        free_spec(spec);
        return err;
    }

    if ((*it)->type != DARROW) {
        print_unexpected_token(**it);
        free_token_arrn(name_array, length);
        free_spec_arrn(spec_array, length);
        free_spec(*ret);
        free(ret);
        return err;
    }
    (*it)++;

    Expr item = parse_expr(it);
    if (item.type == ERROR_EXPR) {
        free_token_arrn(name_array, length);
        free_spec_arrn(spec_array, length);
        free_spec(*ret);
        free(ret);
        return item;
    }

    Expr* body = malloc(sizeof(Expr));
    if (body == NULL) {
        print_malloc_error();
        free_token_arrn(name_array, length);
        free_spec_arrn(spec_array, length);
        free_spec(*ret);
        free(ret);
        free_expr(item);
        return err;
    }
    memcpy(body, &item, sizeof(Expr));

    Expr expr;
    expr.type = LAMBDA_EXPR;
    expr.token = token;
    expr.data.lambda.paramc = length;
    expr.data.lambda.paramv = name_array;
    expr.data.lambda.paramt = spec_array;
    expr.data.lambda.expr = body;
    expr.data.lambda.ret = ret;
    return expr;
}

Expr handle_unary_postfix(OpEnum type, const Token** it, Expr term) {
    Token token = *(*it)++;

    Expr* inner = malloc(sizeof(Expr));
    if (inner == NULL) {
        print_malloc_error();
        return (Expr) { ERROR_EXPR };
    }
    memcpy(inner, &term, sizeof(Expr));

    Expr expr;
    expr.type = UNOP_EXPR;
    expr.token = token;
    expr.data.op.type = type;
    expr.data.op.first = inner;

    Expr next = parse_postfix(it, expr);
    if (next.type == ERROR_EXPR) {
        free(inner);
        return next;
    }
    return next;
}

Expr handle_unary_prefix(OpEnum type, const Token** it) {
    Token token = *(*it)++;

    Expr term = parse_term(it);
    if (term.type == ERROR_EXPR) {
        return term;
    }

    Expr* inner = malloc(sizeof(Expr));
    if (inner == NULL) {
        print_malloc_error();
        free_expr(term);
        return (Expr) { ERROR_EXPR };
    }
    memcpy(inner, &term, sizeof(Expr));

    Expr expr;
    expr.type = UNOP_EXPR;
    expr.token = token;
    expr.data.op.type = type;
    expr.data.op.first = inner;
    return expr;
}

Expr handle_atomic_term(ExprEnum type, const Token** it) {
    Token token = *(*it)++;

    Expr expr;
    expr.type = type;
    expr.token = token;

    return parse_postfix(it, expr);
}

Expr parse_postfix(const Token** it, Expr term) {
    switch ((*it)->type) {
        case DPLUS: return handle_unary_postfix(POSTFIX_INC, it, term);
        case DMINUS: return handle_unary_postfix(POSTFIX_DEC, it, term);

            // TODO
        case LPAREN:
        case LBRACKET:
        case LBRACE:

        case DOT:

        default: return term;
    }
}

Expr parse_term(const Token** it) {
    switch ((*it)->type) {
        case INT_LITERAL:
        case CHR_LITERAL:
        case STR_LITERAL: return handle_atomic_term(LITERAL_EXPR, it);
        case VAR_NAME: return handle_atomic_term(VAR_EXPR, it);

        case PLUS: return handle_unary_prefix(UNARY_PLUS, it);
        case DPLUS: return handle_unary_prefix(POSTFIX_INC, it);
        case MINUS: return handle_unary_prefix(UNARY_PLUS, it);
        case DMINUS: return handle_unary_prefix(POSTFIX_DEC, it);
        case TILDE: return handle_unary_prefix(BINARY_NOT, it);
        case EXCLMARK: return handle_unary_prefix(LOGICAL_NOT, it);
        case STAR: return handle_unary_prefix(DEREFERENCE, it);
        case AND: return handle_unary_prefix(ADDRESS_OF, it);

        case LBRACKET: return parse_array_literal(it);
        case LPAREN:
            const Token* branch = *it;
            error_supress++;
            Expr expr = parse_lambda(it);
            error_supress--;
            if (expr.type == ERROR_EXPR) {
                *it = branch;

                (*it)++;
                expr = parse_expr(it);
                if (expr.type == ERROR_EXPR) return expr;

                if ((*it)->type != RPAREN) {
                    print_unexpected_token(**it);
                    free_expr(expr);
                    return (Expr) { ERROR_EXPR };
                }
                (*it)++;
            }
            return parse_postfix(it, expr);

        default: return (Expr) { ERROR_EXPR };
    }
}

Expr parse_expr(const Token** it) {
    // TODO
}

AST* parse(const Token* program) {
    // TODO
}

void free_spec(TypeSpec spec) {
    switch (spec.type) {
        case ERROR_SPEC: break;
        case INFERRED_SPEC: break;
        case ATOMIC_SPEC: break;

        case ARR_SPEC:
        case PTR_SPEC:
            free_spec(*spec.data.ptr.item_type);
            free(spec.data.ptr.item_type);
            break;
        case FUN_SPEC:
            free_spec_arrn(spec.data.fun.paramt, spec.data.fun.paramc);
            free_spec(*spec.data.fun.ret);
            free(spec.data.fun.ret);
            break;
    }
}

void free_spec_arrn(TypeSpec* arr, size_t n) {
    for (size_t i = 0; i < n; i++) free_spec(arr[i]);
    free(arr);
}

void free_expr(Expr expr) {
    switch (expr.type) {
        case ERROR_EXPR: break;
        case LITERAL_EXPR: break;
        case VAR_EXPR: break;

        case ARR_EXPR:
            free_expr_arrn(expr.data.arr.items, expr.data.arr.len);
            break;
        case LAMBDA_EXPR: break;
            free_token_arrn(expr.data.lambda.paramv, expr.data.lambda.paramc);
            free_spec_arrn(expr.data.lambda.paramt, expr.data.lambda.paramc);
            free_expr(*expr.data.lambda.expr);
            free(expr.data.lambda.expr);
            free_spec(*expr.data.lambda.ret);
            free(expr.data.lambda.ret);
            break;

        case UNOP_EXPR: break;
            free_expr(*expr.data.op.first);
            free(expr.data.op.first);
            break;

        case BINOP_EXPR: break;
            free_expr(*expr.data.op.first);
            free(expr.data.op.first);
            free_expr(*expr.data.op.second);
            free(expr.data.op.second);
            break;

        case TERNOP_EXPR: break;
            free_expr(*expr.data.op.first);
            free(expr.data.op.first);
            free_expr(*expr.data.op.second);
            free(expr.data.op.second);
            free_expr(*expr.data.op.third);
            free(expr.data.op.third);
            break;
    }
}

void free_expr_arrn(Expr* arr, size_t n) {
    for (size_t i = 0; i < n; i++) free_expr(arr[i]);
    free(arr);
}

void free_stmt(Stmt stmt) {
    // TODO
}

void free_ast(AST* ast) {
    // TODO
}
