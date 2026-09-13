#!/usr/bin/env bash
# Headless end-to-end тест: ядро грузится в QEMU без окна, команды подаются
# через COM1, ответы проверяются grep'ом по serial-логу.
set -u
cd "$(dirname "$0")"

make -s bin/kernel.elf || exit 1

LOG="$(mktemp /tmp/dreamos-test.XXXXXX.log)"
# первый байт в pipe QEMU съедает сам (квирк stdio-чардева), поэтому
# ведущий '\n' — пустая команда, она безвредна
printf '%s' $'\nhelp\nabout\ncpuid\nuptime\nmeminfo\nrdtsc\ndiv0\nfire\nx\nmatrix\nx\nnosuchcmd\nreboot\n' \
    | timeout 30 qemu-system-i386 -display none -monitor none -no-reboot \
        -kernel bin/kernel.elf -serial stdio >"$LOG" 2>/dev/null

fail=0
check() {
    if grep -qF -- "$2" "$LOG"; then
        echo "PASS: $1"
    else
        echo "FAIL: $1 (missing: $2)"
        fail=1
    fi
}

check "boot: banner"         "DREAM-OS 1.0"
check "boot: GDT"            "[ok] GDT"
check "boot: IDT/PIC/PIT"    "[ok] IDT"
check "help: fire listed"    "ASCII fire demo"
check "about: freestanding"  "no libc"
check "cpuid: vendor"        "vendor:"
check "uptime: ticks"        "ticks)"
check "meminfo: RAM"         "RAM:"
check "rdtsc: cycles"        "cycles since power-on:"
check "div0: survived"       "still alive"
check "unknown command"      "unknown command"
check "reboot: message"      "rebooting..."

rm -f "$LOG"
if [ "$fail" -eq 0 ]; then
    echo "ALL CHECKS PASSED"
else
    echo "SOME CHECKS FAILED"
fi
exit "$fail"
