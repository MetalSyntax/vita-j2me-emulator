#!/usr/bin/env python3
"""patch_vm_calls.py — redirige llamadas internas de la JVM al wrapper de
vita/vm_patches.c, reescribiendo la instruccion `bl` en el ELF ya enlazado.

POR QUE HACE FALTA ESTO (y no alcanza con -Wl,--wrap)
-----------------------------------------------------
CLDC se compila con *merged sources*: el build de phoneME concatena
decenas de .cpp en un punado de unidades de compilacion
(`_MergedSrc001.obj` .. `_MergedSrc006.obj`). `TaskMirror.cpp`,
`SystemDictionary.cpp` y `Thread.cpp` cayeron todos en
`_MergedSrc003.obj`, asi que las llamadas entre ellos son intra-objeto y
el ensamblador YA las resolvio a un desplazamiento PC-relativo fijo:
`readelf -r _MergedSrc003.obj` no tiene NINGUNA relocacion contra
`clinit_list_remove`. Sin relocacion no hay nada que el linker pueda
redirigir, asi que `--wrap` (y cualquier otro flag de link) es inutil
para esos call sites — solo funciono para `InstanceClass::set_initialized`,
que vive en `_MergedSrc004.obj` y por eso si referencia el simbolo.

La alternativa correcta seria recompilar CLDC con el fix en el fuente
(el fix YA esta aplicado en `TaskMirror.cpp`), pero eso sigue bloqueado
por el SIGSEGV de `romgen.exe` (Bug real #14). Mientras tanto se parchea
el binario, de forma acotada y verificada.

QUE HACE
--------
Para cada (funcion_llamante, destino_viejo, destino_nuevo):
  1. localiza en el desensamblado los `bl` dentro de <funcion_llamante>
     que apuntan a <destino_viejo>,
  2. reescribe la codificacion Thumb BL para que apunte a <destino_nuevo>,
  3. RE-DESENSAMBLA y verifica que ahora diga <destino_nuevo>.

Si la cantidad de call sites encontrados no es la esperada, o si la
verificacion falla, sale con error y rompe el build a proposito: es
preferible un build roto a un eboot.bin parcheado a medias.
"""

import os
import re
import struct
import subprocess
import sys

# (funcion que contiene la llamada, simbolo destino actual,
#  simbolo destino nuevo, cuantos call sites se esperan)
PATCHES = [
    # Bug real #15: clinit_list_remove() derefiere NULL si la clase no esta
    # en la clinit list. Este es el call site que crashea el arranque
    # (dump psp2core-1788744297).
    ("_ZN19JVMSystemDictionary17update_fake_classEP16JVMInstanceClassS1_P8JVMTraps",
     "_ZN13JVMTaskMirror18clinit_list_removeEP6JVMOop",
     "__wrap__ZN13JVMTaskMirror18clinit_list_removeEP6JVMOop",
     1),
    # Mismo destino desde el cleanup de threads. Aca la lista NUNCA deberia
    # estar vacia (itera sobre ella), pero pasa por el wrapper igual para
    # que el comportamiento sea uniforme y quede contabilizado en el log.
    ("thread_task_cleanup",
     "_ZN13JVMTaskMirror18clinit_list_removeEP6JVMOop",
     "__wrap__ZN13JVMTaskMirror18clinit_list_removeEP6JVMOop",
     1),
]


def tool(name):
    prefix = "arm-vita-eabi-"
    sdk = os.environ.get("VITASDK")
    if sdk:
        cand = os.path.join(sdk, "bin", prefix + name)
        if os.path.exists(cand):
            return cand
    return prefix + name


def run(args):
    r = subprocess.run(args, capture_output=True, text=True)
    if r.returncode != 0:
        die(f"fallo {' '.join(args[:2])}: {r.stderr.strip()}")
    return r.stdout


def die(msg):
    sys.stderr.write(f"patch_vm_calls.py: ERROR: {msg}\n")
    sys.exit(1)


def read_symbols(elf):
    """{nombre: (valor_sin_bit_thumb, es_thumb)} para simbolos de funcion."""
    syms = {}
    for line in run([tool("readelf"), "-sW", elf]).splitlines():
        parts = line.split()
        # Num: Value Size Type Bind Vis Ndx Name
        if len(parts) < 8 or not parts[0].endswith(":"):
            continue
        if parts[3] != "FUNC":
            continue
        try:
            val = int(parts[1], 16)
        except ValueError:
            continue
        syms[parts[7]] = (val & ~1, bool(val & 1))
    return syms


