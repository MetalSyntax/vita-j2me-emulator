#!/usr/bin/env python3
"""
check_contract.py — verifica que el contrato de empaquetado entre
javax.microedition.m3g.M3GNative (Java) y m3g_engine.h (C) siga
sincronizado. Los indices IP_*/FP_*/LP_* estan duplicados a mano en los
dos lados porque KNI no permite compartir constantes; este script es la
red que evita que se desincronicen en silencio.

Uso:  python3 jsr184/tools/check_contract.py   (desde la raiz del repo)
Sale con codigo 1 si hay algun desajuste.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
H = os.path.join(ROOT, 'src', 'engine', 'm3g_engine.h')
J = os.path.join(ROOT, 'src', 'core', 'classes', 'javax', 'microedition',
                 'm3g', 'M3GNative.java')


def c_enum(h, block_start, prefix):
    i = h.index(block_start)
    body = h[i:h.index('};', i)]
    body = re.sub(r'/\*.*?\*/', '', body, flags=re.S)
    vals = {}
    nxt = 0
    for tok in body.split(','):
        tok = tok.strip()
        if not tok:
            continue
        if '=' in tok:
            name, expr = tok.split('=', 1)
            name = name.strip()
            expr = expr.strip().replace('M3G_MAX_LIGHTS', '8')
            for k, v in sorted(vals.items(), key=lambda kv: -len(kv[0])):
                expr = expr.replace(k, str(v))
            nxt = eval(expr)
        else:
            name = tok
        if not name.startswith(prefix):
            continue
        vals[name] = nxt
        nxt += 1
    return vals


def main():
    h = open(H).read()
    j = open(J).read()

    groups = {
        'IP': c_enum(h, '    M3G_IP_VERTEX_COUNT = 0', 'M3G_IP_'),
        'FP': c_enum(h, '    M3G_FP_MODELVIEW = 0', 'M3G_FP_'),
        'LP': c_enum(h, '    M3G_LP_MODE = 0', 'M3G_LP_'),
    }

    jvals = {}
    for m in re.finditer(r'static final int (\w+)\s*=\s*(\d+)\s*;', j):
        jvals[m.group(1)] = int(m.group(2))
    for m in re.finditer(
            r'static final int (\w+)\s*=\s*(\d+)\s*\+\s*MAX_LIGHTS\s*\*\s*(\d+)\s*;',
            j):
        jvals[m.group(1)] = int(m.group(2)) + 8 * int(m.group(3))

    bad = 0
    checked = 0
    for vals in groups.values():
        for cname, val in vals.items():
            jname = cname[len('M3G_'):]
            if jname not in jvals:
                continue          # relleno que solo existe en C
            checked += 1
            if jvals[jname] != val:
                print('DESAJUSTE %s: C=%d Java=%d' % (jname, val, jvals[jname]))
                bad += 1

    print('%d constantes del contrato comprobadas, %d desajustes'
          % (checked, bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
