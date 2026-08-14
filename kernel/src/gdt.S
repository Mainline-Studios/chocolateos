.intel_syntax noprefix
.global gdt_load
gdt_load:
    lgdt [rdi]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    push 0x08
    lea rax, [rip + 1f]
    push rax
    retfq
1:
    ret
