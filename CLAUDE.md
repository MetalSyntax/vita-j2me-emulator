# vita-j2me-emulator — Emulador J2ME nativo para PS Vita

> **Antes de tocar nada, sigue `.claude/skills/vitaj2me-dev/SKILL.md`**
> (skill `vitaj2me-dev`): compilar, desplegar (`toolkit/deploy.py`),
> diagnosticar, parchear Java (`toolkit/java_patch.py`), ajustes en 3
> idiomas, documentación (`port_progress.md`, README, RELEASE), commits y
> release. Responde al usuario en español.

**Este proyecto es independiente del port de arriba.** El `CLAUDE.md` de
`Soul-Of-Darkness-vita/` (la carpeta padre) describe un port Android vía
SoLoader/FalsoJNI y dice "usar **psvita-port-toolkit**, la herramienta
standalone" para todo build/deploy/debug — esa instrucción **NO aplica
acá**. `vita-j2me-emulator` no es un port de una APK: es PSPKVM/phoneME
(una JVM completa) portado de MIPS/PSP a ARMv7/Vita. No hay ningún
`.so` de Android, ni JNI, ni jadx, ni Ghidra de por medio.

## Herramienta a usar: SIEMPRE la local

```sh
./toolkit/vitaj2me-toolkit
# o
python3 toolkit/vitaj2me_toolkit.py
```

**No uses el comando global `psvita-toolkit`** (el de
`psvita-port-toolkit-cli`) para nada de este proyecto — ni build, ni
deploy, ni sobre todo **análisis de crash dumps**. Ya pasó en la
práctica (ver `port_progress.md`, Bug real #5): correr
`psvita-toolkit analyze ... --project "/Volumes/Seagate/PSVITA Develop/Soul-Of-Darkness-vita"`
contra un dump de `vitaj2me` resolvió los registros contra símbolos de
`libmidi.so` — una librería **decompilada del otro proyecto**
(`decompiled/libmidi_*`), completamente ajena a este ejecutable nativo.
Fue pura coincidencia de rangos de direcciones del heurístico de
detección de base de `.so` de esa herramienta (diseñada para ports
Android), y el resultado fue una pista falsa que casi lleva a
diagnosticar mal el crash. Ese heurístico no tiene sentido acá: no hay
ningún `.so` que detectar, siempre hay un único ELF
(`vita/build/vitaj2me`).

Ver `toolkit/README.md` para qué hace cada opción (subir VPK/eboot,
bajar logs y crash dumps, analizarlos con `vita-parse-core`, compilar,
limpiar basura de macOS) y por qué es deliberadamente más chica que la
herramienta global.

## Estructura

- `pspkvm/` — clon de PSPKVM (`vadosnaprimer/pspkvm`) con las
  adaptaciones del port (Fases 1-6) aplicadas directamente encima:
  `javacall/implementation/vita_arm/` (APIs traducidas PSP→Vita),
  `jsr184/` (M3G, Fase 5, módulo nuevo y autocontenido).
- `vita/` — frontend nativo (`main.c`) + `CMakeLists.txt`. Enlaza
  contra los `.a` de `_prebuilt/lib/` (NO recompila `pspkvm/` por sí
  solo — ver más abajo).
- `_prebuilt/lib/` — binarios ARM ya compilados de la JVM completa
  (Fase 2b: `libjavacall.a`, `libcldc_vm_r.a`, `libmidp.a`, 5×pcsl,
  `classes.zip`). **Se commitean** (no son build output descartable):
  reproducirlos de cero implicó JDK8 parcheado + Docker i386 para
  romgen + wrappers clang (ver `Comments.md`), no es un `make` trivial.
- `build-vita-cldc.sh` — orquestador de los 4 pasos de la Fase 2b
  (javacall → pcsl → cldc → midp). Si tocás código dentro de
  `pspkvm/javacall/`, alcanza con re-correr el paso `[1/4]` (javacall)
  y copiar el `.a` resultante a `_prebuilt/lib/` — no hace falta la
  cadena completa (pcsl/cldc/midp no cambiaron).
- `PORTING_PLAN_J2ME.md` / `port_progress.md` / `Comments.md` — plan y
  bitácora de ESTE proyecto (no confundir con los `.md` de la raíz, que
  son del port Android).

### Gotcha de rutas: el proyecto vive bajo una carpeta con espacio

`/Volumes/Seagate/PSVITA Develop/...` tiene un espacio en "PSVITA
Develop". Los `include` de GNU Make no toleran rutas con espacio (no
hay forma de quotearlos dentro de una directiva `include`), así que
`make` dentro de `pspkvm/javacall/configuration/phoneMEFeature/vita_arm/`
falla si le pasás `JAVACALL_DIR` con la ruta real. Ya existe un symlink
sin espacios para esto: `/private/tmp/vitaj2me/pspkvm ->
.../vita-j2me-emulator/pspkvm`. Usalo como `JAVACALL_DIR` al invocar
ese `make` a mano.

## Flujo de trabajo esperado

1. Un bug confirmado a la vez, con evidencia real (dump/log de la
   consola vía el toolkit local) — no supuestos.
2. Antes de creer una resolución de símbolos, chequear si
   `vita/build/vitaj2me` es más nuevo que el dump: si se recompiló
   después del crash, la resolución puede ser incorrecta (el toolkit
   local ya avisa esto solo).
3. Actualizar `port_progress.md` con cada bug confirmado.
