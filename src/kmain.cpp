// Точка входа C++: железо по порядку, затем шелл навсегда.
#include "interrupts.h"
#include "io.h"
#include "keyboard.h"
#include "lib.h"
#include "shell.h"
#include "serial.h"
#include "vga.h"

extern const void* g_boot_multiboot_info;

namespace {

const char* const BANNER_D[5] = {" #### ", "#    #", "#    #", "#    #", " #### "};
const char* const BANNER_R[5] = {" #####", "#    #", "##### ", "#   # ", "#    #"};
const char* const BANNER_E[5] = {"######", "#     ", "####  ", "#     ", "######"};
const char* const BANNER_A[5] = {"  ##  ", " #  # ", "######", "#    #", "#    #"};
const char* const BANNER_M[5] = {"#    #", "##  ##", "# ## #", "#    #", "#    #"};
const char* const BANNER_DASH[5] = {"      ", "      ", "######", "      ", "      "};
const char* const BANNER_O[5] = {" #### ", "#    #", "#    #", "#    #", " #### "};
const char* const BANNER_S[5] = {" #####", "#     ", "##### ", "     #", "##### "};

void print_banner() {
    const char* const* letters[8] = {
        BANNER_D, BANNER_R, BANNER_E, BANNER_A,
        BANNER_M, BANNER_DASH, BANNER_O, BANNER_S,
    };
    vga::set_color(vga::Color::LightCyan, vga::Color::Black);
    for (int r = 0; r < 5; r++) {
        vga::putc(' ');
        for (int i = 0; i < 8; i++) {
            vga::puts(letters[i][r]);
            vga::putc(i == 7 ? '\n' : ' ');
        }
    }
}

void boot_ok(const char* msg) {
    vga::puts("  ");
    vga::set_color(vga::Color::LightGreen, vga::Color::Black);
    vga::puts("[ok] ");
    vga::set_color(vga::Color::LightGray, vga::Color::Black);
    vga::puts(msg);
    vga::putc('\n');
}

} // namespace

extern "C" void kmain(const void* multiboot_info) {
    g_boot_multiboot_info = multiboot_info;

    vga::init();
    serial::init();

    print_banner();
    vga::set_color(vga::Color::Yellow, vga::Color::Black);
    vga::puts("        DREAM-OS 1.0 - x86 kernel from scratch (C++ + NASM, ring 0)\n\n");
    vga::set_color(vga::Color::LightGray, vga::Color::Black);
    vga::puts("booting...\n");

    gdt_load();
    boot_ok("GDT: flat code/data segments, 32-bit protected mode (ring 0)");
    interrupts_init();
    boot_ok("IDT: 32 exception + 16 IRQ gates; PIC @ 0x20; PIT @ 100 Hz");
    keyboard::init();
    rand_seed((uint32_t)rdtsc());
    sti();
    boot_ok("interrupts enabled");
    vga::putc('\n');

    shell::run();
}
