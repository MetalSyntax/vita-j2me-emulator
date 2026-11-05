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
set(VITA_VERSION "01.00")

add_definitions(-D_GNU_SOURCE)

# flags ARM Vita: Cortex-A9, softfp (igual que el resto del proyecto)
# -marm: el ASM inline A32 de midp (gxj_graphics_asm.c) lo exige
# (vitasdk compila en Thumb-2 por defecto).
#
# -Wl,--pic-veneer (Bug real #14 -- SIN esto el emulador no arranca):
# libmidp.a es el unico .a en ARM; cldc/javacall/pcsl/newlib son Thumb.
# Cuando una funcion ARM hace un TAIL CALL (`b`) a una Thumb, A32 no tiene
# "branch with exchange" PC-relativo, asi que ld interpone un veneer
# .glue_7 con direccion ABSOLUTA:
#     ldr pc, [pc, #-4] ; .word <destino>
# Ese .word necesitaria una R_ARM_ABS32, pero ld NO emite relocaciones para
# los stubs que el mismo genera -- ni con -Wl,-q (verificado: las relocs de
# .rel.text cortan en 0x81172de0 y los veneers arrancan en 0x81172df0). Sin
# reloc, vita-elf-create no tiene nada que convertir y el .word queda
# congelado en la direccion de LINK; la Vita carga el modulo con delta
# (+0x4000 en el build 019) y el salto cae fuera del modulo -> Prefetch
# Abort. Habia 68 de estos tail calls, todos bombas de tiempo; el primero
# que se pisa al arrancar es JVMSPI_SetSystemProperty -> setSystemProperty.
# --pic-veneer fuerza veneers PC-relativos, que no necesitan relocacion.
# (Los veneers Thumb->ARM, `bx pc; b <destino>`, ya eran PC-relativos: por
# eso el mismo binario funcionaba en todo lo demas.)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -O2 -g -mfloat-abi=softfp -marm -fno-strict-aliasing")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--pic-veneer")

# Parches a la JVM interpuestos en tiempo de link (ver vita/vm_patches.c):
# recompilar libcldc_vm_r.a/libmidp.a esta bloqueado por el SIGSEGV de
# romgen.exe (Bug real #14), asi que los fixes de CLDC se aplican con
# --wrap hasta que ese bloqueador se destrabe. Los simbolos son los
# mangled de C++ (phoneME prefija las clases con JVM via macros).
#   #15: TaskMirror::clinit_list_remove(Oop*) derefiere NULL si la clase
#        no esta en la clinit list.
#   #28: pcsl_file_getpathseparator() devuelve ':' pero CLDC parte el
#        classpath por ';' (OsFile_javacall.hpp): "<jar>:classes.zip" era
#        UNA entrada -> "class not found: java/lang/Object" (HW 040).
set(VITAJ2ME_VM_WRAPS
  _ZN13JVMTaskMirror18clinit_list_removeEP6JVMOop
  pcsl_file_getpathseparator
  Java_java_lang_Thread_sleep
)
foreach(sym ${VITAJ2ME_VM_WRAPS})
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--wrap=${sym}")
endforeach()

add_executable(${PROJECT_NAME} main.c launcher.c gamecfg.c support.c vm_patches.c)

# vm_patches.c va en Thumb (el resto del frontend es -marm): los call
# sites que redirige patch_vm_calls.py son `bl` Thumb dentro de CLDC, y
# manteniendo el wrapper en Thumb el parche es un simple recalculo de
# desplazamiento, sin tener que convertir bl->blx ni cambiar de modo.
set_source_files_properties(vm_patches.c PROPERTIES COMPILE_OPTIONS "-mthumb")

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
  mpg123
  # AMR-NB (sesión 55): build-tools/build-opencore-amr.sh, Apache 2.0
  ${VITAJ2ME_LIBS}/libopencore-amrnb.a
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
  ScePgf_stub
  SceTouch_stub
  SceRtc_stub
  ScePower_stub
  SceLibKernel_stub
)

# Parche post-link: redirige al wrapper de vm_patches.c las llamadas que
# --wrap no puede alcanzar (merged sources de CLDC, sin relocacion). Se
# ejecuta sobre el ELF recien enlazado, ANTES de que vita_create_self lo
# convierta a .velf/eboot.bin. El script verifica su propio trabajo y
# devuelve error si no encuentra exactamente los call sites esperados.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E env "VITASDK=$ENV{VITASDK}"
          python3 "${CMAKE_SOURCE_DIR}/patch_vm_calls.py"
          "$<TARGET_FILE:${PROJECT_NAME}>"
  COMMENT "Parcheando llamadas internas de la JVM (vm_patches.c)"
  VERBATIM)

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
