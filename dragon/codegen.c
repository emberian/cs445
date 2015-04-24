#include "codegen.h"

static void codegen_insn(struct ccx *, struct insn *, bool);

static void codegen_oper(struct ccx *ccx, struct operand operand, bool from_outside) {
    switch (operand.tag) {
        case OPER_ILIT:
            printf("mov rax, %lu\n", operand.ilit);
            break;
        case OPER_BLIT:
            printf("mov rax, %d\n", operand.blit ? 1 : 0);
            break;
        case OPER_REG:
            codegen_insn(ccx, operand.reg, from_outside);
            break;
        case OPER_FLIT:
        case OPER_LABEL:
        case OPER_ARGS:
        case OPER_FUNC:
        case OPER_SYM:
            abort();
            break;
    }
}

static void codegen_insn(struct ccx *ccx, struct insn *i, bool from_outside) {
    printf("; Doing codegen for insn %p\n", i);
    if (from_outside && (i->codegened || i->op == IALLOC)) return;
    i->codegened = true;
    switch (i->op) {
        case IRET:
            printf("mov rax, [rax]\nmov [rbp], rax\\n", ccx->retp_offset);
            break;
        case INOT:
            codegen_oper(ccx, i->a, false);
            printf("not rax\n");
            break;
        case IALLOC:
            printf("mov rax, rbp\nsub rax, %d\n", *(int*)hash_lookup(ccx->alloc_offsets, i));
            break;
        case ILIT:
            printf("mov rax, %lu\n", i->a.ilit);
            break;
        case IPHI:
            break;
        case ISIG:
            break;
        case IBR:
            codegen_oper(ccx, i->a, false);
            printf("cmp rax, 1\nje .L%p\njmp .L%p\n", i->b.label, i->c.label);
            break;
        case ILD:
            if (i->a.tag == OPER_REG && i->a.reg->op == IALLOC) {
                i->a.reg->codegened = true;
                printf("mov rax, [rbp-%d]\n", *(int*)hash_lookup(ccx->alloc_offsets, i->a.reg));
            } else {
                codegen_oper(ccx, i->a, false);
                printf("push [rax]\n");
            }
            break;
        case IADD:
            printf("push rbx\n");
            codegen_oper(ccx, i->a, false);
            printf("mov rbx, rax\n");
            codegen_oper(ccx, i->b, false);
            printf("add rax, rbx\npop rbx\n");
            break;
        case ISUB:
            printf("push rbx\n");
            codegen_oper(ccx, i->a, false);
            printf("mov rbx, rax\n");
            codegen_oper(ccx, i->b, false);
            printf("sub rax, rbx\npop rbx\n");
            break;
        case IMUL:
            printf("push rbx\n");
            codegen_oper(ccx, i->a, false);
            printf("mov rbx, rax\n");
            codegen_oper(ccx, i->b, false);
            printf("imul rax, rbx\npop rbx\n");
            break;
        case IDIV:
            printf("push rdx\n");
            printf("push rbx\n");
            codegen_oper(ccx, i->a, false);
            printf("mov rbx, rax\n");
            codegen_oper(ccx, i->b, false);
            // We want rbx / rax, but idiv computex rdx:rax / ARG; swap them:
            printf("push rax\npush rbx\npop rax\npop rbx\n");
            // Need to sign extend eax into rdx
            printf("cdq\n");
            printf("idiv rbx\n");
            // Result is in rax,  just need to restore rbx and rdx;
            printf("pop rbx\npop rdx\n");
            break;
        case IMOD:
            printf("push rdx\n");
            printf("push rbx\n");
            codegen_oper(ccx, i->a, false);
            printf("mov rbx, rax\n");
            codegen_oper(ccx, i->b, false);
            // We want rbx / rax, but idiv computex rdx:rax / ARG; swap them:
            printf("push rax\npush rbx\npop rax\npop rbx\n");
            // Need to sign extend eax into rdx
            printf("cdq\n");
            printf("idiv rbx\n");
            // Result is in rdx, just need to restore rbx and rdx;
            printf("mov rax, rdx\npop rbx\npop rdx\n");
            break;
        case IAND:
            printf("push rbx\n");
            codegen_oper(ccx, i->a, false);
            printf("mov rbx, rax\n");
            codegen_oper(ccx, i->b, false);
            printf("and rax, rbx\npop rbx\n");
            break;
        case IOR:
            printf("push rbx\n");
            codegen_oper(ccx, i->a, false);
            printf("mov rbx, rax\n");
            codegen_oper(ccx, i->b, false);
            printf("or rax, rbx\npop rbx\n");
            break;
        case IST:
            if (i->a.tag == OPER_REG && i->a.reg->op == IALLOC) {
                i->a.reg->codegened = true;
                codegen_oper(ccx, i->b, false);
                printf("mov [rbp-%d], rax\n", *(int*)hash_lookup(ccx->alloc_offsets, i->a.reg));
            } else {
                printf("push rbx\n");
                codegen_oper(ccx, i->a, false);
                printf("mov rbx, rax\n");
                codegen_oper(ccx, i->b, false);
                printf("mov [rbx], rax\npop rbx\n");
            }
            break;
        case ILT:
            printf("push rbx\n");
            codegen_oper(ccx, i->a, false);
            printf("mov rbx, rax\n");
            codegen_oper(ccx, i->b, false);
            printf("setl rax\npop rbx\n");
            break;
        case ILE:
            printf("push rbx\n");
            codegen_oper(ccx, i->a, false);
            printf("mov rbx, rax\n");
            codegen_oper(ccx, i->b, false);
            printf("setle rax\npop rbx\n");
            break;
        case IGT:
            printf("push rbx\n");
            codegen_oper(ccx, i->a, false);
            printf("mov rbx, rax\n");
            codegen_oper(ccx, i->b, false);
            printf("setg rax\npop rbx\n");
            break;
        case IGE:
            printf("push rbx\n");
            codegen_oper(ccx, i->a, false);
            printf("mov rbx, rax\n");
            codegen_oper(ccx, i->b, false);
            printf("setge rax\npop rbx\n");
            break;
        case IEQ:
            printf("push rbx\n");
            codegen_oper(ccx, i->a, false);
            printf("mov rbx, rax\n");
            codegen_oper(ccx, i->b, false);
            printf("sete rax\npop rbx\n");
            break;
        case INE:
            printf("push rbx\n");
            codegen_oper(ccx, i->a, false);
            printf("mov rbx, rax\n");
            codegen_oper(ccx, i->b, false);
            printf("setne rax\npop rbx\n");
            break;
        case ICALL:
        case IFCALL:
            printf("; do call here\n");
            if (i->b.args == NULL) goto no_args;
            for (int j = 0; j < i->b.args->length; j++) {
                codegen_insn(ccx, i->b.args->data[j], false);
            }
no_args:
            break;
        case ISYMREF:
            printf("mov rax, %s\n", i->a.sym);
            break;
        default:
            fprintf(stderr, "unrecognized insn %d\n", i->op);
    }
}

