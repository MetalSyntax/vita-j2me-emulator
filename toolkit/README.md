# VitaJ2ME Toolkit

TUI mínima (flechas, sin dependencias externas) para agilizar el ciclo
de pruebas de **vita-j2me-emulator** en consola real. Es deliberadamente
mucho más chica que `psvita-port-toolkit-cli`: este proyecto no es un
port de una APK Android, es un emulador J2ME nativo, así que no hace
falta nada de lo específico a ports (JNI, `.so`, jadx/Ghidra). Solo lo
que hace falta para: compilar → subir → ejecutar en consola → bajar
log/crash → analizar.

## Uso

```sh
./toolkit/vitaj2me-toolkit
# o
python3 toolkit/vitaj2me_toolkit.py
```

Requiere Python 3.9+ (solo librería estándar) y una PS Vita en la misma
red con el servidor FTP de VitaShell activo.

## Qué hace cada opción

- **Subir VPK**: sube `vita/build/vitaj2me-bringup.vpk` a
  `ux0:downloads` por FTP, para instalarlo/reinstalarlo desde VitaShell.
- **Subir solo eboot.bin**: sobrescribe el `eboot.bin` ya instalado en
  `ux0:app/VITAJ2ME0/`, sin pasar por el instalador — mucho más rápido
  cuando solo cambió el código nativo y el .vpk ya está instalado.
- **Subir datos del home J2ME (`classes.zip` + `.jar`)**: sube
  `_prebuilt/lib/classes.zip` a `ux0:data/vitaj2me/` y cualquier
  `.jar`/`.jad` de la raíz del repo o de `games/` a
  `ux0:data/vitaj2me/games/`. Sin `classes.zip` en la consola la JVM no
  encuentra ninguna clase del AMS y el bootstrap muere mucho más
  adelante, con un síntoma que no tiene nada que ver — pasó dos sesiones
  seguidas (ver Bug real #16). El frontend loguea un inventario del home
  al arrancar, así que el `bringup-NNN.log` siguiente confirma si quedó
  bien.
- **Descargar log + crash dump más recientes**: baja lo último de
  `ux0:data/vitaj2me/logs/` (`bringup-NNN.log`/`jvm-NNN.log`; también
  busca en `ux0:data/vitaj2me/` por si quedan logs sueltos de antes de
  que existiera la subcarpeta) y el crash dump más reciente de
  `ux0:data/` — sin preguntar cuál, para el ciclo rápido "ejecutar,
  cerrar, bajar". Los crash dumps se guardan sueltos en `ux0:data/`,
  compartidos con cualquier otro homebrew de la consola (el nombre no
  dice de qué app son), así que si el dump encontrado tiene más de una
  hora avisa que probablemente no es de esta sesión.
- **Elegir un log/dump específico**: lista lo que hay en la consola
  ordenado por fecha y elegís cuál bajar.
- **Analizar un crash dump ya descargado**: corre
  [vita-parse-core](https://github.com/xerpi/vita-parse-core) contra el
  dump y `vita/build/vitaj2me` (el ELF con símbolos, antes de convertir
  a `.self`/`eboot.bin`) — registros, PC/LR, desensamblado alrededor del
  crash y backtrace de la pila. Guarda dos archivos junto al dump:
  - `<dump>.analysis.txt`: el informe completo (igual que antes).
  - `<dump>.triage_summary.md`: resumen corto y en Markdown — causa del
    crash, PC/LR resueltos a función/línea, qué registros están en
    `0x0` (posible puntero nulo) y la cadena de direcciones de la pila
    que caen dentro de `vitaj2me`, con una sugerencia según el tipo de
    fallo. Se genera aparte del informe completo (importa
    `vita-parse-core` directo en vez de parsear su salida coloreada).

  **Ojo con el ELF desactualizado**: si `vita/build/vitaj2me` es más
  nuevo que el dump (se recompiló después del crash, algo frecuente
  mientras se itera), la resolución de símbolos puede ser incorrecta —
  el código pudo haberse movido de lugar entre medio. En ese caso
  ambos archivos empiezan con un aviso explícito; si lo ves, recompilá
  y volvé a crashear antes de confiar en los nombres de función.
- **Compilar bringup (Fase 2a)**: corre los mismos `cmake` de
  `vita/BUILDING_VITA.txt`. No toca nada de la Fase 2b (JVM completa).
- **Probar conexión FTP**: chequeo rápido de que la Vita responde.
- **Limpiar archivos basura de macOS**: borra `._*` y `.DS_Store` bajo
  todo el proyecto (nunca se commitean; en disco exFAT/red macOS los
  genera todo el tiempo).
- **Configuración**: IP/puerto de la Vita, ruta a VITASDK y a
  vita-parse-core. Se guarda en `toolkit/config.json` (gitignored, tiene
  la IP de tu red).

## Requisitos externos (opcionales según lo que uses)

- **VITASDK** instalado, para compilar y para el desensamblado del
  análisis de crash (`arm-vita-eabi-objdump`/`addr2line`).
- **vita-parse-core** clonado en `~/vita-tools/vita-parse-core` (con su
  propio venv y `pyelftools` instalado), solo para "Analizar un crash
  dump". Sin esto, todo lo demás funciona igual.

## Por qué no reusar psvita-port-toolkit-cli

Esa herramienta está pensada para ports de una APK Android sobre
SoLoader (detecta base de carga de un `.so`, cruza símbolos con jadx y
Ghidra, etc). Este proyecto es una JVM nativa sin ningún `.so` de por
medio: el ELF a analizar es siempre el mismo
(`vita/build/vitaj2me`), así que el analizador de acá es mucho más
simple y no necesita nada de eso.
