#include "parser.h"
#include "printerr.h"
#include <stdlib.h>
#include <string.h>

// initial precedence for parse_expr
#define MAX_PRECEDENCE 12

TypeSpec parse_type_spec_mod(const Token** it, TypeSpec base);
TypeSpec parse_type_spec(const Token** it);

Expr parse_postfix(const Token** it, Expr term);
Expr parse_term(const Token** it);
Expr parse_expr(const Token** it, size_t precedence);

Stmt parse_stmt(const Token** it);

void free_spec(TypeSpec spec);
void free_spec_arrn(TypeSpec* arr, size_t n);
void free_expr(Expr expr);
void free_expr_arrn(Expr* arr, size_t n);
void free_stmt(Stmt stmt);
void free_stmt_arrn(Stmt* arr, size_t n);

// Write error message to stderr.
void unexpected_token(Token token) {
    error_line = token.line;
    error_col = token.col;
    switch (token.type) {
        case ERROR_TOKEN: syntax_error("unexpected error\n"); return;
        case EOF_TOKEN: syntax_error("unexpected end of file\n"); return;
        case CHR_LITERAL:
        case STR_LITERAL: syntax_error("unexpected token %s\n", token.str); return;
        default: syntax_error("unexpected token '%s'\n", token.str); return;
    }
}

// Find the binary or ternary operation based on token.
OpEnum infix_op_from_token(Token token) {
    switch (token.type) {
        case STAR: return MULTIPLICATION;
        case SLASH: return DIVISION;
        case PERCENT: return MODULO;
        case PLUS: return ADDITION;
        case MINUS: return SUBTRACTION;
        case DLT_TOKEN: return LEFT_SHIFT;
        case DGT_TOKEN: return RIGHT_SHIFT;

        case AND: return BITWISE_AND;
        case CARET: return BITWISE_XOR;
        case PIPE: return BITWISE_OR;

        case LT_TOKEN: return LESS_THAN;
        case LEQ_TOKEN: return LESS_OR_EQUAL;
        case GT_TOKEN: return GREATER_THAN;
        case GEQ_TOKEN: return GREATER_OR_EQUAL;
        case DEQ_TOKEN: return EQUAL;
        case NEQ_TOKEN: return NOT_EQUAL;

        case DAND: return LOGICAL_AND;
        case DPIPE: return LOGICAL_OR;

        case QMARK: return TERNARY;

        case EQ_TOKEN: return ASSIGNMENT;

        default: return ERROR_OP;
    }
}

// Get the operator precedence of op.
size_t operator_precedence(OpEnum op) {
    switch (op) {
        case ERROR_OP: return 0;

        case POSTFIX_INC:
        case POSTFIX_DEC:
        case PREFIX_INC:
        case PREFIX_DEC:
        case UNARY_PLUS:
        case UNARY_MINUS:
        case LOGICAL_NOT:
        case BINARY_NOT:
        case DEREFERENCE:
        case ADDRESS_OF: return 0;

        case MULTIPLICATION:
        case DIVISION:
        case MODULO: return 1;
        case ADDITION:
        case SUBTRACTION: return 2;
        case LEFT_SHIFT:
        case RIGHT_SHIFT: return 3;

        case BITWISE_AND: return 4;
        case BITWISE_XOR: return 5;
        case BITWISE_OR: return 6;

        case LESS_THAN:
        case LESS_OR_EQUAL:
        case GREATER_THAN:
        case GREATER_OR_EQUAL: return 7;
        case EQUAL:
        case NOT_EQUAL: return 8;

        case LOGICAL_AND: return 9;
        case LOGICAL_OR: return 10;

        case TERNARY: return 11;

        case ASSIGNMENT: return 12;
    }

    return 0;
}

// Whether precedence is right-to-left associative.
bool operator_rtl_associative(size_t precedence) {
    return precedence == 11
        || precedence == 12;
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
            case LPAREN: level++; break;
            case RPAREN: level--; break;
            default: break;
        }
        i++;
    }

    return i->type == DARROW;
}

// Checks the next token to see if it could be a statement.
// Preserves the it position.
bool is_statement(const Token* const* it) {
    switch ((*it)->type) {
        case SEMICOLON: // stmt
        case VAR_TOKEN:
        case CONST_TOKEN:
        case TYPE_TOKEN:
        case IF_TOKEN:
        case SWITCH_TOKEN:
        case WHILE_TOKEN:
        case DO_TOKEN:
        case FOR_TOKEN:
        case FN_TOKEN:

        case INT_LITERAL: // expr
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
        case LPAREN: return true;

        default: return false;
    }
}

