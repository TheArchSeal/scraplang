#include <stdlib.h>

#include "parser_common.h"
#include "printerr.h"

Stmt parse_block(const Token** it) {
    Token start = **it;

    Stmt item;

    // initialize array
    DynArr array = dynarr_create(sizeof(Stmt));
    while (is_statement(it)) {
        // next statement
        item = parse_stmt(it);
        if (item.type == ERROR_STMT) goto err_free_arr;
        if (dynarr_append(&array, &item)) goto err_free_item;
    }

    Stmt stmt;
    stmt.type = BLOCK;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.block.len = array.length;
    stmt.block.stmts = array.c_arr;

    return stmt;
err_free_item:
    free_stmt(item);
err_free_arr:
    free_stmt_dynarr(&array);
    return (Stmt) { .type = ERROR_STMT };
}

Stmt parse_decl(const Token** it, bool mut) {
    // var x = y;
    // const x: a = y;

    // var or const
    Token start = *(*it)++;

    // variable name
    Token name = **it;
    if (consume_expected_token(it, IDENTIFIER)) goto err;

    // optionally : and type specifier
    Spec spec = { INFERRED_SPEC, name.line, name.col, {} };
    if ((*it)->type == COLON) {
        (*it)++;

        spec = parse_spec(it);
        if (spec.type == ERROR_SPEC) goto err;
    }

    // =
    if (consume_expected_token(it, EQ_TOKEN)) goto err_free_spec;

    // value
    Expr val = parse_expr(it, MAX_PRECEDENCE);
    if (val.type == ERROR_EXPR) goto err_free_spec;

    // ;
    if (consume_expected_token(it, SEMICOLON)) goto err_free_val;

    Stmt stmt;
    stmt.type = DECL;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.decl.name = name;
    stmt.decl.val = val;
    stmt.decl.spec = spec;
    stmt.decl.mutable = mut;

    return stmt;
err_free_val:
    free_expr(val);
err_free_spec:
    free_spec(spec);
err:
    return (Stmt) { .type = ERROR_STMT };
}

Stmt parse_typedef(const Token** it) {
    // typedef x = a;

    // typedef
    Token start = *(*it)++;

    // variable name =
    Token name = **it;
    if (consume_expected_token(it, IDENTIFIER) || consume_expected_token(it, EQ_TOKEN)) goto err;

    // value
    Spec val = parse_spec(it);
    if (val.type == ERROR_SPEC) goto err;

    // ;
    if (consume_expected_token(it, SEMICOLON)) goto err_free_val;

    Stmt stmt;
    stmt.type = TYPEDEF;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.typedefdata.name = name;
    stmt.typedefdata.val = val;

    return stmt;
err_free_val:
    free_spec(val);
err:
    return (Stmt) { .type = ERROR_STMT };
}

Stmt parse_ifelse(const Token** it) {
    // if (x) f
    // if (x) f else g

    // if
    Token start = *(*it)++;

    // (
    if (consume_expected_token(it, LPAREN)) goto err;

    // condition
    Expr condition = parse_expr(it, MAX_PRECEDENCE);
    if (condition.type == ERROR_EXPR) goto err;

    // )
    if (consume_expected_token(it, RPAREN)) goto err_free_cond;

    // if branch
    Stmt on_true = parse_stmt(it);
    Stmt on_false;
    if (on_true.type == ERROR_STMT) goto err_free_cond;

    Stmt stmt;
    stmt.type = IFELSE_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.ifelse.condition = condition;
    stmt.ifelse.on_true = NULL;
    stmt.ifelse.on_false = NULL;

    // optionally else and branch
    if ((*it)->type == ELSE_TOKEN) {
        (*it)++;

        on_false = parse_stmt(it);
        if (on_false.type == ERROR_STMT) goto err_free_true;

        // allocations
        stmt.ifelse.on_false = MALLOC_STRUCT(on_false);
        if (stmt.ifelse.on_false == NULL) goto err_free_false;
    }

    // allocations
    stmt.ifelse.on_true = MALLOC_STRUCT(on_true);
    if (stmt.ifelse.on_true == NULL) goto err_free_false_alloc;

    return stmt;
err_free_false_alloc:
    if (stmt.ifelse.on_false == NULL) goto err_free_true;
    free(stmt.ifelse.on_false);
err_free_false:
    free_stmt(on_false);
err_free_true:
    free_stmt(on_true);
err_free_cond:
    free_expr(condition);
err:
    return (Stmt) { .type = ERROR_STMT };
}

