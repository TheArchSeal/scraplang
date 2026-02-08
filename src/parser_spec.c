#include <stdlib.h>

#include "parser_common.h"
#include "printerr.h"

Spec parse_spec_mod(const Token** it, Spec base);

Spec parse_spec_group(const Token** it) {
    // (
    Token start = *(*it)++;

    // inner type specifier
    Spec group = parse_spec(it);
    if (group.type == ERROR_SPEC) goto err;

    // )
    if (consume_expected_token(it, RPAREN)) goto err_free_group;

    Spec spec;
    spec.type = GROUPED_SPEC;
    spec.line = start.line;
    spec.col = start.col;

    // allocations
    spec.group = MALLOC_STRUCT(group);
    if (spec.group == NULL) goto err_free_group;

    return spec;
err_free_group:
    free_spec(group);
err:
    return (Spec) { .type = ERROR_SPEC };
}

Spec parse_fun_spec(const Token** it) {
    // (a, b?) => c

    Spec item;

    // (
    Token start = *(*it)++;
    DynArr array = dynarr_create(sizeof(Spec));

    // number of optional parameters
    size_t optional = 0;
    if ((*it)->type != RPAREN) {
        for (;;) {
            // next parameter type
            item = parse_spec(it);
            if (item.type == ERROR_SPEC) goto err_free_arr;

            if (dynarr_append(&array, &item)) goto err_free_item;

            // optionally ?
            if ((*it)->type == QMARK) {
                (*it)++;
                optional++;
            } else if (optional) {
                // ? is required if already seen
                error_line = start.line;
                error_col = start.col;
                syntax_error("non-optional parameter after optional parameter\n");
                goto err_free_arr;
            }

            // comma or closing parenthesis
            if ((*it)->type == COMMA) (*it)++;
            else if ((*it)->type == RPAREN) break;
            else {
                unexpected_token(**it);
                goto err_free_arr;
            }
        }
    }
    (*it)++;

    // =>
    if (consume_expected_token(it, DARROW)) goto err_free_arr;

    // return type
    Spec ret = parse_spec(it);
    if (ret.type == ERROR_SPEC) goto err_free_arr;

    Spec spec;
    spec.type = FUN_SPEC;
    spec.line = start.line;
    spec.col = start.col;
    spec.fun.paramc = array.length;
    spec.fun.optc = optional;
    spec.fun.paramt = array.c_arr;

    // allocations
    spec.fun.ret = MALLOC_STRUCT(ret);
    if (spec.fun.ret == NULL) goto err_free_ret;

    return spec;
err_free_item:
    free_spec(item);
    goto err_free_arr;
err_free_ret:
    free_spec(ret);
err_free_arr:
    free_spec_dynarr(&array);
    return (Spec) { .type = ERROR_SPEC };
}

Spec handle_spec_mod(SpecEnum type, bool mut, const Token** it, Spec base) {
    // * or [
    Token start = *(*it)++;

    if (start.type == LBRACKET) {
        // ]
        if (consume_expected_token(it, RBRACKET)) goto err;
    }

    Spec spec;
    spec.type = type;
    spec.line = base.line;
    spec.col = base.col;
    spec.ptr.mutable = mut;

    // allocations
    spec.ptr.spec = MALLOC_STRUCT(base);
    if (spec.ptr.spec == NULL) goto err;

    // may have another modification
    Spec next = parse_spec_mod(it, spec);
    if (next.type == ERROR_SPEC) goto err_free_alloc;

    return next;
err_free_alloc:
    free(spec.ptr.spec);
err:
    return (Spec) { .type = ERROR_SPEC };
}

Spec parse_spec_mod(const Token** it, Spec base) {
    switch ((*it)->type) {
        // regular modifier
        case LBRACKET: return handle_spec_mod(ARR_SPEC, true, it, base);
        case STAR:     return handle_spec_mod(PTR_SPEC, true, it, base);

        case CONST_TOKEN:  // const modifier
            (*it)++;
            switch ((*it)->type) {
                case LBRACKET: return handle_spec_mod(ARR_SPEC, false, it, base);
                case STAR:     return handle_spec_mod(PTR_SPEC, false, it, base);
                default:       unexpected_token(**it); return (Spec) { .type = ERROR_SPEC };
            }

        default: return base;  // no modifier
    }
}

Spec parse_spec(const Token** it) {
    Spec spec;
    Spec next;

    switch ((*it)->type) {
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
        case IDENTIFIER:
            Token token = *(*it)++;
            spec.type = ATOMIC_SPEC;
            spec.line = token.line;
            spec.col = token.col;
            spec.atom = token;
            return parse_spec_mod(it, spec);

        case LPAREN:
            // check if function type specifier
            spec = is_lambda(it) ? parse_fun_spec(it) : parse_spec_group(it);
            // modifications
            next = parse_spec_mod(it, spec);
            if (next.type == ERROR_SPEC) goto err_free_spec;
            break;

        default: unexpected_token(**it); goto err;
    }

    return next;
err_free_spec:
    free_spec(spec);
err:
    return (Spec) { .type = ERROR_SPEC };
}
