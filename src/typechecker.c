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
        type_error("identifier '%s' is undefined\n", symbol.identifier);
        return (Type) { ERROR_TYPE, false, false, {} };
    }
    for (size_t i = 0; i < table->len; i++) {
        if (strcmp(table->symbols[i], symbol.identifier) == 0) {
            if (table->types[i].type == UNDEFINED_TYPE) {
                error_line = symbol.line;
                error_col = symbol.col;
                type_error("identifier '%s' is undefined\n", symbol.identifier);
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
            Type str = { ARR_TYPE, false, false, { .ptr = { &chr, false } } };
            return clone_type(str);
        case IDENTIFIER: return clone_type(lookup_symbol(table, atom));

        default: return (Type) { ERROR_TYPE, false, false, {} };
    }
}

Type typecheck_expr(Expr* expr, SymbolTable* table) {
    Type type = (Type) { ERROR_TYPE, false, false, {} };
    switch (expr->type) {
        case ERROR_EXPR:   return type;
        case NO_EXPR:      type = (Type) { VOID_TYPE, false, false, {} }; break;
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
            Type type = typecheck_expr(&stmt->expr, table);
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
        case U64_TYPE:       break;

        case ARR_TYPE:
        case PTR_TYPE:
            free_type(*type.ptr.type);
            free(type.ptr.type);
            break;
        case FUN_TYPE:
            free_type(*type.fun.ret);
            free(type.fun.ret);
            free_type_arrn(type.fun.paramt, type.fun.paramc);
            break;
        case STRUCT_TYPE:
            free(type.structtype.paramv);
            free_type_arrn(type.structtype.paramt, type.structtype.paramc);
            break;
        case ENUM_TYPE:      free(type.enumtype.items); break;
        case ENUM_ITEM_TYPE: break;
        case TYPEDEF_TYPE:
            free_type(*type.typedeftype.type);
            free(type.typedeftype.type);
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
        case U64_TYPE:       return clone;

        case ARR_TYPE:
        case PTR_TYPE:
            Type inner = clone_type(*type.ptr.type);
            if (inner.type == ERROR_TYPE) return inner;
            clone.ptr.type = malloc(sizeof(Type));
            if (clone.ptr.type == NULL) {
                malloc_error();
                free_type(inner);
                return (Type) { ERROR_TYPE, false, false, {} };
            }
            memcpy(clone.ptr.type, &inner, sizeof(Type));
            return clone;

        case FUN_TYPE:
            Type ret = clone_type(*type.fun.ret);
            if (ret.type == ERROR_TYPE) return ret;
            clone.fun.ret = malloc(sizeof(Type));
            clone.fun.paramt = malloc(sizeof(Type) * type.fun.paramc);
            if (clone.fun.ret == NULL || (type.fun.paramc && clone.fun.paramt == NULL)) {
                malloc_error();
                free_type(ret);
                free(clone.fun.ret);
                free(clone.fun.paramt);
                return (Type) { ERROR_TYPE, false, false, {} };
            }
            memcpy(clone.ptr.type, &inner, sizeof(Type));
            for (size_t i = 0; i < type.fun.paramc; i++) {
                Type param = clone_type(type.fun.paramt[i]);
                if (param.type == ERROR_TYPE) {
                    free_type(*clone.fun.ret);
                    free(clone.fun.ret);
                    free_type_arrn(clone.fun.paramt, i);
                    return param;
                }
                memcpy(&clone.fun.paramt[i], &param, sizeof(Type));
            }
            return clone;

        case STRUCT_TYPE:
            clone.structtype.paramv = malloc(sizeof(char*) * type.structtype.paramc);
            clone.structtype.paramt = malloc(sizeof(Type) * type.structtype.paramc);
            if (type.structtype.paramc &&
                (clone.structtype.paramt == NULL || clone.structtype.paramt == NULL))
            {
                malloc_error();
                free(clone.structtype.paramv);
                free(clone.structtype.paramt);
                return (Type) { ERROR_TYPE, false, false, {} };
            }
            memcpy(
                clone.structtype.paramv, type.structtype.paramv,
                sizeof(Type) * type.structtype.paramc
            );
            for (size_t i = 0; i < type.structtype.paramc; i++) {
                Type param = clone_type(type.structtype.paramt[i]);
                if (param.type == ERROR_TYPE) {
                    free(clone.structtype.paramv);
                    free_type_arrn(clone.structtype.paramt, i);
                    return param;
                }
                memcpy(&clone.structtype.paramt[i], &param, sizeof(Type));
            }
            return clone;

        case ENUM_TYPE:
            clone.enumtype.items = malloc(sizeof(char*) * type.enumtype.len);
            if (type.enumtype.len && clone.enumtype.items == NULL) {
                malloc_error();
                return (Type) { ERROR_TYPE, false, false, {} };
            }
            memcpy(clone.enumtype.items, type.enumtype.items, sizeof(char*) * type.enumtype.len);
            return clone;

        case ENUM_ITEM_TYPE: return clone;

        case TYPEDEF_TYPE:
            Type value = clone_type(*type.typedeftype.type);
            if (value.type == ERROR_TYPE) return value;
            clone.typedeftype.type = malloc(sizeof(Type));
            if (clone.typedeftype.type == NULL) {
                malloc_error();
                free_type(value);
                return (Type) { ERROR_TYPE, false, false, {} };
            }
            memcpy(clone.typedeftype.type, &value, sizeof(Type));
            return clone;
    }

    // unreachable
    return (Type) { ERROR_TYPE, false, false, {} };
}
