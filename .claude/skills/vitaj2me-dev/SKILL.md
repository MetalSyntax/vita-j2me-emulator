---
name: vitaj2me-dev
description: Flujo completo de desarrollo de VitaJ2ME (Java ME / PSPKVM-phoneME portado a PS Vita): compilar javacall y el frontend, desplegar por FTP, diagnosticar crashes con el toolkit LOCAL, parchear clases Java de classes.zip, añadir ajustes/textos en 3 idiomas, documentar en port_progress.md, commitear y preparar releases. Usar SIEMPRE que se toque cualquier cosa de este repositorio (código, bugs, logs/dumps de la consola, ajustes, documentación, release).
---

# VitaJ2ME — cómo se trabaja en este proyecto

Este documento recoge el método con el que se llevó el emulador hasta la
v1.0. Síguelo entero: cada paso existe porque saltárselo ya rompió algo
(los números de bug están en `port_progress.md`).

## 0. Reglas de oro

1. **Evidencia antes que hipótesis.** Un bug se arregla con un log o un
   dump real de la consola, no con suposiciones. Si falta evidencia,
   primero añade la traza o la instrumentación que la va a dar.
2. **Un bug confirmado a la vez.** Para cada uno: causa raíz, fix,
   compilar, desplegar, documentar y commit.
