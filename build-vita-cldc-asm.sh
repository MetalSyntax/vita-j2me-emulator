#!/bin/sh
# build-vita-cldc-asm.sh — VM CLDC con el intérprete ARM generado por
# loopgen (ENABLE_C_INTERPRETER=false, sin JIT) y ROMIZING=true (clases
# CLDC+MIDP romizadas en libmidp.a). Variante de build-vita-cldc.sh.
# Probado 2026-11-05 en macOS/arm64 + Docker Desktop (i386 por QEMU).
#
# Artefactos -> _prebuilt/lib-asm/ (NO toca _prebuilt/lib ni _prebuilt/data):
#   libcldc_vm_r.a libmidp.a classes.zip cldc_classes.zip
#   Interpreter_arm.s ROMImage.cpp.gz ROMStructs.h.gz   (ver su README)
# libjavacall.a y libpcsl_*.a no cambian: se siguen usando los de _prebuilt/lib.
#
# Cómo (ver pspkvm/cldc/build/javacall_vita_asm/javacall_vita_asm.cfg):
#   - HOST tools (tools, loopgen, romgen) se compilan y ejecutan en Linux
#     i386 (contenedor Docker "vitaasm", mismo path montado).
#   - TARGET (CLDC _release y MIDP) en macOS con arm-vita-eabi-*; los
#     binarios loopgen/romgen se llaman vía wrappers `docker exec`.
#
# Requisitos: VITASDK, JDK8 (adoptopenjdk-8), Docker Desktop encendido,
# /tmp/vitaj2me/{proguard,xalan,serializer}.jar (ver BUILDING_VITA.txt).
# Imagen Docker: vitaj2me-i386:stretch (gcc/g++ i386 + openjdk-8 + make);
# si no existe se crea desde i386/debian:buster-slim con build-tools/deb-setup.sh.
#
# Uso: ./build-vita-cldc-asm.sh [-H]
#   -H  toma pspkvm/javacall de git HEAD (si hay ediciones a medias en él)

set -e
[ -n "${VITASDK}" ] || { echo "ERROR: VITASDK no definido"; exit 1; }
HERE=`pwd`
B=/private/tmp/vitaj2me-asm
R=$B/pspkvm
TOOLS=/tmp/vitaj2me
JDK8=/Library/Java/JavaVirtualMachines/adoptopenjdk-8.jdk/Contents/Home
IMG=vitaj2me-i386:stretch
CT=vitaasm
OUT="${HERE}/_prebuilt/lib-asm"

docker info >/dev/null 2>&1 || { echo "ERROR: Docker no responde (enciende Docker Desktop)"; exit 1; }
for j in proguard xalan serializer; do
    [ -f $TOOLS/$j.jar ] || { echo "FALTA $TOOLS/$j.jar (BUILDING_VITA.txt:1b)"; exit 1; }
done

echo "--- [0/7] copia sin espacios en $R ---"
mkdir -p $B
rsync -a --exclude=.git --exclude=output --exclude=dist "${HERE}/pspkvm/" "$R/"
if [ "$1" = "-H" ]; then
    rm -rf "$R/javacall"; git archive HEAD pspkvm/javacall | tar -x -C $B
fi
# jsr184: properties_jsr184.xml pide configuration.dtd a su lado.
cp "$R/midp/src/configuration/configuration_xml/share/configuration.dtd" "$R/jsr184/src/config/"
# jsr184: sin JPP ENABLE_JSR_184. El classes.zip desplegado no llama a
# com.sun.midp.jsr184.Initializer (que no existe); con el define, javac falla.
sed -i '' 's/^JPP_DEFS     += -DENABLE_JSR_184/# (asm build) sin JPP ENABLE_JSR_184/' "$R/jsr184/src/config/subsystem.gmk"
mkdir -p "$R/cldc/build/classes"   # preverify lo usa como -libraryjars antes de existir

