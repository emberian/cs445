global write_integer@
global write_real@
global write_string@
global write_bool@
global write_char@
global write_void@
global write_newline@
global read_integer@
global read_real@
global read_string@
global read_bool@
global read_char@
global read_void@
global read_newline@

extern printf
extern fflush
extern stdout

; Note: hardcodes calling into libc with the sysv abi.
; for printf, which is a vararg-function, we need:
; al = 0, rdi = arg0, rsi = arg1, since our arguments are all in the INTEGER
; class (for now). rax gets clobbered, as the retval.

; We need to save: rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11. The compiler
; should be modified to save these on-demand.

; Calling convention for these: [rsp+8] is the value to be printed. All
; registers are callee-save.

SECTION .data

newline: db 0xA
percent_ld_cstr: db '%ld',0
percent_s_cstr: db '%s',0

SECTION .text

write_integer@:
    push rsi
    mov rsi, [rsp+16]
    push rax
    push rdi
    push rdx
    push rcx
    push r8
    push r9
    push r10
    push r11

    xor rax, rax
    mov rdi, percent_ld_cstr
    call printf
    mov rdi, [stdout]
    call fflush

    pop r11
    pop r10
    pop r9
    pop r8
    pop rcx
    pop rdx
    pop rdi
    pop rax
    pop rsi
    ret

write_newline@:
    push rax ; rax is the retv for sysv.
    push rdi
    push rsi
    push rdx

    mov rax, 1 ; write
    mov rdi, 1 ; stdout
    mov rsi, newline
    mov rdx, 1 ; only writing 1 byte
    syscall

    pop rdx
    pop rsi
    pop rdi
    pop rax
    ret
