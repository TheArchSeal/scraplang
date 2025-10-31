#include <stdlib.h>
#include <string.h>

#include "parser_common.h"
#include "printerr.h"

TypeSpec parse_type_spec_mod(const Token** it, TypeSpec base);

TypeSpec parse_type_spec_group(const Token** it) {
    // (
    Token start = *(*it)++;

    // inner type specifier
    TypeSpec group = parse_type_spec(it);
    if (group.type == ERROR_SPEC) return group;

    // )
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

    // allocations
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

    // (
    Token start = *(*it)++;

    DynArr array = dynarr_create(sizeof(TypeSpec));
    // number of optional parameters
    size_t optional = 0;
    if ((*it)->type != RPAREN) {
        for (;;) {
            // next parameter type
            TypeSpec item = parse_type_spec(it);
            if (item.type == ERROR_SPEC) {
                free_spec_dynarr(&array);
                return item;
            }

            if (dynarr_append(&array, &item)) {
                malloc_error();
                free_spec(item);
                free_spec_dynarr(&array);
            }

            // optionally ?
            if ((*it)->type == QMARK) {
                (*it)++;
                optional++;
            } else if (optional) {
                // ? is required if already seen
                error_line = start.line;
                error_col = start.col;
                syntax_error("non-optional parameter after optional parameter\n");
                free_spec_dynarr(&array);
                return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
            }

            // comma or closing parenthesis
            if ((*it)->type == COMMA) (*it)++;
            else if ((*it)->type == RPAREN) break;
            else {
                unexpected_token(**it);
                free_spec_dynarr(&array);
                return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
            }
        }
    }
    (*it)++;

    // =>
    if ((*it)->type != DARROW) {
        unexpected_token(**it);
        free_spec_dynarr(&array);
        return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
    }
    (*it)++;

    // return type
    TypeSpec ret = parse_type_spec(it);
    if (ret.type == ERROR_SPEC) {
        free_spec_dynarr(&array);
        return ret;
    }

    TypeSpec spec;
    spec.type = FUN_SPEC;
    spec.line = start.line;
    spec.col = start.col;
    spec.data.fun.paramc = array.length;
    spec.data.fun.optc = optional;
    spec.data.fun.paramt = array.c_arr;

    // allocations
    spec.data.fun.ret = malloc(sizeof(TypeSpec));
    if (spec.data.fun.ret == NULL) {
        malloc_error();
        free_spec_dynarr(&array);
        free_spec(ret);
        return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
    }
    memcpy(spec.data.fun.ret, &ret, sizeof(TypeSpec));

    return spec;
}

TypeSpec handle_type_spec_mod(TypeSpecEnum type, bool mut, const Token** it, TypeSpec base) {
    // * or [
    Token start = *(*it)++;

    if (start.type == LBRACKET) {
        // ]
        if ((*it)->type != RBRACKET) {
            unexpected_token(**it);
            return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
        }
        (*it)++;
    }

    TypeSpec spec;
    spec.type = type;
    spec.line = base.line;
    spec.col = base.col;
    spec.data.ptr.mutable = mut;

    // allocations
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
        case STAR:     return handle_type_spec_mod(PTR_SPEC, true, it, base);

        case CONST_TOKEN:  // const modifier
            (*it)++;
            switch ((*it)->type) {
                case LBRACKET: return handle_type_spec_mod(ARR_SPEC, false, it, base);
                case STAR:     return handle_type_spec_mod(PTR_SPEC, false, it, base);
                default:       unexpected_token(**it); return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
            }

        default: return base;  // no modifier
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
            spec = is_lambda(it) ? parse_fun_spec(it) : parse_type_spec_group(it);
            // modifications
            TypeSpec next = parse_type_spec_mod(it, spec);
            if (next.type == ERROR_SPEC) {
                free_spec(spec);
                return next;
            }
            return next;

        default: unexpected_token(**it); return (TypeSpec) { ERROR_SPEC, 0, 0, {} };
    }
}
