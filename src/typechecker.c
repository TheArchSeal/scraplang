#include "typechecker.h"
#include "printerr.h"
#include <stdlib.h>
#include <string.h>

size_t id = 1;

typedef struct SymbolTable SymbolTable;
struct SymbolTable {
    SymbolTable* parent;
    size_t len;
    char** symbols;
    Type* types;
};

bool typecheck_stmt(Stmt* stmt, SymbolTable* table);

void free_type(Type type);
void free_type_arrn(Type* arr, size_t n);
void free_symbol_table(SymbolTable table);
void free_expr_annots(Expr expr);
void free_expr_arrn_annots(Expr* arr, size_t n);
void free_stmt_annots(Stmt stmt);
void free_stmt_arrn_annots(Stmt* arr, size_t n);

Type lookup_symbol(SymbolTable* table, Token symbol) {
    if (table == NULL) {
        error_line = symbol.line;
        error_col = symbol.col;
        type_error("identifier '%s' is undefined\n", symbol.data.var_name);
        return (Type) { ERROR_TYPE, false, false, {} };
    }
    for (size_t i = 0; i < table->len; i++) {
        if (strcmp(table->symbols[i], symbol.data.var_name) == 0) {
            if (table->types[i].type == UNDEFINED_TYPE) {
                error_line = symbol.line;
                error_col = symbol.col;
                type_error("identifier '%s' is undefined\n", symbol.data.var_name);
                return (Type) { ERROR_TYPE, false, false, {} };
            }
            return table->types[i];
        }
    }
    return lookup_symbol(table->parent, symbol);
}

Type typecheck_atom(Token atom, SymbolTable* table) {
    switch (atom.type) {
        case INT_LITERAL: return (Type) { LITERAL_TYPE, false, false, {} };
        case CHR_LITERAL: return (Type) { U8_TYPE, false, false, {} };
        case STR_LITERAL:
            Type chr = { U8_TYPE, false, false, {} };
            Type str = { ARR_TYPE, false, false, {.ptr = {&chr, false}} };
            return clone_type(str);
        case VAR_NAME: return clone_type(lookup_symbol(table, atom));

        default: return (Type) { ERROR_TYPE, false, false, {} };
    }
}

Type typecheck_expr(Expr* expr, SymbolTable* table) {
    Type type = (Type) { ERROR_TYPE, false, false, {} };
    switch (expr->type) {
        case ERROR_EXPR: return type;
        case NO_EXPR: type = (Type) { VOID_TYPE, false, false, {} }; break;
        case GROUPED_EXPR: type = typecheck_expr(expr->data.group, table); break;
        case ATOMIC_EXPR: type = typecheck_atom(expr->data.atom, table); break;
        case ARR_EXPR:
        case LAMBDA_EXPR:
        case UNOP_EXPR:
        case BINOP_EXPR:
        case TERNOP_EXPR:
        case SUBSRIPT_EXPR:
        case CALL_EXPR:
        case CONSTRUCTOR_EXPR:
        case ACCESS_EXPR:
    }

    expr->annotation = malloc(sizeof(Type));
    if (expr->annotation == NULL) {
        malloc_error();
        free_type(type);
        return (Type) { ERROR_TYPE, false, false, {} };
    }
    memcpy(expr->annotation, &type, sizeof(Type));

    return type;
}

bool typecheck_block(Stmt* stmt, SymbolTable* table) {
    size_t length = 0;
    for (size_t i = 0; i < stmt->data.block.len; i++) {
        switch (stmt->data.block.stmts[i].type) {
            case ERROR_STMT: return true;

            case DECL:
            case TYPEDEF:
            case FUNCTION_STMT:
            case STRUCT_STMT:
            case ENUM_STMT:
                length++;
                break;

            default: break;
        }
    }

    SymbolTable scope;
    scope.parent = table;
    scope.len = length;

    if (length) {
        scope.symbols = malloc(sizeof(char*) * length);
        scope.types = malloc(sizeof(Type) * length);
        if (scope.symbols == NULL || scope.types == NULL) {
            malloc_error();
            free(scope.symbols);
            free(scope.types);
            return true;
        }
    } else {
        scope.symbols = NULL;
        scope.types = NULL;
    }

    for (size_t i = 0; i < length; i++) {
        scope.types[i] = (Type) { UNDEFINED_TYPE, false, false, {} };
    }

    length = 0;
    for (size_t i = 0; i < stmt->data.block.len; i++) {
        Stmt decl = stmt->data.block.stmts[i];
        switch (decl.type) {
            case DECL:
                scope.symbols[length++] = decl.data.decl.name.str;
                break;
            case TYPEDEF:
                scope.symbols[length++] = decl.data.type.name.str;
                break;
            case FUNCTION_STMT:
                scope.symbols[length++] = decl.data.fun.name.str;
                break;
            case STRUCT_STMT:
                scope.symbols[length++] = decl.data.structdef.name.str;
                break;
            case ENUM_STMT:
                scope.symbols[length++] = decl.data.enumdef.name.str;
                break;

            default: break;
        }
    }

    for (size_t i = 0; i < stmt->data.block.len; i++) {
        if (typecheck_stmt(&stmt->data.block.stmts[i], &scope)) {
            free_symbol_table(scope);
            free_stmt_arrn_annots(stmt->data.block.stmts, i);
            return true;
        }
    }

    free_symbol_table(scope);
    return false;
}

