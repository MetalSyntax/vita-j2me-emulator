# Plan v1.1 — controles táctiles y mejoras de J2Vita / J2ME-Loader

Estado de partida (v1.0.1): VM asm + ROM (arranque ~2 s, DH3 20-24 FPS),
mezclador de audio con AMR-NB, mapeo de cada botón a UNA tecla del
teléfono, táctil como puntero, teclado virtual lateral, rotación/espejo,
escalado y filtro por juego.

Referencias: J2Vita (`KiddRwxSsj/j2vita`, GPLv2, mismo PSPKVM) y
J2ME-Loader (`nikita36078/J2ME-Loader`, Android; su código es Java sobre
Android, así que de ahí se toman **ideas y comportamiento**, no código, salvo
que la licencia y la arquitectura lo permitan, y siempre con crédito).

Cada fase: fix/feature → build → deploy → prueba en HW → `port_progress.md`
→ commit. Todo texto visible con `L(en, es, pt)`.

---

## Fase A — juegos táctiles con botones en pantalla (prioridad 1)

Problema: los juegos "Touch" (p. ej. Dungeon Hunter III Touch) dibujan su
propio pad, botones de ataque y menús en la pantalla y solo reaccionan a
`pointerPressed/Dragged/Released`. Hoy la única forma de jugarlos es con el
dedo.

A1. **Botón → toque en un punto.** Cada botón de la Vita puede mandar, además
    de una tecla, un *toque* en una posición (x, y) de la pantalla del juego:
    pulsar = `pointerPressed`, mantener = sigue pulsado, soltar =
    `pointerReleased`. Coordenadas en píxeles del juego, así siguen valiendo
    con cualquier escala, rotación o espejo. Se guarda por juego
    (`tap<N>=x,y` en el `.cfg`).

A2. **Editor visual en el menú en juego** ("Botones táctiles"): la última
    imagen del juego de fondo, se elige un botón y se coloca su marcador con
    el dedo o con el stick; X confirma, △ borra. Los marcadores se ven con
    el nombre del botón (✕, ○, L, R…).

A3. **Stick izquierdo → joystick en pantalla.** Para los pads dibujados por
    el juego: se define un centro y un radio; mover el stick = arrastrar el
    dedo desde el centro (`pointerPressed` en el centro + `pointerDragged`
    al ángulo/radio). Opción equivalente para la cruceta (8 direcciones).

A4. **Un solo puntero.** MIDP solo tiene un punto de toque. Regla: el último
    botón pulsado manda; al soltarlo vuelve al que siga pulsado (stick
    incluido). Se documenta como límite.

A5. **HUD de botones** (opcional, por juego): marcadores translúcidos encima
    del juego para saber qué botón toca dónde. También muestra las teclas
    normales (idea de J2Vita).

A6. **Panel trasero:** mitades o cuadrantes como botones extra, mapeables a
    tecla o a toque. Opción "Nada" para evitar toques accidentales.

A7. **Perfiles compartibles:** el `.cfg` del juego guarda los toques por
    nombre de MIDlet + versión; exportar/importar desde
    `config/profiles/<Nombre>.cfg` para que los usuarios compartan
    configuraciones de cada juego.

Prueba en HW: Dungeon Hunter III Touch (pad + ataque + habilidades), un
juego no táctil (que nada cambie), rotación 90° con toques.

## Fase B — controles (de J2Vita / J2ME-Loader)

B1. **Autodetección de teclas** al primer arranque: contar en el bytecode qué
    códigos de tecla usa el juego (soft keys, 2/4/6/8 o flechas, 5/fire) y
    proponer el mapeo (J2Vita). Reaprovecha el lector de `sipush` de la
    detección de resolución.
B2. **"La cruceta manda":** flechas o 2/4/6/8 con un solo ajuste; stick
    derecho mapeable.
B3. **Teclado virtual:** tamaño y opacidad ajustables; modo "Siempre" con el
    táctil llegando al juego; navegable con la cruceta (para cheats);
    mantener SELECT lo muestra/oculta.
B4. **Entrada de texto con el IME de la Vita** en `TextBox`/`TextField`
    (nombres de partida, cheats). Verificar primero qué hace hoy.

## Fase C — pantalla

C1. **Ajuste por gestos** en el menú en juego: arrastrar/pellizcar para mover
    y hacer zoom de la imagen; se guarda por juego.
C2. **Resolución "Auto" que se corrige sola:** si el juego dibuja a otro
    tamaño del detectado, se adopta ese tamaño en el siguiente arranque
    (J2Vita).
C3. **Overlay de rendimiento "Completo":** FPS + carga de la VM + "límite del
    juego" vs "límite de CPU" (J2Vita), para que los reportes digan algo
    útil.
C4. Color de fondo de las bandas; opción contra parpadeo (equivalente al
    "immediate processing mode" de J2ME-Loader) si algún juego lo necesita.
C5. (Baja) Shaders tipo "sharp bilinear"/scanlines si vita2d lo permite sin
    coste.

## Fase D — biblioteca

D1. Subcarpetas en `games/` y descompresión de `.zip` con JAR dentro.
D2. Instalar juegos copiados con la app abierta; borrar un juego (y su JAR)
    desde la biblioteca, conservando o no sus partidas.
D3. Carpetas/categorías propias; desplazamiento con el dedo y tocar para
    elegir/jugar.
D4. Temas (claro/oscuro/colores).

## Fase E — compatibilidad

E1. **Mascot Capsule v3 (Micro3D)**: muchos juegos 3D de Gameloft/Japón lo
    usan. Hoy no existe. Es la tarea más grande: API Java + renderizador por
    software (o vita2d/GXM). Referencia: la implementación de woesss
    (JL-Mod/J2ME-Loader) y la de J2Vita. Se estudia aparte antes de empezar.
E2. Revisar M3G (JSR-184) con juegos reales y su rendimiento.
E3. APIs de fabricante faltantes como stubs que no rompan: Siemens,
    Samsung, Motorola, Vodafone; vibración y luz de Nokia como no-op.
E4. Banco MIDI opcional más completo (FreePats u otro con licencia libre).

## Fase F — rendimiento (con evidencia)

F1. Perfilar la VM asm en los juegos más lentos (Asphalt 6/Nitro, Die Hard):
    qué parte es intérprete, qué parte es dibujo (`drawRGB`, `drawImage`,
    copia a vita2d) y qué parte es GC.
F2. Optimizar en C las primitivas gráficas calientes que salgan del perfil.
F3. Solo si el perfil lo justifica: estudiar el JIT ARM de phoneME
    (desactivado hoy).

---

Orden propuesto: **A (A1-A5) → B1-B3 → C2-C3 → D1-D2 → A6-A7 → resto**.
v1.1 = Fase A + B1-B3; Mascot Capsule (E1) va en una versión aparte.