// Parse parameter list and return type.
// Parameter start is the first token in the statement and is only used for error messages.
// Results are stored in dst parameters unless they are NULL.
// Returns whether an error occurred.
bool parse_params(const Token** it, Token start,
    size_t* len_dst, size_t* opt_dst,
    Token** names_dst, TypeSpec** types_dst, Expr** defs_dst
) {
    // (x: a, y = 1)

    // opening parenthesis
    if ((*it)->type != LPAREN) {
        unexpected_token(start);
        return true;
    }
    (*it)++;

    // initialize arrays
    size_t length = 0, capacity = 1;
    Token* name_array = malloc(sizeof(Token) * capacity);
    TypeSpec* type_array = malloc(sizeof(TypeSpec) * capacity);
    Expr* def_array = malloc(sizeof(Expr) * capacity);
    if (name_array == NULL || type_array == NULL || def_array == NULL) {
        malloc_error();
        free(name_array);
        free(type_array);
        free(def_array);
        return true;
    }

    size_t optional = 0;
    if ((*it)->type != RPAREN) for (;;) {
        // expand arrays when out of capacity
        if (length >= capacity) {
            capacity *= 2;
            Token* new_name_array = realloc(name_array, sizeof(Token) * capacity);
            if (new_name_array == NULL) {
                malloc_error();
                free(name_array);
                free_spec_arrn(type_array, length);
                free_expr_arrn(def_array, length);
                return true;
            }
            name_array = new_name_array;
            TypeSpec* new_spec_array = realloc(type_array, sizeof(TypeSpec) * capacity);
            if (new_spec_array == NULL) {
                malloc_error();
                free(name_array);
                free_spec_arrn(type_array, length);
                free_expr_arrn(def_array, length);
                return true;
            }
            type_array = new_spec_array;
            Expr* new_def_array = realloc(def_array, sizeof(Expr) * capacity);
            if (new_def_array == NULL) {
                malloc_error();
                free(name_array);
                free_spec_arrn(type_array, length);
                free_expr_arrn(def_array, length);
                return true;
            }
            def_array = new_def_array;
        }

        // next parameter name
        Token name = **it;
        if (name.type != VAR_NAME) {
            unexpected_token(**it);
            free(name_array);
            free_spec_arrn(type_array, length);
            free_expr_arrn(def_array, length);
            return true;
        }
        (*it)++;

        // optional parameter type specifier
        TypeSpec spec = { INFERRED_SPEC, name.line, name.col, {} };
        if ((*it)->type == COLON) {
            (*it)++;

            spec = parse_type_spec(it);
            if (spec.type == ERROR_SPEC) {
                free(name_array);
                free_spec_arrn(type_array, length);
                free_expr_arrn(def_array, length);
                return true;
            }
        }

        // optional default parameter
        Expr def = { NO_EXPR, name.line, name.col, {} };
        if ((*it)->type == EQ_TOKEN) {
            (*it)++;

            def = parse_expr(it, MAX_PRECEDENCE);
            if (def.type == ERROR_EXPR) {
                free(name_array);
                free_spec_arrn(type_array, length);
                free_expr_arrn(def_array, length);
                return true;
            }

            optional++;
        } else if (optional) {
            error_line = start.line;
            error_col = start.col;
            syntax_error("non-optional parameter after optional parameter\n");
            free(name_array);
            free_spec_arrn(type_array, length);
            free_expr_arrn(def_array, length);
            return true;
        }

        // push to arrays
        memcpy(&name_array[length], &name, sizeof(Token));
        memcpy(&type_array[length], &spec, sizeof(TypeSpec));
        memcpy(&def_array[length], &def, sizeof(TypeSpec));
        length++;

        // comma or closing parenthesis
        if ((*it)->type == COMMA) (*it)++;
        else if ((*it)->type == RPAREN) break;
        else {
            unexpected_token(**it);
            free(name_array);
            free_spec_arrn(type_array, length);
            free_expr_arrn(def_array, length);
            return true;
        }
    }
    (*it)++;

    if (len_dst) *len_dst = length;
    if (opt_dst) *opt_dst = optional;
    if (names_dst) *names_dst = name_array;
    if (types_dst) *types_dst = type_array;
    if (defs_dst) *defs_dst = def_array;
    return false;
}

TypeSpec parse_type_spec_group(const Token** it) {
    Token start = **it;
    if (start.type != LPAREN) return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
    (*it)++;

    TypeSpec group = parse_type_spec(it);
    if (group.type == ERROR_SPEC) return group;

    if ((*it)->type != RPAREN) {
        unexpected_token(**it);
        free_spec(group);
        return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
    }
    (*it)++;

    TypeSpec spec;
    spec.type = GROUPED_SPEC;
    spec.line = start.line;
    spec.col = start.col;

    spec.data.group = malloc(sizeof(TypeSpec));
    if (spec.data.group == NULL) {
        malloc_error();
        free_spec(group);
        return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
    }
    memcpy(spec.data.group, &group, sizeof(TypeSpec));

    return spec;
}

TypeSpec parse_fun_spec(const Token** it) {
    // (a, b?) => c

    // opening parenthesis
    Token start = **it;
    if (start.type != LPAREN) {
        unexpected_token(start);
        return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
    }
    (*it)++;

    // initialize array
    size_t length = 0, capacity = 1;
    TypeSpec* array = malloc(sizeof(TypeSpec) * capacity);
    if (array == NULL) {
        malloc_error();
        return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
    }

    // number of optional parameters
    size_t optional = 0;
    if ((*it)->type != RPAREN) for (;;) {
        // expand array when out of capacity
        if (length >= capacity) {
            capacity *= 2;
            TypeSpec* new_array = realloc(array, sizeof(TypeSpec) * capacity);
            if (new_array == NULL) {
                malloc_error();
                free_spec_arrn(array, length);
                return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
            }
            array = new_array;
        }

        // next parameter type
        TypeSpec item = parse_type_spec(it);
        if (item.type == ERROR_SPEC) {
            free_spec_arrn(array, length);
            return item;
        }
        memcpy(&array[length++], &item, sizeof(TypeSpec));

        // optionally an optional parameter
        if ((*it)->type == QMARK) {
            (*it)++;
            optional++;
        } else if (optional) {
            error_line = start.line;
            error_col = start.col;
            syntax_error("non-optional parameter after optional parameter\n");
            free_spec_arrn(array, length);
            return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
        }

        // comma or closing parenthesis
        if ((*it)->type == COMMA) (*it)++;
        else if ((*it)->type == RPAREN) break;
        else {
            unexpected_token(**it);
            free_spec_arrn(array, length);
            return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
        }
    }
    (*it)++;

    // => token
    if ((*it)->type != DARROW) {
        unexpected_token(**it);
        free_spec_arrn(array, length);
        return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
    }
    (*it)++;

    // return type
    TypeSpec ret = parse_type_spec(it);
    if (ret.type == ERROR_SPEC) {
        free_spec_arrn(array, length);
        return ret;
    }

    TypeSpec spec;
    spec.type = FUN_SPEC;
    spec.line = start.line;
    spec.col = start.col;
    spec.data.fun.paramc = length;
    spec.data.fun.optc = optional;
    spec.data.fun.paramt = array;

    spec.data.fun.ret = malloc(sizeof(TypeSpec));
    if (spec.data.fun.ret == NULL) {
        malloc_error();
        free_spec_arrn(array, length);
        free_spec(ret);
        return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
    }
    memcpy(spec.data.fun.ret, &ret, sizeof(TypeSpec));

    return spec;
}