Stmt parse_switch(const Token** it) {
    // switch (x) {
    //     case y: f
    //     default: g
    // }

    // switch
    Token start = *(*it)++;

    Expr case_value;
    Stmt branch_value;

    // (
    if (consume_expected_token(it, LPAREN)) goto err;

    // expression to switch on
    Expr expr = parse_expr(it, MAX_PRECEDENCE);
    if (expr.type == ERROR_EXPR) goto err;

    // ) {
    if (consume_expected_token(it, RPAREN) || consume_expected_token(it, LBRACE)) {
        goto err_free_expr;
    }

    DynArr case_array = dynarr_create(sizeof(Expr));
    DynArr branch_array = dynarr_create(sizeof(Stmt));

    size_t default_index = 0;
    while ((*it)->type != RBRACE) {
        // case or default
        case_value = (Expr) { NO_EXPR, (*it)->line, (*it)->col, {}, NULL };
        switch ((*it)->type) {
            case CASE_TOKEN:
                (*it)++;
                // label value
                case_value = parse_expr(it, MAX_PRECEDENCE);
                if (case_value.type == ERROR_EXPR) goto err_free_arrs;
                // if default not found, keep default index out of bounds
                if (default_index == case_array.length) default_index++;
                break;
            case DEFAULT_TOKEN:
                // already encountered default
                if (default_index != case_array.length) {
                    error_line = (*it)->line;
                    error_col = (*it)->col;
                    syntax_error("multiple default labels in switch\n");
                    goto err_free_arrs;
                }
                (*it)++;
                break;

            default: unexpected_token(**it); goto err_free_arrs;
        }

        // :
        if (consume_expected_token(it, COLON)) goto err_free_case_val;

        // case branch
        branch_value = parse_block(it);
        if (branch_value.type == ERROR_STMT) goto err_free_case_val;

        if (dynarr_append(&case_array, &case_value) || dynarr_append(&branch_array, &branch_value))
        {
            goto err_free_branch_val;
        }
    }
    (*it)++;

    Stmt stmt;
    stmt.type = SWITCH_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.switchcase.expr = expr;
    stmt.switchcase.casec = case_array.length;
    stmt.switchcase.casev = case_array.c_arr;
    stmt.switchcase.branchv = branch_array.c_arr;
    stmt.switchcase.defaulti = default_index;

    return stmt;
err_free_branch_val:
    free_stmt(branch_value);
err_free_case_val:
    free_expr(case_value);
err_free_arrs:
    free_expr_dynarr(&case_array);
    free_stmt_dynarr(&branch_array);
err_free_expr:
    free_expr(expr);
err:
    return (Stmt) { .type = ERROR_STMT };
}

Stmt parse_while(const Token** it) {
    // while (x) f

    // while
    Token start = *(*it)++;

    // (
    if (consume_expected_token(it, LPAREN)) goto err;

    // condition
    Expr condition = parse_expr(it, MAX_PRECEDENCE);
    if (condition.type == ERROR_EXPR) goto err;

    // )
    if (consume_expected_token(it, RPAREN)) goto err_free_cond;

    // loop body
    Stmt body = parse_stmt(it);
    if (body.type == ERROR_STMT) goto err_free_cond;

    Stmt stmt;
    stmt.type = WHILE_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.whileloop.condition = condition;

    // allocations
    stmt.whileloop.body = MALLOC_STRUCT(body);
    if (stmt.whileloop.body == NULL) goto err_free_body;

    return stmt;
err_free_body:
    free_stmt(body);
err_free_cond:
    free_expr(condition);
err:
    return (Stmt) { .type = ERROR_STMT };
}

Stmt parse_dowhile(const Token** it) {
    // do f while (x);

    // do
    Token start = *(*it)++;

    // loop body
    Stmt body = parse_stmt(it);
    if (body.type == ERROR_STMT) goto err;

    // while (
    if (consume_expected_token(it, WHILE_TOKEN) || consume_expected_token(it, LPAREN)) {
        goto err_free_body;
    }

    // condition
    Expr condition = parse_expr(it, MAX_PRECEDENCE);
    if (condition.type == ERROR_EXPR) goto err_free_body;

    // ) ;
    if (consume_expected_token(it, RPAREN) || consume_expected_token(it, SEMICOLON)) {
        goto err_free_cond;
    }

    Stmt stmt;
    stmt.type = DOWHILE_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.whileloop.condition = condition;

    // allocations
    stmt.whileloop.body = MALLOC_STRUCT(body);
    if (stmt.whileloop.body == NULL) goto err_free_cond;

    return stmt;
err_free_cond:
    free_expr(condition);
err_free_body:
    free_stmt(body);
err:
    return (Stmt) { .type = ERROR_STMT };
}

