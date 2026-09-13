; DREAM-OS: точка входа. QEMU/GRUB (multiboot v1) грузит ELF в защищённый
; режим и передаёт управление на _start: eax = magic, ebx = multiboot info.

section .multiboot
align 4
    dd 0x1BADB002              ; magic
    dd 0x00000000              ; flags: ничего специфичного не требуем
    dd -(0x1BADB002)           ; checksum = -(magic + flags)

section .bss
align 16
stack_bottom:
    resb 16384                 ; 16 КБ собственного стека: у bootloader'а
                               ; стек есть, но его размер нам никто не обещал
stack_top:

section .text
global _start
extern kmain
extern __init_array_start
extern __init_array_end

_start:
    cli
    cld
    mov esp, stack_top

    push ebx                   ; аргумент kmain(const void* multiboot_info)

    ; глобальные конструкторы C++: без libc их должен вызвать boot-код
    mov esi, __init_array_start
.ctor_loop:
    cmp esi, __init_array_end
    jae .ctors_done
    call [esi]
    add esi, 4
    jmp .ctor_loop
.ctors_done:

    call kmain

.hang:
    cli
    hlt
    jmp .hang
