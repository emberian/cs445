#include "util.h"
#include "symbol.h"
#include "translate.h"
#include "ast.h"

/*
static struct rec_layout only_rec = { 8, 8 };
*/

void free_rec_layout(struct rec_layout *r) { }

struct rec_layout *compute_rec_layout(struct stab *st, struct list *fields) { return NULL; }

struct cir_func *cfunc_new(struct list *args) {
    struct cir_func *r = M(struct cir_func);
    r->args = args;
    r->entry = cir_bb();
    r->bbs = ptrvec_wcap(16, CB bb_free);
    r->name = NULL;
    r->nest_depth = 1;
    return r;
}

struct cir_bb *cir_bb() {
    struct cir_bb *r = M(struct cir_bb);
    r->insns = ptrvec_wcap(16, CB insn_free);
    return r;
}

struct insn *insn_new(enum cir_op op, ...) {
    struct insn *r = M(struct insn);
    va_list args;

    va_start(args, op);

    r->op = op;

    switch (op) {
        case IRET:
        case INOT:
            r->a = IREG(va_arg(args, struct insn *));
            break;
        case ILD:
            r->a = IREG(va_arg(args, struct insn *));
            r->b = ILIT(va_arg(args, int));
            break;
        case IST:
            r->a = IREG(va_arg(args, struct insn *));
            r->b = IREG(va_arg(args, struct insn *));
            r->c = ILIT(va_arg(args, int));
            break;
        case IPHI:
        case ISIG:
            r->a = oper_new(OPER_ARGS, va_arg(args, struct ptrvec *));
            break;
        case IALLOC:
        case ILIT:
            r->a = ILIT(va_arg(args, int64_t));
            break;
        case IBR:
            r->a = IREG(va_arg(args, struct operand));
            r->b = oper_new(OPER_LABEL, va_arg(args, struct operand));
            r->c = oper_new(OPER_LABEL, va_arg(args, struct operand));
            break;
        case IADD:
        case ISUB:
        case IMUL:
        case IDIV:
        case IMOD:
        case IAND:
        case IOR:
        case ILT:
        case ILE:
        case IGT:
        case IGE:
        case IEQ:
        case INE:
            r->a = va_arg(args, struct operand);
            r->b = va_arg(args, struct operand);
            break;
        case ICALL:
            r->a = oper_new(OPER_FUNC, va_arg(args, struct cir_func *));
            r->b = oper_new(OPER_ARGS, va_arg(args, struct ptrvec *));
            break;
        case IFCALL:
            r->a = oper_new(OPER_SYM, va_arg(args, char *));
            r->b = oper_new(OPER_ARGS, va_arg(args, struct ptrvec *));
            break;
        case ISYMREF:
            r->a = oper_new(OPER_SYM, va_arg(args, char *));
            break;
        default:
            fprintf(stderr, "unrecognized insn %d\n", r->op);
            abort();
    }

    return r;
}

struct operand oper_new(enum operand_ty oty, ...) {
    struct operand o;
    o.tag = oty;

    va_list args;
    va_start(args, oty);

    switch (oty) {
        case OPER_ILIT:
            o.ilit = va_arg(args, int64_t);
            break;
        case OPER_BLIT:
            o.blit = va_arg(args, int);
            break;
        case OPER_FLIT:
            o.flit = va_arg(args, double);
            break;
        case OPER_REG:
            o.reg = va_arg(args, struct insn *);
            break;
        case OPER_LABEL:
            o.label = va_arg(args, struct cir_bb *);
            break;
        case OPER_ARGS:
            o.args = va_arg(args, struct ptrvec *);
            break;
        case OPER_FUNC:
            o.func = va_arg(args, struct cir_func *);
            break;
        case OPER_SYM:
            o.sym = va_arg(args, char *);
            break;
        default:
            fprintf(stderr, "unrecognized operand %d\n", o.tag);
    }
    return o;
}

