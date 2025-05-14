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
        case ERROR_TOKEN: return syntax_error("unexpected error\n");
        case EOF_TOKEN: return syntax_error("unexpected end of file\n");
        case CHR_LITERAL:
        case STR_LITERAL: return syntax_error("unexpected token %s\n", token.str);
        default: return syntax_error("unexpected token '%s'\n", token.str);
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

// Whether precedence is right-to-left associative
bool operator_rtl_associative(size_t precedence) {
    return precedence == 11
        || precedence == 12;
}

TypeSpec parse_fun_spec(const Token** it) {
    // (a, b?) => c

    TypeSpec err = { ERROR_SPEC };

    // opening parenthesis
    Token start = **it;
    if (start.type != LPAREN) {
        unexpected_token(start);
        return err;
    }
    (*it)++;

    // initialize array
    size_t length = 0, capacity = 1;
    TypeSpec* array = malloc(sizeof(TypeSpec) * capacity);
    if (array == NULL) {
        malloc_error();
        return err;
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
                return err;
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
            syntax_error("non-optional parameter after optional parameter");
            free_spec_arrn(array, length);
            return err;
        }

        // comma or closing parenthesis
        if ((*it)->type == COMMA) (*it)++;
        else if ((*it)->type == RPAREN) break;
        else {
            unexpected_token(**it);
            free_spec_arrn(array, length);
            return err;
        }
    }
    (*it)++;

    // => token
    if ((*it)->type != DARROW) {
        unexpected_token(**it);
        free_spec_arrn(array, length);
        return err;
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
        return err;
    }
    memcpy(spec.data.fun.ret, &ret, sizeof(TypeSpec));

    return spec;
}

TypeSpec handle_type_spec_mod(TypeSpecEnum type, bool mut, const Token** it, TypeSpec base) {
    Token start = *(*it)++;

    TypeSpec spec;
    spec.type = type;
    spec.line = start.line;
    spec.col = start.col;
    spec.data.ptr.mutable = mut;

    spec.data.ptr.spec = malloc(sizeof(TypeSpec));
    if (spec.data.ptr.spec == NULL) {
        malloc_error();
        return (TypeSpec) { ERROR_SPEC };
    }
    memcpy(spec.data.ptr.spec, &base, sizeof(TypeSpec));

    // may have another modification
    TypeSpec next = parse_type_spec_mod(it, spec);
    if (next.type == ERROR_SPEC) {
        free_spec(spec);
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
                    return (TypeSpec) { ERROR_SPEC };
            }

        default: return base; // no modifier
    }
}

TypeSpec parse_type_spec(const Token** it) {
    TypeSpec spec;
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
            // try function type specifier
            const Token* branch = *it;
            error_suppress++;
            spec = parse_fun_spec(it);
            error_suppress--;
            if (spec.type == ERROR_SPEC) {
                if (error_indicator) return spec;
                *it = branch;

                // else type specifier grouped by parentheses
                (*it)++;
                spec = parse_type_spec(it);
                if (spec.type == ERROR_SPEC) return spec;

                if ((*it)->type != RPAREN) {
                    unexpected_token(**it);
                    free_spec(spec);
                    return (TypeSpec) { ERROR_SPEC };
                }
                (*it)++;
            }
            return parse_type_spec_mod(it, spec);

        default:
            unexpected_token(token);
            return (TypeSpec) { ERROR_SPEC };
    }
}

