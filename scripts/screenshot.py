#!/usr/bin/env python3
"""Снимает скриншоты VGA-экрана DreamOS (баннер, fire, matrix) в docs/.

Требует: qemu-system-i386, python3 + Pillow, собранное ядро (make).
Запуск из корня репозитория: python3 scripts/screenshot.py
"""
import os
import socket
import subprocess
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOCS = os.path.join(ROOT, "docs")
MON = "/tmp/dreamos-mon-screenshot"

os.makedirs(DOCS, exist_ok=True)
subprocess.run(["make", "-s", "bin/kernel.elf"], cwd=ROOT, check=True)

proc = subprocess.Popen(
    ["qemu-system-i386", "-display", "none",
     "-monitor", f"unix:{MON},server,nowait",
     "-kernel", os.path.join(ROOT, "bin/kernel.elf"), "-serial", "stdio"],
    stdin=subprocess.PIPE,
    stdout=open("/tmp/dreamos-screenshot-serial.log", "wb"),
    stderr=subprocess.DEVNULL)


def screendump(name):
    # дамп всегда в /tmp: пути с пробелами HMP монитора не переваривает
    mon = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    mon.connect(MON)
    time.sleep(0.3)
    mon.recv(4096)  # приглашение монитора
    mon.sendall(f"screendump /tmp/dreamos-shot-{name}.ppm\n".encode())
    time.sleep(0.5)
    mon.close()


def send(s):
    proc.stdin.write(s.encode())
    proc.stdin.flush()


def save_png(name):
    from PIL import Image
    img = Image.open(f"/tmp/dreamos-shot-{name}.ppm")
    img = img.resize((img.width * 2, img.height * 2), Image.NEAREST)
    img.save(os.path.join(DOCS, name + ".png"))


time.sleep(1.5)
send("\n")            # первый байт в pipe QEMU съедает
time.sleep(0.5)
screendump("screenshot-boot")
send("fire\n")
time.sleep(3.0)
screendump("screenshot-fire")
send("x")
time.sleep(0.5)
send("matrix\n")
time.sleep(3.0)
screendump("screenshot-matrix")
send("x")
time.sleep(0.5)
send("reboot\n")
try:
    proc.wait(timeout=5)
except subprocess.TimeoutExpired:
    proc.kill()

for name in ("screenshot-boot", "screenshot-fire", "screenshot-matrix"):
    save_png(name)
print("saved to", DOCS)
