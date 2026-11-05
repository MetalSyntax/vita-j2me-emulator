# VitaJ2ME v1.0.1 — Java ME for PS Vita

A performance, audio and stability update, based on the feedback on v1.0.

## Downloads

| File                       | What it is                                                            |
| -------------------------- | --------------------------------------------------------------------- |
| `VitaJ2ME-v1.0.1.vpk`      | The app (install with VitaShell over v1.0; settings and games stay)   |
| `VitaJ2ME-v1.0.1-data.zip` | Required data. Extract it to `ux0:data/` to get `ux0:data/vitaj2me/`. |

If you already have v1.0 installed, the data folder does not change: just
install the new VPK.

## What's new in v1.0.1

- **Faster VM.** The Java VM now uses phoneME's ARM assembly interpreter
  with the system classes built in (ROM). The VM starts in about 2 seconds
  (it used to take 17-20 s) and games run faster: Dungeon Hunter III went
  from 0-17 FPS to a steady 20-24 FPS.
- **Audio rewritten.** One mixer thread with a single audio port mixes
  every player (up to 32 voices), with smooth resampling instead of the
  metallic "nearest sample" one. Sound effects no longer run out of audio
  ports, and tones no longer freeze the game while they play.
- **AMR-NB sound** (used by many Gameloft games) through opencore-amr.
- **More WAV formats:** IMA ADPCM, A-law, µ-law, 24-bit and extensible
  headers. Files larger than 100 KB no longer fail to load.
- **Fixed: freezes and crashes in the library** with a preloaded game (the
  VM and the library could draw on the screen at the same time). All
  drawing is now serialized.

## Version 1.0 highlights

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

## Fixed in v1.0 (since the test builds)

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
- JARs that could never install (no manifest) were retried on every start,
  with a black screen and a restart each time. They are now skipped.
- 127 MB Java heap that left the system with only 512 KB; it is now capped
  at 64 MB.

## Known limitations

- There is no JIT: heavy games run at around 20 FPS.
- AMR-WB audio is not supported (AMR-NB is).
- One game per process: the Java VM starts again for every game, and going
  back to the library restarts the app.
- Changing the resolution restarts the game (games read the screen size
  only at startup). Set it from the library before playing.
- Automatic screen size is a best guess; some games need a manual size or
  rotation (for example A Good Day to Die Hard).
- Android games renamed to `.jar` are not Java ME games and are skipped.

## Tested on hardware

Soul of Darkness, Asphalt 6 Adrenaline, Asphalt Nitro, Dungeon Hunter III
and A Good Day to Die Hard (Gameloft) on v1.0. The new VM of v1.0.1 was
tested with Dungeon Hunter III and the start-up of the other games; the
new audio is still being tested game by game.
Please send reports with `ux0:data/vitaj2me/logs/*.log` attached.
