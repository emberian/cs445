#include <stdio.h>
#include "token.h"
#include "util.h"
#include "ast.h"

// number of spaces per indentation level
#define INDSZ 2

#define INDENT do { for (int i = 0; i < indent; i++) putchar(' '); } while(0)

/* pretty printers */

void print_expr(struct ast_expr *e, int indent) {
    INDENT;
    switch (e->tag) {
        case EXPR_APP:
            puts("APPLY:");
            print_path(e->apply.name, indent+INDSZ);
            INDENT; puts("TO ARGS:");
            LFOREACH(struct ast_expr *, arg, e->apply.args, print_expr(arg, indent+INDSZ));
            break;

        case EXPR_BIN:
            printf("BINOP ");
            print_token(e->binary.op, NULL);
            indent += INDSZ;
            INDENT;
            printf("LEFT:\n");
            print_expr(e->binary.left, indent+INDSZ);
            INDENT;
            printf("RIGHT:\n");
            print_expr(e->binary.right, indent+INDSZ);
            indent -= INDSZ;
            break;

        case EXPR_DEREF:
            puts("DEREF:");
            print_expr(e->deref.expr, indent+INDSZ);
            break;

        case EXPR_IDX:
            printf("INDEX `");
            print_path(e->idx.path, 0);
            puts("` WITH:");
            print_expr(e->idx.expr, indent+INDSZ);
            break;

        case EXPR_LIT:
            printf("LIT `%s`\n", e->lit);
            break;

        case EXPR_PATH:
            print_path(e->path, 0);
            break;

        case EXPR_UN:
            printf("UNARY ");
            print_token(e->unary.op, NULL);
            puts(":");
            print_expr(e->unary.expr, indent+INDSZ);
            break;

        default:
            fprintf(stderr, "unrecognized expr node...\n");
            abort();
    }
}

void print_stmt(struct ast_stmt *s, int indent) {
    switch (s->tag) {
        case STMT_ASSIGN:
            INDENT; puts("ASSIGN");
            print_expr(s->assign.lvalue, indent+INDSZ);

            INDENT; puts("TO");
            print_expr(s->assign.rvalue, indent+INDSZ);
            break;

        case STMT_ITE:
            INDENT; puts("IF FOLLOWING IS TRUE:");
            print_expr(s->ite.cond, indent+INDSZ);

            INDENT; puts("THEN:");
            print_stmt(s->ite.then, indent+INDSZ);

            INDENT; puts("OTHERWISE:");
            print_stmt(s->ite.elze, indent+INDSZ);
            break;

        case STMT_STMTS:
            LFOREACH(struct ast_stmt *, stmt, s->stmts, print_stmt(stmt, indent));
            break;

        case STMT_PROC:
            INDENT;
            printf("APPLY:\n");
            print_path(s->apply.name, indent+INDSZ);
            if (s->apply.args) {
                INDENT; puts("WITH:");
                LFOREACH(struct ast_expr *, arg, s->apply.args, print_expr(arg, indent+INDSZ));
            }
            break;

        case STMT_WDO:
            INDENT; puts("WHILE THE FOLLOWING IS TRUE:");
            print_expr(s->wdo.cond, indent+INDSZ);

            INDENT; puts("DO:");
            print_stmt(s->wdo.body, indent+INDSZ);

            break;

        case STMT_FOR:
            INDENT;
            printf("FOR `%s` STARTING AT:\n", s->foor.id);
            print_expr(s->foor.start, indent+INDSZ);
            INDENT; puts("AND GOING TO:");
            print_expr(s->foor.end, indent+INDSZ);
            INDENT; puts("DO:");
            print_stmt(s->foor.body, indent+INDSZ);
            break;

        default:
            fprintf(stderr, "unrecognized stmt node...\n");
            abort();
    }
}