TypeSpec handle_type_spec_mod(TypeSpecEnum type, bool mut, const Token** it, TypeSpec base) {
    Token start = *(*it)++;

    if (start.type == LBRACKET) {
        if ((*it)->type != RBRACKET) return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
        (*it)++;
    }

    TypeSpec spec;
    spec.type = type;
    spec.line = base.line;
    spec.col = base.col;
    spec.data.ptr.mutable = mut;

    spec.data.ptr.spec = malloc(sizeof(TypeSpec));
    if (spec.data.ptr.spec == NULL) {
        malloc_error();
        return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
    }
    memcpy(spec.data.ptr.spec, &base, sizeof(TypeSpec));

    // may have another modification
    TypeSpec next = parse_type_spec_mod(it, spec);
    if (next.type == ERROR_SPEC) {
        free_spec(*spec.data.ptr.spec);
        free(spec.data.ptr.spec);
        return next;
    }
    return next;
}

TypeSpec parse_type_spec_mod(const Token** it, TypeSpec base) {
    switch ((*it)->type) {
        // regular modifier
        case LBRACKET: return handle_type_spec_mod(ARR_SPEC, true, it, base);
        case STAR: return handle_type_spec_mod(PTR_SPEC, true, it, base);

        case CONST_TOKEN: // const modifier
            (*it)++;
            switch ((*it)->type) {
                case LBRACKET: return handle_type_spec_mod(ARR_SPEC, false, it, base);
                case STAR: return handle_type_spec_mod(PTR_SPEC, false, it, base);
                default:
                    unexpected_token(**it);
                    return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
            }

        default: return base; // no modifier
    }
}

TypeSpec parse_type_spec(const Token** it) {
    switch ((*it)->type) {
        TypeSpec spec;

        // atomic types
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
            spec.type = ATOMIC_SPEC;
            spec.line = token.line;
            spec.col = token.col;
            spec.data.atom = token;
            return parse_type_spec_mod(it, spec);

        case LPAREN:
            // check if function type specifier
            spec = is_lambda(it)
                ? parse_fun_spec(it)
                : parse_type_spec_group(it);
            TypeSpec next = parse_type_spec_mod(it, spec);
            if (next.type == ERROR_SPEC) {
                free_spec(spec);
                return next;
            }
            return next;

        default:
            unexpected_token(**it);
            return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
    }
}

Expr parse_expr_group(const Token** it) {
    Token start = **it;
    if (start.type != LPAREN) return (Expr) { ERROR_EXPR, 0, 0, {} };
    (*it)++;

    Expr group = parse_expr(it, MAX_PRECEDENCE);
    if (group.type == ERROR_EXPR) return group;

    if ((*it)->type != RPAREN) {
        unexpected_token(**it);
        free_expr(group);
        return (Expr) { ERROR_EXPR, 0, 0, {} };
    }
    (*it)++;

    Expr expr;
    expr.type = GROUPED_EXPR;
    expr.line = start.line;
    expr.col = start.col;

    expr.data.group = malloc(sizeof(Expr));
    if (expr.data.group == NULL) {
        malloc_error();
        free_expr(group);
        return (Expr) { ERROR_EXPR, 0, 0, {} };
    }
    memcpy(expr.data.group, &group, sizeof(Expr));

    return expr;
}

Expr parse_array_literal(const Token** it) {
    // [x, y, z]

    Token start = *(*it)++;

    // initialize array
    size_t length = 0, capacity = 1;
    Expr* array = malloc(sizeof(Expr) * capacity);
    if (array == NULL) {
        malloc_error();
        return (Expr) { ERROR_EXPR, 0, 0, {} };
    }

    if ((*it)->type != RBRACKET) for (;;) {
        // expand array when out of capacity
        if (length >= capacity) {
            capacity *= 2;
            Expr* new_array = realloc(array, sizeof(Expr) * capacity);
            if (new_array == NULL) {
                malloc_error();
                free_expr_arrn(array, length);
                return (Expr) { ERROR_EXPR, 0, 0, {} };
            }
            array = new_array;
        }

        // element in array
        Expr item = parse_expr(it, MAX_PRECEDENCE);
        if (item.type == ERROR_EXPR) {
            free_expr_arrn(array, length);
            return item;
        }
        memcpy(&array[length++], &item, sizeof(Expr));

        // comma or closing parenthesis
        if ((*it)->type == COMMA) (*it)++;
        else if ((*it)->type == RBRACKET) break;
        else {
            unexpected_token(**it);
            free_expr_arrn(array, length);
            return (Expr) { ERROR_EXPR, 0, 0, {} };
        }
    }
    (*it)++;

    Expr expr;
    expr.type = ARR_EXPR;
    expr.line = start.line;
    expr.col = start.col;
    expr.data.arr.len = length;
    expr.data.arr.items = array;
    return expr;
}

