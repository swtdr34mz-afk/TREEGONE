/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
/* TREEGONE's engine: a Trigon-6, voice by voice as its panel shows it.
 *
 *   voices     6 (the Trigon), or 12 (two of them poly-chained)
 *   per voice  three oscillators, each any of triangle, sawtooth (osc 3: or reverse sawtooth) and pulse at once, its
 *              pulse width and volume; osc 2 hard-synced to osc 1; osc 3 with an LFO range (LO) and KEYBOARD on/off;
 *              white noise; FDBK <> DRIVE (to the left the voice's own output back into its filter, the Minimoog's
 *              trick, to the right the filter driven harder); the filter: the Trigon's ladder (4-pole, or 2-pole)
 *              and TREEGONE's two more, the Pro 3's: LP1, a 4-pole OTA low-pass, and SVF, a 2-pole OTA state
 *              variable from low-pass through band-pass to high-pass (MODE); keyboard tracking off, half, full; two
 *              ADSR (filter, amplifier), velocity on each; VINTAGE (each voice its own small differences)
 *   modulation one LFO (five shapes, free to audio rate or clock synced) to eight places; aftertouch to eight; POLY
 *              MOD: osc 3 and the filter envelope (each bipolar) to the oscillators' pitch, pulse width, the filter,
 *              the feedback, at audio rate; the mod wheel raises the LFO's amount
 *   after      the stereo analog DISTORTION, effects A then B, PAN SPREAD
 *   playing    glide (fixed rate or time, legato or always), unison of 1 to 6 voices or the chord memory, six key
 *              modes, HOLD, the clock with its divisions, the arpeggiator, the poly sequencer (64 steps of six notes)
 *
 * The engine renders in chunks of 16 samples (the knobs' ramps); notes land on the next chunk; what moves at audio
 * rate (the LFO, poly mod, the envelopes) moves per sample. All calls from the audio thread. */
#pragma once
#include "Dsp.h"
#include "Fx.h"
#include "Params.h"

namespace tg
{
constexpr int MAXV = 12, CH = 16, NKEYS = 32;
enum { SM_G1, SM_G2, SM_G3, SM_GN, SM_CUT, SM_ENV, SM_RES, SM_FB, SM_DRV, SM_AMP, SM_PGM, SM_PAN, SM_PW1, SM_PW2, SM_PW3,
       SM_MODE, NSM };

class Engine
{
public:
    Engine();
    void prepare(double sampleRate);
    void setProgram(const Program &p);           /* the whole program, any time (a block's start) */
    const Program &program() const { return prog; }
    void setParam(int p, int v);
    void setVoices(int n);                       /* 6 or 12 */
    void setQuality(int q) { quality = q ? 1 : 0; }   /* 1 HIGH: the filters at 4x; 0 ECO: 2x */
    int voiceCount() const { return nVoices; }

    void noteOn(int note, float vel);
    void noteOff(int note);
    void sustain(bool on);
    void bend(float v);                          /* -1..1 */
    void modWheel(float v);                      /* 0..1 */
    void pressure(float v);
    void allOff();
    void panic();
    void host(bool playing, double ppq, double bpm, bool sync);

    void render(float *L, float *R, int n);

    void seqPlay(bool on);
    void seqRecord(bool on);
    void seqRest();
    void seqTie();
    void seqBack();
    int seqRecStep() const { return recStep; }
    bool seqRecording() const { return rec; }
    bool seqPlaying() const { return playing; }
    void hold(bool on);
    bool holding() const { return holdOn; }
    void unisonChord();                          /* UNISON pressed with keys held: their chord kept */

    int voicesSounding() const;
    float voiceLevel(int v) const { return v >= 0 && v < MAXV ? V[v].ae.v : 0.0f; }
    bool splitSounding() const { return V[MAXV].on && V[MAXV].gate; }
    int seqStep() const;
    bool clockBeat() const;
    float bpmShown() const { return bpmNow; }
    float lfoShown() const { return lfoShow; }

private:
    struct Env {
        float v = 0.0f;
        uint8_t st = 0;
    };
    enum { ES_IDLE, ES_ATT, ES_DEC, ES_REL };