void print_type(struct ast_type *t, int indent) {
    INDENT;
    switch (t->tag) {
        case TYPE_INTEGER:
            puts("INTEGER");
            break;

        case TYPE_REAL:
            puts("REAL");
            break;

        case TYPE_ARRAY:
            printf("ARRAY [`%s` .. `%s`] OF:\n", t->array.lower, t->array.upper);
            print_type(t->array.elts, indent+INDSZ);
            break;

        case TYPE_POINTER:
            puts("POINTER TO:");
            print_type(t->pointer, indent+INDSZ);
            break;

        case TYPE_FUNCTION:
            puts("<function types not supported>");
            break;

        case TYPE_RECORD:
            puts("<record types not supported>");
            break;

        default:
            fprintf(stderr, "unrecognized type node...\n");
            abort();
    }
}

void print_decls(struct ast_decls *d, int indent) {
    INDENT; puts("DECLARATION WITH TYPE");
    print_type(d->type, indent+INDSZ);
    INDENT; puts("OF NAMES:");
    LFOREACH(char *, name, d->names, INDENT; printf("`%s`\n", name););
}

void print_subprogram_head(struct ast_subhead *h, int indent) {
    char *kind = h->type == SUB_FUNCTION ?
        "FUNCTION" : "PROCEDURE";
    INDENT;
    printf("%s `%s` WITH ARGS:\n", kind, h->name);
    LFOREACH(char *, arg, h->args, INDENT; printf("`%s`\n", arg));
    INDENT;
    printf("RETURNING:");
    print_type(h->retty, indent+INDSZ);
    puts("");
}

void print_subprogram_decl(struct ast_subdecl *d, int indent) {
    print_subprogram_head(d->head, indent);

    INDENT; puts("WITH SUBPROGRAMS:");
    LFOREACH(struct ast_subdecl *, sub, d->subprogs, print_subprogram_decl(sub, indent+INDSZ));

    INDENT; puts("AND VARIABLES");
    LFOREACH(struct ast_decls *, decl, d->decls, print_decls(decl, indent+INDSZ));

    INDENT; puts("DOES:");
    print_stmt(d->body, indent+INDSZ);
}

void print_path(struct ast_path *p, int indent) {
    INDENT;
    for (struct node *temp = &p->components->inner; temp; temp = temp->next) {
        printf("%s", temp->elt);
        if (temp->next) {
            putchar('.');
        }
    }
    puts("");
}

void print_program(struct ast_program *p, int indent) {
    INDENT;

    printf("PROGRAM `%s` WITH ARGS:\n", p->name);
    LFOREACH(char *, arg, p->args, INDENT; printf("`%s`\n", arg));

    INDENT; puts("AND VARIABLES:");
    LFOREACH(struct ast_decls *, var, p->decls, print_decls(var, indent+INDSZ));

    INDENT; puts("AND SUBPROGRAMS:");
    LFOREACH(struct ast_subdecl *, sub, p->subprogs, print_subprogram_decl(sub, indent+INDSZ));

    INDENT; puts("DOES:");
    print_stmt(p->body, indent+INDSZ);
}

#undef INDENT

/* destructors */

void free_expr(struct ast_expr *e) {
    switch (e->tag) {
        case EXPR_APP:
            LFOREACH(struct ast_expr *, arg, e->apply.args, free_expr(arg));
            free_path(e->apply.name);
            break;

        case EXPR_BIN:
            free_expr(e->binary.left);
            free_expr(e->binary.right);
            break;

        case EXPR_DEREF:
            free_expr(e->deref.expr);
            break;

        case EXPR_IDX:
            free_path(e->idx.path);
            free_expr(e->idx.expr);
            break;

        case EXPR_LIT:
            free(e->lit);
            break;

        case EXPR_PATH:
            free_path(e->path);
            break;

        case EXPR_UN:
            free_expr(e->unary.expr);
            break;

        default:
            fprintf(stderr, "unrecognized expr node...\n");
            abort();
    }

    free(e);
}

