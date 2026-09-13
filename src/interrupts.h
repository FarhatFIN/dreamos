// IDT, обработка исключений и IRQ, ремап PIC, PIT. Заглушки на стеке
// собирают кадр формата Registers (см. src/isr.asm).
#pragma once
#include <stdint.h>

struct Registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp_, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, user_esp, ss;
};

extern "C" void gdt_load();

void interrupts_init(); // IDT + PIC + PIT
uint32_t ticks();
