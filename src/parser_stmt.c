#include <stdlib.h>
#include <string.h>

#include "parser_common.h"
#include "printerr.h"

Stmt parse_block(const Token** it) {
    Token start = **it;

    // initialize array
    DynArr array = dynarr_create(sizeof(Stmt));
    while (is_statement(it)) {
        // next statement
        Stmt item = parse_stmt(it);
        if (item.type == ERROR_STMT) {
            free_stmt_dynarr(&array);
            return item;
        }

        if (dynarr_append(&array, &item)) {
            malloc_error();
            free_stmt(item);
            free_stmt_dynarr(&array);
            return (Stmt) { ERROR_STMT, 0, 0, {} };
        }
    }

    Stmt stmt;
    stmt.type = BLOCK;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.data.block.len = array.length;
    stmt.data.block.stmts = array.c_arr;
    return stmt;
}

Stmt parse_decl(const Token** it, bool mut) {
    // var x = y;
    // const x: a = y;

    // var or const
    Token start = *(*it)++;

    // variable name
    Token name = **it;
    if (name.type != VAR_NAME) {
        unexpected_token(name);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // optionally : and type specifier
    TypeSpec spec = { INFERRED_SPEC, name.line, name.col, {} };
    if ((*it)->type == COLON) {
        (*it)++;

        spec = parse_type_spec(it);
        if (spec.type == ERROR_SPEC) return (Stmt) { ERROR_STMT, 0, 0, {} };
    }

    // =
    if ((*it)->type != EQ_TOKEN) {
        unexpected_token(**it);
        free_spec(spec);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // value
    Expr val = parse_expr(it, MAX_PRECEDENCE);
    if (val.type == ERROR_EXPR) {
        free_spec(spec);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }

    // ;
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

    // type
    Token start = *(*it)++;

    // variable name
    Token name = **it;
    if (name.type != VAR_NAME) {
        unexpected_token(name);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // =
    if ((*it)->type != EQ_TOKEN) {
        unexpected_token(**it);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // value
    TypeSpec val = parse_type_spec(it);
    if (val.type == ERROR_SPEC) return (Stmt) { ERROR_STMT, 0, 0, {} };

    // ;
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

    // if
    Token start = *(*it)++;

    // (
    if ((*it)->type != LPAREN) {
        unexpected_token(**it);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // condition
    Expr condition = parse_expr(it, MAX_PRECEDENCE);
    if (condition.type == ERROR_EXPR) return (Stmt) { ERROR_STMT, 0, 0, {} };

    // )
    if ((*it)->type != RPAREN) {
        unexpected_token(**it);
        free_expr(condition);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // if branch
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

    // optionally else and branch
    if ((*it)->type == ELSE_TOKEN) {
        (*it)++;

        Stmt on_false = parse_stmt(it);
        if (on_false.type == ERROR_STMT) {
            free_expr(condition);
            free_stmt(on_true);
            return on_false;
        }

        // allocations
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

    // allocations
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

    // switch
    Token start = *(*it)++;

    // (
    if ((*it)->type != LPAREN) {
        unexpected_token(**it);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // expression to switch on
    Expr expr = parse_expr(it, MAX_PRECEDENCE);
    if (expr.type == ERROR_EXPR) return (Stmt) { ERROR_STMT, 0, 0, {} };

    // )
    if ((*it)->type != RPAREN) {
        unexpected_token(**it);
        free_expr(expr);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // {
    if ((*it)->type != LBRACE) {
        unexpected_token(**it);
        free_expr(expr);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    DynArr case_array = dynarr_create(sizeof(Expr));
    DynArr branch_array = dynarr_create(sizeof(Stmt));

    size_t default_index = 0;
    while ((*it)->type != RBRACE) {
        // case or default
        Expr case_value = { NO_EXPR, (*it)->line, (*it)->col, {}, NULL };
        switch ((*it)->type) {
            case CASE_TOKEN:
                (*it)++;
                // label value
                case_value = parse_expr(it, MAX_PRECEDENCE);
                if (case_value.type == ERROR_EXPR) {
                    free_expr(expr);
                    free_expr_dynarr(&case_array);
                    free_stmt_dynarr(&branch_array);
                    return (Stmt) { ERROR_STMT, 0, 0, {} };
                }
                // if default not found, keep default index out of bounds
                if (default_index == case_array.length) default_index++;
                break;
            case DEFAULT_TOKEN:
                // already encountered default
                if (default_index != case_array.length) {
                    error_line = (*it)->line;
                    error_col = (*it)->col;
                    syntax_error("multiple default labels in switch\n");
                    free_expr(expr);
                    free_expr_dynarr(&case_array);
                    free_stmt_dynarr(&branch_array);
                    return (Stmt) { ERROR_STMT, 0, 0, {} };
                }
                (*it)++;
                break;

            default:
                unexpected_token(**it);
                free_expr(expr);
                free_expr_dynarr(&case_array);
                free_stmt_dynarr(&branch_array);
                return (Stmt) { ERROR_STMT, 0, 0, {} };
        }

        // :
        if ((*it)->type != COLON) {
            unexpected_token(**it);
            free_expr(expr);
            free_expr(case_value);
            free_expr_dynarr(&case_array);
            free_stmt_dynarr(&branch_array);
            return (Stmt) { ERROR_STMT, 0, 0, {} };
        }
        (*it)++;

        // case branch
        Stmt branch_value = parse_block(it);
        if (branch_value.type == ERROR_STMT) {
            free_expr(expr);
            free_expr(case_value);
            free_expr_dynarr(&case_array);
            free_stmt_dynarr(&branch_array);
            return (Stmt) { ERROR_STMT, 0, 0, {} };
        }

        if (dynarr_append(&case_array, &case_value) || dynarr_append(&branch_array, &branch_value))
        {
            malloc_error();
            free_expr(expr);
            free_expr(case_value);
            free_stmt(branch_value);
            free_expr_dynarr(&case_array);
            free_stmt_dynarr(&branch_array);
            return (Stmt) { ERROR_STMT, 0, 0, {} };
        }
    }
    (*it)++;

    Stmt stmt;
    stmt.type = SWITCH_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.data.switchcase.expr = expr;
    stmt.data.switchcase.casec = case_array.length;
    stmt.data.switchcase.casev = case_array.c_arr;
    stmt.data.switchcase.branchv = branch_array.c_arr;
    stmt.data.switchcase.defaulti = default_index;
    return stmt;
}

Stmt parse_while(const Token** it) {
    // while (x) f

    // while
    Token start = *(*it)++;

    // (
    if ((*it)->type != LPAREN) {
        unexpected_token(**it);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // condition
    Expr condition = parse_expr(it, MAX_PRECEDENCE);
    if (condition.type == ERROR_EXPR) return (Stmt) { ERROR_STMT, 0, 0, {} };

    // )
    if ((*it)->type != RPAREN) {
        unexpected_token(**it);
        free_expr(condition);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // loop body
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

    // allocations
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

    // do
    Token start = *(*it)++;

    // loop body
    Stmt body = parse_stmt(it);
    if (body.type == ERROR_STMT) return body;

    // while
    if ((*it)->type != WHILE_TOKEN) {
        unexpected_token(**it);
        free_stmt(body);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // (
    if ((*it)->type != LPAREN) {
        unexpected_token(**it);
        free_stmt(body);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // condition
    Expr condition = parse_expr(it, MAX_PRECEDENCE);
    if (condition.type == ERROR_EXPR) {
        free_stmt(body);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }

    // )
    if ((*it)->type != RPAREN) {
        unexpected_token(**it);
        free_stmt(body);
        free_expr(condition);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // ;
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

    // allocations
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

    // for
    Token start = *(*it)++;

    // (
    if ((*it)->type != LPAREN) {
        unexpected_token(**it);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // expr, decl or nop
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

    // middle expression
    Expr condition = { NO_EXPR, (*it)->line, (*it)->col, {}, NULL };
    if ((*it)->type != SEMICOLON) {
        condition = parse_expr(it, MAX_PRECEDENCE);
        if (condition.type == ERROR_EXPR) {
            free_stmt(init);
            return (Stmt) { ERROR_STMT, 0, 0, {} };
        }
    }
    // ;
    if ((*it)->type != SEMICOLON) {
        unexpected_token(**it);
        free_stmt(init);
        free_expr(condition);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // rightmost expression
    Expr expr = { NO_EXPR, (*it)->line, (*it)->col, {}, NULL };
    if ((*it)->type != RPAREN) {
        expr = parse_expr(it, MAX_PRECEDENCE);
        if (expr.type == ERROR_EXPR) {
            free_stmt(init);
            free_expr(condition);
            return (Stmt) { ERROR_STMT, 0, 0, {} };
        }
    }
    // )
    if ((*it)->type != RPAREN) {
        unexpected_token(**it);
        free_stmt(init);
        free_expr(condition);
        free_expr(expr);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // loop body
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

    // allocations
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

    // fn
    Token start = *(*it)++;

    // variable name
    Token name = **it;
    if (name.type != VAR_NAME) {
        unexpected_token(name);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // (
    if ((*it)->type != LPAREN) {
        unexpected_token(**it);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Stmt stmt;
    stmt.type = FUNCTION_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.data.fun.name = name;

    // parameters
    if (parse_params(
            it, &stmt.data.fun.paramc, &stmt.data.fun.optc, &stmt.data.fun.paramv,
            &stmt.data.fun.paramt, &stmt.data.fun.paramd
        ))
    {
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }

    // )
    if ((*it)->type != RPAREN) {
        unexpected_token(**it);
        free(stmt.data.fun.paramv);
        free_spec_arrn(stmt.data.fun.paramt, stmt.data.fun.paramc);
        free_expr_arrn(stmt.data.fun.paramd, stmt.data.fun.paramc);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // optionally : and type specifier
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

    // {
    if ((*it)->type != LBRACE) {
        unexpected_token(**it);
        free(stmt.data.fun.paramv);
        free_spec_arrn(stmt.data.fun.paramt, stmt.data.fun.paramc);
        free_expr_arrn(stmt.data.fun.paramd, stmt.data.fun.paramc);
        free_spec(stmt.data.fun.ret);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // function body
    Stmt body = parse_block(it);
    if (body.type == ERROR_STMT) {
        free(stmt.data.fun.paramv);
        free_spec_arrn(stmt.data.fun.paramt, stmt.data.fun.paramc);
        free_expr_arrn(stmt.data.fun.paramd, stmt.data.fun.paramc);
        free_spec(stmt.data.fun.ret);
        return body;
    }

    // }
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

    // allocations
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

Stmt parse_struct(const Token** it) {
    // struct s {
    //     x: a,
    //     y: b = 1
    // }

    // struct
    Token start = *(*it)++;

    // variable name
    Token name = **it;
    if (name.type != VAR_NAME) {
        unexpected_token(name);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // {
    if ((*it)->type != LBRACE) {
        unexpected_token(**it);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    Stmt stmt;
    stmt.type = STRUCT_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.data.structdef.name = name;

    // members
    if (parse_params(
            it, &stmt.data.structdef.paramc, &stmt.data.structdef.optc, &stmt.data.structdef.paramv,
            &stmt.data.structdef.paramt, &stmt.data.structdef.paramd
        ))
    {
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }

    // }
    if ((*it)->type != RBRACE) {
        unexpected_token(**it);
        free(stmt.data.structdef.paramv);
        free_spec_arrn(stmt.data.structdef.paramt, stmt.data.structdef.paramc);
        free_expr_arrn(stmt.data.structdef.paramd, stmt.data.structdef.paramc);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    return stmt;
}

Stmt parse_enum(const Token** it) {
    // enum e { x, y, z }

    // enum
    Token start = *(*it)++;

    // variable name
    Token name = **it;
    if (name.type != VAR_NAME) {
        unexpected_token(name);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // {
    if ((*it)->type != LBRACE) {
        unexpected_token(**it);
        return (Stmt) { ERROR_STMT, 0, 0, {} };
    }
    (*it)++;

    // initialize array
    DynArr array = dynarr_create(sizeof(Token));

    if ((*it)->type != RBRACE) {
        for (;;) {
            // element in enum
            Token item = **it;
            if (item.type != VAR_NAME) {
                unexpected_token(item);
                dynarr_destroy(&array);
                return (Stmt) { ERROR_STMT, 0, 0, {} };
            }
            (*it)++;

            if (dynarr_append(&array, &item)) {
                malloc_error();
                dynarr_destroy(&array);
                return (Stmt) { ERROR_STMT, 0, 0, {} };
            }

            // comma or closing parenthesis
            if ((*it)->type == COMMA) (*it)++;
            else if ((*it)->type == RBRACE) break;
            else {
                unexpected_token(**it);
                dynarr_destroy(&array);
                return (Stmt) { ERROR_STMT, 0, 0, {} };
            }
        }
    }
    (*it)++;

    Stmt stmt;
    stmt.type = ENUM_STMT;
    stmt.line = start.line;
    stmt.col = start.col;
    stmt.data.enumdef.name = name;
    stmt.data.enumdef.len = array.length;
    stmt.data.enumdef.items = array.c_arr;
    return stmt;
}

Stmt parse_stmt(const Token** it) {
    switch ((*it)->type) {
        Stmt stmt;
        Expr expr;

        case SEMICOLON:
            stmt.type = NOP;
            stmt.line = (*it)->line;
            stmt.col = (*it)->col;
            (*it)++;
            return stmt;

        case VAR_TOKEN:   return parse_decl(it, true);
        case CONST_TOKEN: return parse_decl(it, false);
        case TYPE_TOKEN:  return parse_typedef(it);

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
                stmt.data.expr = (Expr) { NO_EXPR, stmt.line, stmt.col, {}, NULL };
                return stmt;
            }
            expr = parse_expr(it, MAX_PRECEDENCE);
            if (expr.type == ERROR_EXPR) return (Stmt) { ERROR_STMT, 0, 0, {} };
            // ;
            if ((*it)->type != SEMICOLON) {
                unexpected_token(**it);
                free_expr(expr);
                return (Stmt) { ERROR_STMT, 0, 0, {} };
            }
            (*it)++;
            stmt.data.expr = expr;
            return stmt;
        case BREAK_TOKEN:
            stmt.type = BREAK_STMT;
            stmt.line = (*it)->line;
            stmt.col = (*it)->col;
            (*it)++;
            // ;
            if ((*it)->type != SEMICOLON) {
                unexpected_token(**it);
                return (Stmt) { ERROR_STMT, 0, 0, {} };
            }
            (*it)++;
            return stmt;
        case CONTINUE_TOKEN:
            stmt.type = CONTINUE_STMT;
            stmt.line = (*it)->line;
            stmt.col = (*it)->col;
            (*it)++;
            // ;
            if ((*it)->type != SEMICOLON) {
                unexpected_token(**it);
                return (Stmt) { ERROR_STMT, 0, 0, {} };
            }
            (*it)++;
            return stmt;

        default:  // expr
            expr = parse_expr(it, MAX_PRECEDENCE);
            if (expr.type == ERROR_EXPR) return (Stmt) { ERROR_STMT, 0, 0, {} };
            // ;
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