Stmt parse_for(const Token** it) {
    // for (x; y; z) f
    // for (var x = y; z; w) f
    // for (;;) f

    // for
    Token start = *(*it)++;

    // (
    if (consume_expected_token(it, LPAREN)) goto err;

    // expr, decl or nop
    const Token* branch = *it;
    Stmt init = parse_stmt(it);
    switch (init.type) {
        case NOP:
        case DECL:
        case EXPR_STMT: break;

        case ERROR_STMT: goto err;

        default:
            *it = branch;
            unexpected_token(**it);
            goto err_free_init;
    }

    // middle expression
    Expr condition = { NO_EXPR, (*it)->line, (*it)->col, {}, NULL };
    if ((*it)->type != SEMICOLON) {
        condition = parse_expr(it, MAX_PRECEDENCE);
        if (condition.type == ERROR_EXPR) goto err_free_init;
    }
    // ;
    if (consume_expected_token(it, SEMICOLON)) goto err_free_cond;

    // rightmost expression
    Expr expr = { NO_EXPR, (*it)->line, (*it)->col, {}, NULL };
    if ((*it)->type != RPAREN) {
        expr = parse_expr(it, MAX_PRECEDENCE);
        if (expr.type == ERROR_EXPR) goto err_free_cond;
    }
    // )
    if (consume_expected_token(it, RPAREN)) goto err_free_expr;

    // loop body
    Stmt body = parse_stmt(it);
    if (body.type == ERROR_STMT) goto err_free_expr;

    Stmt stmt;
    stmt.type = FOR_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.forloop.condition = condition;
    stmt.forloop.expr = expr;

    // allocations
    stmt.forloop.init = MALLOC_STRUCT(init);
    stmt.forloop.body = MALLOC_STRUCT(body);
    if (stmt.forloop.init == NULL || stmt.forloop.body == NULL) goto err_free_allocs;

    return stmt;
err_free_allocs:
    free(stmt.forloop.init);
    free(stmt.forloop.body);
    free_stmt(body);
err_free_expr:
    free_expr(expr);
err_free_cond:
    free_expr(condition);
err_free_init:
    free_stmt(init);
err:
    return (Stmt) { .type = ERROR_STMT };
}

Stmt parse_function(const Token** it) {
    // fn f(x: a, y: b = 1): 1 {...}

    // fn
    Token start = *(*it)++;

    // variable name (
    Token name = **it;
    if (consume_expected_token(it, IDENTIFIER) || consume_expected_token(it, LPAREN)) goto err;

    Stmt stmt;
    stmt.type = FUNCTION_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.fun.name = name;

    // parameters
    if (parse_params(
            it, &stmt.fun.paramc, &stmt.fun.optc, &stmt.fun.paramv, &stmt.fun.paramt,
            &stmt.fun.paramd
        ))
    {
        goto err;
    }

    // )
    if (consume_expected_token(it, RPAREN)) goto err_free_params;

    // optionally : and type specifier
    stmt.fun.ret = (Spec) { INFERRED_SPEC, start.line, start.col, {} };
    if ((*it)->type == COLON) {
        (*it)++;

        stmt.fun.ret = parse_spec(it);
        if (stmt.fun.ret.type == ERROR_SPEC) goto err_free_params;
    }

    // {
    if (consume_expected_token(it, LBRACE)) goto err_free_ret;

    // function body
    Stmt body = parse_block(it);
    if (body.type == ERROR_STMT) goto err_free_ret;

    // }
    if (consume_expected_token(it, RBRACE)) goto err_free_body;

    // allocations
    stmt.fun.body = MALLOC_STRUCT(body);
    if (stmt.fun.body == NULL) goto err_free_body;

    return stmt;
err_free_body:
    free_stmt(body);
err_free_ret:
    free_spec(stmt.fun.ret);
err_free_params:
    free(stmt.fun.paramv);
    free_spec_arrn(stmt.fun.paramt, stmt.fun.paramc);
    free_expr_arrn(stmt.fun.paramd, stmt.fun.paramc);
err:
    return (Stmt) { .type = ERROR_STMT };
}

