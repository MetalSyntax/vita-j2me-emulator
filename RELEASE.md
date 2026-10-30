# VitaJ2ME v1.0 — Java ME for PS Vita

The first public release. VitaJ2ME runs Java ME (MIDP 2.0 / CLDC 1.1) games
natively on the PS Vita, using a port of PSPKVM (phoneME) with a new native
interface. The interface is in English, Spanish and Portuguese.

## Downloads

| File | What it is |
| --- | --- |
| `VitaJ2ME-v1.0.vpk` | The app (install with VitaShell) |
| `VitaJ2ME-v1.0-data.zip` | Required data. Extract it to `ux0:data/` to get `ux0:data/vitaj2me/`. |

Then copy your `.jar` games to `ux0:data/vitaj2me/games/` and start the
app. See `README.md` for the full guide.

## Highlights

- Native game library. All the JARs in `games/` are installed in one
  batch, with a progress screen. Nothing from the old PSPKVM manager is
  ever shown.
- VM preloading: the last played game is ready almost instantly.
- Automatic screen size per game (manifest, file name or images), up to
  480x854, or a manual choice.
- Scaling (fit / integer / stretch / original) and a sharp or smooth
  filter.
- Rotation 0/90/180/270 and mirror, with touch and D-pad following the
  image.
- Virtual phone keypad on the sides, which you can toggle in game with
  L+R+Triangle.
- Touch screen as pointer, and per-game button mapping.
- In-game menu (L+R+START) that pauses the game; L+R+X quits to the
  library.
- Speed setting (Fast / Turbo, capped at 30 FPS).
- Sound: WAV, MIDI (software synthesizer) and MP3.
- Global options: language, FPS counter, and default settings for all
  games.
- English LiveArea ("Java ME for PS Vita").

## Fixed since the test builds

- Crash in the garbage collector when changing stage or loading big games
  (the VM's ROM data block was never initialized when built without
  ROMIZING).
- Wrong colors (red and blue swapped).
- Games that load resources without a leading `/` (for example
  `Image.createImage("logo.png")`) looped forever on the loading screen.
- Frame drops caused by games printing to the log.
- WAV playback read the wrong memory.
- Crash at the end of installing a game (the install screen kept drawing
  while the VM shut the graphics down).
- 127 MB Java heap that left the system with only 512 KB; it is now capped
  at 64 MB.

## Known limitations

- Heavy games run at 6-15 FPS: the VM is an interpreter, with no JIT.
- AMR audio is not supported.
- Going back to the library restarts the app (one game per process).

## Tested on hardware

Soul of Darkness, Asphalt Nitro and A Good Day to Die Hard (Gameloft).
Please send reports with `ux0:data/vitaj2me/logs/*.log` attached.
