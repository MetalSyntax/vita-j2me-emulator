# Plan de Desarrollo: Emulador Nativo de J2ME para PS Vita (Basado en PSPKVM)

Este plan documenta la estrategia para migrar y actualizar **PSPKVM** (el emulador de Java ME original para PSP basado en la máquina virtual *phoneME Feature* de Sun) para que se ejecute de forma nativa en **PS Vita**, aprovechando la potencia del hardware moderno y la resolución de pantalla.

## 0. Contexto Técnico y Desafíos de la Migración
- **Arquitectura de Procesador (MIPS a ARMv7):** La PSP utilizaba procesadores MIPS, mientras que la PS Vita usa ARMv7. Afortunadamente, *phoneME* ya tiene soporte para ARM, pero será necesario ajustar las banderas de compilación y desactivar cualquier ensamblador específico de MIPS usado en el port de PSP.
- **Sistema Gráfico (GU a Vita2D/VitaGL):** PSPKVM dependía del motor gráfico de la PSP (Hardware GU o renderizado por software al framebuffer). Tendremos que reescribir la capa de renderizado utilizando **Vita2D** o accediendo a los framebuffers de la Vita.
- **Resolución de Pantalla:** PSPKVM asume una pantalla de 480x272. La PS Vita tiene 960x544 (exactamente el doble, un escalado 2x perfecto). Hay que ajustar el framebuffer virtual para escalar correctamente los juegos.
- **Entrada (Input):** Las APIs de Sony son similares, pero habrá que actualizar funciones de lectura de botones (ej. de `sceCtrlReadBufferPositive` de PSP a las funciones correspondientes del SDK de Vita).

## Fase 1: Investigación y Preparación del Código (Completada)
- [x] Crear repositorio y estructura de directorios.
- [x] Seleccionar el código fuente de PSPKVM en GitHub como base para no reinventar la rueda del frontend, la interfaz, los menús de selección de JAR y la configuración de teclas.
- [x] Clonar el repositorio.