bool typecheck_stmt(Stmt* stmt, SymbolTable* table) {
    switch (stmt->type) {
        case ERROR_STMT: return true;
        case NOP: return false;
        case BLOCK: return typecheck_block(stmt, table);
        case EXPR_STMT:
            Type type = typecheck_expr(&stmt->data.expr, table);
            free_type(type);
            return type.type == ERROR_TYPE;

        case DECL:
        case TYPEDEF:
        case IFELSE_STMT:
        case SWITCH_STMT:
        case WHILE_STMT:
        case DOWHILE_STMT:
        case FOR_STMT:
        case FUNCTION_STMT:
        case STRUCT_STMT:
        case ENUM_STMT:
        case RETURN_STMT:
        case BREAK_STMT:
        case CONTINUE_STMT:
    }

    // unreachable
    return true;
}

bool typecheck(AST* ast) {
    if (ast == NULL) return true;
    return typecheck_stmt(ast, NULL);
}

void free_type(Type type) {
    switch (type.type) {
        case ERROR_TYPE:
        case UNDEFINED_TYPE:
        case VOID_TYPE:
        case BOOL_TYPE:
        case I8_TYPE:
        case I16_TYPE:
        case I32_TYPE:
        case I64_TYPE:
        case U8_TYPE:
        case U16_TYPE:
        case U32_TYPE:
        case U64_TYPE: break;
        case ARR_TYPE:
        case PTR_TYPE:
            free_type(*type.data.ptr.type);
            free(type.data.ptr.type);
            break;
        case FUN_TYPE:
            free_type(*type.data.fun.ret);
            free(type.data.fun.ret);
            free_type_arrn(type.data.fun.paramt, type.data.fun.paramc);
            break;
        case STRUCT_TYPE:
            free(type.data.structtype.paramv);
            free_type_arrn(type.data.structtype.paramt, type.data.structtype.paramc);
            break;
        case ENUM_TYPE:
            free(type.data.enumtype.items);
            break;
        case ENUM_ITEM_TYPE: break;
        case TYPEDEF_TYPE:
            free_type(*type.data.typedeftype.type);
            free(type.data.typedeftype.type);
            break;
    }
}

void free_type_arrn(Type* arr, size_t n) {
    for (size_t i = 0; i < n; i++) free_type(arr[i]);
    free(arr);
}

void free_symbol_table(SymbolTable table) {
    free(table.symbols);
    free_type_arrn(table.types, table.len);
}

void free_expr_annots(Expr expr) {
    if (expr.annotation) {
        free_type(*expr.annotation);
        free(expr.annotation);
    }
    switch (expr.type) {
        case ERROR_EXPR: break;
        case NO_EXPR: break;
        case GROUPED_EXPR:
            free_expr_annots(*expr.data.group);
            break;
        case ATOMIC_EXPR: break;
        case ARR_EXPR:
        case LAMBDA_EXPR:
        case UNOP_EXPR:
        case BINOP_EXPR:
        case TERNOP_EXPR:
        case SUBSRIPT_EXPR:
        case CALL_EXPR:
        case CONSTRUCTOR_EXPR:
        case ACCESS_EXPR:
    }
}

void free_expr_arrn_annots(Expr* arr, size_t n) {
    for (size_t i = 0; i < n; i++) free_expr_annots(arr[i]);
}

void free_stmt_annots(Stmt stmt) {
    switch (stmt.type) {
        case ERROR_STMT: break;
        case NOP: break;
        case BLOCK:
            free_stmt_arrn_annots(stmt.data.block.stmts, stmt.data.block.len);
            break;
        case EXPR_STMT:
            free_expr_annots(stmt.data.expr);
            break;
        case DECL:
        case TYPEDEF:
        case IFELSE_STMT:
        case SWITCH_STMT:
        case WHILE_STMT:
        case DOWHILE_STMT:
        case FOR_STMT:
        case FUNCTION_STMT:
        case STRUCT_STMT:
        case ENUM_STMT:
        case RETURN_STMT:
        case BREAK_STMT:
        case CONTINUE_STMT:
    }
}

