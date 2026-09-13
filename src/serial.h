// COM1 (0x3F8), 115200 8N1. В QEMU это одновременно консоль для автоматических
// тестов и зеркало экрана. Опрос, без IRQ — для текстовой консоли хватает.
#pragma once

namespace serial {

void init();
void write(char c);
bool read_ready();
char read();

} // namespace serial
