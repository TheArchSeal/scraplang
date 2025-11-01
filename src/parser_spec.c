#include <stdlib.h>

#include "parser_common.h"
#include "printerr.h"

TypeSpec parse_type_spec_mod(const Token** it, TypeSpec base);

TypeSpec parse_type_spec_group(const Token** it) {
    // (
    Token start = *(*it)++;

    // inner type specifier
    TypeSpec group = parse_type_spec(it);
    if (group.type == ERROR_SPEC) goto err;

    // )
    if (consume_expected_token(it, RPAREN)) goto err_free_group;

    TypeSpec spec;
    spec.type = GROUPED_SPEC;
    spec.line = start.line;
    spec.col = start.col;

    // allocations
    spec.data.group = malloc_struct(&group, sizeof(TypeSpec));
    if (spec.data.group == NULL) goto err_free_group;

    return spec;
err_free_group:
    free_spec(group);
err:
    return (TypeSpec) { .type = ERROR_SPEC };
}

TypeSpec parse_fun_spec(const Token** it) {
    // (a, b?) => c

    TypeSpec item;

    // (
    Token start = *(*it)++;
    DynArr array = dynarr_create(sizeof(TypeSpec));

    // number of optional parameters
    size_t optional = 0;
    if ((*it)->type != RPAREN) {
        for (;;) {
            // next parameter type
            item = parse_type_spec(it);
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
    TypeSpec ret = parse_type_spec(it);
    if (ret.type == ERROR_SPEC) goto err_free_arr;

    TypeSpec spec;
    spec.type = FUN_SPEC;
    spec.line = start.line;
    spec.col = start.col;
    spec.data.fun.paramc = array.length;
    spec.data.fun.optc = optional;
    spec.data.fun.paramt = array.c_arr;

    // allocations
    spec.data.fun.ret = malloc_struct(&ret, sizeof(TypeSpec));
    if (spec.data.fun.ret == NULL) goto err_free_ret;

    return spec;
err_free_item:
    free_spec(item);
    goto err_free_arr;
err_free_ret:
    free_spec(ret);
err_free_arr:
    free_spec_dynarr(&array);
    return (TypeSpec) { .type = ERROR_SPEC };
}

TypeSpec handle_type_spec_mod(TypeSpecEnum type, bool mut, const Token** it, TypeSpec base) {
    // * or [
    Token start = *(*it)++;

    if (start.type == LBRACKET) {
        // ]
        if (consume_expected_token(it, RBRACKET)) goto err;
    }

    TypeSpec spec;
    spec.type = type;
    spec.line = base.line;
    spec.col = base.col;
    spec.data.ptr.mutable = mut;

    // allocations
    spec.data.ptr.spec = malloc_struct(&base, sizeof(TypeSpec));
    if (spec.data.ptr.spec == NULL) goto err;

    // may have another modification
    TypeSpec next = parse_type_spec_mod(it, spec);
    if (next.type == ERROR_SPEC) goto err_free_alloc;

    return next;
err_free_alloc:
    free(spec.data.ptr.spec);
err:
    return (TypeSpec) { .type = ERROR_SPEC };
}

TypeSpec parse_type_spec_mod(const Token** it, TypeSpec base) {
    switch ((*it)->type) {
        // regular modifier
        case LBRACKET: return handle_type_spec_mod(ARR_SPEC, true, it, base);
        case STAR:     return handle_type_spec_mod(PTR_SPEC, true, it, base);

        case CONST_TOKEN:  // const modifier
            (*it)++;
            switch ((*it)->type) {
                case LBRACKET: return handle_type_spec_mod(ARR_SPEC, false, it, base);
                case STAR:     return handle_type_spec_mod(PTR_SPEC, false, it, base);
                default:       unexpected_token(**it); return (TypeSpec) { .type = ERROR_SPEC };
            }

        default: return base;  // no modifier
    }
}

TypeSpec parse_type_spec(const Token** it) {
    TypeSpec spec;
    TypeSpec next;

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
        case VAR_NAME:
            Token token = *(*it)++;
            spec.type = ATOMIC_SPEC;
            spec.line = token.line;
            spec.col = token.col;
            spec.data.atom = token;
            return parse_type_spec_mod(it, spec);

        case LPAREN:
            // check if function type specifier
            spec = is_lambda(it) ? parse_fun_spec(it) : parse_type_spec_group(it);
            // modifications
            next = parse_type_spec_mod(it, spec);
            if (next.type == ERROR_SPEC) goto err_free_spec;
            break;

        default: unexpected_token(**it); goto err;
    }

    return next;
err_free_spec:
    free_spec(spec);
err:
    return (TypeSpec) { .type = ERROR_SPEC };
}
