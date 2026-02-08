#include "typechecker.h"

#include <stdlib.h>
#include <string.h>

#include "printerr.h"

size_t id = 1;

typedef struct SymbolTable SymbolTable;
struct SymbolTable {
    SymbolTable* parent;
    size_t len;
    char** symbols;
    Annot* types;
};

bool typecheck_stmt(Stmt* stmt, SymbolTable* table);

void free_type(Annot type);
void free_type_arrn(Annot* arr, size_t n);
void free_symbol_table(SymbolTable table);
void free_expr_annots(Expr expr);
void free_expr_arrn_annots(Expr* arr, size_t n);
void free_stmt_annots(Stmt stmt);
void free_stmt_arrn_annots(Stmt* arr, size_t n);

Annot lookup_symbol(SymbolTable* table, Token symbol) {
    if (table == NULL) {
        error_line = symbol.line;
        error_col = symbol.col;
        type_error("identifier '%s' is undefined\n", symbol.identifier);
        return (Annot) { ERROR_ANNOT, false, false, {} };
    }
    for (size_t i = 0; i < table->len; i++) {
        if (strcmp(table->symbols[i], symbol.identifier) == 0) {
            if (table->types[i].type == UNDEFINED_ANNOT) {
                error_line = symbol.line;
                error_col = symbol.col;
                type_error("identifier '%s' is undefined\n", symbol.identifier);
                return (Annot) { ERROR_ANNOT, false, false, {} };
            }
            return table->types[i];
        }
    }
    return lookup_symbol(table->parent, symbol);
}

Annot typecheck_atom(Token atom, SymbolTable* table) {
    switch (atom.type) {
        case INT_LITERAL: return (Annot) { LITERAL_TYPE, false, false, {} };
        case CHR_LITERAL: return (Annot) { U8_ANNOT, false, false, {} };
        case STR_LITERAL:
            Annot chr = { U8_ANNOT, false, false, {} };
            Annot str = { ARR_ANNOT, false, false, { .ptr = { &chr, false } } };
            return clone_type(str);
        case IDENTIFIER: return clone_type(lookup_symbol(table, atom));

        default: return (Annot) { ERROR_ANNOT, false, false, {} };
    }
}

