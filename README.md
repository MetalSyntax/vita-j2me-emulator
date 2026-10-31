# VitaJ2ME — Java ME for PS Vita

VitaJ2ME runs Java ME (J2ME / MIDP 2.0, CLDC 1.1) games natively on the
PlayStation Vita. It is a port of **PSPKVM** (Sun's phoneME Feature VM, as
adapted for the PSP) from MIPS to ARMv7, with a new native front end: a game
library, per-game settings, a virtual keypad, touch support and an in-game
menu.

Interface languages: **English** (default), **Español**, **Português**.

## Features

- **Game library** with icons read from each JAR. Games you copy to the
  memory card are installed automatically on startup.
- **Preloading:** the Java VM starts in the background with your last
  played game while you browse the library, so it opens much faster.
- **Automatic screen size** per game, from the JAR manifest
  (`Nokia-MIDlet-*-Display-Size`, `SEMC-Screen-Size`, `MIDlet-Screen-Size`),
  from the file name (`..._480x800.jar`) or from full-screen images. You can
  also pick a size by hand, from 128x128 up to 480x854.
- **Scaling:** fit (keeps the aspect ratio), integer (sharp), stretch or
  original size, with a sharp or smooth filter.
- **Rotation** of 0/90/180/270 degrees and **horizontal/vertical mirror**.
  Touch and the D-pad rotate with the image.
- **Virtual phone keypad** on the left, right or both sides: soft keys,
  D-pad with OK, and 0-9, `*`, `#`. You can show or hide it in game.
- **Touch screen** as the MIDlet pointer, for touch games.
- **Button mapping:** every Vita button (and the left stick) can send any
  phone key.
- **Speed:** shortens the games' `Thread.sleep()` waits (Fast / Turbo,
  capped at 30 FPS).
- **Sound:** WAV, **MIDI** (TinySoundFont plus a General MIDI bank) and
  **MP3** (libmpg123).
- **Global options:** language, FPS counter, and default settings for
  every game.

## Installation

1. Install `VitaJ2ME-v1.0.vpk` with VitaShell.
2. Extract `VitaJ2ME-v1.0-data.zip` to `ux0:data/`. You should end up with
   `ux0:data/vitaj2me/` containing `classes.zip`, `lib/`, `fonts/`,
   `appdb/` and `games/`.
3. Copy your `.jar` games (and `.jad` files, if you have them) to
   `ux0:data/vitaj2me/games/`.
4. Start VitaJ2ME. All new games are installed in one batch on a native
   progress screen ("Installing games 3 / 26"), and then the library
   opens.

Unusual characters in file names (`'`, `&`, parentheses, spaces) are
replaced with `_` before installing. The name shown in the library comes
from the game itself.

## Controls

### Library

| Button | Action |
| --- | --- |
| Left / Right, L / R, touch | Choose a game |
| Cross | Play |
| Square | Settings for this game |
| Triangle | Details |
| SELECT | Global options (language, FPS counter, defaults) |
| Circle | Exit |

### In game

| Buttons | Action |
| --- | --- |
| L + R + START | Game menu. The game pauses and the settings apply live. Circle returns to the game; Triangle quits to the library. |
| L + R + X | Quit to the library |
| L + R + Triangle | Show / hide the virtual keypad |

Default mapping: D-pad and left stick = arrows, Cross = OK/Fire,
Circle = `0`, Square = `1`, Triangle = `3`, L = `*`, R = `#`,
SELECT = left soft key, START = right soft key. You can change it per game
or globally.

## Settings

**Per game** (Square in the library, or L+R+START in game): resolution,
scaling, filter, rotation, mirror, virtual keypad, touch, speed and button
mapping. They are stored in `ux0:data/vitaj2me/config/<suite id>.cfg`.

**Global** (SELECT in the library): language, FPS counter, and the defaults
that every game without its own settings uses (scaling, filter, keypad,
touch, speed, buttons). They are stored in
`ux0:data/vitaj2me/config/global.cfg`.

Changing the resolution or the touch setting restarts the game. Everything
else applies immediately.

## Known limitations

- **Performance.** The VM is the phoneME C interpreter: there is no JIT and
  no ARM assembly interpreter. Light games run fine; heavy 3D-like ones
  (Asphalt, for example) run at 6-15 FPS. The Speed setting only helps
  games that spend time sleeping.
- **AMR audio** is not supported (there is no decoder library in VitaSDK).
  Games that use it run without those sounds.
- **MIDI** sounds like a small General MIDI bank (TimGM6mb), not like the
  original phone's synthesizer.
- Seeking inside MP3 is limited to rewinding to the start, which is what
  games use.
- One game per process: the Java VM starts again for every game, and going
  back to the library restarts the app. Preloading hides most of the wait
  for the last game you played.
- Changing the resolution restarts the game, because J2ME games read the
  screen size only once, when they start. Set it from the library (Square)
  before playing.
- Automatic screen size is a best guess. A few games need a manual size or
  rotation (for example A Good Day to Die Hard).
- Only Java ME games work. `.jar` files made from Android games (no
  `META-INF/MANIFEST.MF`, Android classes inside) are skipped and written
  to the log.

## Logs and bug reports

Every session writes `ux0:data/vitaj2me/logs/bringup-NNN.log` (front end)
and `jvm-NNN.log` (Java VM output). If the app crashes, the system leaves a
`ux0:data/psp2core-*.psp2dmp` crash dump. Please attach the logs (and the
dump) to bug reports.

## Building from source

You need VitaSDK. The Java VM libraries are prebuilt in `_prebuilt/lib/`,
because rebuilding them needs the original toolchain described in
`vita/BUILDING_VITA.txt`. The front end builds with CMake:

```sh
export VITASDK=~/vitasdk PATH=$VITASDK/bin:$PATH
cmake -S vita -B vita/build
cmake --build vita/build
```

To change code under `pspkvm/javacall/`, rebuild only `libjavacall.a`
(step `[1/4]` of `build-vita-cldc.sh`) and copy it to `_prebuilt/lib/`.
Porting notes and the full bug history are in `port_progress.md`.

## Credits and licenses

VitaJ2ME is free software under the **GNU GPL v2** (see `LICENSE`), like
the phoneME code it is based on.

- **PSPKVM / phoneME Feature** (Sun Microsystems): GPLv2. The VM, MIDP and
  the data files come from this distribution.
- **Fonts** in `fonts/`: DejaVu (Bitstream Vera license) and the Firefly
  fallback font (Arphic Public License), as shipped with PSPKVM. Their
  licenses are in `pspkvm/psp/fontpkgs/1/`.
- **TinySoundFont** (`tsf.h`, `tml.h`) by Bernhard Schelling: MIT.
- **TimGM6mb.sf2** General MIDI bank by Tim Brechbill: GPLv2.
- **libmpg123**: LGPL 2.1, as packaged by VitaSDK.
- **vita2d**, **VitaSDK**, zlib, libpng, FreeType.

Java and Java ME are trademarks of Oracle. PlayStation and PS Vita are
trademarks of Sony Interactive Entertainment. This project is not affiliated
with either company. Bring your own games.