void free_stmt_arrn_annots(Stmt* arr, size_t n) {
    for (size_t i = 0; i < n; i++) free_stmt_annots(arr[i]);
}

void free_typed_ast_p(AST* ast) {
    free_stmt_annots(*ast);
    free_ast_p(ast);
}

Type clone_type(Type type) {
    Type clone = type;
    switch (type.type) {
        case ERROR_TYPE:
        case UNDEFINED_TYPE:
        case VOID_TYPE:
        case BOOL_TYPE:
        case I8_TYPE:
        case I16_TYPE:
        case I32_TYPE:
        case I64_TYPE:
        case U8_TYPE:
        case U16_TYPE:
        case U32_TYPE:
        case U64_TYPE: return clone;

        case ARR_TYPE:
        case PTR_TYPE:
            Type inner = clone_type(*type.data.ptr.type);
            if (inner.type == ERROR_TYPE) return inner;
            clone.data.ptr.type = malloc(sizeof(Type));
            if (clone.data.ptr.type == NULL) {
                malloc_error();
                free_type(inner);
                return (Type) { ERROR_TYPE, false, false, {} };
            }
            memcpy(clone.data.ptr.type, &inner, sizeof(Type));
            return clone;

        case FUN_TYPE:
            Type ret = clone_type(*type.data.fun.ret);
            if (ret.type == ERROR_TYPE) return ret;
            clone.data.fun.ret = malloc(sizeof(Type));
            clone.data.fun.paramt = malloc(sizeof(Type) * type.data.fun.paramc);
            if (clone.data.fun.ret == NULL
                || (type.data.fun.paramc && clone.data.fun.paramt == NULL)
            ) {
                malloc_error();
                free_type(ret);
                free(clone.data.fun.ret);
                free(clone.data.fun.paramt);
                return (Type) { ERROR_TYPE, false, false, {} };
            }
            memcpy(clone.data.ptr.type, &inner, sizeof(Type));
            for (size_t i = 0; i < type.data.fun.paramc; i++) {
                Type param = clone_type(type.data.fun.paramt[i]);
                if (param.type == ERROR_TYPE) {
                    free_type(*clone.data.fun.ret);
                    free(clone.data.fun.ret);
                    free_type_arrn(clone.data.fun.paramt, i);
                    return param;
                }
                memcpy(&clone.data.fun.paramt[i], &param, sizeof(Type));
            }
            return clone;

        case STRUCT_TYPE:
            clone.data.structtype.paramv = malloc(sizeof(char*) * type.data.structtype.paramc);
            clone.data.structtype.paramt = malloc(sizeof(Type) * type.data.structtype.paramc);
            if (type.data.structtype.paramc
                && (clone.data.structtype.paramt == NULL || clone.data.structtype.paramt == NULL)
            ) {
                malloc_error();
                free(clone.data.structtype.paramv);
                free(clone.data.structtype.paramt);
                return (Type) { ERROR_TYPE, false, false, {} };
            }
            memcpy(
                clone.data.structtype.paramv,
                type.data.structtype.paramv,
                sizeof(Type) * type.data.structtype.paramc
            );
            for (size_t i = 0; i < type.data.structtype.paramc; i++) {
                Type param = clone_type(type.data.structtype.paramt[i]);
                if (param.type == ERROR_TYPE) {
                    free(clone.data.structtype.paramv);
                    free_type_arrn(clone.data.structtype.paramt, i);
                    return param;
                }
                memcpy(&clone.data.structtype.paramt[i], &param, sizeof(Type));
            }
            return clone;

        case ENUM_TYPE:
            clone.data.enumtype.items = malloc(sizeof(char*) * type.data.enumtype.len);
            if (type.data.enumtype.len && clone.data.enumtype.items == NULL) {
                malloc_error();
                return (Type) { ERROR_TYPE, false, false, {} };
            }
            memcpy(
                clone.data.enumtype.items,
                type.data.enumtype.items,
                sizeof(char*) * type.data.enumtype.len
            );
            return clone;

        case ENUM_ITEM_TYPE: return clone;

        case TYPEDEF_TYPE:
            Type value = clone_type(*type.data.typedeftype.type);
            if (value.type == ERROR_TYPE) return value;
            clone.data.typedeftype.type = malloc(sizeof(Type));
            if (clone.data.typedeftype.type == NULL) {
                malloc_error();
                free_type(value);
                return (Type) { ERROR_TYPE, false, false, {} };
            }
            memcpy(clone.data.typedeftype.type, &value, sizeof(Type));
            return clone;
    }

    // unreachable
    return (Type) { ERROR_TYPE, false, false, {} };
}
