#include "serial.h"
#include "io.h"

namespace serial {

namespace {
constexpr uint16_t COM1 = 0x3F8;
}

void init() {
    outb(COM1 + 1, 0x00); // IRQ UART'а не нужны — работаем опросом
    outb(COM1 + 3, 0x80); // DLAB: открываем доступ к делителю
    outb(COM1 + 0, 0x01); // делитель 1 -> 115200 бод
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03); // 8 бит, без чётности, 1 стоп-бит
    outb(COM1 + 2, 0xC7); // FIFO: включить и очистить
    outb(COM1 + 4, 0x0B); // RTS/DSR/out2
}

void write(char c) {
    while (!(inb(COM1 + 5) & 0x20))
        ; // ждём, пока передатчик освободится
    outb(COM1, (uint8_t)c);
}

bool read_ready() {
    return inb(COM1 + 5) & 0x01;
}

char read() {
    return (char)inb(COM1);
}

} // namespace serial
