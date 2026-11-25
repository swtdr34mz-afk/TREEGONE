/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
/* TREEGONE's parameters, laid out as a Trigon-6 program: every knob and button of its panel, each an integer in the
 * Trigon's own range (CUTOFF 0..164, RESONANCE 0..255, the bipolar knobs 0..254 with 127 the middle, ...), so a
 * Trigon program read from a SysEx file lands on the knobs as it was set. TREEGONE's own additions: the FILTER TYPE
 * (the Pro 3's three: LP1 4-pole OTA, LP2 4-pole ladder - the Trigon's -, SVF 2-pole OTA) and the SVF's MODE. */
#pragma once
#include <cstdint>

namespace tg
{
enum FilterType { FT_OTA, FT_LADDER, FT_SVF, NFILTERTYPE };
enum LfoShape { LS_TRI, LS_SAW, LS_REVSAW, LS_SQUARE, LS_RANDOM, NLFOSHAPE };
enum GlideMode { GM_RATE, GM_RATE_LEGATO, GM_TIME, GM_TIME_LEGATO, NGLIDEMODE };
enum KeyMode { KM_LOW, KM_HIGH, KM_LAST, KM_LOW_R, KM_HIGH_R, KM_LAST_R, NKEYMODE };
enum ArpMode { AM_UP, AM_DOWN, AM_UPDOWN, AM_RANDOM, AM_ASSIGN, NARPMODE };
enum Divide { DV_HALF, DV_QTR, DV_8D, DV_8, DV_8S, DV_8T, DV_16, DV_16S, DV_16T, DV_32, NDIVIDE };
/* the effects: A has the first ten, B all fourteen (the reverbs only there, as on the Trigon) */
enum FxType {
    FX_OFF, FX_DDL, FX_BBD, FX_CHORUS, FX_PHASER1, FX_PHASER2, FX_PHASER3, FX_RING, FX_FLANGER1, FX_FLANGER2,
    FX_ROOM, FX_HALL, FX_PLATE, FX_SPRING, NFXTYPE
};
constexpr int NFXTYPE_A = 10;
constexpr int NPARAM_V1 = 102;                      /* (the parameters of 1.0: the MPC keeps their numbers) */
enum { NFXDIV = 9 };

enum Param {
    /* OSCILLATORS: 1 */
    P_O1_OCT, P_O1_TRI, P_O1_SAW, P_O1_PULSE, P_O1_PW, P_O1_LEVEL, P_SYNC,
    /* 2 */
    P_O2_OCT, P_O2_PITCH, P_O2_TRI, P_O2_SAW, P_O2_PULSE, P_O2_PW, P_O2_LEVEL,
    /* 3 (OCTAVE's first place: LO; SAW: off, sawtooth, reverse sawtooth) */
    P_O3_OCT, P_O3_PITCH, P_O3_TRI, P_O3_SAW, P_O3_PULSE, P_O3_PW, P_O3_LEVEL, P_O3_KEY,
    P_NOISE, P_FDBK_DRIVE,
    /* FILTER */
    P_F_TYPE, P_CUTOFF, P_RESO, P_F_ENV_AMT, P_POLE2, P_F_KEY, P_F_VEL, P_SVF_MODE,
    P_F_ATT, P_F_DEC, P_F_SUS, P_F_REL,
    /* AMPLIFIER */
    P_A_ATT, P_A_DEC, P_A_SUS, P_A_REL, P_A_VEL,
    /* LFO: its eight destinations */
    P_LFO_FREQ, P_LFO_SYNC, P_LFO_SHAPE, P_LFO_AMT,
    P_LFO_F1, P_LFO_F2, P_LFO_F3, P_LFO_PW1, P_LFO_PW2, P_LFO_PW3, P_LFO_FILT, P_LFO_AMP,
    /* AFTERTOUCH */
    P_AT_AMT, P_AT_F1, P_AT_F2, P_AT_F3, P_AT_FILT, P_AT_AMP, P_AT_LFO, P_AT_FXA, P_AT_FXB,
    /* POLY MOD */
    P_PM_OSC3, P_PM_FENV, P_PM_F1, P_PM_F2, P_PM_F3, P_PM_FILT, P_PM_PW1, P_PM_PW2, P_PM_FDBK,
    /* GLIDE, UNISON, KEY MODE */
    P_GLIDE_RATE, P_GLIDE, P_GLIDE_MODE, P_UNISON, P_UNI_VOICES, P_UNI_DETUNE, P_KEY_MODE,
    /* CLOCK */
    P_BPM, P_DIVIDE,
    /* EFFECTS: on/off (the bypass), then A and B */
    P_FX_ON, P_FXA_TYPE, P_FXA_MIX, P_FXA_P1, P_FXA_P2, P_FXA_SYNC, P_FXA_DIV,
    P_FXB_TYPE, P_FXB_MIX, P_FXB_P1, P_FXB_P2, P_FXB_SYNC, P_FXB_DIV,
    /* ARPEGGIATOR, SEQUENCER */
    P_ARP_ON, P_ARP_OCT, P_ARP_MODE, P_SEQ_ON,
    /* the strip under the program buttons */
    P_DISTORTION, P_VINTAGE, P_PAN_SPREAD, P_PGM_VOL, P_PB_RANGE,
    /* LOW SPLIT (TREEGONE's, 1.1): under the split point one more voice of the same sound, monophonic, for a bass
     * line: its octave, its own glide, its volume; +1 VOICE: a voice of its own (else one of the six) */
    P_SPLIT_ON, P_SPLIT_POINT, P_SPLIT_OCT, P_SPLIT_GLIDE, P_SPLIT_VOL, P_SPLIT_EXTRA,
    NPARAM
};

enum Kind : uint8_t {
    K_NUM,      /* as it is */
    K_BIP,      /* 0..254, 127 the middle: shown -127..+127 */
    K_CHOICE,
    K_ONOFF,
    K_OCT,      /* -2..+2 (osc 3: LO first) */
    K_PITCH,    /* osc 2 / 3 PITCH: semitones and cents */
    K_LFOFREQ,  /* Hz, or a clock division when synced */
    K_FXP1,     /* the effect's first parameter */
    K_NOTE      /* a MIDI note's name (middle C: C3, as Cubase names it) */
};

struct ParamDef {
    const char *id;           /* the host's id (stable: saved projects) */
    const char *name;         /* the host's and the display's name */
    int lo, hi, def;
    Kind kind;
    const char *const *choice;
};

extern const ParamDef PARAMS[NPARAM];
extern const char *const FXTYPE_NAME[NFXTYPE];
extern const char *const FXTYPE_SHORT[NFXTYPE];  /* the three letters of its display */
extern const char *const DIVIDE_NAME[NDIVIDE];
extern const char *const FXDIV_NAME[NFXDIV];
extern const char *const LFOSHAPE_NAME[NLFOSHAPE];
extern const char *const FILTERTYPE_NAME[NFILTERTYPE];
extern const char *const ARPMODE_NAME[NARPMODE];
extern const char *const KEYMODE_NAME[NKEYMODE];

/* PITCH: the knob's value as semitones (fine round the middle, wide at the ends: the Trigon's) */
float pitchSemis(int v);
/* the LFO's rate (Hz), free */
float lfoHzOf(int v);
/* the value as the display shows it */
void paramText(int p, int v, char *out, int n, bool synced = false);
/* the three characters of the Trigon's LED display for a value */
void paramShort(int p, int v, char *out, int n);

/* the poly sequencer: 64 steps of up to six notes, each with its velocity */
constexpr int PSEQ_STEPS = 64, PSEQ_NOTES = 6;
constexpr uint8_t PN_REST = 0, PN_TIE = 1;          /* a step's note slot: 0 rest, 1 tie, else note + 2 */
struct SeqData {
    uint8_t note[PSEQ_STEPS][PSEQ_NOTES];
    uint8_t vel[PSEQ_STEPS][PSEQ_NOTES];
    uint8_t length;                                  /* steps (0: none recorded) */
};
void seqClear(SeqData &s);

constexpr int NAME_LEN = 21, NCHORD = 6;
struct Program {
    char name[NAME_LEN];
    int16_t p[NPARAM];
    SeqData seq;
    int8_t chord[NCHORD];                            /* UNISON's chord memory: notes over the lowest (-1: none) */
};
void programInit(Program &p);                         /* the basic program: one sawtooth, the filter open */
} // namespace tg
