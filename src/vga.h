// Текстовый VGA (0xB8000, 80x25, CP437). Весь вывод зеркалится в COM1,
// пока зеркалирование не выключено демками, которые рисуют напрямую.
#pragma once
#include <stdint.h>

namespace vga {

constexpr int WIDTH = 80;
constexpr int HEIGHT = 25;

enum class Color : uint8_t {
    Black = 0, Blue, Green, Cyan, Red, Magenta, Brown, LightGray,
    DarkGray, LightBlue, LightGreen, LightCyan, LightRed, Pink, Yellow, White,
};

void init();
void clear();
void putc(char c);
void puts(const char* s);
void set_color(Color fg, Color bg = Color::Black);
void set_mirror(bool on);
void raw_cell(int row, int col, uint8_t ch, uint8_t attr);

} // namespace vga
