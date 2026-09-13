#include "lib.h"
#include "vga.h"

void print_u32_dec(uint32_t value) {
    char buf[11];
    int pos = 10;
    buf[pos] = '\0';
    if (value == 0) {
        vga::putc('0');
        return;
    }
    while (value) {
        buf[--pos] = (char)('0' + value % 10);
        value /= 10;
    }
    vga::puts(&buf[pos]);
}

// 64-битное деление на 10 без libgcc: раскладываем на два 32-битных DIV
// (EDX:EAX / operand) — длинное деление старшей и младшей половин.
static uint64_t u64_divmod10(uint64_t value, uint32_t* remainder) {
    uint32_t hi = (uint32_t)(value >> 32);
    uint32_t lo = (uint32_t)value;
    uint32_t q_hi, r_hi, q_lo, r_lo;
    asm volatile("divl %2" : "=a"(q_hi), "=d"(r_hi) : "r"(10u), "0"(hi), "1"(0u));
    asm volatile("divl %2" : "=a"(q_lo), "=d"(r_lo) : "r"(10u), "0"(lo), "1"(r_hi));
    *remainder = r_lo;
    return ((uint64_t)q_hi << 32) | q_lo;
}

void print_u64_dec(uint64_t value) {
    char buf[21];
    int pos = 20;
    buf[pos] = '\0';
    if (value == 0) {
        vga::putc('0');
        return;
    }
    uint32_t rem;
    while (value) {
        value = u64_divmod10(value, &rem);
        buf[--pos] = (char)('0' + rem);
    }
    vga::puts(&buf[pos]);
}

// печатает ровно 8 hex-цифр без префикса — "0x" добавляет вызывающий
void print_hex32(uint32_t value) {
    static const char digits[] = "0123456789ABCDEF";
    for (int shift = 28; shift >= 0; shift -= 4)
        vga::putc(digits[(value >> shift) & 0xF]);
}

void print_hex64(uint64_t value) {
    print_hex32((uint32_t)(value >> 32));
    print_hex32((uint32_t)value);
}

// LCG (числа Левина — простые и предсказуемые, для демо хватит с головой)
static uint32_t rng_state = 0x12345678;

void rand_seed(uint32_t seed) {
    rng_state = seed ? seed : 0x12345678;
}

uint32_t rand_next() {
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state >> 8;  // младшие биты LCG плохие — выбрасываем
}

extern "C" {

void* memcpy(void* dst, const void* src, __SIZE_TYPE__ n) {
    auto* d = (unsigned char*)dst;
    const auto* s = (const unsigned char*)src;
    while (n--)
        *d++ = *s++;
    return dst;
}

void* memset(void* dst, int value, __SIZE_TYPE__ n) {
    auto* d = (unsigned char*)dst;
    while (n--)
        *d++ = (unsigned char)value;
    return dst;
}

void* memmove(void* dst, const void* src, __SIZE_TYPE__ n) {
    auto* d = (unsigned char*)dst;
    const auto* s = (const unsigned char*)src;
    if (d < s) {
        while (n--)
            *d++ = *s++;
    } else if (d > s) {
        while (n--)
            d[n] = s[n];
    }
    return dst;
}

__SIZE_TYPE__ strlen(const char* s) {
    __SIZE_TYPE__ n = 0;
    while (s[n])
        n++;
    return n;
}

int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

} // extern "C"
