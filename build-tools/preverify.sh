#!/bin/sh
# preverify wrapper (Fase 2b Vita): emula la CLI del preverificador CLDC
# de Sun (binario ausente para macOS-arm64) con ProGuard -microedition.
# Sintaxis Sun: preverify [-classpath CP] -d OUTDIR INDIR...
# ProGuard solo preverify: sin shrink/optimize/obfuscate, todo kept.
# Sun permite IN==OUT y solape input/library; ProGuard lo prohíbe:
# se usa staging y se excluyen los INDIR de -libraryjars.
# PREVERIFY_EXTRA_LIBS (separado por :) añade bibliotecas extra
# (p.ej. cldc_classes.zip al preverifyar MIDP).
PG=/tmp/vitaj2me/proguard.jar
CP=""
OUT=""
INJARS=""
INDIRS=""
while [ $# -gt 0 ]; do
    case "$1" in
        -classpath) CP="$2"; shift 2;;
        -cp) CP="$2"; shift 2;;
        -d) OUT="$2"; shift 2;;
        -*) shift;; # ignora otras flags Sun
        *) INJARS="$INJARS -injars $1"; INDIRS="${INDIRS:+$INDIRS }$1"; shift;;
    esac
done
if [ -z "$OUT" ] || [ -z "$INJARS" ]; then
    echo "uso: preverify [-classpath CP] -d OUTDIR INDIR..." >&2
    exit 2
fi
# libraryjars = entradas de CP que no sean INDIR + extras
LIBS=""
OLDIFS="$IFS"
IFS=":"
for e in $CP; do
    skip=0
    for d in $INDIRS; do
        if [ "$e" = "$d" ]; then skip=1; break; fi
    done
    if [ $skip -eq 0 ] && [ -n "$e" ]; then LIBS="$LIBS -libraryjars $e"; fi
done
for e in $PREVERIFY_EXTRA_LIBS; do
    if [ -n "$e" ]; then LIBS="$LIBS -libraryjars $e"; fi
done
IFS="$OLDIFS"
STAGE=$(mktemp -d /tmp/pgstage.XXXXXX) || exit 2
# shellcheck disable=SC2086
java -jar "$PG" $INJARS -outjars "$STAGE" \
    $LIBS \
    -microedition \
    -dontshrink -dontoptimize -dontobfuscate \
    -dontwarn -dontnote \
    -keep 'class ** { *; }'
RC=$?
if [ $RC -ne 0 ]; then
    rm -rf "$STAGE"
    exit $RC
fi
mkdir -p "$OUT"
cp -R "$STAGE"/. "$OUT"/
RC=$?
rm -rf "$STAGE"
exit $RC
