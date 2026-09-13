#include "keyboard.h"

namespace keyboard {

namespace {
constexpr int BUF_SIZE = 256;

volatile char buf[BUF_SIZE];
volatile uint8_t head = 0;
volatile uint8_t tail = 0;
bool shift = false;
bool caps = false;

// раскладка US; индексы — scancodes set 1
const char row1[] = "1234567890-=";
const char row1_shifted[] = "!@#$%^&*()_+";

char enqueue(char c) {
    if ((uint8_t)(head + 1) % BUF_SIZE == tail)
        return 0; // буфер полон — молча теряем, ввод это не ломает
    buf[head] = c;
    head = (uint8_t)((head + 1) % BUF_SIZE);
    return c;
}

char letter(char base, bool upper) {
    return upper ? (char)(base - 'a' + 'A') : base;
}

char translate(uint8_t sc) {
    if (sc >= 0x02 && sc <= 0x0D)
        return shift ? row1_shifted[sc - 0x02] : row1[sc - 0x02];
    if (sc >= 0x10 && sc <= 0x19)
        return letter("qwertyuiop"[sc - 0x10], shift ^ caps);
    if (sc == 0x1A)
        return shift ? '{' : '[';
    if (sc == 0x1B)
        return shift ? '}' : ']';
    if (sc >= 0x1E && sc <= 0x26)
        return letter("asdfghjkl"[sc - 0x1E], shift ^ caps);
    if (sc == 0x27)
        return shift ? ':' : ';';
    if (sc == 0x28)
        return shift ? '"' : '\'';
    if (sc == 0x29)
        return shift ? '~' : '`';
    if (sc == 0x2B)
        return shift ? '|' : '\\';
    if (sc >= 0x2C && sc <= 0x32)
        return letter("zxcvbnm"[sc - 0x2C], shift ^ caps);
    if (sc == 0x33)
        return shift ? '<' : ',';
    if (sc == 0x34)
        return shift ? '>' : '.';
    if (sc == 0x35)
        return shift ? '?' : '/';
    if (sc == 0x39)
        return ' ';
    return 0;
}
} // namespace

void init() {
    head = 0;
    tail = 0;
    shift = false;
    caps = false;
}

void push_scancode(uint8_t sc) {
    if (sc & 0x80) { // отпускание клавиши
        uint8_t key = sc & 0x7F;
        if (key == 0x2A || key == 0x36)
            shift = false;
        return;
    }
    switch (sc) {
    case 0x2A:
    case 0x36:
        shift = true;
        return;
    case 0x3A:
        caps = !caps;
        return;
    case 0x1D:
    case 0x38:
        return; // Ctrl/Alt не используем
    case 0x0E:
        enqueue('\b');
        return;
    case 0x1C:
        enqueue('\n');
        return;
    case 0x0F:
        return; // Tab в шелле не нужен
    default:
        enqueue(translate(sc));
        return;
    }
}

bool try_pop(char* out) {
    if (head == tail)
        return false;
    *out = buf[tail];
    tail = (uint8_t)((tail + 1) % BUF_SIZE);
    return true;
}

} // namespace keyboard
