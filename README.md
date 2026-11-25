# TREEGONE

A 6-voice (or 12-voice) polysynth in the manner of the Sequential **Trigon-6**, as a **VST3** for Windows and the Mac,
an **AU** for the Mac, a **standalone app** and a plugin for **Akai MPC** hardware. By **swtdr34mz**. Three oscillators
and a ladder filter per voice, as the Trigon has them, and two more filters from the Pro 3: **LP1** (4-pole OTA),
**LP2** (4-pole ladder, the Trigon's, with its 2-pole mode) and **SVF** (2-pole OTA state variable, low-pass through
band-pass to high-pass).

Unofficial: not made, endorsed or supported by Sequential. No Trigon programs are in it: your own Trigon-6 `.syx`
files can be loaded into it on your own computer (see below).

## Download

[Actions](https://github.com/swtdr34mz-afk/TREEGONE/actions) → the newest green run → **Artifacts** at the bottom:
`TREEGONE-Windows`, `TREEGONE-macOS` or `TREEGONE-MPC` (a GitHub sign-in is needed for artifacts). Releases carry the
same zips.

## Installieren (kurz auf Deutsch)

**Windows (Cubase):**
1. Zip entpacken.
2. Den Ordner `TREEGONE.vst3` nach `C:\Program Files\Common Files\VST3\` kopieren.
3. Cubase starten (oder *Studio → VST-PlugIn-Manager → Aktualisieren*). TREEGONE steht unter den Instrumenten,
   Hersteller swtdr34mz.
4. `TREEGONE.exe` ist die Standalone-App. Audio- und MIDI-Gerät stellt man unter *Options* ein.

**Mac (Ableton, Logic, ...):** Am besten die `.tar.gz` nehmen. `TREEGONE.vst3` nach `~/Library/Audio/Plug-Ins/VST3/`,
`TREEGONE.component` nach `~/Library/Audio/Plug-Ins/Components/`. Dann einmal im Terminal:
```
xattr -cr ~/Library/Audio/Plug-Ins/VST3/TREEGONE.vst3 ~/Library/Audio/Plug-Ins/Components/TREEGONE.component
```

**MPC:** siehe [mpc/plugin/README.md](mpc/plugin/README.md).

## Trigon-6-Programme laden (.syx)

**LOAD SYX** auf dem Panel (oder *GLOBALS → Load Trigon-6 programs*), oder die `.syx`-Datei einfach auf das Panel
ziehen. TREEGONE liest jede Programm-Nachricht darin (Programm-Dumps und Edit-Buffer) und legt eine Kopie der Datei in
`Dokumente/TREEGONE/Imported` ab: beim nächsten Start sind die Programme wieder da. Sie stehen unter der Quelle
**TRIGON .SYX** (Taste SOURCE) mit ihrer Trigon-Nummer (000-499...), BANK / TENS / 0-9 wählen wie am Trigon; im
Programm-Menü (Klick aufs Display) nach Bänken sortiert. Ein Projekt speichert den Sound selbst mit: es klingt auch auf
einem Rechner ohne die Datei.

Gelesen wird das ganze Panel: die drei Oszillatoren mit Oktave, Pitch, Wellen, Pulsbreite und Lautstärke, Sync,
Osc 3 LO und Keyboard, Noise, Fdbk/Drive, Filter (Cutoff, Resonance, Env Amt, 2 Pole, Keyboard, Velocity) und die
Hüllkurven, LFO mit seinen Zielen, Aftertouch, Poly Mod, Glide, Unison mit Chord Memory, Key Mode, Clock, Arp, die
Effekte A und B, Distortion, Vintage, Pan Spread, Program Volume, der Name und die Poly-Sequenz (64 Schritte, sechs
Noten). Die Trigon-Programme kommen mit dem Ladder-Filter (LP2); TYPE schaltet auf die anderen beiden um.

## What it has

- **6 or 12 voices** (GLOBALS: one Trigon, or two poly-chained).
- Per voice: **three oscillators**, each with **triangle, sawtooth and pulse at once** (osc 3: sawtooth or reverse
  sawtooth), pulse width and volume; **SYNC OSC 2**; osc 3 with an **LO** range and **KEYBOARD** on/off; **noise**;
  **FDBK <> DRIVE** (left: the voice's output back into its filter; right: the filter driven harder).
- **Three filters** (TYPE): LP1 OTA 4-pole, LP2 ladder 4-pole (**2 POLE**: the Trigon's 12 dB mode, half band-pass at
  high resonance), SVF OTA 2-pole with **SVF MODE** (LP → BP → HP); all self-oscillating, keyboard tracking off / half
  / full; at 4x (HIGH) or 2x (ECO) the sample rate.
- Two **ADSR** (filter, amplifier), **velocity** on each.
- **LFO**: triangle, sawtooth, reverse sawtooth, square, random; free up to audio rate, or clock synced; eight
  destinations; the mod wheel raises its amount.
- **Aftertouch** (bipolar) to eight destinations; **POLY MOD**: osc 3 and the filter envelope (bipolar) to the three
  pitches, PW 1-2, the filter and the feedback, at audio rate.
- **VINTAGE** (each voice its own small differences in tuning, cutoff and timing), the stereo analog **DISTORTION**.
- **Effects A then B**: DDL and BBD delays (syncable), chorus, three phasers, ring mod, two flangers; B also room, hall,
  plate, spring.
- **Glide** (fixed rate or time, legato or always), **unison** of 1-6 voices or the **chord memory**, six key modes,
  **HOLD**, **TRANSPOSE**, the **clock** (ten divisions, TAP), the **arpeggiator** (up, down, up/down, random, assign;
  1-3 octaves), the **poly sequencer** (64 steps of six notes, RECORD with REST and TIE).
- **LOW SPLIT** (TREEGONE's own, 1.1): under a split point (SET: the next key played) one more voice of the same
  sound, monophonic, for a bass line under the chords: its OCTAVE, its own legato GLIDE, its VOLUME; +1 VOICE: a voice
  of its own (else it takes one of the six).
- **HOST SYNC**: the clock, synced LFO and delays follow the DAW.
- 24 factory programs of its own; WRITE keeps your own (Documents/TREEGONE/Programs).

## LOW SPLIT (kurz auf Deutsch)

Der Block unten rechts: **ON/OFF** schaltet ihn ein, **SET** drücken und dann die Taste spielen, ab der es oben
weitergeht (die Taste selbst gehört schon zum oberen Teil). Darunter spielt eine einzelne Stimme mit dem gleichen
Sound eine Basslinie: **OCTAVE** verschiebt sie (−2 bis +1), **GLIDE** lässt sie gleiten, solange noch eine Taste
gehalten wird, **VOLUME** stellt sie gegen die Akkorde ein. **+1 VOICE** an: der Bass ist eine siebte Stimme; aus: er
nimmt eine der sechs. Arp, Sequencer und Unison gelten nur für den oberen Teil. Der Split wird mit dem Programm
gespeichert.

## Build

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
JUCE 8 comes from GitHub. The engine's tests: `cmake -S tests -B build-tests && cmake --build build-tests &&
build-tests/engine_tests [your.syx]`. The MPC plugin: `python3 mpc/build.py --sdk <mpc-plugin-builder-sdk>`.

## Licence

MPL-2.0. Fonts: Fira Sans and Anonymous Pro, SIL Open Font License (fonts/).