def section_map(elf):
    """[(vaddr, size, file_offset)] de las secciones con contenido."""
    out = []
    for line in run([tool("readelf"), "-SW", elf]).splitlines():
        m = re.match(r"\s*\[\s*\d+\]\s+(\S+)\s+(\S+)\s+([0-9a-f]+)\s+"
                     r"([0-9a-f]+)\s+([0-9a-f]+)", line)
        if not m:
            continue
        name, stype, addr, off, size = m.groups()
        if stype in ("NOBITS", "NULL"):
            continue
        addr, off, size = int(addr, 16), int(off, 16), int(size, 16)
        if addr:
            out.append((addr, size, off))
    return out


def vaddr_to_offset(secs, vaddr):
    for addr, size, off in secs:
        if addr <= vaddr < addr + size:
            return off + (vaddr - addr)
    die(f"0x{vaddr:08x} no cae en ninguna seccion con contenido")


def disassemble(elf):
    return run([tool("objdump"), "-d", elf])


def find_call_sites(dis, caller, target):
    """Direcciones de los `bl` a <target> dentro de <caller>."""
    sites = []
    cur = None
    for line in dis.splitlines():
        m = re.match(r"^([0-9a-f]{8}) <(.+)>:$", line)
        if m:
            cur = m.group(2)
            continue
        if cur != caller:
            continue
        m = re.match(r"^\s*([0-9a-f]+):\s+[0-9a-f ]+\s+(bl|blx)\s+"
                     r"([0-9a-f]+) <([^>+]+)>\s*$", line)
        if m and m.group(4) == target:
            if m.group(2) != "bl":
                die(f"{caller}: esperaba `bl` a {target}, encontre "
                    f"`{m.group(2)}` en 0x{m.group(1)}")
            sites.append(int(m.group(1), 16))
    return sites


def encode_thumb_bl(addr, target):
    """Codifica BL (Thumb T1, 4 bytes) de <addr> a <target>."""
    off = target - (addr + 4)
    if off & 1:
        die(f"destino 0x{target:08x} no esta alineado a 2 bytes")
    if not (-(1 << 24) <= off < (1 << 24)):
        die(f"salto fuera de rango para Thumb BL: {off}")
    # El bit 0 del desplazamiento es implicito (siempre 0).
    s = (off >> 24) & 1
    i1 = (off >> 23) & 1
    i2 = (off >> 22) & 1
    imm10 = (off >> 12) & 0x3FF
    imm11 = (off >> 1) & 0x7FF
    j1 = ((~i1) ^ s) & 1
    j2 = ((~i2) ^ s) & 1
    hw1 = 0xF000 | (s << 10) | imm10
    hw2 = 0xD000 | (j1 << 13) | (j2 << 11) | imm11
    return struct.pack("<HH", hw1, hw2)


def main():
    if len(sys.argv) != 2:
        die("uso: patch_vm_calls.py <elf>")
    elf = sys.argv[1]
    if not os.path.exists(elf):
        die(f"no existe {elf}")

    syms = read_symbols(elf)
    secs = section_map(elf)
    dis = disassemble(elf)

    planned = []
    for caller, old, new, expected in PATCHES:
        if new not in syms:
            die(f"el ELF no define {new} (falta vm_patches.c en el build?)")
        new_addr, new_thumb = syms[new]
        if not new_thumb:
            die(f"{new} es codigo ARM; este parche solo emite Thumb BL. "
                f"Compila vm_patches.c con -mthumb.")
        sites = find_call_sites(dis, caller, old)
        if len(sites) != expected:
            die(f"{caller}: esperaba {expected} llamada(s) a {old}, "
                f"encontre {len(sites)}. El binario cambio: revisa PATCHES "
                f"antes de seguir (no parcheo nada).")
        for addr in sites:
            planned.append((addr, new_addr | 1, caller, new))

    with open(elf, "r+b") as f:
        for addr, target, caller, new in planned:
            enc = encode_thumb_bl(addr, target & ~1)
            f.seek(vaddr_to_offset(secs, addr))
            f.write(enc)

    # Verificacion: re-desensamblar y confirmar que ahora apunta al wrapper.
    dis2 = disassemble(elf)
    for addr, _target, caller, new in planned:
        sites = find_call_sites(dis2, caller, new)
        if addr not in sites:
            die(f"verificacion fallida: 0x{addr:08x} en {caller} no quedo "
                f"apuntando a {new}")
        print(f"patch_vm_calls.py: 0x{addr:08x} {caller} -> {new}  OK")

    print(f"patch_vm_calls.py: {len(planned)} llamada(s) redirigida(s) "
          f"y verificada(s) en {os.path.basename(elf)}")


if __name__ == "__main__":
    main()
