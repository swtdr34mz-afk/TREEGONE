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

| Taste | Was drauf ist |
| --- | --- |
| **OSC** | die drei Oszillatoren (Octave, Pitch, Wellenformen, Pulse Width, Volume), Sync, Osc-3-Keyboard, Noise, Fdbk/Drive |
| **FILTER** | Filter-Typ (LP1 OTA, LP2 Ladder, SVF), Cutoff, Resonance, Env Amt, SVF Mode, 2 Pole, Keyboard, Velocity, Filter-Hüllkurve; Amp-Hüllkurve |
| **MOD** | LFO (Frequency, Sync, Shape, Amount, acht Ziele); Poly Mod (Osc 3, Filter Env, sieben Ziele) |
| **AT/FX** | Aftertouch (Amount, acht Ziele); Effekte A und B (Typ, Dry/Wet, zwei Werte, Sync), FX On |
| **PLAY** | Arp, Clock Divide, Sequencer PLAY; Glide, Unison, Key Mode, Pitch-Wheel |
| **MASTER** | Distortion, Vintage, Pan Spread, Program-Volume, Lautstärke; 6/12 Stimmen, Qualität, Hold |

Ein Knopf wird mit dem Finger gezogen oder mit seinem Q-Link gedreht; zweimal tippen öffnet den großen Regler der MPC
(bei einer Auswahl die Liste). Ein Taster schaltet um, eine LED-Liste wählt aus.

**QUALITY**: ECO (2x) ist voreingestellt und reicht auf der MPC; HIGH (4x) braucht fast doppelt so viel Rechenzeit.

Die Programme: die 24 Werks-Sounds von TREEGONE. (Trigon-Programme aus einer .syx-Datei lädt die PC/Mac-Version; die
MPC hat dafür keinen Dateidialog.)

## Licence

MPL-2.0. Fonts: Fira Sans and Anonymous Pro, SIL Open Font License.