Expr parse_lambda(const Token** it) {
    // (x, y: a): b => z

    Token start = **it;

    Expr expr;
    expr.type = LAMBDA_EXPR;
    expr.line = start.line;
    expr.col = start.col;

    if (parse_params(it, start,
        &expr.data.lambda.paramc, &expr.data.lambda.optc,
        &expr.data.lambda.paramv, &expr.data.lambda.paramt, &expr.data.lambda.paramd
    )) return (Expr) { ERROR_EXPR, 0, 0, {} };

    // => token
    if ((*it)->type != DARROW) {
        unexpected_token(**it);
        free(expr.data.lambda.paramv);
        free_spec_arrn(expr.data.lambda.paramt, expr.data.lambda.paramc);
        free_expr_arrn(expr.data.lambda.paramd, expr.data.lambda.paramc);
        return (Expr) { ERROR_EXPR, 0, 0, {} };
    }
    (*it)++;

    // lambda body
    Expr body = parse_expr(it, MAX_PRECEDENCE);
    if (body.type == ERROR_EXPR) {
        free(expr.data.lambda.paramv);
        free_spec_arrn(expr.data.lambda.paramt, expr.data.lambda.paramc);
        free_expr_arrn(expr.data.lambda.paramd, expr.data.lambda.paramc);
        return body;
    }

    expr.data.lambda.expr = malloc(sizeof(Expr));
    if (expr.data.lambda.expr == NULL) {
        malloc_error();
        free(expr.data.lambda.paramv);
        free_spec_arrn(expr.data.lambda.paramt, expr.data.lambda.paramc);
        free_expr_arrn(expr.data.lambda.paramd, expr.data.lambda.paramc);
        free_expr(body);
        return (Expr) { ERROR_EXPR, 0, 0, {} };
    }
    memcpy(expr.data.lambda.expr, &body, sizeof(Expr));

    return expr;
}

Expr handle_unary_postfix(OpEnum type, const Token** it, Expr term) {
    Token token = *(*it)++;

    Expr expr;
    expr.type = UNOP_EXPR;
    expr.line = term.line;
    expr.col = term.col;
    expr.data.op.type = type;
    expr.data.op.token = token;

    expr.data.op.first = malloc(sizeof(Expr));
    if (expr.data.op.first == NULL) {
        malloc_error();
        return (Expr) { ERROR_EXPR, 0, 0, {} };
    }
    memcpy(expr.data.op.first, &term, sizeof(Expr));

    // may have another postfix operator
    Expr next = parse_postfix(it, expr);
    if (next.type == ERROR_EXPR) {
        free_expr(expr);
        return next;
    }
    return next;
}

Expr handle_unary_prefix(OpEnum type, const Token** it) {
    Token token = *(*it)++;

    // operand
    Expr term = parse_term(it);
    if (term.type == ERROR_EXPR) {
        return term;
    }

    Expr expr;
    expr.type = UNOP_EXPR;
    expr.line = token.line;
    expr.col = token.col;
    expr.data.op.type = type;
    expr.data.op.token = token;

    expr.data.op.first = malloc(sizeof(Expr));
    if (expr.data.op.first == NULL) {
        malloc_error();
        free_expr(term);
        return (Expr) { ERROR_EXPR, 0, 0, {} };
    }
    memcpy(expr.data.op.first, &term, sizeof(Expr));

    return expr;
}

Expr handle_atomic_term(const Token** it) {
    Token token = *(*it)++;

    Expr expr;
    expr.type = ATOMIC_EXPR;
    expr.line = token.line;
    expr.col = token.col;
    expr.data.atom = token;

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
        // atom
        case INT_LITERAL:
        case CHR_LITERAL:
        case STR_LITERAL: return handle_atomic_term(it);
        case VAR_NAME: return handle_atomic_term(it);

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
            // check if lambda expression
            Expr expr = is_lambda(it)
                ? parse_lambda(it)
                : parse_expr_group(it);
            Expr next = parse_postfix(it, expr);
            if (next.type == ERROR_EXPR) {
                free_expr(expr);
                return next;
            }
            return next;

        default:
            unexpected_token(**it);
            return (Expr) { ERROR_EXPR, 0, 0, {} };
    }
}

