#!/bin/bash
# hostcc wrapper (Fase 2b Vita): el build phoneME compila las HOST tools
# (romgen/loopgen) con Apple clang, pero les pasa flags solo-GCC.
# Se filtran aquí; el resto llega intacto a clang.
# Uso: c++|g++ -> clang++; cc|gcc -> clang.
REAL=/usr/bin/clang
case "$(basename "$0")" in
    *++*) REAL=/usr/bin/clang++;;
esac
ARGS=()
for a in "$@"; do
    case "$a" in
        -fno-optional-diags|-fexpensive-optimizations) ;;
        *) ARGS+=("$a");;
    esac
done
exec "$REAL" "${ARGS[@]}"
