#ifndef _AST_H
#define _AST_H

#include "util.h"
#include "parser.tab.h"

enum exprs {
    EXPR_APP,
    EXPR_BIN,
    EXPR_DEREF,
    EXPR_IDX,
    EXPR_LIT,
    EXPR_PATH,
    EXPR_UN,
    EXPR_ADDROF,
};

enum stmts {
    STMT_ASSIGN,
    STMT_FOR,
    STMT_ITE,
    STMT_PROC,
    STMT_STMTS,
    STMT_WDO,
};

enum subprogs {
    SUB_FUNCTION,
    SUB_PROCEDURE,
};

enum types {
    TYPE_ARRAY,
    TYPE_BOOLEAN,
    TYPE_CHAR,
    TYPE_FUNCTION,
    TYPE_INTEGER,
    TYPE_POINTER,
    TYPE_REAL,
    TYPE_RECORD,
    TYPE_REF,
    TYPE_STRING,
    TYPE_VOID,
};

struct ast_type {
    union {
        struct {
            char *lower, *upper;
            struct ast_type *elt_type;
        } array;
        struct {
            enum subprogs type; // func or proc?
            struct list *args; // arg types
            struct ast_type *retty; // ret ty
        } func;
        struct ast_type *pointer;
        char *ref;
        struct list *record;
    };
    enum types tag;
};

/**
 * Declare all of the `names` to have `type`
 */
struct ast_decls {
    struct list *names;
    struct ast_type *type;
};

struct ast_type_decl {
    char *name;
    struct ast_type *type;
};

struct ast_stmt {
    union {
        struct list *stmts;

        struct {
            struct ast_expr *rvalue, *lvalue;
        } assign;

        struct {
            struct ast_expr *cond;
            struct ast_stmt *then, *elze;
        } ite;

        struct {
            struct ast_expr *cond;
            struct ast_stmt *body;
        } wdo;

        struct {
            char *id;
            struct ast_expr *start, *end;
            struct ast_stmt *body;
        } foor;

        struct {
            struct ast_path *name;
            struct list *args;
        } apply;
    };
    enum stmts tag;
};

struct ast_expr {
    union {
        struct ast_path *path;
        char *lit;
        struct ast_expr *addrof;
        struct ast_expr *deref;

        struct {
            struct ast_path *path;
            struct ast_expr *expr;
        } idx;

        struct {
            struct ast_path *name;
            struct list *args;
        } apply;

        struct {
            enum yytokentype op;
            struct ast_expr *expr;
        } unary;

        struct {
            enum yytokentype op;
            struct ast_expr *left, *right;
        } binary;
    };
    enum exprs tag;
};

struct ast_subdecl {
    struct ast_type *head;
    char *name;
    struct list *decls, *subprogs, *types;
    struct ast_stmt *body;
};

struct ast_path {
    struct list *components;
};

struct ast_program {
    char *name;
    struct list *args, *decls, *subprogs, *types;
    struct ast_stmt *body;
};

struct ast_record_field {
    char *name;
    struct ast_type *type;
};

struct ast_decls *ast_decls               ( struct list *, struct ast_type *);
struct ast_subdecl *ast_subprogram_decl   ( struct ast_type *, char *, struct list *, struct list *, struct list *, struct ast_stmt *);
struct ast_path *ast_path                 ( char *);
struct ast_program *ast_program           ( char *, struct list *, struct list *, struct list *, struct list *, struct ast_stmt *);
struct ast_expr *ast_expr                 ( enum exprs, ...);
struct ast_stmt *ast_stmt                 ( enum stmts, ...);
struct ast_type *ast_type                 ( enum types, ...);
struct ast_type_decl *ast_type_decl       ( char *, struct ast_type *);
struct ast_record_field *ast_record_field ( char *, struct ast_type *);

void ast_path_append(struct ast_path *, char *);

void print_expr            ( struct ast_expr *, int);
void print_stmt            ( struct ast_stmt *, int);
void print_type            ( struct ast_type *, int);
void print_decls           ( struct ast_decls *, int);
void print_subprogram_decl ( struct ast_subdecl *, int);
void print_path            ( struct ast_path *, int);
void print_program         ( struct ast_program *, int);
void print_type_decl       ( struct ast_type_decl *, int);
void print_record_field    ( struct ast_record_field *, int);

void free_expr            ( struct ast_expr *);
void free_stmt            ( struct ast_stmt *);
void free_type            ( struct ast_type *);
void free_decls           ( struct ast_decls *);
void free_subprogram_decl ( struct ast_subdecl *);
void free_path            ( struct ast_path *);
void free_program         ( struct ast_program *);
void free_type_decl       ( struct ast_type_decl *);
void free_record_field    ( struct ast_record_field *);

bool is_relop(int);

#endif