Expr parse_expr(const Token** it, size_t precedence) {
    if (precedence == 0) return parse_term(it);

    // whether current precedence is left-to-right associative
    bool right_to_left = operator_rtl_associative(precedence);
    // leftmost operand
    // must only contain operators of lesser precedence
    Expr lhs = parse_expr(it, precedence - 1);
    if (lhs.type == ERROR_EXPR) return lhs;

    for (;;) {
        Token token = **it;
        OpEnum op = infix_op_from_token(token);
        // check if operation should be handled in this recursive step
        if (op == ERROR_OP || operator_precedence(op) > precedence) return lhs;
        (*it)++;

        // whether operation is ternary
        bool ternary = op == TERNARY;
        Expr middle;
        if (ternary) {
            // middle operator is unaffected by precedence
            middle = parse_expr(it, MAX_PRECEDENCE);
            if (middle.type == ERROR_EXPR) {
                free_expr(lhs);
                return middle;
            }

            if ((*it)->type != COLON) {
                unexpected_token(**it);
                free_expr(lhs);
                free_expr(middle);
                return (Expr) { ERROR_EXPR, 0, 0, {} };
            }
            (*it)++;
        }

        // rightmost operand
        // can contain the same precedence operator iff right-to-left associative
        Expr rhs = parse_expr(it, precedence - !right_to_left);
        if (rhs.type == ERROR_EXPR) {
            free_expr(lhs);
            if (ternary) free_expr(middle);
            return rhs;
        }

        Expr expr;
        expr.type = ternary ? TERNOP_EXPR : BINOP_EXPR;
        expr.line = lhs.line;
        expr.col = lhs.col;
        expr.data.op.type = op;
        expr.data.op.token = token;

        expr.data.op.first = malloc(sizeof(Expr));
        expr.data.op.second = malloc(sizeof(Expr));
        if (expr.data.op.first == NULL || expr.data.op.second == NULL) {
            malloc_error();
            free_expr(lhs);
            if (ternary) free_expr(middle);
            free_expr(rhs);
            free(expr.data.op.first);
            free(expr.data.op.second);
            return (Expr) { ERROR_EXPR, 0, 0, {} };
        }
        memcpy(expr.data.op.first, &lhs, sizeof(Expr));
        memcpy(expr.data.op.second, &rhs, sizeof(Expr));

        // rightmost operand is third and middle is second if ternary
        if (ternary) {
            expr.data.op.third = expr.data.op.second;
            expr.data.op.second = malloc(sizeof(Expr));
            if (expr.data.op.second == NULL) {
                malloc_error();
                free_expr(lhs);
                free_expr(middle);
                free_expr(rhs);
                free(expr.data.op.first);
                free(expr.data.op.third);
                return (Expr) { ERROR_EXPR, 0, 0, {} };
            }
            memcpy(expr.data.op.second, &middle, sizeof(Expr));
        }

        // right-to-left will be done here but left-to-right must loop
        // to parse next term for this precedence level
        if (right_to_left) {
            return expr;
        } else {
            lhs = expr;
        }
    }
}

Stmt parse_block(const Token** it) {
    // stmt ...

    Token start = **it;
    // initialize array
    size_t length = 0, capacity = 1;
    Stmt* array = malloc(sizeof(Stmt) * capacity);
    if (array == NULL) {
        malloc_error();
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }

    while (is_statement(it)) {
        // expand array when out of capacity
        if (length >= capacity) {
            capacity *= 2;
            Stmt* new_array = realloc(array, sizeof(Stmt) * capacity);
            if (new_array == NULL) {
                malloc_error();
                free_stmt_arrn(array, length);
                return (Stmt) { ERROR_STMT, 0, 0, {} };
            }
            array = new_array;
        }

        // next statement
        Stmt item = parse_stmt(it);
        if (item.type == ERROR_STMT) {
            free_stmt_arrn(array, length);
            return item;
        }
        memcpy(&array[length++], &item, sizeof(Stmt));
    }

    Stmt stmt;
    stmt.type = BLOCK;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.data.block.len = length;
    stmt.data.block.stmts = array;
    return stmt;
}

