#include "demos.h"
#include "io.h"
#include "keyboard.h"
#include "lib.h"
#include "serial.h"
#include "vga.h"
#include "interrupts.h"

namespace demos {
namespace {

constexpr int W = vga::WIDTH;
constexpr int H = vga::HEIGHT;

// нажата ли какая-нибудь клавиша (клавиатура или serial) — выход из демо
bool key_pressed() {
    char c;
    if (keyboard::try_pop(&c))
        return true;
    if (serial::read_ready()) {
        (void)serial::read();
        return true;
    }
    return false;
}

// тактирование кадра от PIT; hlt спит до следующего прерывания
void wait_frame(uint32_t ticks_per_frame, uint32_t& last) {
    while (ticks() - last < ticks_per_frame)
        hlt();
    last += ticks_per_frame;
}

uint8_t heat[W][H];

void paint_fire_cell(int x, int y, uint8_t h) {
    // палитра прижата к низу: только самое горячее ядро — жёлто-белое,
    // основная масса пламени — красные блоки, верх тает в чёрное
    uint8_t ch, attr;
    if (h < 16) {
        ch = ' ';
        attr = 0x00;
    } else if (h < 40) {
        ch = '.';
        attr = 0x04; // тёмно-красный
    } else if (h < 70) {
        ch = 0xB0;   // ░
        attr = 0x04;
    } else if (h < 100) {
        ch = 0xB1;   // ▒
        attr = 0x0C; // светло-красный
    } else if (h < 130) {
        ch = 0xB2;   // ▓
        attr = 0x0C;
    } else if (h < 160) {
        ch = 0xDB;   // █
        attr = 0x0C;
    } else if (h < 190) {
        ch = 0xDB;
        attr = 0x0E; // жёлтый
    } else {
        ch = 0xDB;
        attr = 0x0F; // белый
    }
    vga::raw_cell(y, x, ch, attr);
}

} // namespace

void run_fire() {
    vga::set_mirror(false);
    memset(heat, 0, sizeof(heat));
    uint32_t last = ticks();
    for (;;) {
        // "топливо": горячая нижняя строка с перепадами — даёт языки пламени
        for (int x = 0; x < W; x++)
            heat[x][H - 1] = (uint8_t)(120 + rand_next() % 136);
        // тепло поднимается: усредняем окрестность нижнего ряда; случайный
        // горизонтальный сдвиг даёт завихрения, затухание — градиент к верху
        for (int y = 0; y < H - 1; y++) {
            for (int x = 0; x < W; x++) {
                int dx = (int)(rand_next() % 3) - 1;
                int sx = (x + dx + W) % W;
                int below = heat[sx][y + 1];
                uint32_t sum = heat[(sx + W - 1) % W][y + 1] + below +
                               heat[(sx + 1) % W][y + 1] +
                               ((y + 2 < H) ? heat[x][y + 2] : below);
                int v = (int)(sum / 4) - (int)(rand_next() % 13);
                heat[x][y] = (uint8_t)(v > 0 ? v : 0);
            }
        }
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++)
                paint_fire_cell(x, y, heat[x][y]);
        if (key_pressed())
            break;
        wait_frame(4, last); // ~25 fps
    }
    vga::clear();
    vga::set_mirror(true);
}

constexpr int TRAIL_MAX = 14;

struct Column {
    int head; // строка, где сейчас "голова" потока
    int len;  // длина хвоста
    uint8_t glyphs[TRAIL_MAX];
};

Column columns[W];

uint8_t rand_glyph() {
    uint32_t r = rand_next();
    if ((r & 3) == 0)
        return (uint8_t)('A' + (r >> 2) % 26);
    // CP437 0xB0..0xDF — псевдокатакана из псевдографики
    return (uint8_t)(0xB0 + (r >> 2) % 48);
}

void run_matrix() {
    vga::set_mirror(false);
    for (int x = 0; x < W; x++) {
        columns[x].head = -(int)(rand_next() % 60) - 1; // старт с задержкой
        columns[x].len = 6 + (int)(rand_next() % 9);
        for (int i = 0; i < TRAIL_MAX; i++)
            columns[x].glyphs[i] = rand_glyph();
    }
    uint32_t last = ticks();
    for (;;) {
        for (int x = 0; x < W; x++) {
            Column& col = columns[x];
            col.head++;
            if (col.head - col.len > H && (rand_next() & 7) == 0) {
                col.head = -(int)(rand_next() % 40);
                col.len = 6 + (int)(rand_next() % 9);
            }
            if (col.head < 0)
                continue; // колонка ещё не начала падать
            for (int i = col.len - 1; i > 0; i--)
                col.glyphs[i] = col.glyphs[i - 1];
            col.glyphs[0] = rand_glyph();
            for (int i = 0; i < col.len; i++) {
                int y = col.head - i;
                if (y < 0 || y >= H)
                    continue;
                uint8_t attr = (i == 0) ? 0x0F : (i < 3 ? 0x0A : 0x02);
                vga::raw_cell(y, x, col.glyphs[i], attr);
            }
            int tail_y = col.head - col.len;
            if (tail_y >= 0 && tail_y < H)
                vga::raw_cell(tail_y, x, ' ', 0x00);
        }
        if (key_pressed())
            break;
        wait_frame(6, last); // ~16 fps
    }
    vga::clear();
    vga::set_mirror(true);
}

} // namespace demos
