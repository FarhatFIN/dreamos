// Вещи, которые в нормальной программе дал бы libc: память, строки, печать
// чисел и генератор случайных чисел. Без libc и без зависимости от libgcc:
// деление 64-битных чисел сделано вручную через 32-битный DIV.
#pragma once
#include <stdint.h>

void print_u32_dec(uint32_t value);
void print_u64_dec(uint64_t value);
void print_hex32(uint32_t value);
void print_hex64(uint64_t value);

uint32_t rand_next();
void rand_seed(uint32_t seed);

extern "C" {
void* memcpy(void* dst, const void* src, __SIZE_TYPE__ n);
void* memset(void* dst, int value, __SIZE_TYPE__ n);
void* memmove(void* dst, const void* src, __SIZE_TYPE__ n);
__SIZE_TYPE__ strlen(const char* s);
int strcmp(const char* a, const char* b);
}