Stmt parse_decl(const Token** it) {
    // var x = y;
    // const x: a = y;

    Token start = **it;
    bool mut;
    switch (start.type) {
        case VAR_TOKEN: mut = true; break;
        case CONST_TOKEN: mut = false; break;
        default:
            unexpected_token(start);
            return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Token name = **it;
    if (name.type != VAR_NAME) {
        unexpected_token(name);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    TypeSpec spec = { INFERRED_SPEC, name.line, name.col, {} };
    if ((*it)->type == COLON) {
        (*it)++;

        spec = parse_type_spec(it);
        if (spec.type == ERROR_SPEC) return (Stmt) { ERROR_STMT, 0, 0, {} };
    }

    if ((*it)->type != EQ_TOKEN) {
        unexpected_token(**it);
        free_spec(spec);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Expr val = parse_expr(it, MAX_PRECEDENCE);
    if (val.type == ERROR_EXPR) return (Stmt) { ERROR_STMT, 0, 0, {} };

    if ((*it)->type != SEMICOLON) {
        unexpected_token(**it);
        free_spec(spec);
        free_expr(val);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Stmt stmt;
    stmt.type = DECL;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.data.decl.name = name;
    stmt.data.decl.val = val;
    stmt.data.decl.spec = spec;
    stmt.data.decl.mutable = mut;
    return stmt;
}

Stmt parse_typedef(const Token** it) {
    // type x = a;

    Token start = **it;
    if (start.type != TYPE_TOKEN) {
        unexpected_token(start);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Token name = **it;
    if (name.type != VAR_NAME) {
        unexpected_token(name);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    if ((*it)->type != EQ_TOKEN) {
        unexpected_token(**it);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    TypeSpec val = parse_type_spec(it);
    if (val.type == ERROR_SPEC) return (Stmt) { ERROR_STMT, 0, 0, {} };

    if ((*it)->type != SEMICOLON) {
        unexpected_token(**it);
        free_spec(val);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Stmt stmt;
    stmt.type = TYPEDEF;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.data.type.name = name;
    stmt.data.type.val = val;
    return stmt;
}

Stmt parse_ifelse(const Token** it) {
    // if (x) f
    // if (x) f else g

    Token start = **it;
    if (start.type != IF_TOKEN) {
        unexpected_token(start);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    if ((*it)->type != LPAREN) {
        unexpected_token(**it);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Expr condition = parse_expr(it, MAX_PRECEDENCE);
    if (condition.type == ERROR_EXPR) return (Stmt) { ERROR_STMT, 0, 0, {} };

    if ((*it)->type != RPAREN) {
        unexpected_token(**it);
        free_expr(condition);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Stmt on_true = parse_stmt(it);
    if (on_true.type == ERROR_STMT) {
        free_expr(condition);
        return on_true;
    }

    Stmt stmt;
    stmt.type = IFELSE_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.data.ifelse.condition = condition;
    stmt.data.ifelse.on_true = NULL;
    stmt.data.ifelse.on_false = NULL;

    if ((*it)->type == ELSE_TOKEN) {
        (*it)++;

        Stmt on_false = parse_stmt(it);
        if (on_false.type == ERROR_STMT) {
            free_expr(condition);
            free_stmt(on_true);
            return on_false;
        }

        stmt.data.ifelse.on_false = malloc(sizeof(Stmt));
        if (stmt.data.ifelse.on_false == NULL) {
            malloc_error();
            free_expr(condition);
            free_stmt(on_true);
            free_stmt(on_false);
            return (Stmt) { ERROR_STMT, 0, 0, {} };
        }
        memcpy(stmt.data.ifelse.on_false, &on_false, sizeof(Stmt));
    }

    stmt.data.ifelse.on_true = malloc(sizeof(Stmt));
    if (stmt.data.ifelse.on_true == NULL) {
        malloc_error();
        free_expr(condition);
        free_stmt(on_true);
        if (stmt.data.ifelse.on_false) {
            free_stmt(*stmt.data.ifelse.on_false);
            free(stmt.data.ifelse.on_false);
        }
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    memcpy(stmt.data.ifelse.on_true, &on_true, sizeof(Stmt));

    return stmt;
}

Stmt parse_switch(const Token** it) {
    // switch (x) {
    //     case y: f
    //     default: g
    // }

    Token start = **it;
    if (start.type != SWITCH_TOKEN) {
        unexpected_token(start);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    if ((*it)->type != LPAREN) {
        unexpected_token(**it);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Expr expr = parse_expr(it, MAX_PRECEDENCE);
    if (expr.type == ERROR_EXPR) return (Stmt) { ERROR_STMT, 0, 0, {} };

    if ((*it)->type != RPAREN) {
        unexpected_token(**it);
        free_expr(expr);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    if ((*it)->type != LBRACE) {
        unexpected_token(**it);
        free_expr(expr);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // initialize arrays
    size_t length = 0, capacity = 1;
    Expr* case_array = malloc(sizeof(Expr) * capacity);
    Stmt* branch_array = malloc(sizeof(Stmt) * capacity);
    if (case_array == NULL || branch_array == NULL) {
        malloc_error();
        free_expr(expr);
        free(case_array);
        free(branch_array);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }

    size_t default_index = length;
    while ((*it)->type != RBRACE) {
        // expand arrays when out of capacity
        if (length >= capacity) {
            capacity *= 2;
            Expr* new_case_array = realloc(case_array, sizeof(Expr) * capacity);
            if (new_case_array == NULL) {
                malloc_error();
                free_expr(expr);
                free_expr_arrn(case_array, length);
                free_stmt_arrn(branch_array, length);
                return (Stmt) { ERROR_STMT, 0, 0, {} };
            }
            case_array = new_case_array;
            Stmt* new_branch_array = realloc(branch_array, sizeof(Stmt) * capacity);
            if (new_branch_array == NULL) {
                malloc_error();
                free_expr(expr);
                free_expr_arrn(case_array, length);
                free_stmt_arrn(branch_array, length);
                return (Stmt) { ERROR_STMT, 0, 0, {} };
            }
            branch_array = new_branch_array;
        }

        Expr case_value = { NO_EXPR, (*it)->line, (*it)->col, {} };
        switch ((*it)->type) {
            case CASE_TOKEN:
                (*it)++;
                case_value = parse_expr(it, MAX_PRECEDENCE);
                if (case_value.type == ERROR_EXPR) {
                    free_expr(expr);
                    free_expr_arrn(case_array, length);
                    free_stmt_arrn(branch_array, length);
                    return (Stmt) { ERROR_STMT, 0, 0, {} };
                }
                if (default_index == length) default_index++;
                break;
            case DEFAULT_TOKEN:
                if (default_index != length) {
                    error_line = (*it)->line;
                    error_col = (*it)->col;
                    syntax_error("multiple default labels in switch\n");
                    free_expr(expr);
                    free_expr_arrn(case_array, length);
                    free_stmt_arrn(branch_array, length);
                    return (Stmt) { ERROR_STMT, 0, 0, {} };
                }
                (*it)++;
                break;

            default:
                unexpected_token(**it);
                free_expr(expr);
                free_expr_arrn(case_array, length);
                free_stmt_arrn(branch_array, length);
                return (Stmt) { ERROR_STMT, 0, 0, {} };
        }

        if ((*it)->type != COLON) {
            unexpected_token(**it);
            free_expr(expr);
            free_expr(case_value);
            free_expr_arrn(case_array, length);
            free_stmt_arrn(branch_array, length);
            return (Stmt) { ERROR_STMT, 0, 0, {} };
        }
        (*it)++;

        Stmt branch_value = parse_block(it);
        if (branch_value.type == ERROR_STMT) {
            free_expr(expr);
            free_expr(case_value);
            free_expr_arrn(case_array, length);
            free_stmt_arrn(branch_array, length);
            return (Stmt) { ERROR_STMT, 0, 0, {} };
        }

        memcpy(&case_array[length], &case_value, sizeof(Expr));
        memcpy(&branch_array[length], &branch_value, sizeof(Stmt));
        length++;
    }
    (*it)++;

    Stmt stmt;
    stmt.type = SWITCH_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.data.switchcase.expr = expr;
    stmt.data.switchcase.casec = length;
    stmt.data.switchcase.casev = case_array;
    stmt.data.switchcase.branchv = branch_array;
    stmt.data.switchcase.defaulti = default_index;
    return stmt;
}

Stmt parse_while(const Token** it) {
    // while (x) f

    Token start = **it;
    if (start.type != WHILE_TOKEN) {
        unexpected_token(start);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    if ((*it)->type != LPAREN) {
        unexpected_token(**it);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Expr condition = parse_expr(it, MAX_PRECEDENCE);
    if (condition.type == ERROR_EXPR) return (Stmt) { ERROR_STMT, 0, 0, {} };

    if ((*it)->type != RPAREN) {
        unexpected_token(**it);
        free_expr(condition);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Stmt body = parse_stmt(it);
    if (body.type == ERROR_STMT) {
        free_expr(condition);
        return body;
    }

    Stmt stmt;
    stmt.type = WHILE_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.data.whileloop.condition = condition;

    stmt.data.whileloop.body = malloc(sizeof(Stmt));
    if (stmt.data.whileloop.body == NULL) {
        malloc_error();
        free_expr(condition);
        free_stmt(body);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    memcpy(stmt.data.whileloop.body, &body, sizeof(Stmt));

    return stmt;
}

Stmt parse_dowhile(const Token** it) {
    // do f while (x);

    Token start = **it;
    if (start.type != DO_TOKEN) {
        unexpected_token(start);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Stmt body = parse_stmt(it);
    if (body.type == ERROR_STMT) return body;

    if ((*it)->type != WHILE_TOKEN) {
        unexpected_token(**it);
        free_stmt(body);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    if ((*it)->type != LPAREN) {
        unexpected_token(**it);
        free_stmt(body);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Expr condition = parse_expr(it, MAX_PRECEDENCE);
    if (condition.type == ERROR_EXPR) {
        free_stmt(body);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }

    if ((*it)->type != RPAREN) {
        unexpected_token(**it);
        free_stmt(body);
        free_expr(condition);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    if ((*it)->type != SEMICOLON) {
        unexpected_token(**it);
        free_stmt(body);
        free_expr(condition);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Stmt stmt;
    stmt.type = DOWHILE_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.data.whileloop.condition = condition;

    stmt.data.whileloop.body = malloc(sizeof(Stmt));
    if (stmt.data.whileloop.body == NULL) {
        malloc_error();
        free_expr(condition);
        free_stmt(body);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    memcpy(stmt.data.whileloop.body, &body, sizeof(Stmt));

    return stmt;
}

Stmt parse_for(const Token** it) {
    // for (x; y; z) f
    // for (var x = y; z; w) f
    // for (;;) f

    Token start = **it;
    if (start.type != FOR_TOKEN) {
        unexpected_token(start);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    if ((*it)->type != LPAREN) {
        unexpected_token(**it);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    const Token* branch = *it;
    Stmt init = parse_stmt(it);
    switch (init.type) {
        case NOP:
        case DECL:
        case EXPR_STMT: break;

        case ERROR_STMT: return init;

        default:
            *it = branch;
            unexpected_token(**it);
            free_stmt(init);
            return (Stmt) { ERROR_STMT, 0, 0, {} };
    }

    Expr condition = { NO_EXPR, (*it)->line, (*it)->col, {} };
    if ((*it)->type != SEMICOLON) {
        condition = parse_expr(it, MAX_PRECEDENCE);
        if (condition.type == ERROR_EXPR) {
            free_stmt(init);
            return (Stmt) { ERROR_STMT, 0, 0, {} };
        }
    }
    if ((*it)->type != SEMICOLON) {
        unexpected_token(**it);
        free_stmt(init);
        free_expr(condition);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Expr expr = { NO_EXPR, (*it)->line, (*it)->col, {} };
    if ((*it)->type != RPAREN) {
        expr = parse_expr(it, MAX_PRECEDENCE);
        if (expr.type == ERROR_EXPR) {
            free_stmt(init);
            free_expr(condition);
            return (Stmt) { ERROR_STMT, 0, 0, {} };
        }
    }
    if ((*it)->type != RPAREN) {
        unexpected_token(**it);
        free_stmt(init);
        free_expr(condition);
        free_expr(expr);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Stmt body = parse_stmt(it);
    if (body.type == ERROR_STMT) {
        free_stmt(init);
        free_expr(condition);
        free_expr(expr);
        return body;
    }

    Stmt stmt;
    stmt.type = FOR_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.data.forloop.condition = condition;
    stmt.data.forloop.expr = expr;

    stmt.data.forloop.init = malloc(sizeof(Stmt));
    stmt.data.forloop.body = malloc(sizeof(Stmt));
    if (stmt.data.forloop.init == NULL || stmt.data.forloop.body == NULL) {
        malloc_error();
        free_stmt(init);
        free_expr(condition);
        free_expr(expr);
        free_stmt(body);
        free(stmt.data.forloop.init);
        free(stmt.data.forloop.body);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    memcpy(stmt.data.forloop.init, &init, sizeof(Stmt));
    memcpy(stmt.data.forloop.body, &body, sizeof(Stmt));

    return stmt;
}

Stmt parse_function(const Token** it) {
    // fn f(x: a, y: b = 1): 1 {...}

    Token start = **it;
    if (start.type != FN_TOKEN) {
        unexpected_token(start);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Token name = **it;
    if (name.type != VAR_NAME) {
        unexpected_token(name);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Stmt stmt;
    stmt.type = FUNCTION_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.data.fun.name = name;

    if (parse_params(it, start,
        &stmt.data.fun.paramc, &stmt.data.fun.optc,
        &stmt.data.fun.paramv, &stmt.data.fun.paramt, &stmt.data.fun.paramd
    )) return (Stmt) { ERROR_STMT, 0, 0, {} };

    // optional return type specifier
    TypeSpec ret = { INFERRED_SPEC, start.line, start.col, {} };
    if ((*it)->type == COLON) {
        (*it)++;

        ret = parse_type_spec(it);
        if (ret.type == ERROR_SPEC) {
            free(stmt.data.fun.paramv);
            free_spec_arrn(stmt.data.fun.paramt, stmt.data.fun.paramc);
            free_expr_arrn(stmt.data.fun.paramd, stmt.data.fun.paramc);
            free_spec(stmt.data.fun.ret);
            return (Stmt) { ERROR_STMT, 0, 0, {} };
        }
    }
    stmt.data.fun.ret = ret;

    if ((*it)->type != LBRACE) {
        unexpected_token(**it);
        free(stmt.data.fun.paramv);
        free_spec_arrn(stmt.data.fun.paramt, stmt.data.fun.paramc);
        free_expr_arrn(stmt.data.fun.paramd, stmt.data.fun.paramc);
        free_spec(stmt.data.fun.ret);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Stmt body = parse_block(it);
    if (body.type == ERROR_STMT) {
        unexpected_token(**it);
        free(stmt.data.fun.paramv);
        free_spec_arrn(stmt.data.fun.paramt, stmt.data.fun.paramc);
        free_expr_arrn(stmt.data.fun.paramd, stmt.data.fun.paramc);
        free_spec(stmt.data.fun.ret);
        return body;
    }

    if ((*it)->type != RBRACE) {
        unexpected_token(**it);
        free(stmt.data.fun.paramv);
        free_spec_arrn(stmt.data.fun.paramt, stmt.data.fun.paramc);
        free_expr_arrn(stmt.data.fun.paramd, stmt.data.fun.paramc);
        free_spec(stmt.data.fun.ret);
        free_stmt(body);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    stmt.data.fun.body = malloc(sizeof(Stmt));
    if (stmt.data.fun.body == NULL) {
        malloc_error();
        free(stmt.data.fun.paramv);
        free_spec_arrn(stmt.data.fun.paramt, stmt.data.fun.paramc);
        free_expr_arrn(stmt.data.fun.paramd, stmt.data.fun.paramc);
        free_spec(stmt.data.fun.ret);
        free_stmt(body);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    memcpy(stmt.data.fun.body, &body, sizeof(Stmt));

    return stmt;
}

Stmt parse_stmt(const Token** it) {
    switch ((*it)->type) {
        Stmt stmt;

        case SEMICOLON: // ;
            stmt.type = NOP;
            stmt.line = (*it)->line;
            stmt.col = (*it)->col;
            (*it)++;
            return stmt;

        case VAR_TOKEN:
        case CONST_TOKEN: return parse_decl(it);
        case TYPE_TOKEN: return parse_typedef(it);

        case IF_TOKEN: return parse_ifelse(it);
        case SWITCH_TOKEN: return parse_switch(it);
        case WHILE_TOKEN: return parse_while(it);
        case DO_TOKEN: return parse_dowhile(it);
        case FOR_TOKEN: return parse_for(it);

        case FN_TOKEN: return parse_function(it);

        default: // expr ;
            Expr expr = parse_expr(it, MAX_PRECEDENCE);
            if (expr.type == ERROR_EXPR) return (Stmt) { ERROR_STMT, 0, 0, {} };
            // terminating semicolon
            if ((*it)->type != SEMICOLON) {
                unexpected_token(**it);
                free_expr(expr);
                return (Stmt) { ERROR_STMT, 0, 0, {} };
            }
            (*it)++;
            stmt.type = EXPR_STMT;
            stmt.line = expr.line;
            stmt.col = expr.col;
            stmt.data.expr = expr;
            return stmt;
    }
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
        case ERROR_SPEC: break;
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

// Free all data inside expression.
void free_expr(Expr expr) {
    switch (expr.type) {
        case ERROR_EXPR: break;
        case NO_EXPR: break;

        case GROUPED_EXPR:
            free_expr(*expr.data.group);
            free(expr.data.group);
            break;
        case ATOMIC_EXPR: break;
        case ARR_EXPR:
            free_expr_arrn(expr.data.arr.items, expr.data.arr.len);
            break;
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
    }
}

// Free expression array of length n all all data inside it.
void free_expr_arrn(Expr* arr, size_t n) {
    for (size_t i = 0; i < n; i++) free_expr(arr[i]);
    free(arr);
}

// Free all data inside statement.
void free_stmt(Stmt stmt) {
    switch (stmt.type) {
        case ERROR_STMT: break;
        case NOP: break;

        case BLOCK:
            free_stmt_arrn(stmt.data.block.stmts, stmt.data.block.len);
            break;
        case EXPR_STMT:
            free_expr(stmt.data.expr);
            break;
        case DECL:
            free_expr(stmt.data.decl.val);
            free_spec(stmt.data.decl.spec);
            break;
        case TYPEDEF:
            free_spec(stmt.data.type.val);
            break;

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
    }
}

// Free statement array of length n all all data inside it.
void free_stmt_arrn(Stmt* arr, size_t n) {
    for (size_t i = 0; i < n; i++) free_stmt(arr[i]);
    free(arr);
}

// Free non-tagged abstract syntax tree token and all data inside it.
void free_ast_p(AST* ast) {
    if (ast == NULL) return;
    free_stmt(*ast);
    free(ast);
}
