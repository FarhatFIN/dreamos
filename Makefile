CXX   := g++
AS    := nasm
QEMU  := qemu-system-i386

# freestanding: ядро живёт без libc и рантайма; mgeneral-regs-only — чтобы GCC
# не генерировал SSE-код, требующий выровненного стека на пустом месте
CXXFLAGS := -m32 -ffreestanding -nostdlib -fno-exceptions -fno-rtti \
            -fno-stack-protector -fno-pic -fno-pie \
            -fno-asynchronous-unwind-tables -mgeneral-regs-only \
            -O2 -Wall -Wextra -g
ASFLAGS  := -f elf32 -g
LDFLAGS  := -m32 -nostdlib -no-pie -T linker.ld

ASMSRC := boot/boot.asm src/gdt.asm src/isr.asm
CXXSRC := src/lib.cpp src/vga.cpp src/serial.cpp src/keyboard.cpp \
          src/interrupts.cpp src/demos.cpp src/shell.cpp src/kmain.cpp
OBJ := $(ASMSRC:%.asm=build/%.o) $(CXXSRC:%.cpp=build/%.o)

all: bin/kernel.elf

bin/kernel.elf: $(OBJ) linker.ld
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJ)

build/%.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	$(QEMU) -kernel bin/kernel.elf -serial stdio

test: all
	./test.sh

clean:
	rm -rf build bin

.PHONY: all run test clean
