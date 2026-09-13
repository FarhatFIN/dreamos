// Порт I/O, CPUID, RDTSC и прочая работа с железом — только inline asm.
#pragma once
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// ~1 мкс задержки на порту 0x80 — пишем в него мусор, это стандартный трюк
// для старого железа, которому нужен gap между port I/O.
static inline void io_wait() {
    outb(0x80, 0);
}

static inline void hlt() {
    asm volatile("hlt");
}

static inline void sti() {
    asm volatile("sti");
}

static inline uint64_t rdtsc() {
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static inline void cpuid_leaf(uint32_t leaf, uint32_t* a, uint32_t* b,
                              uint32_t* c, uint32_t* d) {
    asm volatile("cpuid"
                 : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
                 : "a"(leaf), "c"(0u));
}
