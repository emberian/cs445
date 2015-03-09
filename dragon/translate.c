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
        case ILD:
            r->a = IREG(va_arg(args, struct insn *));
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
        case IST:
        case ILT:
        case ILE:
        case IGT:
        case IGE:
        case IEQ:
        case INE:
            r->a = IREG(va_arg(args, struct operand));
            r->b = IREG(va_arg(args, struct operand));
            break;
        case ICALL:
            r->a = oper_new(OPER_FUNC, va_arg(args, struct cir_func *));
            r->b = oper_new(OPER_ARGS, va_arg(args, struct ptrvec *));
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
    }
    /* most operands are non-owning */
}

void func_print(struct cir_func *f, char *name) {
    int indent = 0;
    printf("CIR_FUNC %p (%s)\n", f, name);
    if (f->args) {
        printf("+ Arguments\n");
        LFOREACH(struct ast_decls *d, f->args)
            print_decls(d, indent+INDSZ);
        ENDLFOREACH;
    }
    printf("+ Body\n");
    bb_print(f->entry, indent);
    for (int i = 0; i < f->bbs->length; i++) {
        bb_print(f->bbs->data[i], indent);
    }
    printf("+ End\n");
}

void bb_print(struct cir_bb *b, int indent) {
    INDENT; printf("Block %p\n", b);
    for (int i = 0; i < b->insns->length; i++) {
        struct insn *in = ((struct insn**)b->insns->data)[i];
        insn_print(in, indent+INDSZ);
    }
}

void insn_print(struct insn *i, int indent) {
    INDENT;
    if (!i) {
        puts("(insn is nil)");
        return;
    }

    switch (i->op) {
        case IRET:
            puts("RET");
            oper_print(i->a, indent+INDSZ);
            break;
        case INOT:
            puts("NOT");
            oper_print(i->a, indent+INDSZ);
            break;
        case IALLOC:
            puts("ALLOC");
            oper_print(i->a, indent+INDSZ);
            break;
        case ILIT:
            puts("LIT");
            oper_print(i->a, indent+INDSZ);
            break;
        case IPHI:
            puts("PHI");
            oper_print(i->a, indent+INDSZ);
            break;
        case ISIG:
            puts("SIG");
            oper_print(i->a, indent+INDSZ);
            break;
        case IBR:
            puts("BR");
            oper_print(i->a, indent+INDSZ);
            oper_print(i->b, indent+INDSZ);
            oper_print(i->c, indent+INDSZ);
            break;
        case ILD:
            puts("LD");
            oper_print(i->a, indent+INDSZ);
            oper_print(i->b, indent+INDSZ);
            break;
        case IADD:
            puts("ADD");
            oper_print(i->a, indent+INDSZ);
            oper_print(i->b, indent+INDSZ);
            break;
        case ISUB:
            puts("SUB");
            oper_print(i->a, indent+INDSZ);
            oper_print(i->b, indent+INDSZ);
            break;
        case IMUL:
            puts("MUL");
            oper_print(i->a, indent+INDSZ);
            oper_print(i->b, indent+INDSZ);
            break;
        case IDIV:
            puts("DIV");
            oper_print(i->a, indent+INDSZ);
            oper_print(i->b, indent+INDSZ);
            break;
        case IMOD:
            puts("MOD");
            oper_print(i->a, indent+INDSZ);
            oper_print(i->b, indent+INDSZ);
            break;
        case IAND:
            puts("AND");
            oper_print(i->a, indent+INDSZ);
            oper_print(i->b, indent+INDSZ);
            break;
        case IOR:
            puts("OR");
            oper_print(i->a, indent+INDSZ);
            oper_print(i->b, indent+INDSZ);
            break;
        case IST:
            puts("ST");
            oper_print(i->a, indent+INDSZ);
            oper_print(i->b, indent+INDSZ);
            break;
        case ILT:
            puts("LT");
            oper_print(i->a, indent+INDSZ);
            oper_print(i->b, indent+INDSZ);
            break;
        case ILE:
            puts("LE");
            oper_print(i->a, indent+INDSZ);
            oper_print(i->b, indent+INDSZ);
            break;
        case IGT:
            puts("GT");
            oper_print(i->a, indent+INDSZ);
            oper_print(i->b, indent+INDSZ);
            break;
        case IGE:
            puts("GE");
            oper_print(i->a, indent+INDSZ);
            oper_print(i->b, indent+INDSZ);
            break;
        case IEQ:
            puts("EQ");
            oper_print(i->a, indent+INDSZ);
            oper_print(i->b, indent+INDSZ);
            break;
        case INE:
            puts("NE");
            oper_print(i->a, indent+INDSZ);
            oper_print(i->b, indent+INDSZ);
            break;
        case ICALL:
            puts("CALL");
            oper_print(i->a, indent+INDSZ);
            oper_print(i->b, indent+INDSZ);
            break;
        default:
            fprintf(stderr, "unrecognized insn %d\n", i->op);
    }
}

void oper_print(struct operand o, int indent) {
    INDENT;
    switch (o.tag) {
        case OPER_ILIT:
            printf("%ld\n", o.ilit);
            break;
        case OPER_BLIT:
            printf("%s\n", o.blit ? "true" : "false");
            break;
        case OPER_FLIT:
            printf("%f\n", o.flit);
            break;
        case OPER_REG:
            printf("<%p>\n", o.reg);
            break;
        case OPER_LABEL:
            printf("L<%p>\n", o.label);
            break;
        case OPER_ARGS:
            printf("<args>\n");
            break;
        case OPER_FUNC:
            printf("CIR_FUNC %p\n", o.func);
            break;
        default:
            printf("unrecognized operand %d\n", o.tag);
    }
}
