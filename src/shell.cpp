#include "shell.h"
#include "demos.h"
#include "io.h"
#include "keyboard.h"
#include "lib.h"
#include "serial.h"
#include "vga.h"
#include "interrupts.h"

// multiboot info от bootloader'а, заполняется в kmain
const void* g_boot_multiboot_info = nullptr;

namespace {
constexpr int LINE_MAX = 96;

void cmd_about();
void cmd_clear();
void cmd_cpuid();
void cmd_div0();
void cmd_fire();
void cmd_help();
void cmd_matrix();
void cmd_meminfo();
void cmd_reboot();
void cmd_rdtsc();
void cmd_uptime();

struct Command {
    const char* name;
    const char* hint;
    void (*run)();
};

const Command commands[] = {
    {"about",   "what this thing is",                       cmd_about},
    {"clear",   "clear the screen",                         cmd_clear},
    {"cpuid",   "identify the CPU (CPUID instruction)",     cmd_cpuid},
    {"div0",    "trigger divide-by-zero and survive it",    cmd_div0},
    {"fire",    "ASCII fire demo (any key to exit)",        cmd_fire},
    {"help",    "this list",                                cmd_help},
    {"matrix",  "matrix rain demo (any key to exit)",       cmd_matrix},
    {"meminfo", "RAM reported by the bootloader",           cmd_meminfo},
    {"reboot",  "reset the machine",                        cmd_reboot},
    {"rdtsc",   "raw CPU cycle counter (RDTSC)",            cmd_rdtsc},
    {"uptime",  "seconds since boot (PIT @ 100 Hz)",        cmd_uptime},
};

char line[LINE_MAX];
int len = 0;

void prompt() {
    vga::set_color(vga::Color::LightGreen, vga::Color::Black);
    vga::puts("dream");
    vga::set_color(vga::Color::LightGray, vga::Color::Black);
    vga::puts("> ");
}

void cmd_help() {
    vga::puts("commands:\n");
    for (const Command& c : commands) {
        vga::puts("  ");
        vga::set_color(vga::Color::LightCyan, vga::Color::Black);
        vga::puts(c.name);
        for (unsigned i = strlen(c.name); i < 9; i++)
            vga::putc(' ');
        vga::set_color(vga::Color::LightGray, vga::Color::Black);
        vga::puts("- ");
        vga::puts(c.hint);
        vga::putc('\n');
    }
}

void cmd_about() {
    vga::puts("DREAM-OS 1.0 - an x86 kernel written from scratch:\n");
    vga::puts("  * boot, GDT/IDT and interrupt stubs: NASM assembly\n");
    vga::puts("  * kernel: C++ built with -ffreestanding, no libc, no runtime\n");
    vga::puts("  * console: VGA text 80x25 + COM1 serial, PS/2 keyboard\n");
    vga::puts("  * timers: PIT @ 100 Hz, cycle counter via RDTSC\n");
    vga::puts("type 'help' to see what it can do.\n");
}

void cmd_clear() {
    vga::clear();
}

void cmd_cpuid() {
    uint32_t a, b, c, d;
    cpuid_leaf(0, &a, &b, &c, &d);
    char vendor[13];
    // vendor string лежит в регистрах EBX:EDX:ECX — именно в таком порядке
    memcpy(vendor + 0, &b, 4);
    memcpy(vendor + 4, &d, 4);
    memcpy(vendor + 8, &c, 4);
    vendor[12] = '\0';
    vga::puts("vendor: ");
    vga::puts(vendor);
    vga::putc('\n');

    cpuid_leaf(1, &a, &b, &c, &d);
    uint32_t family = (a >> 8) & 0xF;
    if (family == 0xF)
        family += (a >> 20) & 0xFF;
    uint32_t model = (a >> 4) & 0xF;
    if (family == 0x6 || family >= 0xF)
        model |= ((a >> 16) & 0xF) << 4;
    vga::puts("signature: 0x");
    print_hex32(a);
    vga::puts(" (family ");
    print_u32_dec(family);
    vga::puts(", model ");
    print_u32_dec(model);
    vga::puts(", stepping ");
    print_u32_dec(a & 0xF);
    vga::puts(")\n");

    cpuid_leaf(0x80000000, &a, &b, &c, &d);
    if (a >= 0x80000004) {
        uint32_t words[12];
        int w = 0;
        for (uint32_t leaf = 0x80000002; leaf <= 0x80000004; leaf++) {
            cpuid_leaf(leaf, &words[w], &words[w + 1], &words[w + 2], &words[w + 3]);
            w += 4;
        }
        char brand[49];
        memcpy(brand, words, 48);
        brand[48] = '\0';
        const char* p = brand;
        while (*p == ' ')
            p++;
        vga::puts("brand: ");
        vga::puts(p);
        vga::putc('\n');
    }
}

void cmd_rdtsc() {
    uint64_t cycles = rdtsc();
    vga::puts("cycles since power-on: 0x");
    print_hex64(cycles);
    vga::puts(" (");
    print_u64_dec(cycles);
    vga::puts(")\n");
}

void cmd_uptime() {
    uint32_t t = ticks();
    vga::puts("uptime: ");
    print_u32_dec(t / 100);
    vga::putc('.');
    if (t % 100 < 10)
        vga::putc('0');
    print_u32_dec(t % 100);
    vga::puts(" s (");
    print_u32_dec(t);
    vga::puts(" ticks)\n");
}

void cmd_meminfo() {
    // нас интересуют только первые три поля multiboot info
    struct MultibootInfo {
        uint32_t flags;
        uint32_t mem_lower;
        uint32_t mem_upper;
    };
    const MultibootInfo* mbi = (const MultibootInfo*)g_boot_multiboot_info;
    if (!mbi || !(mbi->flags & 1)) {
        vga::puts("RAM: bootloader did not report memory size\n");
        return;
    }
    uint32_t total_kb = mbi->mem_lower + mbi->mem_upper;
    vga::puts("RAM: ");
    print_u32_dec(mbi->mem_lower);
    vga::puts(" KB low + ");
    print_u32_dec(mbi->mem_upper);
    vga::puts(" KB high = ");
    print_u32_dec(total_kb / 1024);
    vga::putc('.');
    print_u32_dec((total_kb % 1024) * 10 / 1024);
    vga::puts(" MiB\n");
}

// F7 F1 = div ecx: ровно 2 байта, поэтому обработчик #DE может перепрыгнуть
// их через eip += 2 — ядро переживает деление на ноль и продолжает работать
void trigger_div0() {
    asm volatile("xorl %%ecx, %%ecx\n\t"
                 "movl $0x7FFFFFFF, %%eax\n\t"
                 ".byte 0xF7, 0xF1"
                 :
                 :
                 : "eax", "ecx", "edx", "cc");
}

void cmd_div0() {
    vga::puts("dividing 2147483647 by 0...\n");
    trigger_div0();
    vga::set_color(vga::Color::LightGreen, vga::Color::Black);
    vga::puts("still alive: #DE caught, faulting instruction skipped\n");
    vga::set_color(vga::Color::LightGray, vga::Color::Black);
}

void cmd_reboot() {
    vga::puts("rebooting...\n");
    // 8042: слить входной буфер, затем pulse reset
    for (int i = 0; i < 100 && (inb(0x64) & 2); i++)
        io_wait();
    outb(0x64, 0xFE);
    for (;;)
        hlt();
}

void cmd_fire() {
    demos::run_fire();
}

void cmd_matrix() {
    demos::run_matrix();
}

void execute(const char* cmd) {
    if (cmd[0] == '\0')
        return;
    for (const Command& c : commands) {
        if (strcmp(cmd, c.name) == 0) {
            c.run();
            return;
        }
    }
    vga::puts("unknown command '");
    vga::puts(cmd);
    vga::puts("' - type 'help'\n");
}

void feed(char c) {
    if (c == '\n' || c == '\r') {
        vga::putc('\n');
        line[len] = '\0';
        execute(line);
        len = 0;
        prompt();
    } else if (c == '\b' || c == 0x7F) {
        if (len > 0) {
            len--;
            vga::putc('\b');
        }
    } else if (c >= 32 && c < 127) {
        if (len < LINE_MAX - 1) {
            line[len++] = c;
            vga::putc(c);
        }
    }
}

} // namespace

namespace shell {

void run() {
    len = 0;
    prompt();
    for (;;) {
        char c;
        if (keyboard::try_pop(&c)) {
            feed(c);
        } else if (serial::read_ready()) {
            feed(serial::read());
        } else {
            hlt(); // спим до ближайшего прерывания (PIT/клавиатура)
        }
    }
}

} // namespace shell
