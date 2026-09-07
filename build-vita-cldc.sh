#!/bin/sh
# build-vita-cldc.sh — Fase 2b: build completo phoneME para Vita.
# Estado: FUNCIONAL en macOS/arm64 (probado 2026-09-03, genera las 8 .a).
#
# Artefactos finales en ../_prebuilt/lib/:
#   libjavacall.a libcldc_vm.a libmidp.a libpcsl_*.a (ARM)
#   classes.zip (MIDP) + cldc_classes.zip
#
# Requisitos en el Mac:
#   - vitasdk:        export VITASDK=~/vitasdk
#   - JDK 8:          AdoptOpenJDK/Temurin 8 (javac -source 1.4 + tools)
#   - Docker Desktop: SOLO para generar OopMaps.cpp (romgen 32-bit).
#     Sin Docker no hay OopMaps (el VM C-interp de 2006 asume host 32b).
#   - Red: descarga ProGuard 7.4.2 + Xalan-J 2.7.3 (una vez, a /tmp/vitaj2me).
#
# Limitaciones conocidas del árbol 2006 (documentadas, no bugs nuestros):
#   - Los makefiles no toleran espacios: se compila en una copia sin
#     espacios (/tmp/vitaj2me-real) y se reimportan solo los artefactos.
#   - preverify/lib de host: wrappers en /tmp/vitaj2me/hostbin (ver abajo).
#   - ROM CVM desactivada (ENABLE_ROM_GENERATOR=false): sin ROM binaria,
#     classes.zip en runtime. ROMImage.cpp de MIDP SÍ se genera (Docker).
#
# Uso:
#   ./build-vita-cldc.sh [-C] [-d]
#
# Pasos manuales extra (una vez): ver BUILDING_VITA.txt (Docker/OopMaps).

USAGE="`basename $0`: [-Cdh]"
HELP="Options:
-C               clean antes de compilar (borra outputs en la copia /tmp)
-d               debug build (USE_DEBUG=true)
-h               esta ayuda"

if [ -z "${VITASDK}" ]; then
    echo "ERROR: VITASDK no definido. Ej: export VITASDK=\$HOME/vitasdk"
    exit 1
fi

unset DEBUG_OPTION
unset CLEAN

while getopts \?Cdh c
do
    case $c in
    C)  CLEAN="true";;
    d)  DEBUG_OPTION="USE_DEBUG=true";;
    h)  echo "${USAGE}"
        echo "${HELP}"
        exit 2;;
    \?) echo "${USAGE}"
        echo "${HELP}"
        exit 2;;
    esac
done

HERE=`pwd`
WS_REAL="${HERE}/pspkvm"
BUILDROOT=/tmp/vitaj2me-real
R=${BUILDROOT}/pspkvm
TOOLS=/tmp/vitaj2me
JDK8=/Library/Java/JavaVirtualMachines/adoptopenjdk-8.jdk/Contents/Home

if [ ! -x "${JDK8}/bin/javac" ]; then
    echo "ERROR: JDK 8 no encontrado en ${JDK8}"
    exit 1
fi

echo "-------------- BUILD VITA_JAVACALL (Fase 2b) ----------------"
echo "VITASDK=${VITASDK}  JDK8=${JDK8}"

# --- 0. Copia sin espacios (los makefiles de 2006 parten rutas con ' ') ---
if [ "${CLEAN}" = "true" ]; then
    rm -rf "${BUILDROOT}"
fi
mkdir -p "${BUILDROOT}"
rsync -a --exclude=.git --exclude=output --exclude=dist \
    "${WS_REAL}/" "$R/"
echo "fuentes sincronizadas a ${R}"

# NOTA: output/ y dist/ NO se sincronizan del repo (contienen rutas
# absolutas generadas); se reconstruyen aquí. Los .a/.zip finales se
# copian a ../_prebuilt/lib al final.