static void codegen_bb(struct ccx *ccx, struct cir_bb *bb) {
    printf(".L%p:\n", bb);
    /*
    for (int i = bb->insns->length - 1; i >= 0; i--) {
        codegen_insn(ccx, bb->insns->data[i], true);
    }
    */
    for (int i = 0; i < bb->insns->length; i++) {
        codegen_insn(ccx, bb->insns->data[i], true);
    }
}

static void codegen_func(struct acx *acx, struct cir_func *f) {
    printf("func_%p@:\n", f);
    struct ccx ccx;
    ccx.alloc_offsets = hash_new(1 << 8, hash_pointer, compare_pointer, dummy_free, free);

    int offset_from_bp = ABI_POINTER_SIZE; // offset 0 used for return val

    if (f->args == NULL) goto skip_args;
    LFOREACH(void *arg, f->args)
        int *local_offset_from_bp = M(int);
        *local_offset_from_bp = offset_from_bp;
        struct insn *i = STAB_VAR(acx->st, (size_t)arg)->loc;
        offset_from_bp += i->a.ilit;
        hash_insert(ccx.alloc_offsets, i, local_offset_from_bp);
    ENDLFOREACH;

skip_args:
    ccx.retp_offset = offset_from_bp;
    offset_from_bp += ABI_POINTER_SIZE;

    ptrvec_push(f->bbs, f->entry);
    for (int i = 0; i < f->bbs->length; i++) {
        struct ptrvec *insns = ((struct cir_bb*)f->bbs->data[i])->insns;
        for (int j = 0; j < insns->length; j++) {
            struct insn *i = insns->data[j];
            if (i->op == IALLOC) {
                int *local_offset_from_bp = M(int);
                *local_offset_from_bp = offset_from_bp;
                offset_from_bp += i->a.ilit;
                hash_insert(ccx.alloc_offsets, i, local_offset_from_bp);
            }
        }
    }
    ptrvec_pop(f->bbs);
    printf("sub rsp, %d\n", offset_from_bp - ccx.retp_offset + ABI_POINTER_SIZE);

    codegen_bb(&ccx, f->entry);
    for (int i = 0; i < f->bbs->length; i++) {
        codegen_bb(&ccx, f->bbs->data[i]);
    }

    // put the stack pointer back where it belongs,
    printf("add rsp, %d\n", offset_from_bp - ccx.retp_offset + ABI_POINTER_SIZE);
    printf("ret\n");
    hash_free(ccx.alloc_offsets);
}

void codegen(struct acx *acx) {
    printf(";--- ASM LISTING ---\n");
    printf("display@: resq %d\n", acx->disp_offset * ABI_POINTER_ALIGN);
    printf("global _start\n_start:\nmov rbp, rsp\ncall func_%p@\nmov rax, 60\nxor rdi, rdi\nsyscall\n", acx->main);
    codegen_func(acx, acx->main);
    for (int i = 0; i < acx->funcs->length; i++) {
        codegen_func(acx, acx->funcs->data[i]);
    }
}