void free_stmt(struct ast_stmt *s) {
    switch (s->tag) {
        case STMT_ASSIGN:
            free_expr(s->assign.lvalue);
            free_expr(s->assign.rvalue);
            break;

        case STMT_ITE:
            free_expr(s->ite.cond);
            free_stmt(s->ite.then);
            free_stmt(s->ite.elze);
            break;

        case STMT_STMTS:
            LFOREACH(struct ast_stmt *, stmt, s->stmts, free_stmt(stmt));
            break;

        case STMT_PROC:
            free_path(s->apply.name);
            if (s->apply.args) {
                LFOREACH(struct ast_expr *, arg, s->apply.args, free_expr(arg));
            }
            break;

        case STMT_WDO:
            free_expr(s->wdo.cond);
            free_stmt(s->wdo.body);
            break;

        case STMT_FOR:
            free(s->foor.id);
            free_expr(s->foor.start);
            free_expr(s->foor.end);
            free_stmt(s->foor.body);
            break;

        default:
            fprintf(stderr, "unrecognized stmt node...\n");
            abort();
    }
    free(s);
}

void free_type(struct ast_type *t) {
    switch (t->tag) {
        case TYPE_INTEGER:
            break;

        case TYPE_REAL:
            break;

        case TYPE_ARRAY:
            free(t->array.lower);
            free(t->array.upper);
            free_type(t->array.elts);
            break;

        case TYPE_POINTER:
            free_type(t->pointer);
            break;

        case TYPE_FUNCTION:
            break;

        case TYPE_RECORD:
            break;

        default:
            fprintf(stderr, "unrecognized type node...\n");
            abort();
    }

    free(t);
}

void free_decls(struct ast_decls *d) {
    free_type(d->type);
    LFOREACH(char *, name, d->names, free(name));
    free(d);
}

void free_subprogram_head(struct ast_subhead *h) {
    free(h->name);
    LFOREACH(char *, arg, h->args, free(arg));
    free_type(h->retty);
    free(h);
}

void free_subprogram_decl(struct ast_subdecl *d) {
    free_subprogram_head(d->head);

    LFOREACH(struct ast_subdecl *, sub, d->subprogs, free_subprogram_decl(sub));
    LFOREACH(struct ast_decls *, decl, d->decls, free_decls(decl));
    free_stmt(d->body);
    free(d);
}

void free_path(struct ast_path *p) {
    LFOREACH(char *, comp, p->components, free(comp));
    free(p);
}

void free_program(struct ast_program *p) {
    free(p->name);
    LFOREACH(char *, arg, p->args, free(arg));
    LFOREACH(struct ast_decls *, var, p->decls, free_decls(var));
    LFOREACH(struct ast_subdecl *, sub, p->subprogs, free_subprogram_decl(sub));
    free_stmt(p->body);
    free(p);
}

/* constructors */

struct ast_expr *ast_expr (enum exprs tag, ...) {
    return NULL;
}

struct ast_stmt *ast_stmt (enum stmts tag, ...) {
    return NULL;
}

struct ast_type *ast_type (enum types tag, ...) {
    return NULL;
}

struct ast_path *ast_path (char *comp) {
    struct ast_path *p = M(struct ast_path);
    p->components = list_new(comp, free);
    return p;
}

void ast_path_append(struct ast_path *p, char *name) {
    list_add(p->components, name);
}

struct ast_program *ast_program(char *name, struct list *args, struct list *decls,
                                 struct list *subprogs, struct ast_stmt *body) {
    struct ast_program *p = M(struct ast_program);
    p->name = name;
    p->args = args;
    p->decls = decls;
    p->subprogs = subprogs;
    p->body = body;
    return p;
}

struct ast_subhead *ast_subprogram_head(enum subprogs t, char *name, struct list *args, struct ast_type *retty) {
    struct ast_subhead *n = M(struct ast_subhead);
    n->type = t;
    n->name = name;
    n->args = args;
    n->retty = retty;
    return n;
}

struct ast_subdecl *ast_subprogram_decl(struct ast_subhead *head, struct list *decls, struct list *subprogs, struct ast_stmt *body) {
    struct ast_subdecl *n = M(struct ast_subdecl);
    n->head = head;
    n->decls = decls;
    n->subprogs = subprogs;
    n->body = body;
    return n;
}

struct ast_decls *ast_decls(struct list *names, struct ast_type *type) {
    struct ast_decls *n = M(struct ast_decls);
    n->names = names;
    n->type = type;
    return n;
}