# --- 1. Toolchain y wrappers de host (ver build-tools/README.md) ---
mkdir -p "${TOOLS}/hostbin"
echo "--- [1/6] wrappers de host ---"
cp "${HERE}/build-tools/hostcxx.sh" "${TOOLS}/hostbin/hostcxx.sh"
for t in cc gcc c++ g++; do ln -sf hostcxx.sh "${TOOLS}/hostbin/$t"; done
cp "${HERE}/build-tools/java8wrap.sh" "${TOOLS}/hostbin/java8"
cp "${HERE}/build-tools/javac14.sh" "${TOOLS}/hostbin/javac14"
cp "${HERE}/build-tools/xalan-java.sh" "${TOOLS}/hostbin/xalan-java"
cp "${HERE}/build-tools/preverify.sh" "${TOOLS}/preverify"
chmod +x "${TOOLS}/hostbin/"* "${TOOLS}/preverify"
# Ajusta rutas absolutas a esta máquina (los scripts traen /tmp y JDK8
# por defecto; si tu JDK8 está en otro sitio, edítalos).
# Descargas externas (una vez, ver BUILDING_VITA.txt:1b):
#   proguard.jar   <- proguard-7.4.2.zip (GitHub Guardsquare, lib/)
#   xalan.jar      <- Maven Central xalan/xalan/2.7.3
#   serializer.jar <- Maven Central xalan/serializer/2.7.3
if [ ! -f "${TOOLS}/proguard.jar" ] || [ ! -f "${TOOLS}/xalan.jar" ]; then
    echo "FALTA: proguard.jar/xalan.jar en ${TOOLS} (ver BUILDING_VITA.txt:1b)"
    exit 1
fi

export PATH="${TOOLS}/hostbin:${VITASDK}/bin:${PATH}"
JAVACALL_OUT="$R/javacall/configuration/phoneMEFeature/vita_arm/output"
PCSL_OUT="$R/pcsl/output"
CLDC_DIST="$R/cldc/build/javacall_vita_gcc/dist"
export PREVERIFY_EXTRA_LIBS="${CLDC_DIST}/lib/cldc_classes.zip"

CLDC_VARS="JDK_DIR=${JDK8} JAVA=${TOOLS}/hostbin/java8
    GNU_TOOLS_DIR=${VITASDK} ENABLE_ISOLATES=true ENABLE_PCSL=true
    ENABLE_COMPILATION_WARNINGS=true PCSL_OUTPUT_DIR=${PCSL_OUT}
    PCSL_DIST_DIR=${PCSL_OUT}/javacall_arm
    JAVACALL_OUTPUT_DIR=${JAVACALL_OUT} JVMWorkSpace=$R/cldc
    PREVERIFY=${TOOLS}/preverify"

# --- 2. javacall (libjavacall.a) ---
echo "--- [2/6] javacall (vita_arm) ---"
make -C "$R/javacall/configuration/phoneMEFeature/vita_arm" \
    JAVACALL_DIR="$R/javacall" JAVACALL_OUTPUT_DIR="${JAVACALL_OUT}" \
    USE_JSR_135=true USE_JSR_75=true USE_JSR_179=true \
    USE_PROPERTIES_FROM_FS=true ${DEBUG_OPTION}
if [ $? != 0 ]; then echo "ERROR en javacall"; exit 1; fi

# --- 3. pcsl ---
echo "--- [3/6] pcsl (javacall_vita_gcc) ---"
make -C "$R/pcsl" PCSL_DIR="$R/pcsl" \
    JAVACALL_OUTPUT_DIR="${JAVACALL_OUT}" \
    PCSL_PLATFORM=javacall_vita_gcc PCSL_OUTPUT_DIR="${PCSL_OUT}" \
    GNU_TOOLS_DIR=${VITASDK} ${DEBUG_OPTION}
if [ $? != 0 ]; then echo "ERROR en pcsl"; exit 1; fi

