cmake_minimum_required(VERSION 3.14)

# vita-j2me-emulator/vita/CMakeLists.txt — Fase 2 bringup
# Mapeo del antiguo psp/Makefile (pspsdk) al sistema moderno vitasdk.
#
# Uso:
#   export VITASDK=~/vitasdk
#   cmake -S vita -B vita/build -DCMAKE_BUILD_TYPE=Release
#   cmake --build vita/build
# Resultado: vitaj2me.vpk instalable (TITLEID VITAJ2ME0).

if (NOT DEFINED ENV{VITASDK} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "Define VITASDK (ej. export VITASDK=~/vitasdk)")
endif ()

set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "Toolchain file")

project(vitaj2me C)

include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "VitaJ2ME")
# TITLEID de exactamente 9 caracteres (el instalador da 0xF0030000 si no).
set(VITA_TITLEID "VITAJ2ME0")
set(VITA_VPKNAME "vitaj2me-bringup")
set(VITA_VERSION "00.60")

add_definitions(-D_GNU_SOURCE)

# flags ARM Vita: Cortex-A9, softfp (igual que el resto del proyecto)
# -marm: el ASM inline A32 de midp (gxj_graphics_asm.c) lo exige
# (vitasdk compila en Thumb-2 por defecto).
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -O2 -g -mfloat-abi=softfp -marm -fno-strict-aliasing")

add_executable(${PROJECT_NAME} main.c support.c)

# Headers javacall (logging, generados por el build de libjavacall).
target_include_directories(${PROJECT_NAME} PRIVATE
  "${CMAKE_SOURCE_DIR}/../pspkvm/javacall/configuration/phoneMEFeature/vita_arm/output/inc"
)

# Libs JVM Fase 2b (prebuilt en ../_prebuilt/lib, ver BUILDING_VITA.txt):
# midp -> cldc_vm -> javacall -> pcsl (grupo por dependencias cruzadas).
# m/z van al FINAL (con --as-needed el linker descarta lo que aún no se usa).
set(VITAJ2ME_LIBS "${CMAKE_SOURCE_DIR}/../_prebuilt/lib")
target_link_libraries(${PROJECT_NAME}
  -Wl,--start-group
  ${VITAJ2ME_LIBS}/libmidp.a
  ${VITAJ2ME_LIBS}/libcldc_vm_r.a
  ${VITAJ2ME_LIBS}/libjavacall.a
  stdc++
  ${VITAJ2ME_LIBS}/libpcsl_file.a
  ${VITAJ2ME_LIBS}/libpcsl_memory.a
  ${VITAJ2ME_LIBS}/libpcsl_print.a
  ${VITAJ2ME_LIBS}/libpcsl_string.a
  ${VITAJ2ME_LIBS}/libpcsl_network.a
  -Wl,--end-group
  -Wl,--whole-archive pthread -Wl,--no-whole-archive
  vita2d
  freetype
  png16
  jpeg
  bz2
  m
  z
  SceDisplay_stub
  SceCtrl_stub
  SceAudio_stub
  SceAppUtil_stub
  SceNet_stub
  SceNetCtl_stub
  SceGxm_stub
  SceCommonDialog_stub
  SceAppMgr_stub
  SceLibKernel_stub
)

vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)
# NOTA: vita_create_vpk(FILE ...) no tolera espacios en rutas absolutas
# (este repo vive en "PSVITA Develop/...": vita-pack-vpk parte el path).
# Se genera el VPK base y se inyecta el LiveArea con Python (ver patch_vpk.py).
file(COPY ${CMAKE_SOURCE_DIR}/livearea DESTINATION ${CMAKE_BINARY_DIR})
vita_create_vpk(${VITA_VPKNAME}.vpk ${VITA_TITLEID} eboot.bin
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
)
add_custom_target(${VITA_VPKNAME}-livearea ALL
  COMMAND python3 ${CMAKE_SOURCE_DIR}/patch_vpk.py ${CMAKE_BINARY_DIR}/${VITA_VPKNAME}.vpk ${CMAKE_BINARY_DIR}/livearea
  DEPENDS ${VITA_VPKNAME}.vpk-vpk
  VERBATIM
)
