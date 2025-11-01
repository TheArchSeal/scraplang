#include <stdlib.h>

#include "parser_common.h"
#include "printerr.h"

Expr parse_postfix(const Token** it, Expr term);
Expr parse_term(const Token** it);

// Find the binary or ternary operation based on token.
OpEnum infix_op_from_token(Token token) {
    switch (token.type) {
        case STAR:      return MULTIPLICATION;
        case SLASH:     return DIVISION;
        case PERCENT:   return MODULO;
        case PLUS:      return ADDITION;
        case MINUS:     return SUBTRACTION;
        case DLT_TOKEN: return LEFT_SHIFT;
        case DGT_TOKEN: return RIGHT_SHIFT;

        case AND:   return BITWISE_AND;
        case CARET: return BITWISE_XOR;
        case PIPE:  return BITWISE_OR;

        case LT_TOKEN:  return LESS_THAN;
        case LEQ_TOKEN: return LESS_OR_EQUAL;
        case GT_TOKEN:  return GREATER_THAN;
        case GEQ_TOKEN: return GREATER_OR_EQUAL;
        case DEQ_TOKEN: return EQUAL;
        case NEQ_TOKEN: return NOT_EQUAL;

        case DAND:  return LOGICAL_AND;
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
        case ADDRESS_OF:  return 0;

        case MULTIPLICATION:
        case DIVISION:
        case MODULO:         return 1;
        case ADDITION:
        case SUBTRACTION:    return 2;
        case LEFT_SHIFT:
        case RIGHT_SHIFT:    return 3;

        case BITWISE_AND: return 4;
        case BITWISE_XOR: return 5;
        case BITWISE_OR:  return 6;

        case LESS_THAN:
        case LESS_OR_EQUAL:
        case GREATER_THAN:
        case GREATER_OR_EQUAL: return 7;
        case EQUAL:
        case NOT_EQUAL:        return 8;

        case LOGICAL_AND: return 9;
        case LOGICAL_OR:  return 10;

        case TERNARY: return 11;

        case ASSIGNMENT: return 12;
    }

    // unreachable
    return 0;
}

// Whether precedence is right-to-left associative.
bool operator_rtl_associative(size_t precedence) {
    return precedence == 11 || precedence == 12;
}

Expr parse_expr_group(const Token** it) {
    // (
    Token start = *(*it)++;

    // inner expression
    Expr group = parse_expr(it, MAX_PRECEDENCE);
    if (group.type == ERROR_EXPR) goto err;

    // )
    if (consume_expected_token(it, RPAREN)) goto err_free_group;

    Expr expr;
    expr.type = GROUPED_EXPR;
    expr.line = start.line;
    expr.col = start.col;
    expr.annotation = NULL;

    // allocations
    expr.data.group = malloc_struct(&group, sizeof(Expr));
    if (expr.data.group == NULL) goto err_free_group;

    return expr;
err_free_group:
    free_expr(group);
err:
    return (Expr) { .type = ERROR_EXPR };
}

Expr parse_array_literal(const Token** it) {
    // [x, y, z]

    // [
    Token start = *(*it)++;

    Expr expr;
    expr.type = ARR_EXPR;
    expr.line = start.line;
    expr.col = start.col;
    expr.annotation = NULL;

    // items
    if (parse_args(it, &expr.data.arr.len, &expr.data.arr.items)) goto err;

    // ]
    if (consume_expected_token(it, RBRACKET)) goto err_free_args;

    return expr;
err_free_args:
    free_expr_arrn(expr.data.arr.items, expr.data.arr.len);
err:
    return (Expr) { .type = ERROR_EXPR };
}

Expr parse_lambda(const Token** it) {
    // (x, y: a): b => z

    // (
    Token start = *(*it)++;

    Expr expr;
    expr.type = LAMBDA_EXPR;
    expr.line = start.line;
    expr.col = start.col;
    expr.annotation = NULL;

    // parameters
    if (parse_params(
            it, &expr.data.lambda.paramc, &expr.data.lambda.optc, &expr.data.lambda.paramv,
            &expr.data.lambda.paramt, &expr.data.lambda.paramd
        ))
    {
        goto err;
    }

    // ) =>
    if (consume_expected_token(it, RPAREN) || consume_expected_token(it, DARROW)) {
        goto err_free_params;
    }

    // lambda body
    Expr body = parse_expr(it, MAX_PRECEDENCE);
    if (body.type == ERROR_EXPR) goto err_free_params;

    // allocations
    expr.data.lambda.expr = malloc_struct(&body, sizeof(Expr));
    if (expr.data.lambda.expr == NULL) goto err_free_body;

    return expr;
err_free_body:
    free_expr(body);
err_free_params:
    free(expr.data.lambda.paramv);
    free_spec_arrn(expr.data.lambda.paramt, expr.data.lambda.paramc);
    free_expr_arrn(expr.data.lambda.paramd, expr.data.lambda.paramc);
err:
    return (Expr) { .type = ERROR_EXPR };
}

