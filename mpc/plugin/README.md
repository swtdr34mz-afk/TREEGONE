# TREEGONE for the MPC

The TREEGONE synth as a plugin for Akai MPC standalone hardware: the same engine as the TREEGONE VST3, built with
Akai's MPC Plugin SDK. A polysynth in the manner of the Sequential Trigon-6: three oscillators per voice (triangle,
sawtooth, pulse at once; osc 3 also reverse sawtooth and an LFO range), sync, noise, FDBK <> DRIVE, three filters as on
the Pro 3 (LP1 4-pole OTA, LP2 4/2-pole ladder, SVF 2-pole OTA), two ADSR, the LFO, poly mod, aftertouch, vintage,
distortion, two effects, glide, unison with chord memory, the arp and the poly sequencer, 6 or 12 voices.

Unofficial: not made, endorsed or supported by Sequential.

## Installieren (kurz auf Deutsch)

1. MPC OS **3.10.1 oder neuer**. Auf der MPC: **Preferences → Enhancements (Beta) → Enable Enhancements** und
   **External Plugin Support** einschalten.
2. Den Ordner **`swtdr34mz - VST - TREEGONE`** (aus dem Download `TREEGONE-MPC.zip`) auf einen USB-Stick, eine
   SD-Karte oder das interne Laufwerk der MPC kopieren, in einen Ordner namens **`VST2 Plugins`** ganz oben auf dem
   Laufwerk: `VST2 Plugins/swtdr34mz - VST - TREEGONE/`.
3. Die MPC neu starten. TREEGONE steht dann im Plugin-Browser unter **External Plugins**.

Die MPC XL, Live III (64 Bit) und die 32-Bit-Geräte (X, X SE, Live, Live II, One, One+, Key 61, Key 37) nehmen
denselben Ordner; jedes Gerät lädt seine eigene Datei darin.

## Die Seiten

Aufgeteilt wie das Panel des Trigon-6: jeder Bereich eine Seite, Titel und Linien statt Kästen.

| Taste | Was drauf ist |
| --- | --- |
| **OSC** | OSCILLATORS 1 2 3: Octave (mit LEDs drumherum, Osc 3 auch LO), Sync Osc 2 / Pitch, Dreieck, Säge (Osc 3: Saw/Rev Saw), Puls, Pulse Width, Volume; Fdbk/Drive, Noise, Osc-3-Keyboard |
| **FLT/AMP** | FILTER: Typ (LP1 OTA, LP2 Ladder, SVF), Cutoff, Resonance, Env Amt, SVF Mode, 2 Pole, Keyboard (Half/Full), Hüllkurve, Velocity; AMPLIFIER: Hüllkurve, Velocity |
| **LFO/PM** | LFO: Frequency, Sync, Shape, Amount, acht Ziele; POLYMOD: Osc 3, Filter Env, sieben Ziele |
| **FX** | EFFECTS: On/Off, Displays A und B, A/B (welcher Effekt unten bearbeitet wird), Type; Parameter 1 und 2 mit Displays, Sync, Sync Time, Dry/Wet |
| **AT/ARP** | AFTER TOUCH: Amount, acht Ziele; ARPEGGIATOR: On/Off, Octaves, Mode; CLOCK: Divide, Hold |
| **PLAY** | Hold, Glide Rate, Glide, Unison, Distortion, Vintage, Pan Spread, Pgm Volume, P Whl Range, Key Mode; Glide Mode, Unison-Stimmen und -Detune, 6/12 Stimmen, Qualität, Lautstärke; LOW SPLIT |

Ein Knopf wird mit dem Finger gezogen oder mit seinem Q-Link gedreht; zweimal tippen öffnet den großen Regler der MPC
(bei einer Auswahl die Liste). Ein Taster schaltet um, eine LED wählt aus (auch die LEDs um die Octave-Knöpfe). Auf FX wählen A/B und die LEDs A und B, welcher Effekt unten bearbeitet wird (der Parameter FX EDIT, ohne eigenen Klang).

**QUALITY**: ECO (2x) ist voreingestellt und reicht auf der MPC; HIGH (4x) braucht fast doppelt so viel Rechenzeit.

Die Programme: die 24 Werks-Sounds von TREEGONE. (Trigon-Programme aus einer .syx-Datei lädt die PC/Mac-Version; die
MPC hat dafür keinen Dateidialog.)

## Licence

MPL-2.0. Fonts: Fira Sans and Anonymous Pro, SIL Open Font License.