## Fase 2: Adaptación del Entorno (Build System) y JVM Bringup — COMPLETADA
- [x] Mapear los antiguos Makefiles del PSPSDK a un sistema moderno (`vita/CMakeLists.txt` con `vitacmake`/`vita.cmake`; `psp/Makefile`+`build.mak` -> CMake).
- [x] Ajustar variables de entorno para usar el backend ARM de *phoneME* en lugar del de MIPS (configs nuevas `vita_arm`/`javacall_vita_gcc`; C-interpreter `arch=c` como en PSP, ver `vita/BUILDING_VITA.txt`).
- [x] Frontend nativo con "Hola Mundo" validado en HW (ver `port_progress.md` bug #1).
- [x] Fase 2b: JVM COMPLETA enlazada (eboot 3.5MB, 0 indefinidos): `libjavacall.a` + `libcldc_vm_r.a` (sin ROM; ROMImage MIDP sí generado) + `libmidp.a` + 5×pcsl + `classes.zip`/`cldc_classes.zip`. Frontend `vita/main.c`+`support.c` arranca AMS (`javanotify_start` → `JavaTask`). Receta reproducible en `build-vita-cldc.sh` + `build-tools/` + `vita/BUILDING_VITA.txt` (JDK8, ProGuard-preverify, Xalan-J, Docker i386 para romgen/OopMaps).

## Fase 3: Traducción de APIs del Sistema (PSP a Vita) — COMPLETADA (código; build total en 2b)
- [x] **Controles:** `vita_arm/midp/keypress.c`+`keymap.c` reescritos a `sceCtrl` Vita (`PeekBufferPositive(0)`, `.buttons/.lx/.ly`; bits `SCE_CTRL_*` idénticos a PSP; hilo 7 args). `input.c` locale vía `sceAppUtilSystemParamGetInt` (requiere `sceAppUtilInit` en frontend 2b).
- [x] **Almacenamiento (I/O y RecordStore):** JSR-75 `ms0:/` -> `ux0:/data/vitaj2me/{root,private,music,photo,video,recordings,tones,graphics}`; `dir.c` a `sceIoDopen` Vita (`SCE_SO_IFDIR`); `file/memory/logging` POSIX; `events.c` mailboxes (solo kernel) -> message pipes; `time.c` alarmas (solo kernel) -> hilo worker + `time()`; `network/socket.c` a `sceNetInit`+`sceNetCtl`+`getaddrinfo` (sin Apctl: la Vita conecta vía ajustes); `misc.c` relojes ignorados (Vita fija 444MHz).
- [x] **Audio:** `jsr135_mmapi/multimedia.c` sobre `SceAudioOut` 48kHz (WAV/PCM por buffering Java + tonos con seno; fallback MAIN->BGM). MIDI/MP3/vídeo/grabación -> FAIL seguro (decode software en Fase 5/6). JSR-179 GPS -> stub (sin GPS en Vita).
- [x] Verificación: 23/23 ficheros `vita_arm` pasan `arm-vita-eabi-gcc -fsyntax-only`, 0 errores (warnings = paridad PSP). Gráficos (`lcd/font/image`) quedan intactos para Fase 4.

## Fase 4: Gráficos y Escalado de Interfaz (Frontend) — COMPLETADA (código; test en HW en Fase 6)
- [x] Capa Canvas sobre vita2d (`vita_arm/midp/lcd.c`): textura 512x512 U5U6U5 (cero conversiones, scbuff RGB565 directo a GPU); se eliminan swizzle/GU/VRAM directa.
- [x] Escalado entero 2x (480x272 -> 960x544 pantalla completa) + aspect-fit/stretch con la misma política PSP (`fit_screen`); filtro configurable (`vita_filter`=nearest por defecto, linear=bilineal como GU_LINEAR); rotación 90 por CPU.
- [x] Fuentes FreeType (`ft_support.c` compila con freetype moderno vía compat `FT_CALLBACK_DEF`; rutas `ux0:data/vitaj2me/fonts/`, 13 TTF + utility.sym a copiar por FTP); imágenes PNG/JPEG con decode nativo (`image.c` libpng/libjpeg, en PSP eran stubs); `alpha_blend`/`imageRom` (splash Duke) portables.
- [x] VKB: es Java-side (dibuja en scbuff vía Graphics) — funciona sin código extra una vez el flush existe. Verificación: 6/6 ficheros con 0 errores (`arm-vita-eabi-gcc -fsyntax-only`).

## Fase 5: Gráficos 3D (M3G - JSR 184) — COMPLETADA (código; test en HW en Fase 6)
Módulo nuevo `pspkvm/jsr184/`, **autocontenido**: no toca `midp/build/common` ni ningún fichero de la Fase 2B. PSPKVM no traía JSR-184 (el de phoneME dependía del motor Swerve de Superscape, ausente del árbol), así que se implementan las dos mitades.
- [x] **API Java completa** (33 clases CLDC 1.1 en `src/core/classes/javax/microedition/m3g/`): Object3D, Transform, Transformable, Node, Group, World, Camera, Light, Mesh, MorphingMesh, SkinnedMesh, Sprite3D, VertexArray, VertexBuffer, IndexBuffer, TriangleStripArray, Appearance, Material, PolygonMode, CompositingMode, Fog, Texture2D, Image2D, Background, KeyframeSequence, AnimationTrack, AnimationController, RayIntersection, Graphics3D y Loader.
- [x] **Rasterizador por software** (`src/engine/`, 5 `.c`): transformación, iluminación por vértice (ambiental/direccional/omni/foco con atenuación y especular), niebla lineal y exponencial, recorte por los 6 planos del frustum, culling con winding, Gouraud y plano, z-buffer 16 bits con depth offset, 2 unidades de textura con los 5 modos de texture env de GL ES, wrap REPEAT/CLAMP, nearest y bilineal, matriz de textura, los 5 modos de mezcla de CompositingMode, alpha test, máscaras de escritura y two-sided lighting.
- [x] **Decisión software, no GPU** (motivada en `jsr184/README_VITA.txt`): la Fase 4 ya es dueña del contexto GXM vía vita2d y `sceGxmInitialize()` solo admite uno por proceso — meter vitaGL obligaría a rehacer Fase 4 ya verificada; además M3G debe componer con lo que `Graphics` dibuja antes/después y respetar su clip, cosa que sale exacta rasterizando sobre el mismo buffer RGB565. El contrato Java↔motor no presupone software: la puerta a GPU queda abierta.
- [x] **Reparto Java/nativo**: por nodo en Java (grafo, animación, orden por capa, carga `.m3g`), por vértice y por píxel en C — el port usa el intérprete C de la CVM (`arch=c`), donde un bucle por vértice en Java costaría un orden de magnitud más. Los datos masivos no se copian: viven en arrays Java y el C los lee con `SNI_GetRawArrayPointer`, así que no hay memoria nativa colgando de objetos Java (CLDC 1.1 no tiene `finalize()`).
- [x] **Loader `.m3g`** (secciones sin comprimir y zlib) y PNG/JPEG. El inflate reutiliza `inflateData()` del subsistema `jarutil` de MIDP porque CLDC 1.1 no trae `java.util.zip`.
- [x] **Picking** con `RayIntersection` (Möller-Trumbore en C, atributos interpolados en Java).
- [x] Verificación: 5/5 `.c` del motor + `m3g_kni.c` con `arm-vita-eabi-gcc -fsyntax-only -Wall` y **cero avisos**; prueba funcional en host (cubo texturizado con luz, z-buffer y perspectiva → PPM correcto, picking con `t` exacto); 33/33 clases Java compilan; `tools/check_contract.py` valida las 83 constantes del contrato Java↔C.
- [ ] Activación en el build: **deliberadamente sin aplicar** para no romper la Fase 2B en curso. Son dos cambios de una línea, documentados en `jsr184/README_VITA.txt`: `USE_JSR_184 = true` en `midp/build/javacall_vita/Options.gmk` y `JSR_184_DIR=${WS_ROOT}/jsr184` en el paso [4/4] de `build-vita-cldc.sh`.

## Fase 6: Pruebas y Empaquetado — COMPLETADA (infra; test en HW pendiente de consola)
- [x] Presupuesto de memoria: heap CVM 192MB en Vita (`memory.c` `#ifdef VITA`; frente a 64MB PSP). Resto: texturas vita2d ~2MB, audio, red 64K, eboot. Total << 400MB de usuario.
- [x] `.vpk` instalable con LiveArea (`vita/livearea/`: icon0/pic0/bg0/startup + template; `patch_vpk.py` porque vita-pack-vpk no tolera espacios en rutas). Bringup v00.60 valida además AppUtil/locale y malloc contiguo en el log.
- [ ] Test en consola real (checklist en `port_progress.md`): instalar VPK, traer `Soul of Darkness.jar` + TTFs por FTP, boot del MIDlet tras Fase 2b, un bug a la vez con `so-crash-triage`.

## Herramientas Necesarias
- **vitasdk** para la compilación cruzada a ARM.
- El código original clonado de `vadosnaprimer/pspkvm`.