void insn_free(struct insn *i) {
    switch (i->op) {
        case IRET:
        case INOT:
        case IALLOC:
        case ILIT:
        case IPHI:
        case ISIG:
        case ISYMREF:
            oper_free(i->a);
            break;
        case IBR:
            oper_free(i->a);
            oper_free(i->b);
            oper_free(i->c);
            break;
        case ILD:
        case IADD:
        case ISUB:
        case IMUL:
        case IDIV:
        case IMOD:
        case IAND:
        case IOR:
        case IST:
        case ILT:
        case ILE:
        case IGT:
        case IGE:
        case IEQ:
        case INE:
        case ICALL:
        case IFCALL:
            oper_free(i->a);
            oper_free(i->b);
            break;
        default:
            fprintf(stderr, "unrecognized insn %d\n", i->op);
    }
    D(i);
}

void bb_free(struct cir_bb *b) {
    ptrvec_free(b->insns);
    D(b);
}

void func_free(struct cir_func *f) {
    // args is part of the AST, leave it be
    bb_free(f->entry);
    ptrvec_free(f->bbs);
    D(f);
}

void oper_free(struct operand o) {
    if (o.tag == OPER_ARGS) {
        ptrvec_free(o.args);
    } else if (o.tag == OPER_SYM) {
        free(o.sym);
    }
    /* most operands are non-owning */
}

void func_print(struct cir_func *f, char *name) {
    int indent = 0;
    printf("subgraph clusterfunc_%p { shape=box label=\"%s (%d)\" color=black\n", f, name, f->nest_depth);
    if (f->args) {
        /*
        LFOREACH(struct ast_decls *d, f->args)
            printf("decl_%p[label=\"", d);
            LFOREACH(char *name, d->names)
                printf("%s,", name);
            ENDLFOREACH;
            printf("\b : ");
            print_type(d->type, 0);
            printf("\"];\n");
        ENDLFOREACH;
        */
    }
    bb_print(f->entry, indent);
    printf("start_%p[label=\"start\" shape=parallelogram]; "
           "start_%p -> clusterblock_%p_DUMMY[lhead=clusterblock_%p];",
           f, f, f->entry, f->entry);
    for (int i = 0; i < f->bbs->length; i++) {
        bb_print(f->bbs->data[i], indent);
    }
    printf("}\n");
}

static void oper_connect(struct insn *i, char *slot, struct operand o) {
    switch (o.tag) {
        case OPER_ILIT:
        case OPER_BLIT:
        case OPER_FLIT:
        case OPER_SYM:
        case OPER_FUNC:
            break;
        case OPER_REG:
            if (o.reg) {
                printf("insn_%p:%s -> insn_%p[style=dotted];\n", i, slot, o.reg);
            }
            break;
        case OPER_LABEL:
            if (o.label) {
                printf("insn_%p:%s -> clusterblock_%p_DUMMY [style=dashed lhead=clusterblock_%p];\n", i, slot, o.label, o.label);
            }
            break;
        case OPER_ARGS:
            if (o.args == NULL) break;
            for (int j = 0; j < o.args->length; j++) {
                printf("insn_%p:f%d -> insn_%p[style=dotted];\n", i, 3+j, ((struct resu*)o.args->data[j])->op);
            }
            break;
        default:
            break;
    }
}

static void insn_print_connection(struct insn *i, int indent) {
    if (!i) {
        return;
    }

    bool print_a = true, print_b = false, print_c = false;

    switch (i->op) {
        case IRET:
        case INOT:
        case IALLOC:
        case ILIT:
        case IPHI:
        case ISIG:
        case ISYMREF:
            break;
        case IBR:
        case IST:
            print_b = true;
            print_c = true;
            break;
        case ILD:
        case IADD:
        case ISUB:
        case IMUL:
        case IDIV:
        case IMOD:
        case IAND:
        case IOR:
        case ILT:
        case ILE:
        case IGT:
        case IGE:
        case IEQ:
        case INE:
        case ICALL:
        case IFCALL:
            print_b = true;
            break;
        default:
            fprintf(stderr, "unrecognized insn %d\n", i->op);
    }

    if (print_a) {
        oper_connect(i, "f1", i->a);
    }
    if (print_b) {
        oper_connect(i, "f2", i->b);
    }
    if (print_c) {
        oper_connect(i, "f3", i->c);
    }
}