3. **Toolkit LOCAL, nunca el global.** Usa `./toolkit/vitaj2me-toolkit`,
   `toolkit/deploy.py` y `toolkit/java_patch.py`. **Prohibido**
   `psvita-toolkit`: es para ports de Android y resuelve los dumps contra
   `.so` ajenos (bug #5).
4. **Frescura del binario.** Antes de creer una resolución de símbolos,
   comprueba que `vita/build/vitaj2me` NO es más nuevo que el dump. Si se
   recompiló después del crash, las direcciones no valen.
5. **Nada de la UI vieja de PSPKVM llega a pantalla.** Ni el AMS ni el
   instalador gráfico: todo es nativo (`vita/launcher.c`). La VM siempre
   corre detrás de un hook de `lcd.c`.
6. **Toda cadena visible va en 3 idiomas:** `L("English", "Español",
   "Português")`, con inglés por defecto. Los logs pueden quedar en
   español.
7. **Nunca subir juegos a git** (JAR/JAD), ni logs, ni dumps. Tampoco
   código descargado sin revisar: ejecutar código bajado de internet en el
   Mac requiere permiso explícito del usuario.
8. **No digas que algo funciona si no se probó en la consola.** Indica
   siempre qué quedó sin probar en HW.
9. **El usuario habla español:** responde en español, claro y sin jerga
   innecesaria.

## 1. Mapa del código

| Dónde | Qué |
| --- | --- |
| `vita/main.c` | Arranque: logs de sesión, watchdog HB, `rom_data_init` (bug #30), instalación por lotes, precarga, `-play`, relanzar, hooks de menú/salida |
| `vita/launcher.c` | UI nativa (vita2d + PGF): biblioteca, ajustes por juego y globales (`settings_run`, `settings_rows`), menú en juego, pantallas de carga/instalación, detección de resolución del JAR |
| `vita/gamecfg.c/.h` | `vj_cfg` por juego + `global.cfg`, `L()`/idioma, layout del teclado virtual, táctil, rotación/espejo (`game_key`), velocidad, `cfg_apply` |
| `vita/vm_patches.c` | Parches a la VM en link: `--wrap` (`clinit_list_remove`, `pcsl_file_getpathseparator`, `Thread.sleep`) |
| `vita/patch_vm_calls.py` | Redirige `bl` internos de CLDC (fuentes fusionadas: `--wrap` no llega). Tiene que decir **"2 llamada(s)"** |
| `pspkvm/javacall/implementation/vita_arm/` | Capa de plataforma (C): `midp/lcd.c` (pantalla, hooks, rotación, tope de FPS), `midp/keypress.c` (botones, combos L+R+…), `midp/keymap.c`, `common/logging.c`, `common/memory.c` (Java heap de 64 MB), `jsr135_mmapi/multimedia.c` (WAV, MIDI con TinySoundFont, MP3 con libmpg123) |
| `pspkvm/midp/src/...`, `pspkvm/cldc/src/javaapi/...` | Fuentes Java de `classes.zip`. Solo se parchean con `toolkit/java_patch.py` |
| `_prebuilt/lib/` | `.a` de la VM + `classes.zip` (se commitean) |
| `_prebuilt/data/` | Datos de `ux0:data/vitaj2me` (`lib/gm.sf2`, fuentes…) |
| `ui-theme/build_theme.py` | Genera LiveArea (`vita/livearea/`) y el splash nativo (`imageRom.c`) |
| `toolkit/` | TUI, `deploy.py`, `java_patch.py`, `logs/`, `dumps/` |

Consola: `ux0:app/VITAJ2ME0/` (app) y `ux0:data/vitaj2me/` (`classes.zip`,
`lib/`, `fonts/`, `appdb/`, `games/`, `config/`, `logs/`). Crash dumps en
`ux0:data/psp2core-*.psp2dmp`. FTP de VitaShell en el puerto 1337 (sin
login; la IP está en `toolkit/config.json`). El FTP **no** soporta `NLST`:
usa `LIST` + `cwd`.

## 2. Compilar

```sh
export VITASDK=~/vitasdk PATH=~/vitasdk/bin:$PATH
# a) javacall (si tocaste pspkvm/javacall/**). Ruta SIN espacios: symlink.
R=/private/tmp/vitaj2me/pspkvm   # symlink -> .../vita-j2me-emulator/pspkvm
(cd $R/javacall/configuration/phoneMEFeature/vita_arm && \
 make JAVACALL_DIR=$R/javacall \
      JAVACALL_OUTPUT_DIR=$R/javacall/configuration/phoneMEFeature/vita_arm/output \
      USE_JSR_135=true USE_JSR_75=true USE_JSR_179=true USE_PROPERTIES_FROM_FS=true)
cp pspkvm/javacall/configuration/phoneMEFeature/vita_arm/output/lib/libjavacall.a _prebuilt/lib/
rm -f _prebuilt/lib/._libjavacall.a
# b) frontend + eboot + VPK
cmake --build vita/build
```

Si no existe el symlink: `mkdir -p /private/tmp/vitaj2me && ln -s
"$PWD/pspkvm" /private/tmp/vitaj2me/pspkvm`. Si la caché de CMake apunta
a otra ruta: `cmake -S vita -B vita/build -DCMAKE_POLICY_VERSION_MINIMUM=3.5`.

**Verificación obligatoria tras cada build:**
- `arm-vita-eabi-nm vita/build/vitaj2me | grep -c ' U '` debe dar **0**.
- La salida del build debe incluir `patch_vm_calls.py: 2 llamada(s)
  redirigida(s) y verificada(s)`.
- Si añadiste un `--wrap`, comprueba en el ELF que la tabla apunta al
  wrapper (que haya referencias a `__wrap_X`).

Los warnings "implicit declaration" de javacall son antiguos; los avisos
de clang del IDE en `vita/*.c` también, porque no conoce las rutas de
VitaSDK.

## 3. Desplegar y bajar evidencia

```sh
python3 toolkit/deploy.py eboot            # tras cambios en C
python3 toolkit/deploy.py classes s52      # tras java_patch; backup .bak-s52
python3 toolkit/deploy.py jars ~/Downloads/JAR
python3 toolkit/deploy.py livearea         # app + caché ur0:appmeta
python3 toolkit/deploy.py logs 4           # bringup-NNN.log + jvm-NNN.log
python3 toolkit/deploy.py dump             # dump más nuevo + análisis
```

`deploy.py` comprueba el tamaño de cada subida. Antes de sobrescribir
`classes.zip`, **siempre** se guarda la anterior como `classes.zip.bak-<tag>`
(es el rollback). Si el FTP no responde, dilo y pide al usuario que active
el FTP de VitaShell: no inventes que se subió.

## 4. Diagnóstico de un crash o bug

1. Baja los logs y el dump (`deploy.py logs` / `deploy.py dump`).
2. **Lee el bringup** (frontend + HB cada 10 s: FPS, CPU del hilo de la
   VM `clocks`, `st=8 wait…(cola eventos)` = VM ociosa, `io ms` del log) y
   el **jvm** (salida Java, `VITAJ2ME-*`).
3. Dump: mira qué hilo crasheó y dónde está el PC.
   - Si el PC está en un **módulo del sistema** (SceGxm, SceLibKernel), la
     desensambladura del analizador sobre nuestro ELF es falsa. Razona por
     hilo, registros (`0xdeadbeef`, LR=0) y qué estaba haciendo.
   - Las direcciones del dump = ELF + desplazamiento de carga
     (`vitaj2me@1 + off`). Resuelve con `arm-vita-eabi-addr2line -f -e
     vita/build/vitaj2me` y con `nm -n`.
   - Los dumps son mínimos (solo pilas): no esperes leer el heap ni la BSS.
     Si necesitas memoria, añade instrumentación que la registre.
4. Busca la causa raíz en la fuente (`pspkvm/cldc/src/vm/...`,
   `pspkvm/midp/src/...`). Comprueba que el símbolo/valor del binario
   coincide con la fuente: los flags de build cambian cosas (ej.
   `UseROM=0` por `ROMIZING=false`, bug #30).
5. Arregla en el sitio correcto:
   - **Lógica del frontend o de javacall** → C directo.
   - **VM (CLDC/MIDP nativo)** → `--wrap` en `vm_patches.c` si la llamada
     cruza objetos; si no, `patch_vm_calls.py`. Nunca recompilar CLDC/MIDP
     (romgen/Docker, bug #14).
   - **Java de MIDP/CLDC** → `toolkit/java_patch.py` (sección 5).
6. Compila, verifica, despliega y documenta (sección 8). Di qué falta por
   probar en HW.

### Lecciones que no hay que repetir

- **Un proceso = un START de la VM** (bug #20). Para cambiar de juego o
  volver a la biblioteca hay que relanzar con
  `sceAppMgrLoadExec("app0:eboot.bin", args)` (`-play N`).
- **vita2d desde varios hilos:** solo dibuja uno a la vez. Antes de que la
  VM toque la pantalla hay que parar los hilos de carga, instalación o
  biblioteca (hook de `lcd.c`, valor 2 = "vita2d se va"). Nunca
  `vita2d_fini` (bug #31).
- **CLDC sin ROMIZING:** `_rom_data_block` se copia a mano en
  `rom_data_init` (bug #30).
- **Log:** System.out llega carácter a carácter. `logging.c` usa buffer,
  colapsa repeticiones y vuelca por tiempo; el HB lo vuelca cada 2 s. No
  añadas E/S por línea en el hilo de la VM (provoca bajones de FPS).
- **Memoria:** el Java heap tiene un tope de 64 MB para dejar sitio al
  heap de C (banco MIDI, fuentes, vita2d).
- **Hilos:** `sceKernelStartThread(t, sizeof(p), &p)` entrega un
  *puntero al puntero* (bug de los WAV). Los buffers del hilo, por
  instancia, nunca `static` compartidos.
- **Rutas con espacio:** GNU Make no las soporta; usa el symlink.
- **Nombres de JAR** con `' & ( )` y espacios rompen
  `file:///games/…`; `sanitize_games_dir()` los renombra.

## 5. Parchear Java de classes.zip

```sh
python3 toolkit/java_patch.py verify <Fuente.java>   # ANTES de editar: 0 diferencias
#   ... editar la fuente (codificación ISO-8859-1) ...
python3 toolkit/java_patch.py apply <Fuente.java>    # javac 1.4 + ProGuard -microedition
python3 toolkit/deploy.py classes <tag>
```

- Si `verify` no da 0, la fuente NO es la desplegada: no se parchea.
- `apply` muestra el diff de `javap` de cada clase: revísalo (solo debe
  cambiar lo previsto) y comprueba que quede en versión 48.
- Necesita JDK 8 y `toolkit/tools/proguard.jar` (ProGuard 7.4.2 oficial,
  zip sha256 `164b26a6ca655296bede9b1474ee47e24369d59860fdcebaa64d210221bce5fa`;
  `toolkit/tools/` está ignorado por git).
- Para pasar datos del frontend a Java usa `-Dvita.flags=...`
  (`vita_start_game`; máximo 10 argv en total).

## 6. Añadir un ajuste o una función de la UI

- **Ajuste por juego:** campo en `vj_cfg` (`gamecfg.h`), valor por defecto
  en `cfg_factory`, clave en `cfg_parse` y `cfg_write`, y aplicarlo en
  `cfg_apply` (con `live=1` debe poder aplicarse en caliente si es
  posible). En `launcher.c`: `ROW_*` en el enum, `row_text`,
  `row_change`, `row_help` (3 idiomas) y, si no es global,
  excluirlo en `settings_rows` para `SET_GLOBAL`.
- **Ajuste global** (no depende del juego): igual, pero solo en
  `SET_GLOBAL`. Se guarda en `config/global.cfg` vía `cfg_global_save`.
  La resolución, la rotación y el espejo son **por juego**.
- **Atajos en juego:** `keypress.c` (combos exactos L+R+…) → flag o hook
  hacia el frontend. Ocupados: START = menú, X = salir, △ = teclado,
  SELECT = volcado pss.
- **Textos:** siempre con `L(en, es, pt)`. La fuente PGF cubre Latin-1
  (á, ñ, ã, ç).
- **Pantallas nuevas:** con el estilo de la biblioteca (`fill_rrect`,
  `text`, `hint`, `glyph_*`, colores `C_*`). Nada de la UI de PSPKVM.
- Pide la validación en HW de cada pantalla nueva.

## 7. LiveArea, splash e idiomas visibles

- Los textos de imagen están en `ui-theme/build_theme.py` (`splash()`,
  `build_livearea()`). Regenera solo lo necesario:
  `cd ui-theme && python3 -c "import os,build_theme as b; os.chdir(b.ROOT); b.PREVIEW.mkdir(exist_ok=True); b.build_imagerom(); b.build_livearea()"`
  y revisa los PNG (léelos). `build_imagerom` cambia `imageRom.c`, así que
  hay que recompilar javacall.
- `deploy.py livearea` actualiza también la caché del sistema, sin
  reinstalar.

## 8. Documentar y commitear (siempre, en cada cambio)

1. **`port_progress.md`:** nueva entrada ARRIBA de las sesiones,
   `## Sesión N — título (fecha, logs/dump HW NNN, probado o SIN probar en
   HW)`, con evidencia, causa raíz (`Bug real #N` si es un bug),
   fix y qué queda por verificar.
2. Si cambia algo visible o de instalación: `README.md` (en inglés) y,
   si afecta al release, `RELEASE.md`.
3. Commit en español, con el formato de la casa: título + viñetas, y al
   final `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. No
   hagas push sin que el usuario lo pida.
4. Antes de cada commit: `git status` sin basura (`._*`, `.DS_Store`),
   sin JAR, logs ni dumps.

## 9. Release

1. Pruebas en HW de lo cambiado (instalar, jugar, menú, cambio de stage).
2. Sube `VITA_VERSION` en `vita/CMakeLists.txt` y compila.
3. `release/` (ignorado por git):
   - `VitaJ2ME-vX.Y.vpk` = `vita/build/vitaj2me-bringup.vpk`.
   - `VitaJ2ME-vX.Y-data.zip` = carpeta `vitaj2me/` con lo **probado en
     consola** (`classes.zip` vigente, `cldc_classes.zip`, `lib/` con
     `gm.sf2`, `fonts/`, la base de `appdb/` y `games/` vacía). No uses
     `_prebuilt/data/lib`: es un reskin que nunca se probó.
4. `RELEASE.md` (notas en inglés: highlights, fixed, known limitations).
5. Comprobación para GitHub:
   - `git log --all --name-only` sin `.jar/.jad/.psp2dmp/.log`;
   - `git grep` sin correos ni secretos;
   - ningún blob mayor de 50 MB;
   - `LICENSE` (GPLv2) presente.
   Avisa al usuario de que su correo aparece como autor de los commits.

## 10. Al terminar cada tarea, al usuario

Responde en español:
- qué se encontró, con la evidencia;
- qué se cambió;
- qué se subió a la consola;
- qué falta probar en HW y qué hacer para probarlo (logs a enviar);
- los riesgos conocidos.

Sé breve, sin jerga innecesaria.