    struct Voice {
        bool on = false;                         /* sounding (or about to) */
        int note = -1, key = -1;
        bool gate = false, sus = false;
        float vel = 0.0f, uniDet = 0.0f;
        float p = 60.0f, t = 60.0f, gr = 0.0f;   /* pitch now, where it glides, how fast (semitones a second) */
        Osc o[3];
        Env fe, ae;
        float f[4] = {}, ffb = 0.0f;             /* the filter's stages (the SVF: the first two), the ladder's feedback */
        HalfBand os1, os2;
        float sm[NSM] = {};
        bool smOk = false;
        float vr[6] = {};                        /* VINTAGE: this voice's own differences (-1..1) */
        float dr[3] = {}, dt[3] = {};            /* and its slow drift */
        int drn = 0;
        float fbOut = 0.0f;                      /* its last output (FDBK) */
        float o3last = 0.0f, kick = 0.0f;
        uint32_t seed = 1, age = 0;
        float fade = 1.0f;
        int pnote = -1, pkey = -1;
        float pvel = 0.0f, pdet = 0.0f;
        bool pend = false;
        float panSlot = 0.0f;
        float gmul = 1.0f;                       /* (the LOW SPLIT voice: its VOLUME) */
    };

    Program prog;
    Voice V[MAXV + 1];                           /* the last: LOW SPLIT's voice */
    Fx fxA, fxB;
    int nVoices = 6, quality = 1;
    float sr = 44100.0f;
    const float *tab = nullptr;
    uint32_t ageN = 0, noiseSeed = 0x2545F491u, lfoSeed = 0x9E3779B9u;
    float bendV = 0, mwV = 0, prV = 0, ctl[3] = {};
    bool susOn = false, holdOn = false;
    bool hostPlaying = false, hostSync = true;
    double hostPpq = 0.0, hostBpm = 120.0;
    float outL[CH] = {}, outR[CH] = {};
    int gpos = CH;
    float lfoBuf[CH] = {}, lfoPh = 0.0f, lfoSh = 0.0f, lfoShow = 0.0f;
    float distLp[2] = {}, dcL = 0, dcR = 0, dcxL = 0, dcxR = 0, dcK = 0.999f;
    /* keys held, in order */
    int8_t keys[NKEYS];
    float keyVel[NKEYS];
    int nKeys = 0;
    bool anyHeld = false;
    float lastPitch = 60.0f;
    /* the clock */
    double pos = 0.0;
    float bpmNow = 120.0f;
    long stepIdx = -1;
    bool stepHigh = false;
    /* the arp */
    int8_t arpNotes[NKEYS];
    float arpVel[NKEYS];
    int arpN = 0, arpI = -1, arpDir = 1, arpPlaying = -1;
    bool arpLatched = false;
    /* the poly sequencer */
    bool playing = false, rec = false;
    int pStep = -1, recStep = 0, recHeld = 0;
    bool recFresh = true;
    uint8_t sounding[PSEQ_NOTES] = {};
    int uniN = 1;
    /* LOW SPLIT: the keys held under the split point, in order */
    int8_t bKeys[NKEYS];
    float bVel[NKEYS];
    int nB = 0;

    int cap() const { return nVoices - (prog.p[P_SPLIT_ON] && !prog.p[P_SPLIT_EXTRA] ? 1 : 0); }
    bool bassKey(int note, float vel, bool down);
    void bassPlay(int note, float vel, bool legato);
    void bassStop();
    void key(int note, float vel, bool down);
    void playNote(int key, int note, float vel);
    void stopNote(int key);
    void startVoice(Voice &v, int key, int note, float vel, float det, bool retrig);
    int findVoice();
    void unisonUpdate(bool newKey);
    void releaseVoice(Voice &v);
    void voiceKill(Voice &v);
    void chunk();
    void clockChunk();
    void arpStep();
    void arpRebuild();
    void polyStep();
    void voiceChunk(Voice &v, float *L, float *R);
    float envTarget(Env &e, int base, float tscale, float *tgt, bool *att) const;
};
} // namespace tg