Expr parse_subscript(const Token** it, Expr term) {
    // x[y]

    // [
    (*it)++;

    // index
    Expr idx = parse_expr(it, MAX_PRECEDENCE);
    if (idx.type == ERROR_EXPR) goto err;

    // ]
    if (consume_expected_token(it, RBRACKET)) goto err_free_idx;

    Expr expr;
    expr.type = SUBSRIPT_EXPR;
    expr.line = term.line;
    expr.col = term.col;
    expr.annotation = NULL;

    // allocations
    expr.data.subscript.arr = malloc_struct(&term, sizeof(Expr));
    expr.data.subscript.idx = malloc_struct(&idx, sizeof(Expr));
    if (expr.data.subscript.arr == NULL || expr.data.subscript.idx == NULL) goto err_free_allocs;

    // may have another postfix operator
    Expr next = parse_postfix(it, expr);
    if (next.type == ERROR_EXPR) goto err_free_allocs;

    return next;
err_free_allocs:
    free(expr.data.subscript.arr);
    free(expr.data.subscript.idx);
err_free_idx:
    free_expr(idx);
err:
    return (Expr) { .type = ERROR_EXPR };
}

Expr parse_call(const Token** it, Expr term) {
    // x(y, z)

    // (
    (*it)++;

    Expr expr;
    expr.type = CALL_EXPR;
    expr.line = term.line;
    expr.col = term.col;
    expr.annotation = NULL;

    // arguments
    if (parse_args(it, &expr.data.call.argc, &expr.data.call.argv)) goto err;

    // )
    if (consume_expected_token(it, RPAREN)) goto err_free_args;

    // allocations
    expr.data.call.fun = malloc_struct(&term, sizeof(Expr));
    if (expr.data.call.fun == NULL) goto err_free_args;

    // may have another postfix operator
    Expr next = parse_postfix(it, expr);
    if (next.type == ERROR_EXPR) goto err_free_alloc;

    return next;
err_free_alloc:
    free(expr.data.call.fun);
err_free_args:
    free_expr_arrn(expr.data.call.argv, expr.data.call.argc);
err:
    return (Expr) { .type = ERROR_EXPR };
}

Expr parse_constructor(const Token** it, Expr term) {
    // x { y, z }

    // {
    (*it)++;

    Expr expr;
    expr.type = CONSTRUCTOR_EXPR;
    expr.line = term.line;
    expr.col = term.col;
    expr.annotation = NULL;

    // arguments
    if (parse_args(it, &expr.data.call.argc, &expr.data.call.argv)) goto err;

    // }
    if (consume_expected_token(it, RBRACE)) goto err_free_args;

    // allocations
    expr.data.call.fun = malloc_struct(&term, sizeof(Expr));
    if (expr.data.call.fun == NULL) goto err_free_args;

    // may have another postfix operator
    Expr next = parse_postfix(it, expr);
    if (next.type == ERROR_EXPR) goto err_free_expr_alloc;

    return next;
err_free_expr_alloc:
    free(expr.data.call.fun);
err_free_args:
    free_expr_arrn(expr.data.call.argv, expr.data.call.argc);
err:
    return (Expr) { .type = ERROR_EXPR };
}

Expr parse_access(const Token** it, Expr term) {
    // x.y

    // .
    (*it)++;

    // variable name
    Token member = **it;
    if (consume_expected_token(it, VAR_NAME)) goto err;

    Expr expr;
    expr.type = ACCESS_EXPR;
    expr.line = term.line;
    expr.col = term.col;
    expr.annotation = NULL;
    expr.data.access.memeber = member;

    // allocations
    expr.data.access.obj = malloc_struct(&term, sizeof(Expr));
    if (expr.data.access.obj == NULL) goto err;

    // may have another postfix operator
    Expr next = parse_postfix(it, expr);
    if (next.type == ERROR_EXPR) goto err_free_alloc;

    return next;
err_free_alloc:
    free(expr.data.access.obj);
err:
    return (Expr) { .type = ERROR_EXPR };
}

Expr parse_unary_postfix(OpEnum type, const Token** it, Expr term) {
    // operator
    Token token = *(*it)++;

    Expr expr;
    expr.type = UNOP_EXPR;
    expr.line = term.line;
    expr.col = term.col;
    expr.annotation = NULL;
    expr.data.op.type = type;
    expr.data.op.token = token;

    // allocations
    expr.data.op.first = malloc_struct(&term, sizeof(Expr));
    if (expr.data.op.first == NULL) goto err;

    // may have another postfix operator
    Expr next = parse_postfix(it, expr);
    if (next.type == ERROR_EXPR) goto err_free_alloc;

    return next;
err_free_alloc:
    free(expr.data.op.first);
err:
    return (Expr) { .type = ERROR_EXPR };
}

Expr parse_unary_prefix(OpEnum type, const Token** it) {
    // operator
    Token token = *(*it)++;

    // operand
    Expr term = parse_term(it);
    if (term.type == ERROR_EXPR) goto err;

    Expr expr;
    expr.type = UNOP_EXPR;
    expr.line = token.line;
    expr.col = token.col;
    expr.annotation = NULL;
    expr.data.op.type = type;
    expr.data.op.token = token;

    // allocations
    expr.data.op.first = malloc_struct(&term, sizeof(Expr));
    if (expr.data.op.first == NULL) goto err_free_term;

    return expr;
err_free_term:
    free_expr(term);
err:
    return (Expr) { .type = ERROR_EXPR };
}