# --- 4. cldc (libcldc_vm_r.a, SIN romgen: ver paso 5 Docker) ---
# Fase 2b-bug#7: "_release" no define -DPRODUCT (solo "_product" lo
# hace, ver cldc/build/share/jvm.make: CPP_DEF_FLAGS_product=-DPRODUCT).
# Sin -DPRODUCT queda compilado el auto-chequeo de oopmaps de depuracion
# (loopgen_check_oopmaps/romgen_check_oopmaps -> jvm_check_oopmap, todo
# bajo #ifndef PRODUCT en JVM.cpp/Generator.cpp) que nunca se probo en
# HW real -- crasheaba con Prefetch Abort al llegar por primera vez a
# JVM::start() (confirmado en consola, dump psp2core-1788464409-...).
# Se probo migrar todo el build a "_product" para sacar ese codigo, pero
# abria mas huecos de los que cerraba (BUILD_EXT_product vacio -> nombre
# de lib distinto, y referencias sin resolver a jvm_check_oopmap desde
# el ROMImage.cpp generado y a _jvm_in_raw_pointers_block desde
# m3g_kni.c, ambos simbolos que "_product" tambien elimina). Se revirtio
# a "_release" (probado en 6+ sesiones) y en su lugar se parcheo
# JVM.cpp para excluir solo esas dos llamadas cuando VITA esta definido
# (ver runtime/JVM.cpp, JVM::start()) -- cambio minimo, sin tocar el
# resto del pipeline de build ni el codegen de otras plataformas.
echo "--- [4/6] cldc target (make _release, ROMIZING=false) ---"
echo "NOTA: requiere OopMaps.cpp (paso 5, Docker) + romgen.exe falso"
echo "con fecha vieja en cldc/build/javacall_vita_gcc/romgen/app/."
cd "$R/cldc/build/javacall_vita_gcc" && \
make _release ROMIZING=false ${CLDC_VARS} && cd "${HERE}"
if [ $? != 0 ]; then echo "ERROR en cldc"; exit 1; fi

# --- 5. midp (libmidp.a + classes.zip) ---
echo "--- [5/6] midp (javacall_vita) ---"
echo "NOTA: requiere ROMImage.cpp (paso 5, Docker)."
cd "$R/midp/build/javacall_vita" && make \
    JDK_DIR=${JDK8} JAVA=${TOOLS}/hostbin/java8 \
    JAVAC_CMD=${TOOLS}/hostbin/javac14 \
    USE_NATIVE_AMS=false USE_CLDC_RELEASE=true \
    JAVACALL_PLATFORM=vita_arm JAVACALL_OUTPUT_DIR="${JAVACALL_OUT}" \
    PCSL_OUTPUT_DIR="${PCSL_OUT}" CLDC_DIST_DIR="${CLDC_DIST}" \
    TOOLS_DIR="$R/tools" RESTRICTED_CRYPTO_DIR="$R/restricted_crypto" \
    USE_MULTIPLE_ISOLATES=true USE_NOKIA_UI=true NOKIA_UI_DIR="$R/ext/nokia" \
    USE_PSPKVM_EXT=true PSPKVM_EXT_DIR="$R/ext/pspkvm" \
    USE_JSR_75=true JSR_75_DIR="$R/jsr75" \
    JSR_135_DIR="$R/jsr135" JSR_120_DIR="$R/jsr120" \
    USE_JSR_179=true JSR_179_DIR="$R/jsr179" \
    JSR_172_DIR="$R/jsr172" USE_JSR_172=true \
    USE_JSR_226=true JSR_226_DIR="$R/jsr226" \
    USE_JSR_184=true JSR_184_DIR="$R/jsr184" SWERVE_DIR="$R/jsr184" \
    PISCES_DIR="$R/pisces" JPEG_DIR="$R/jpeg" \
    SOFT_KEYBOARD_ENABLED=true PREVERIFY=${TOOLS}/preverify \
    /private/tmp/vitaj2me-real/pspkvm/midp/build/javacall_vita/output/bin/arm/libmidp.a \
    && cd "${HERE}"
if [ $? != 0 ]; then echo "ERROR en midp"; exit 1; fi

# --- 6. Reimportar artefactos a ../_prebuilt/lib ---
echo "--- [6/6] staging en _prebuilt/lib ---"
mkdir -p "${HERE}/_prebuilt/lib"
cp "${JAVACALL_OUT}/lib/libjavacall.a" "${HERE}/_prebuilt/lib/"
cp "${PCSL_OUT}/javacall_arm/lib/libpcsl_"*.a "${HERE}/_prebuilt/lib/"
cp "${CLDC_DIST}/lib/libcldc_vm_r.a" \
   "${CLDC_DIST}/lib/cldc_classes.zip" "${HERE}/_prebuilt/lib/"
cp "$R/midp/build/javacall_vita/output/bin/arm/libmidp.a" \
   "$R/midp/build/javacall_vita/output/classes.zip" "${HERE}/_prebuilt/lib/"
ls -la "${HERE}/_prebuilt/lib/"
echo "OK. Reconstruye el VPK con: cmake --build vita/build"