echo "--- [1/7] wrappers de host ---"
mkdir -p $TOOLS/hostbin
cp "${HERE}/build-tools/java8wrap.sh" $TOOLS/hostbin/java8
cp "${HERE}/build-tools/javac14.sh" $TOOLS/hostbin/javac14
cp "${HERE}/build-tools/xalan-java.sh" $TOOLS/hostbin/xalan-java
cp "${HERE}/build-tools/preverify.sh" $TOOLS/preverify
chmod +x $TOOLS/hostbin/* $TOOLS/preverify
export PATH="$TOOLS/hostbin:${VITASDK}/bin:${PATH}"
JO=$R/javacall/configuration/phoneMEFeature/vita_arm/output
PO=$R/pcsl/output
C=$R/cldc/build/javacall_vita_asm
CD=$C/dist

echo "--- [2/7] javacall + pcsl (solo para headers/consistencia) ---"
make -C $R/javacall/configuration/phoneMEFeature/vita_arm JAVACALL_DIR=$R/javacall \
    JAVACALL_OUTPUT_DIR=$JO USE_JSR_135=true USE_JSR_75=true USE_JSR_179=true \
    USE_PROPERTIES_FROM_FS=true
make -C $R/pcsl PCSL_DIR=$R/pcsl JAVACALL_OUTPUT_DIR=$JO PCSL_PLATFORM=javacall_vita_gcc \
    PCSL_OUTPUT_DIR=$PO GNU_TOOLS_DIR=${VITASDK}

echo "--- [3/7] contenedor i386 ---"
if ! docker image inspect $IMG >/dev/null 2>&1; then
    docker run --platform linux/386 --name vitaasm-setup -v "${HERE}/build-tools:/bt:ro" \
        i386/debian:buster-slim bash /bt/deb-setup.sh
    docker commit vitaasm-setup $IMG && docker rm vitaasm-setup
fi
docker inspect $CT >/dev/null 2>&1 || docker run -d --platform linux/386 --name $CT \
    -v $B:$B -v /private/tmp/vitaj2me:/tmp/vitaj2me $IMG sleep infinity
docker start $CT >/dev/null

echo "--- [4/7] cldc HOST: tools + loopgen + romgen (Linux i386) ---"
# Si ya estaban los wrappers, vuelve a poner los binarios reales para que
# make no los vea "más nuevos" ni los pise a medias.
for f in $C/loopgen/app/loopgen $C/romgen/app/romgen; do
    [ -f $f.linux ] && mv -f $f.linux $f
done
docker exec -w $C -e LANG=C.UTF-8 -e LC_ALL=C.UTF-8 \
    -e JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF-8 $CT \
    make tools loopgen romgen JDK_DIR=/usr/lib/jvm/java-8-openjdk-i386 \
    ENABLE_ISOLATES=true ENABLE_COMPILATION_WARNINGS=true JVMWorkSpace=$R/cldc \
    PREVERIFY=/tmp/vitaj2me/preverify JAVACALL_OUTPUT_DIR=$JO
# Wrappers: los binarios Linux se ejecutan en el contenedor (mismo path).
for f in $C/loopgen/app/loopgen $C/romgen/app/romgen; do
    mv -f $f $f.linux
    printf '#!/bin/sh\nexec docker exec -w "$(pwd -P)" -e LANG=C.UTF-8 %s "$0.linux" "$@"\n' $CT > $f
    chmod +x $f
done
printf '#!/bin/sh\nexec docker exec -w "$(pwd -P)" -e LANG=C.UTF-8 %s %s "$@"\n' $CT $C/romgen/app/romgen.linux > $CD/bin/romgen
chmod +x $CD/bin/romgen
cp $TOOLS/preverify $CD/bin/preverify   # MIDP la copia a su bin/

echo "--- [5/7] cldc TARGET (make _release, ROMIZING=true, ARM) ---"
rm -f $C/target/release/Interpreter_arm.s   # loopgen nuevo -> loop nuevo
(cd $C && make _release JDK_DIR=$JDK8 JAVA=$JDK8/bin/java GNU_TOOLS_DIR=${VITASDK} \
    ENABLE_ISOLATES=true ENABLE_PCSL=true ENABLE_COMPILATION_WARNINGS=true \
    PCSL_OUTPUT_DIR=$PO PCSL_DIST_DIR=$PO/javacall_arm JAVACALL_OUTPUT_DIR=$JO \
    JVMWorkSpace=$R/cldc PREVERIFY=$TOOLS/preverify)

echo "--- [6/7] midp (classes.zip + ROMImage.cpp vía romgen + libmidp.a) ---"
MO=$R/midp/build/javacall_vita/output
mkdir -p $MO/obj/arm $MO/bin/arm
[ -s $MO/generated/merged.xml ] || rm -f $MO/generated/merged.xml
export PREVERIFY_EXTRA_LIBS=$CD/lib/cldc_classes.zip
midp_make() {
    (cd $R/midp/build/javacall_vita && make \
    JDK_DIR=$JDK8 JAVA=$TOOLS/hostbin/java8 JAVA_CMD=$TOOLS/hostbin/xalan-java \
    JAVAC_CMD=$TOOLS/hostbin/javac14 \
    USE_NATIVE_AMS=false USE_CLDC_RELEASE=true \
    JAVACALL_PLATFORM=vita_arm JAVACALL_OUTPUT_DIR=$JO \
    PCSL_OUTPUT_DIR=$PO CLDC_DIST_DIR=$CD \
    TOOLS_DIR=$R/tools RESTRICTED_CRYPTO_DIR=$R/restricted_crypto \
    USE_MULTIPLE_ISOLATES=true USE_NOKIA_UI=true NOKIA_UI_DIR=$R/ext/nokia \
    USE_PSPKVM_EXT=true PSPKVM_EXT_DIR=$R/ext/pspkvm \
    USE_JSR_75=true JSR_75_DIR=$R/jsr75 \
    JSR_135_DIR=$R/jsr135 JSR_120_DIR=$R/jsr120 \
    USE_JSR_179=true JSR_179_DIR=$R/jsr179 \
    JSR_172_DIR=$R/jsr172 USE_JSR_172=true \
    USE_JSR_226=true JSR_226_DIR=$R/jsr226 \
    USE_JSR_184=true JSR_184_DIR=$R/jsr184 SWERVE_DIR=$R/jsr184 \
    PISCES_DIR=$R/pisces JPEG_DIR=$R/jpeg \
    SOFT_KEYBOARD_ENABLED=true PREVERIFY=$TOOLS/preverify "$@")
}
# "all" genera classes.zip, ROMImage.cpp/ROMStructs.h y los .o; falla al
# final enlazando el ejecutable runMidlet de Linux (no se usa): se ignora.
midp_make || echo "(fallo esperado al enlazar runMidlet; sigue con libmidp.a)"
midp_make $MO/bin/arm/libmidp.a

echo "--- [7/7] staging en $OUT ---"
mkdir -p "$OUT"
cp $CD/lib/libcldc_vm_r.a $CD/lib/cldc_classes.zip $MO/bin/arm/libmidp.a $MO/classes.zip "$OUT/"
cp $C/target/release/Interpreter_arm.s "$OUT/"
gzip -9 -c $MO/ROMImage.cpp > "$OUT/ROMImage.cpp.gz"
gzip -9 -c $MO/ROMStructs.h > "$OUT/ROMStructs.h.gz"
rm -f "$OUT"/._*
# Verificación: el intérprete generado está y no hay bl condicional reubicado.
N=${VITASDK}/bin/arm-vita-eabi-
${N}nm "$OUT/libcldc_vm_r.a" 2>/dev/null | grep -q " T interpreter_method_entry" || { echo "ERROR: falta el loop ARM"; exit 1; }
if ${N}objdump -d "$OUT/libcldc_vm_r.a" | grep -qE "\sbl(eq|ne|lt|gt|le|ge|cs|cc|hi|ls|mi|pl|vs|vc|hs|lo)\s"; then
    echo "ERROR: bl condicional en la VM (el loader de Vita lo rompe, ver README)"; exit 1
fi
ls -la "$OUT"
echo "OK. Enlazar con: cmake -S vita -B vita/build-asm -DVJ_VM=asm (ver _prebuilt/lib-asm/README.md)"