Expr parse_atomic_term(const Token** it) {
    // value
    Token token = *(*it)++;

    Expr expr;
    expr.type = ATOMIC_EXPR;
    expr.line = token.line;
    expr.col = token.col;
    expr.annotation = NULL;
    expr.data.atom = token;

    return parse_postfix(it, expr);
}

Expr parse_postfix(const Token** it, Expr term) {
    switch ((*it)->type) {
        case DPLUS:  return parse_unary_postfix(POSTFIX_INC, it, term);
        case DMINUS: return parse_unary_postfix(POSTFIX_DEC, it, term);

        case LBRACKET: return parse_subscript(it, term);
        case LPAREN:   return parse_call(it, term);
        case LBRACE:   return parse_constructor(it, term);
        case DOT:      return parse_access(it, term);

        default: return term;
    }
}

Expr parse_term(const Token** it) {
    Expr expr;
    Expr next;

    switch ((*it)->type) {
        // atom
        case INT_LITERAL:
        case CHR_LITERAL:
        case STR_LITERAL:
        case VAR_NAME:    return parse_atomic_term(it);

        case PLUS:     return parse_unary_prefix(UNARY_PLUS, it);
        case DPLUS:    return parse_unary_prefix(POSTFIX_INC, it);
        case MINUS:    return parse_unary_prefix(UNARY_PLUS, it);
        case DMINUS:   return parse_unary_prefix(POSTFIX_DEC, it);
        case TILDE:    return parse_unary_prefix(BINARY_NOT, it);
        case EXCLMARK: return parse_unary_prefix(LOGICAL_NOT, it);
        case STAR:     return parse_unary_prefix(DEREFERENCE, it);
        case AND:      return parse_unary_prefix(ADDRESS_OF, it);

        case LBRACKET: return parse_array_literal(it);
        case LPAREN:
            // check if lambda expression
            expr = is_lambda(it) ? parse_lambda(it) : parse_expr_group(it);
            // postfix operators
            next = parse_postfix(it, expr);
            if (next.type == ERROR_EXPR) goto err_free_expr;
            break;

        default: unexpected_token(**it); goto err;
    }

    return next;
err_free_expr:
    free_expr(expr);
err:
    return (Expr) { .type = ERROR_EXPR };
}

Expr parse_expr(const Token** it, size_t precedence) {
    // base case
    if (precedence == 0) return parse_term(it);

    bool ternary;
    Expr middle;
    Expr rhs;
    Expr expr;

    // whether current precedence is left-to-right associative
    bool right_to_left = operator_rtl_associative(precedence);
    // leftmost operand
    // must only contain operators of lesser precedence
    Expr lhs = parse_expr(it, precedence - 1);
    if (lhs.type == ERROR_EXPR) goto err;

    for (;;) {
        Token token = **it;
        OpEnum op = infix_op_from_token(token);
        // check if operation should be handled in this recursive step
        if (op == ERROR_OP || operator_precedence(op) > precedence) return lhs;
        (*it)++;

        // whether operation is ternary
        ternary = op == TERNARY;
        if (ternary) {
            // middle operator is unaffected by precedence
            middle = parse_expr(it, MAX_PRECEDENCE);
            if (middle.type == ERROR_EXPR) goto err_free_lhs;

            // :
            if (consume_expected_token(it, COLON)) goto err_free_middle;
        }

        // rightmost operand
        // can contain the same precedence operator iff right-to-left associative
        rhs = parse_expr(it, precedence - !right_to_left);
        if (rhs.type == ERROR_EXPR) goto err_free_middle;

        expr.type = ternary ? TERNOP_EXPR : BINOP_EXPR;
        expr.line = lhs.line;
        expr.col = lhs.col;
        expr.annotation = NULL;
        expr.data.op.type = op;
        expr.data.op.token = token;

        // allocations
        expr.data.op.first = malloc_struct(&lhs, sizeof(Expr));
        expr.data.op.second = malloc_struct(&rhs, sizeof(Expr));
        if (expr.data.op.first == NULL || expr.data.op.second == NULL) goto err_free_allocs;

        // rightmost operand is third and middle is second if ternary
        if (ternary) {
            expr.data.op.third = expr.data.op.second;
            expr.data.op.second = malloc_struct(&middle, sizeof(Expr));
            if (expr.data.op.second == NULL) goto err_free_ternary_alloc;
        }

        // right-to-left will be done here but left-to-right must loop
        // to parse next term for this precedence level
        if (right_to_left) {
            return expr;
        } else {
            lhs = expr;
        }
    }

err_free_ternary_alloc:
    free(expr.data.op.third);
err_free_allocs:
    free(expr.data.op.second);
    free(expr.data.op.first);
    free_expr(rhs);
err_free_middle:
    if (ternary) free_expr(middle);
err_free_lhs:
    free_expr(lhs);
err:
    return (Expr) { .type = ERROR_EXPR };
}