Stmt parse_struct(const Token** it) {
    // struct s {
    //     x: a,
    //     y: b = 1
    // }

    // struct
    Token start = *(*it)++;

    // variable name {
    Token name = **it;
    if (consume_expected_token(it, IDENTIFIER) || consume_expected_token(it, LBRACE)) goto err;

    Stmt stmt;
    stmt.type = STRUCT_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.structdef.name = name;

    // members
    if (parse_params(
            it, &stmt.structdef.paramc, &stmt.structdef.optc, &stmt.structdef.paramv,
            &stmt.structdef.paramt, &stmt.structdef.paramd
        ))
    {
        goto err;
    }

    // }
    if (consume_expected_token(it, RBRACE)) goto err_free_params;

    return stmt;
err_free_params:
    free(stmt.structdef.paramv);
    free_spec_arrn(stmt.structdef.paramt, stmt.structdef.paramc);
    free_expr_arrn(stmt.structdef.paramd, stmt.structdef.paramc);
err:
    return (Stmt) { .type = ERROR_STMT };
}

Stmt parse_enum(const Token** it) {
    // enum e { x, y, z }

    // enum
    Token start = *(*it)++;

    // variable name {
    Token name = **it;
    if (consume_expected_token(it, IDENTIFIER) || consume_expected_token(it, LBRACE)) goto err;

    // initialize array
    DynArr array = dynarr_create(sizeof(Token));

    if ((*it)->type != RBRACE) {
        for (;;) {
            // element in enum
            Token item = **it;
            if (consume_expected_token(it, IDENTIFIER)) goto err_free_arr;
            if (dynarr_append(&array, &item)) goto err_free_arr;

            // comma or closing parenthesis
            if ((*it)->type == COMMA) (*it)++;
            else if ((*it)->type == RBRACE) break;
            else {
                unexpected_token(**it);
                goto err_free_arr;
            }
        }
    }
    (*it)++;

    Stmt stmt;
    stmt.type = ENUM_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.enumdef.name = name;
    stmt.enumdef.len = array.length;
    stmt.enumdef.items = array.c_arr;

    return stmt;
err_free_arr:
    dynarr_destroy(&array);
err:
    return (Stmt) { .type = ERROR_STMT };
}

Stmt parse_stmt(const Token** it) {
    Stmt stmt;
    Expr expr;

    switch ((*it)->type) {
        case SEMICOLON:
            stmt.type = NOP;
            stmt.line = (*it)->line;
            stmt.col = (*it)->col;
            (*it)++;
            break;

        case VAR_TOKEN:     return parse_decl(it, true);
        case CONST_TOKEN:   return parse_decl(it, false);
        case TYPEDEF_TOKEN: return parse_typedef(it);

        case IF_TOKEN:     return parse_ifelse(it);
        case SWITCH_TOKEN: return parse_switch(it);
        case WHILE_TOKEN:  return parse_while(it);
        case DO_TOKEN:     return parse_dowhile(it);
        case FOR_TOKEN:    return parse_for(it);

        case FN_TOKEN:     return parse_function(it);
        case STRUCT_TOKEN: return parse_struct(it);
        case ENUM_TOKEN:   return parse_enum(it);

        case RETURN_TOKEN:
            stmt.type = RETURN_STMT;
            stmt.line = (*it)->line;
            stmt.col = (*it)->col;
            (*it)++;
            if ((*it)->type == SEMICOLON) {
                (*it)++;
                stmt.expr = (Expr) { NO_EXPR, stmt.line, stmt.col, {}, NULL };
                break;
            }
            expr = parse_expr(it, MAX_PRECEDENCE);
            if (expr.type == ERROR_EXPR) goto err;
            // ;
            if (consume_expected_token(it, SEMICOLON)) goto err_free_expr;
            stmt.expr = expr;
            break;
        case BREAK_TOKEN:
            stmt.type = BREAK_STMT;
            stmt.line = (*it)->line;
            stmt.col = (*it)->col;
            (*it)++;
            // ;
            if (consume_expected_token(it, SEMICOLON)) goto err;
            break;
        case CONTINUE_TOKEN:
            stmt.type = CONTINUE_STMT;
            stmt.line = (*it)->line;
            stmt.col = (*it)->col;
            (*it)++;
            // ;
            if (consume_expected_token(it, SEMICOLON)) goto err;
            break;

        default:  // expr
            expr = parse_expr(it, MAX_PRECEDENCE);
            if (expr.type == ERROR_EXPR) goto err;
            // ;
            if (consume_expected_token(it, SEMICOLON)) goto err_free_expr;
            stmt.type = EXPR_STMT;
            stmt.line = expr.line;
            stmt.col = expr.col;
            stmt.expr = expr;
            break;
    }

    return stmt;
err_free_expr:
    free_expr(expr);
err:
    return (Stmt) { .type = ERROR_STMT };
}
