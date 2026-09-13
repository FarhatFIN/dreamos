#include "interrupts.h"
#include "io.h"
#include "keyboard.h"
#include "lib.h"
#include "vga.h"

namespace {
constexpr uint16_t PIC1_CMD = 0x20, PIC1_DATA = 0x21;
constexpr uint16_t PIC2_CMD = 0xA0, PIC2_DATA = 0xA1;

volatile uint32_t tick_count = 0;

struct Gate {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

Gate idt[256];

struct IdtPointer {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

IdtPointer idt_pointer;

Gate make_gate(uint32_t offset, uint16_t selector, uint8_t type_attr) {
    Gate gate;
    gate.offset_low = (uint16_t)(offset & 0xFFFF);
    gate.selector = selector;
    gate.zero = 0;
    gate.type_attr = type_attr;
    gate.offset_high = (uint16_t)(offset >> 16);
    return gate;
}

const char* const exception_names[32] = {
    "Divide Error (#DE)",         "Debug",
    "NMI",                        "Breakpoint",
    "Overflow (#OF)",             "BOUND Range Exceeded",
    "Invalid Opcode (#UD)",       "Device Not Available (#NM)",
    "Double Fault (#DF)",         "Coprocessor Segment Overrun",
    "Invalid TSS (#TS)",          "Segment Not Present (#NP)",
    "Stack Fault (#SS)",          "General Protection (#GP)",
    "Page Fault (#PF)",           "reserved",
    "x87 FPU Error (#MF)",        "Alignment Check (#AC)",
    "Machine Check (#MC)",        "SIMD FP Exception (#XM)",
    "Virtualization (#VE)",       "Control Protection (#CP)",
    "reserved",                   "reserved",
    "reserved",                   "reserved",
    "reserved",                   "reserved",
    "reserved",                   "reserved",
    "reserved",                   "reserved",
};

void pic_init() {
    // ICW1: начать инициализацию, каскад 2x8259
    outb(PIC1_CMD, 0x11);
    io_wait();
    outb(PIC2_CMD, 0x11);
    io_wait();
    // ICW2: векторы 0x20..0x27 и 0x28..0x2F (по умолчанию PIC конфликтует
    // с CPU-исключениями 0..15, поэтому его сдвигаем)
    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();
    // ICW3: slave висит на IRQ2
    outb(PIC1_DATA, 0x04);
    io_wait();
    outb(PIC2_DATA, 0x02);
    io_wait();
    // ICW4: 8086 mode
    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();
    // маски: открыты IRQ0 (таймер), IRQ1 (клавиатура) и IRQ2 (каскад)
    outb(PIC1_DATA, 0xF8);
    outb(PIC2_DATA, 0xFF);
}

void pit_init() {
    constexpr uint32_t BASE_HZ = 1193182;
    constexpr uint32_t TARGET_HZ = 100;
    uint16_t divisor = (uint16_t)(BASE_HZ / TARGET_HZ);
    outb(0x43, 0x36); // канал 0, lo/hi байты, mode 3 (square wave)
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)(divisor >> 8));
}

} // namespace

// заглушки из src/isr.asm — все extern "C" на уровне файла
#define FOR_EACH_ISR(X) \
    X(0) X(1) X(2) X(3) X(4) X(5) X(6) X(7) X(8) X(9) X(10) X(11) X(12) \
    X(13) X(14) X(15) X(16) X(17) X(18) X(19) X(20) X(21) X(22) X(23) \
    X(24) X(25) X(26) X(27) X(28) X(29) X(30) X(31)
#define FOR_EACH_IRQ(X) \
    X(0) X(1) X(2) X(3) X(4) X(5) X(6) X(7) X(8) X(9) X(10) X(11) X(12) \
    X(13) X(14) X(15)

extern "C" {
#define DECLARE_ISR(n) void isr##n();
FOR_EACH_ISR(DECLARE_ISR)
#define DECLARE_IRQ(n) void irq##n();
FOR_EACH_IRQ(DECLARE_IRQ)
}

uint32_t ticks() {
    return tick_count;
}

extern "C" void isr_handler(Registers* regs) {
    if (regs->int_no == 0) {
        // #DE от триггера в шелле: код инструкции div — ровно 2 байта
        // (F7 /6), поэтому безопасно перепрыгнуть её через eip += 2.
        // Произвольное #DE так лечить нельзя — в общем случае выводим
        // и останавливаемся.
        vga::set_color(vga::Color::LightRed, vga::Color::Black);
        vga::puts("\n[CPU] exception #DE (divide by zero) at eip=0x");
        print_hex32(regs->eip);
        vga::puts(" - skipping the faulting instruction\n");
        vga::set_color(vga::Color::LightGray, vga::Color::Black);
        regs->eip += 2;
        return;
    }

    vga::set_color(vga::Color::LightRed, vga::Color::Black);
    vga::puts("\n[CPU] unhandled exception ");
    print_u32_dec(regs->int_no);
    vga::puts(" (");
    vga::puts(exception_names[regs->int_no & 31]);
    vga::puts("), err=0x");
    print_hex32(regs->err_code);
    vga::puts(", eip=0x");
    print_hex32(regs->eip);
    vga::puts("\nsystem halted\n");
    vga::set_color(vga::Color::LightGray, vga::Color::Black);
    for (;;)
        hlt();
}

extern "C" void irq_handler(Registers* regs) {
    if (regs->int_no >= 40)
        outb(PIC2_CMD, 0x20); // EOI slave
    outb(PIC1_CMD, 0x20);     // EOI master

    switch (regs->int_no) {
    case 32: // IRQ0: таймер
        tick_count++;
        break;
    case 33: // IRQ1: клавиатура, scancode надо забрать до выхода
        keyboard::push_scancode(inb(0x60));
        break;
    default:
        break;
    }
}

void interrupts_init() {
    tick_count = 0;

    // объявления и заполнение таблицы — macro-петлями, чтобы не писать
    // 48 заглушек руками
#define SET_ISR_GATE(n) idt[n] = make_gate((uint32_t)&isr##n, 0x08, 0x8E);
    FOR_EACH_ISR(SET_ISR_GATE)
#define SET_IRQ_GATE(n) idt[n + 32] = make_gate((uint32_t)&irq##n, 0x08, 0x8E);
    FOR_EACH_IRQ(SET_IRQ_GATE)

    idt_pointer.limit = sizeof(idt) - 1;
    idt_pointer.base = (uint32_t)idt;
    asm volatile("lidt %0" : : "m"(idt_pointer));

    pic_init();
    pit_init();
}
