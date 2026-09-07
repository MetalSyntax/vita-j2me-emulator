#!/usr/bin/env python3
"""Genera los assets LiveArea de VitaJ2ME (Fase 6) sin dependencias.
Diseño: degradado navy + marco ámbar + retícula sutil (estética J2ME).
Uso: python3 gen_livearea.py <dir-salida>
"""
import struct
import sys
import zlib


def png(path, w, h, px):
    def chunk(t, d):
        c = t + d
        return struct.pack(">I", len(d)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)
    raw = b"".join(b"\x00" + bytes(px[y * w * 4:(y + 1) * w * 4]) for y in range(h))
    data = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr)
            + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))
    with open(path, "wb") as f:
        f.write(data)
    print(f"{path} {w}x{h}")


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def scene(w, h, top, bottom, grid=True, border=0, diamond=False):
    px = []
    for y in range(h):
        t = y / max(h - 1, 1)
        row = []
        for x in range(w):
            c = list(lerp(top, bottom, t))
            if grid and ((x % 32 == 0) or (y % 32 == 0)):
                c = [min(255, v + 14) for v in c]  # reticula tenue
            # vineta
            dx = (x - w / 2) / (w / 2)
            dy = (y - h / 2) / (h / 2)
            v = max(0.0, 1.0 - 0.45 * (dx * dx + dy * dy) / 2)
            c = [int(vv * v) for vv in c]
            if border and (x < border or y < border or x >= w - border or y >= h - border):
                c = [245, 166, 35]  # ambar
            row.extend(c + [255])
        px.extend(row)
    if diamond:  # rombo central (motivo "JVM corriendo")
        cx, cy, r = w // 2, h // 2, min(w, h) // 4
        for y in range(h):
            for x in range(w):
                if abs(x - cx) + abs(y - cy) < r:
                    edge = abs(x - cx) + abs(y - cy) > r - max(3, r // 6)
                    i = (y * w + x) * 4
                    if edge:
                        px[i:i + 4] = [245, 166, 35, 255]
                    else:
                        px[i:i + 4] = [16, 22, 44, 255]
    return px


NAVY_TOP = (11, 16, 38)
NAVY_BOT = (27, 42, 74)

out = sys.argv[1] if len(sys.argv) > 1 else "."
png(f"{out}/icon0.png", 128, 128,
    scene(128, 128, (20, 30, 62), (8, 12, 30), grid=False, border=6, diamond=True))
png(f"{out}/pic0.png", 960, 544, scene(960, 544, NAVY_TOP, NAVY_BOT))
png(f"{out}/bg0.png", 840, 500, scene(840, 500, (8, 12, 30), (20, 30, 62)))
png(f"{out}/startup.png", 280, 158,
    scene(280, 158, (27, 42, 74), (11, 16, 38), grid=False, border=4, diamond=True))