Annot typecheck_expr(Expr* expr, SymbolTable* table) {
    Annot type = (Annot) { ERROR_ANNOT, false, false, {} };
    switch (expr->type) {
        case ERROR_EXPR:   return type;
        case NO_EXPR:      type = (Annot) { VOID_ANNOT, false, false, {} }; break;
        case GROUPED_EXPR: type = typecheck_expr(expr->group, table); break;
        case ATOMIC_EXPR:  type = typecheck_atom(expr->atom, table); break;
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

    expr->annotation = malloc(sizeof(Annot));
    if (expr->annotation == NULL) {
        malloc_error();
        free_type(type);
        return (Annot) { ERROR_ANNOT, false, false, {} };
    }
    memcpy(expr->annotation, &type, sizeof(Annot));

    return type;
}

bool typecheck_block(Stmt* stmt, SymbolTable* table) {
    size_t length = 0;
    for (size_t i = 0; i < stmt->block.len; i++) {
        switch (stmt->block.stmts[i].type) {
            case ERROR_STMT: return true;

            case DECL:
            case TYPEDEF:
            case FUNCTION_STMT:
            case STRUCT_STMT:
            case ENUM_STMT:     length++; break;

            default: break;
        }
    }

    SymbolTable scope;
    scope.parent = table;
    scope.len = length;

    if (length) {
        scope.symbols = malloc(sizeof(char*) * length);
        scope.types = malloc(sizeof(Annot) * length);
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
        scope.types[i] = (Annot) { UNDEFINED_ANNOT, false, false, {} };
    }

    length = 0;
    for (size_t i = 0; i < stmt->block.len; i++) {
        Stmt decl = stmt->block.stmts[i];
        switch (decl.type) {
            case DECL:          scope.symbols[length++] = decl.decl.name.str; break;
            case TYPEDEF:       scope.symbols[length++] = decl.typedefdata.name.str; break;
            case FUNCTION_STMT: scope.symbols[length++] = decl.fun.name.str; break;
            case STRUCT_STMT:   scope.symbols[length++] = decl.structdef.name.str; break;
            case ENUM_STMT:     scope.symbols[length++] = decl.enumdef.name.str; break;

            default: break;
        }
    }

    for (size_t i = 0; i < stmt->block.len; i++) {
        if (typecheck_stmt(&stmt->block.stmts[i], &scope)) {
            free_symbol_table(scope);
            free_stmt_arrn_annots(stmt->block.stmts, i);
            return true;
        }
    }

    free_symbol_table(scope);
    return false;
}

bool typecheck_stmt(Stmt* stmt, SymbolTable* table) {
    switch (stmt->type) {
        case ERROR_STMT: return true;
        case NOP:        return false;
        case BLOCK:      return typecheck_block(stmt, table);
        case EXPR_STMT:
            Annot type = typecheck_expr(&stmt->expr, table);
            free_type(type);
            return type.type == ERROR_ANNOT;

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

SymbolTable* typecheck(AST* ast) {
    if (ast == NULL) return true;
    return typecheck_stmt(ast, NULL);
}

void free_type(Annot type) {
    switch (type.type) {
        case ERROR_ANNOT:
        case UNDEFINED_ANNOT:
        case VOID_ANNOT:
        case BOOL_ANNOT:
        case I8_ANNOT:
        case I16_ANNOT:
        case I32_ANNOT:
        case I64_ANNOT:
        case U8_ANNOT:
        case U16_ANNOT:
        case U32_ANNOT:
        case U64_ANNOT:       break;

        case ARR_ANNOT:
        case PTR_ANNOT:
            free_type(*type.ptr.type);
            free(type.ptr.type);
            break;
        case FUN_ANNOT:
            free_type(*type.fun.ret);
            free(type.fun.ret);
            free_type_arrn(type.fun.paramt, type.fun.paramc);
            break;
        case STRUCT_ANNOT:
            free(type.structtype.paramv);
            free_type_arrn(type.structtype.paramt, type.structtype.paramc);
            break;
        case ENUM_ANNOT:     free(type.enumtype.items); break;
        case ENUM_ITEM_TYPE: break;
        case TYPEDEF_ANNOT:
            free_type(*type.typedeftype.type);
            free(type.typedeftype.type);
            break;
    }
}

void free_type_arrn(Annot* arr, size_t n) {
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
        case ERROR_EXPR:   break;
        case NO_EXPR:      break;
        case GROUPED_EXPR: free_expr_annots(*expr.group); break;
        case ATOMIC_EXPR:  break;
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
        case NOP:        break;
        case BLOCK:      free_stmt_arrn_annots(stmt.block.stmts, stmt.block.len); break;
        case EXPR_STMT:  free_expr_annots(stmt.expr); break;
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

Annot clone_type(Annot type) {
    Annot clone = type;
    switch (type.type) {
        case ERROR_ANNOT:
        case UNDEFINED_ANNOT:
        case VOID_ANNOT:
        case BOOL_ANNOT:
        case I8_ANNOT:
        case I16_ANNOT:
        case I32_ANNOT:
        case I64_ANNOT:
        case U8_ANNOT:
        case U16_ANNOT:
        case U32_ANNOT:
        case U64_ANNOT:       return clone;

        case ARR_ANNOT:
        case PTR_ANNOT:
            Annot inner = clone_type(*type.ptr.type);
            if (inner.type == ERROR_ANNOT) return inner;
            clone.ptr.type = malloc(sizeof(Annot));
            if (clone.ptr.type == NULL) {
                malloc_error();
                free_type(inner);
                return (Annot) { ERROR_ANNOT, false, false, {} };
            }
            memcpy(clone.ptr.type, &inner, sizeof(Annot));
            return clone;

        case FUN_ANNOT:
            Annot ret = clone_type(*type.fun.ret);
            if (ret.type == ERROR_ANNOT) return ret;
            clone.fun.ret = malloc(sizeof(Annot));
            clone.fun.paramt = malloc(sizeof(Annot) * type.fun.paramc);
            if (clone.fun.ret == NULL || (type.fun.paramc && clone.fun.paramt == NULL)) {
                malloc_error();
                free_type(ret);
                free(clone.fun.ret);
                free(clone.fun.paramt);
                return (Annot) { ERROR_ANNOT, false, false, {} };
            }
            memcpy(clone.ptr.type, &inner, sizeof(Annot));
            for (size_t i = 0; i < type.fun.paramc; i++) {
                Annot param = clone_type(type.fun.paramt[i]);
                if (param.type == ERROR_ANNOT) {
                    free_type(*clone.fun.ret);
                    free(clone.fun.ret);
                    free_type_arrn(clone.fun.paramt, i);
                    return param;
                }
                memcpy(&clone.fun.paramt[i], &param, sizeof(Annot));
            }
            return clone;

        case STRUCT_ANNOT:
            clone.structtype.paramv = malloc(sizeof(char*) * type.structtype.paramc);
            clone.structtype.paramt = malloc(sizeof(Annot) * type.structtype.paramc);
            if (type.structtype.paramc &&
                (clone.structtype.paramt == NULL || clone.structtype.paramt == NULL))
            {
                malloc_error();
                free(clone.structtype.paramv);
                free(clone.structtype.paramt);
                return (Annot) { ERROR_ANNOT, false, false, {} };
            }
            memcpy(
                clone.structtype.paramv, type.structtype.paramv,
                sizeof(Annot) * type.structtype.paramc
            );
            for (size_t i = 0; i < type.structtype.paramc; i++) {
                Annot param = clone_type(type.structtype.paramt[i]);
                if (param.type == ERROR_ANNOT) {
                    free(clone.structtype.paramv);
                    free_type_arrn(clone.structtype.paramt, i);
                    return param;
                }
                memcpy(&clone.structtype.paramt[i], &param, sizeof(Annot));
            }
            return clone;

        case ENUM_ANNOT:
            clone.enumtype.items = malloc(sizeof(char*) * type.enumtype.len);
            if (type.enumtype.len && clone.enumtype.items == NULL) {
                malloc_error();
                return (Annot) { ERROR_ANNOT, false, false, {} };
            }
            memcpy(clone.enumtype.items, type.enumtype.items, sizeof(char*) * type.enumtype.len);
            return clone;

        case ENUM_ITEM_TYPE: return clone;

        case TYPEDEF_ANNOT:
            Annot value = clone_type(*type.typedeftype.type);
            if (value.type == ERROR_ANNOT) return value;
            clone.typedeftype.type = malloc(sizeof(Annot));
            if (clone.typedeftype.type == NULL) {
                malloc_error();
                free_type(value);
                return (Annot) { ERROR_ANNOT, false, false, {} };
            }
            memcpy(clone.typedeftype.type, &value, sizeof(Annot));
            return clone;
    }

    // unreachable
    return (Annot) { ERROR_ANNOT, false, false, {} };
}