Expr parse_array_literal(const Token** it) {
    // [x, y, z]

    Token start = *(*it)++;

    // initialize array
    size_t length = 0, capacity = 1;
    Expr* array = malloc(sizeof(Expr) * capacity);
    if (array == NULL) {
        malloc_error();
        return (Expr) { ERROR_EXPR };
    }

    if ((*it)->type != RBRACKET) for (;;) {
        // expand array when out of capacity
        if (length >= capacity) {
            capacity *= 2;
            Expr* new_array = realloc(array, sizeof(Expr) * capacity);
            if (new_array == NULL) {
                malloc_error();
                free_expr_arrn(array, length);
                return (Expr) { ERROR_EXPR };
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
            return (Expr) { ERROR_EXPR };
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

    Expr err = { ERROR_EXPR };

    // opening parenthesis
    Token start = **it;
    if (start.type != LPAREN) {
        unexpected_token(start);
        return err;
    }
    (*it)++;

    // initialize arrays
    size_t length = 0, capacity = 1;
    Token* name_array = malloc(sizeof(Token) * capacity);
    TypeSpec* spec_array = malloc(sizeof(TypeSpec) * capacity);
    if (name_array == NULL || spec_array == NULL) {
        malloc_error();
        free(name_array);
        free(spec_array);
        return err;
    }

    if ((*it)->type != RPAREN) for (;;) {
        // expand arrays when out of capacity
        if (length >= capacity) {
            capacity *= 2;
            Token* new_name_array = realloc(name_array, sizeof(Token) * capacity);
            if (new_name_array == NULL) {
                malloc_error();
                free(name_array);
                free_spec_arrn(spec_array, length);
                return err;
            }
            name_array = new_name_array;
            TypeSpec* new_spec_array = realloc(spec_array, sizeof(TypeSpec) * capacity);
            if (new_spec_array == NULL) {
                malloc_error();
                free(name_array);
                free_spec_arrn(spec_array, length);
                return err;
            }
            spec_array = new_spec_array;
        }

        // next parameter name
        Token name = **it;
        if (name.type != VAR_NAME) {
            unexpected_token(**it);
            free(name_array);
            free_spec_arrn(spec_array, length);
            return err;
        }
        (*it)++;

        // optional parameter type specifier
        TypeSpec spec = { INFERRED_SPEC, name.line, name.col, {} };
        if ((*it)->type == COLON) {
            (*it)++;

            spec = parse_type_spec(it);
            if (spec.type == ERROR_SPEC) {
                free(name_array);
                free_spec_arrn(spec_array, length);
                return err;
            }
        }

        // push to arrays
        memcpy(&name_array[length], &name, sizeof(Token));
        memcpy(&spec_array[length], &spec, sizeof(TypeSpec));
        length++;

        // comma or closing parenthesis
        if ((*it)->type == COMMA) (*it)++;
        else if ((*it)->type == RPAREN) break;
        else {
            unexpected_token(**it);
            free(name_array);
            free_spec_arrn(spec_array, length);
            return err;
        }
    }
    (*it)++;

    // optional return type specifier
    TypeSpec ret = { INFERRED_SPEC, start.line, start.col, {} };
    if ((*it)->type == COLON) {
        (*it)++;

        ret = parse_type_spec(it);
        if (ret.type == ERROR_SPEC) {
            free(name_array);
            free_spec_arrn(spec_array, length);
            return err;
        }
    }

    // => token
    if ((*it)->type != DARROW) {
        unexpected_token(**it);
        free(name_array);
        free_spec_arrn(spec_array, length);
        free_spec(ret);
        return err;
    }
    (*it)++;

    // lambda body
    Expr body = parse_expr(it, MAX_PRECEDENCE);
    if (body.type == ERROR_EXPR) {
        free(name_array);
        free_spec_arrn(spec_array, length);
        free_spec(ret);
        return body;
    }

    Expr expr;
    expr.type = LAMBDA_EXPR;
    expr.line = start.line;
    expr.col = start.col;
    expr.data.lambda.paramc = length;
    expr.data.lambda.paramv = name_array;
    expr.data.lambda.paramt = spec_array;

    expr.data.lambda.expr = malloc(sizeof(Expr));
    expr.data.lambda.ret = malloc(sizeof(TypeSpec));
    if (expr.data.lambda.expr == NULL || expr.data.lambda.ret == NULL) {
        malloc_error();
        free(name_array);
        free_spec_arrn(spec_array, length);
        free_spec(ret);
        free_expr(body);
        free(expr.data.lambda.expr);
        free(expr.data.lambda.ret);
    }
    memcpy(expr.data.lambda.expr, &body, sizeof(Expr));
    memcpy(expr.data.lambda.ret, &ret, sizeof(TypeSpec));

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
        return (Expr) { ERROR_EXPR };
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
    expr.data.op.token = token;
    expr.data.op.type = type;

    expr.data.op.first = malloc(sizeof(Expr));
    if (expr.data.op.first == NULL) {
        malloc_error();
        free_expr(term);
        return (Expr) { ERROR_EXPR };
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
            // try lambda expression
            const Token* branch = *it;
            error_suppress++;
            Expr expr = parse_lambda(it);
            error_suppress--;
            if (expr.type == ERROR_EXPR) {
                if (error_indicator) return expr;
                *it = branch;

                // else expression grouped by parentheses
                (*it)++;
                expr = parse_expr(it, MAX_PRECEDENCE);
                if (expr.type == ERROR_EXPR) return expr;

                if ((*it)->type != RPAREN) {
                    unexpected_token(**it);
                    free_expr(expr);
                    return (Expr) { ERROR_EXPR };
                }
                (*it)++;
            }
            return parse_postfix(it, expr);

        default:
            unexpected_token(**it);
            return (Expr) { ERROR_EXPR };
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
                return (Expr) { ERROR_EXPR };
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
            return (Expr) { ERROR_EXPR };
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
                return (Expr) { ERROR_EXPR };
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
            return (Stmt) { ERROR_STMT };
    }
    (*it)++;

    Token name = **it;
    if (name.type != VAR_NAME) {
        unexpected_token(**it);
        return (Stmt) { ERROR_STMT };
    }
    (*it)++;

    TypeSpec spec = { INFERRED_SPEC, name.line, name.col, {} };
    if ((*it)->type == COLON) {
        (*it)++;

        spec = parse_type_spec(it);
        if (spec.type == ERROR_SPEC) return (Stmt) { ERROR_STMT };
    }

    if ((*it)->type != EQ_TOKEN) {
        unexpected_token(**it);
        free_spec(spec);
        return (Stmt) { ERROR_STMT };
    }
    (*it)++;

    Expr val = parse_expr(it, MAX_PRECEDENCE);
    if (val.type == ERROR_EXPR) return (Stmt) { ERROR_STMT };

    if ((*it)->type != SEMICOLON) {
        unexpected_token(**it);
        free_spec(spec);
        free_expr(val);
        return (Stmt) { ERROR_STMT };
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

Stmt parse_stmt(const Token** it) {
    Stmt stmt;
    switch ((*it)->type) {
        case SEMICOLON: // ;
            stmt.type = NOP;
            stmt.line = (*it)->line;
            stmt.col = (*it)->col;
            (*it)++;
            return stmt;

        case VAR_TOKEN:
        case CONST_TOKEN: return parse_decl(it);

        default: // expr ;
            Expr expr = parse_expr(it, MAX_PRECEDENCE);
            if (expr.type == ERROR_EXPR) return (Stmt) { ERROR_STMT };
            // terminating semicolon
            if ((*it)->type != SEMICOLON) {
                unexpected_token(**it);
                free_expr(expr);
                return (Stmt) { ERROR_STMT };
            }
            (*it)++;
            stmt.type = EXPR_STMT;
            stmt.line = expr.line;
            stmt.col = expr.col;
            stmt.data.expr = expr;
            return stmt;
    }
}

Stmt parse_block(const Token** it, TokenEnum end) {
    // stmt ...

    Token start = **it;
    // initialize array
    size_t length = 0, capacity = 1;
    Stmt* array = malloc(sizeof(Stmt) * capacity);
    if (array == NULL) {
        malloc_error();
        return (Stmt) { ERROR_STMT };
    }

    while ((*it)->type != end) {
        // expand array when out of capacity
        if (length >= capacity) {
            capacity *= 2;
            Stmt* new_array = realloc(array, sizeof(Stmt) * capacity);
            if (new_array == NULL) {
                malloc_error();
                free_stmt_arrn(array, length);
                return (Stmt) { ERROR_STMT };
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

// Parse EOF_TOKEN terminated token array.
// Result is not tagged.
// Returns NULL if an error occurred.
AST* parse(const Token* program) {
    if (program == NULL) return NULL;

    const Token** it = &program;
    Stmt stmt = parse_block(it, EOF_TOKEN);
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
        case ATOMIC_EXPR: break;

        case ARR_EXPR:
            free_expr_arrn(expr.data.arr.items, expr.data.arr.len);
            break;
        case LAMBDA_EXPR: break;
            free(expr.data.lambda.paramv);
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