void bb_print(struct cir_bb *b, int indent) {
    printf("subgraph clusterblock_%p { shape=box label=\"BB %p\"\n", b, b);
    printf("clusterblock_%p_DUMMY[shape=point style=invis];\n", b);
    struct insn *prev = NULL;
    for (int i = 0; i < b->insns->length; i++) {
        struct insn *in = ((struct insn**)b->insns->data)[i];
        insn_print(in, indent+INDSZ);
        if (prev) {
            printf("insn_%p -> insn_%p;\n", prev, in);
        }
        prev = in;
    }
    puts("}");
    for (int i = 0; i < b->insns->length; i++) {
        struct insn *in = ((struct insn**)b->insns->data)[i];
        insn_print_connection(in, indent+INDSZ);
    }
}

void insn_print(struct insn *i, int indent) {
    printf("insn_%p[shape=record label=\"", i);
    if (!i) {
        printf("nil?\"];");
        return;
    }

    bool print_a = true, print_b = false, print_c = false;
    bool print_args = false;

    printf("<f0>");
    switch (i->op) {
        case IRET:
            printf("RET");
            break;
        case INOT:
            printf("NOT");
            break;
        case IALLOC:
            printf("ALLOC");
            break;
        case ILIT:
            printf("LIT");
            break;
        case IPHI:
            printf("PHI");
            break;
        case ISIG:
            printf("SIG");
            break;
        case IBR:
            printf("BR");
            print_b = true;
            print_c = true;
            break;
        case ILD:
            printf("LD");
            print_b = true;
            break;
        case IADD:
            printf("ADD");
            print_b = true;
            break;
        case ISUB:
            printf("SUB");
            print_b = true;
            break;
        case IMUL:
            printf("MUL");
            print_b = true;
            break;
        case IDIV:
            printf("DIV");
            print_b = true;
            break;
        case IMOD:
            printf("MOD");
            print_b = true;
            break;
        case IAND:
            printf("AND");
            print_b = true;
            break;
        case IOR:
            printf("OR");
            print_b = true;
            break;
        case IST:
            printf("ST");
            print_b = true;
            print_c = true;
            break;
        case ILT:
            printf("LT");
            print_b = true;
            break;
        case ILE:
            printf("LE");
            print_b = true;
            break;
        case IGT:
            printf("GT");
            print_b = true;
            break;
        case IGE:
            printf("GE");
            print_b = true;
            break;
        case IEQ:
            printf("EQ");
            print_b = true;
            break;
        case INE:
            printf("NE");
            print_b = true;
            break;
        case ICALL:
            printf("CALL");
            print_b = true;
            print_args = true;
            break;
        case IFCALL:
            printf("FCALL");
            print_b = true;
            print_args = true;
            break;
        case ISYMREF:
            printf("SYMREF");
            break;
        default:
            fprintf(stderr, "unrecognized insn %d\n", i->op);
    }

    if (print_a) {
        printf("|<f1>");
        oper_print(i->a, 0);
        if (print_b) {
            if (i->b.tag != OPER_ARGS) printf("|<f2>");
            oper_print(i->b, 0);
            if (print_c) {
                printf("|<f3>");
                oper_print(i->c, 0);
            }
        }
    }

    printf("\"];\n");

    if (print_args) {
        if (i->b.args == NULL) return;
        for (int j = 0; j < i->b.args->length; j++) {
            insn_print(((struct resu*)i->b.args->data[j])->op, indent);
        }
    }
}

void oper_print(struct operand o, int indent) {
    INDENT;
    switch (o.tag) {
        case OPER_ILIT:
            printf("%ld", o.ilit);
            break;
        case OPER_BLIT:
            printf("%s", o.blit ? "true" : "false");
            break;
        case OPER_FLIT:
            printf("%f", o.flit);
        case OPER_REG:
            if (o.reg != NULL) printf(".");
            break;
        case OPER_LABEL:
            if (o.label != NULL) printf(".");
            break;
        case OPER_FUNC:
            printf("fn %s", o.func->name);
            break;
        case OPER_ARGS:
            if (o.args == NULL) break;
            for (int i = 0; i < o.args->length; i++) {
                printf("|<f%d>.", 3 + i);
            }
            break;
        case OPER_SYM:
            printf("sym %s", o.sym);
            break;
        default:
            printf("unrecognized operand %d", o.tag);
    }
}
