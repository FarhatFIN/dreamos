#!/usr/bin/env bash
# Интерактивный запуск: VGA + клавиатура + serial-консоль.
# Выход из QEMU: Ctrl-A, затем X.
set -e
cd "$(dirname "$0")"
make -s bin/kernel.elf
exec qemu-system-i386 -kernel bin/kernel.elf -serial stdio
