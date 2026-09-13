#include "vga.h"
#include "io.h"
#include "serial.h"

namespace vga {

namespace {
constexpr uint16_t CRTC_INDEX = 0x3D4;
constexpr uint16_t CRTC_DATA = 0x3D5;

volatile uint16_t* const buffer = (volatile uint16_t*)0xB8000;

int row_ = 0;
int col_ = 0;
uint8_t attr_ = 0x07;
bool mirror_ = true;

void put_cell(int r, int c, char ch) {
    buffer[r * WIDTH + c] = (uint16_t)attr_ << 8 | (uint8_t)ch;
}

void enable_cursor(uint8_t start, uint8_t end) {
    outb(CRTC_INDEX, 0x0A);
    outb(CRTC_DATA, (inb(CRTC_DATA) & 0xC0) | start);
    outb(CRTC_INDEX, 0x0B);
    outb(CRTC_DATA, (inb(CRTC_DATA) & 0xE0) | end);
}

void move_cursor(int r, int c) {
    uint16_t pos = (uint16_t)(r * WIDTH + c);
    outb(CRTC_INDEX, 0x0F);
    outb(CRTC_DATA, pos & 0xFF);
    outb(CRTC_INDEX, 0x0E);
    outb(CRTC_DATA, (pos >> 8) & 0xFF);
}
} // namespace

void init() {
    set_color(Color::LightGray, Color::Black);
    mirror_ = true;
    clear();
    enable_cursor(0, 15);
}

void clear() {
    for (int i = 0; i < WIDTH * HEIGHT; i++)
        buffer[i] = (uint16_t)attr_ << 8 | ' ';
    row_ = 0;
    col_ = 0;
    move_cursor(row_, col_);
}

void scroll() {
    for (int y = 1; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
            buffer[(y - 1) * WIDTH + x] = buffer[y * WIDTH + x];
    for (int x = 0; x < WIDTH; x++)
        buffer[(HEIGHT - 1) * WIDTH + x] = (uint16_t)attr_ << 8 | ' ';
    row_--;
}

void putc(char ch) {
    switch (ch) {
    case '\n':
        col_ = 0;
        row_++;
        break;
    case '\r':
        col_ = 0;
        break;
    case '\b':
        if (col_ > 0)
            col_--;
        else if (row_ > 0) {
            row_--;
            col_ = WIDTH - 1;
        }
        put_cell(row_, col_, ' ');
        break;
    case '\t':
        col_ = (col_ + 8) & ~7;
        if (col_ >= WIDTH) {
            col_ = 0;
            row_++;
        }
        break;
    default:
        put_cell(row_, col_, ch);
        if (++col_ >= WIDTH) {
            col_ = 0;
            row_++;
        }
        break;
    }
    if (row_ >= HEIGHT)
        scroll();
    move_cursor(row_, col_);
    if (mirror_)
        serial::write(ch);
}

void puts(const char* s) {
    for (; *s; s++)
        putc(*s);
}

void set_color(Color fg, Color bg) {
    attr_ = (uint8_t)fg | ((uint8_t)bg << 4);
}

void set_mirror(bool on) {
    mirror_ = on;
}

void raw_cell(int row, int col, uint8_t ch, uint8_t attr) {
    buffer[row * WIDTH + col] = (uint16_t)attr << 8 | ch;
}

} // namespace vga
