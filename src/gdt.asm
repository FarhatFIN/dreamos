; GDT: null + flat-сегменты кода и данных. Ядро работает в ring 0,
; единственное кольцо — страницы пока не включаем, ограничений нет.

section .rodata
align 8
gdt_start:
    dq 0x0000000000000000        ; 0x00: обязательный null-дескриптор
    dq 0x00CF9A000000FFFF        ; 0x08: код, base 0, limit 4G, 32-bit, RX
    dq 0x00CF92000000FFFF        ; 0x10: данные, base 0, limit 4G, RW
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

global gdt_load
gdt_load:
    lgdt [gdt_descriptor]
    jmp 0x08:.reload             ; far jump: единственный способ перезагрузить CS
.reload:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret
