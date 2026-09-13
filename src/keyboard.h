// PS/2-клавиатура (IRQ1, scancode set 1). IRQ кладёт готовые символы
// в кольцевой буфер, остальной код только читает.
#pragma once
#include <stdint.h>

namespace keyboard {

void init();
void push_scancode(uint8_t scancode);
bool try_pop(char* out);

} // namespace keyboard
