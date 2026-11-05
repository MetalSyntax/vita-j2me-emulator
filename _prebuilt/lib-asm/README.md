# _prebuilt/lib-asm — VM CLDC con intérprete ARM generado + ROM

Variante de la VM de `_prebuilt/lib/` (no la reemplaza). Generada con
`./build-vita-cldc-asm.sh -H` (2026-11-05), configuración
`pspkvm/cldc/build/javacall_vita_asm/javacall_vita_asm.cfg`, adaptada de
J2Vita (https://github.com/KiddRwxSsj/j2vita, GPLv2, mismo origen PSPKVM).

| Fichero | Qué es |
| --- | --- |
| `libcldc_vm_r.a` | CLDC `_release`: `ENABLE_C_INTERPRETER=false`, `ENABLE_COMPILER=false` (sin JIT), `ROMIZING=true`, `ENABLE_ISOLATES=true`, toda la VM en modo **ARM** (`-marm -mcpu=cortex-a9`, `ENABLE_THUMB_VM=false`), softfp, floats por runtime C con VFP (`ENABLE_SOFT_FLOAT=false`). Incluye `Interpreter_arm.o` (loop generado por loopgen). |
| `libmidp.a` | MIDP compilado contra los headers de esta VM, con `ROMImage.o` = TODAS las clases del sistema (CLDC+MIDP+JSR, 1348 clases, ~2 MB de objetos) romizadas por `romgen -romize =HeapCapacity96M`. |
| `classes.zip` | El `classes.zip` que se romizó. Bytecode idéntico al de `_prebuilt/lib/classes.zip` (solo difieren 4 clases en el atributo `SourceFile`). |
| `cldc_classes.zip` | Clases CLDC de este build. |
| `Interpreter_arm.s` | Loop ARM generado (para leer crash dumps: direcciones de `bc_impl_*`, `interpreter_*`, `shared_*`). |
| `ROMImage.cpp.gz`, `ROMStructs.h.gz` | Fuentes generadas de la ROM de MIDP (referencia/depuración). |

`libjavacall.a` y `libpcsl_*.a` NO cambian: se enlazan los de `_prebuilt/lib/`.

## Cómo se genera

- Host tools (`tools`, `loopgen`, `romgen`): Linux i386 en Docker
  (contenedor `vitaasm`, imagen `vitaj2me-i386:stretch`, gcc 6.3 + openjdk 8).
- Target (CLDC y MIDP): macOS con `arm-vita-eabi-*`; `loopgen`/`romgen`
  se ejecutan vía wrappers `docker exec` con el mismo path montado.
- Parches de fuente (en el repo): `cldc/src/vm/cpu/arm/InterpreterStubs_arm.cpp`
  y `SharedStubs_arm.cpp`: sin `bl` condicional hacia un símbolo (el loader de
  la Vita reubica `bleq/blne` como `beq/bne` sin LR; fix de J2Vita `69ef8c9c`).
- Solo en la copia de build (no en el repo): `configuration.dtd` junto a
  `jsr184/src/config/properties_jsr184.xml` y sin `JPP_DEFS += -DENABLE_JSR_184`
  (igual que el classes.zip desplegado).

## Enlazar

`vita/CMakeLists.txt` con la opción propuesta `-DVJ_VM=asm` (ver informe):
`cmake -S vita -B vita/build-asm -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DVJ_VM=asm`.
Con esta VM: sin `--wrap` de `clinit_list_remove` y sin `patch_vm_calls.py`
(el fix #15 ya está compilado en `TaskMirror.cpp`); `rom_data_init()` sobra
(`UseROM=1`, la VM copia el bloque) pero es inofensivo.

## Ojo

- Con ROM, las clases del sistema salen de la ROM, no de `classes.zip`:
  `toolkit/java_patch.py` ya NO tiene efecto en esta variante. Un cambio en
  Java exige re-ejecutar `build-vita-cldc-asm.sh`.
- Sin probar en HW.
