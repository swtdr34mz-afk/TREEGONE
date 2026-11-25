/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
#include "Params.h"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tg
{
static const char *const ONOFF[] = {"Off", "On"};
static const char *const O3SAW[] = {"Off", "Sawtooth", "Rev Saw"};
static const char *const KEYTRACK[] = {"Off", "Half", "Full"};
static const char *const GLIDEMODE[] = {"Fixed Rate", "Rate Legato", "Fixed Time", "Time Legato"};
static const char *const UNIVOICES[] = {"1", "2", "3", "4", "5", "6", "Chord"};
static const char *const ARPOCT[] = {"1", "2", "3"};
static const char *const SPLITOCT[] = {"-2", "-1", "0", "+1"};
const char *const LFOSHAPE_NAME[NLFOSHAPE] = {"Triangle", "Sawtooth", "Rev Saw", "Square", "Random"};
const char *const FILTERTYPE_NAME[NFILTERTYPE] = {"LP1 OTA 4-Pole", "LP2 Ladder 4-Pole", "SVF OTA 2-Pole"};
const char *const ARPMODE_NAME[NARPMODE] = {"Up", "Down", "Up/Down", "Random", "Assign"};
const char *const KEYMODE_NAME[NKEYMODE] = {"Low", "High", "Last", "Low Retrig", "High Retrig", "Last Retrig"};
const char *const FXTYPE_NAME[NFXTYPE] = {"Off",       "DDL Delay",  "BBD Delay", "Chorus",    "Phaser 1",
                                          "Phaser 2",  "Phaser 3",   "Ring Mod",  "Flanger 1", "Flanger 2",
                                          "Room",      "Hall",       "Plate",     "Spring"};
const char *const FXTYPE_SHORT[NFXTYPE] = {"OFF", "DDL", "BBD", "CHO", "PH1", "PH2", "PH3",
                                           "RNG", "FL1", "FL2", "ROO", "HAL", "PLA", "SPR"};
const char *const DIVIDE_NAME[NDIVIDE] = {"Half", "Quarter", "8th Dot", "8th", "8th Swing",
                                          "8th Trip", "16th", "16th Swing", "16th Trip", "32nd"};
const char *const FXDIV_NAME[NFXDIV] = {"Whole", "Half", "Qtr Dot", "Quarter", "8th Dot",
                                        "Qtr Trip", "8th", "8th Trip", "16th"};

#define N(id, name, lo, hi, def) {id, name, lo, hi, def, K_NUM, nullptr}
#define BP(id, name) {id, name, 0, 254, 127, K_BIP, nullptr}
#define C(id, name, n, def, list) {id, name, 0, (n) - 1, def, K_CHOICE, list}
#define B(id, name, def) {id, name, 0, 1, def, K_ONOFF, ONOFF}

const ParamDef PARAMS[NPARAM] = {
    {"o1_oct", "Osc 1 Octave", 0, 4, 2, K_OCT, nullptr},
    B("o1_tri", "Osc 1 Triangle", 0),
    B("o1_saw", "Osc 1 Sawtooth", 1),
    B("o1_pulse", "Osc 1 Pulse", 0),
    N("o1_pw", "Osc 1 Pulse Width", 0, 255, 127),
    N("o1_level", "Osc 1 Volume", 0, 127, 127),
    B("sync", "Sync Osc 2", 0),
    {"o2_oct", "Osc 2 Octave", 0, 4, 2, K_OCT, nullptr},
    {"o2_pitch", "Osc 2 Pitch", 0, 254, 188, K_PITCH, nullptr},
    B("o2_tri", "Osc 2 Triangle", 0),
    B("o2_saw", "Osc 2 Sawtooth", 1),
    B("o2_pulse", "Osc 2 Pulse", 0),
    N("o2_pw", "Osc 2 Pulse Width", 0, 255, 127),
    N("o2_level", "Osc 2 Volume", 0, 127, 0),
    {"o3_oct", "Osc 3 Octave", 0, 5, 3, K_OCT, nullptr},
    {"o3_pitch", "Osc 3 Pitch", 0, 254, 188, K_PITCH, nullptr},
    B("o3_tri", "Osc 3 Triangle", 0),
    C("o3_saw", "Osc 3 Saw", 3, 1, O3SAW),
    B("o3_pulse", "Osc 3 Pulse", 0),
    N("o3_pw", "Osc 3 Pulse Width", 0, 255, 127),
    N("o3_level", "Osc 3 Volume", 0, 127, 0),
    B("o3_key", "Osc 3 Keyboard", 1),
    N("noise", "Noise", 0, 127, 0),
    BP("fdbk_drive", "Fdbk <> Drive"),
    C("f_type", "Filter Type", NFILTERTYPE, FT_LADDER, FILTERTYPE_NAME),
    N("cutoff", "Cutoff", 0, 164, 164),
    N("reso", "Resonance", 0, 255, 0),
    BP("f_env_amt", "Filter Env Amount"),
    B("pole2", "2 Pole", 0),
    C("f_key", "Filter Keyboard", 3, 0, KEYTRACK),
    B("f_vel", "Filter Velocity", 1),
    N("svf_mode", "SVF Mode", 0, 127, 0),
    N("f_att", "Filter Attack", 0, 127, 0),
    N("f_dec", "Filter Decay", 0, 127, 0),
    N("f_sus", "Filter Sustain", 0, 127, 0),
    N("f_rel", "Filter Release", 0, 127, 0),
    N("a_att", "Amp Attack", 0, 127, 0),
    N("a_dec", "Amp Decay", 0, 127, 0),
    N("a_sus", "Amp Sustain", 0, 127, 127),
    N("a_rel", "Amp Release", 0, 127, 20),
    B("a_vel", "Amp Velocity", 1),
    {"lfo_freq", "LFO Frequency", 0, 255, 134, K_LFOFREQ, nullptr},
    B("lfo_sync", "LFO Sync", 0),
    C("lfo_shape", "LFO Shape", NLFOSHAPE, LS_TRI, LFOSHAPE_NAME),
    N("lfo_amt", "LFO Amount", 0, 255, 0),
    B("lfo_f1", "LFO > Freq 1", 0),
    B("lfo_f2", "LFO > Freq 2", 0),
    B("lfo_f3", "LFO > Freq 3", 0),
    B("lfo_pw1", "LFO > PW 1", 0),
    B("lfo_pw2", "LFO > PW 2", 0),
    B("lfo_pw3", "LFO > PW 3", 0),
    B("lfo_filt", "LFO > Filter", 0),
    B("lfo_amp", "LFO > Amp", 0),
    BP("at_amt", "Aftertouch Amount"),
    B("at_f1", "AT > Freq 1", 0),
    B("at_f2", "AT > Freq 2", 0),
    B("at_f3", "AT > Freq 3", 0),
    B("at_filt", "AT > Filter", 0),
    B("at_amp", "AT > Amp", 0),
    B("at_lfo", "AT > LFO", 0),
    B("at_fxa", "AT > FX Mix A", 0),
    B("at_fxb", "AT > FX Mix B", 0),
    BP("pm_osc3", "Poly Mod Osc 3 Amount"),
    BP("pm_fenv", "Poly Mod Filter Env"),
    B("pm_f1", "PM > Freq 1", 0),
    B("pm_f2", "PM > Freq 2", 0),
    B("pm_f3", "PM > Freq 3", 0),
    B("pm_filt", "PM > Filter", 0),
    B("pm_pw1", "PM > PW 1", 0),
    B("pm_pw2", "PM > PW 2", 0),
    B("pm_fdbk", "PM > Fdbk", 0),
    N("glide_rate", "Glide Rate", 0, 127, 70),
    B("glide", "Glide", 0),
    C("glide_mode", "Glide Mode", NGLIDEMODE, GM_RATE, GLIDEMODE),
    B("unison", "Unison", 0),
    C("uni_voices", "Unison Voices", 7, 5, UNIVOICES),
    N("uni_detune", "Unison Detune", 0, 15, 3),
    C("key_mode", "Key Mode", NKEYMODE, KM_LAST, KEYMODE_NAME),
    N("bpm", "BPM", 30, 250, 120),
    C("divide", "Clock Divide", NDIVIDE, DV_16, DIVIDE_NAME),
    B("fx_on", "FX On/Off", 1),
    C("fxa_type", "FX A Type", NFXTYPE_A, FX_OFF, FXTYPE_NAME),
    N("fxa_mix", "FX A Dry/Wet", 0, 127, 40),
    {"fxa_p1", "FX A Param 1", 0, 255, 100, K_FXP1, nullptr},
    N("fxa_p2", "FX A Param 2", 0, 127, 64),
    B("fxa_sync", "FX A Sync", 0),
    C("fxa_div", "FX A Sync Time", NFXDIV, 3, FXDIV_NAME),
    C("fxb_type", "FX B Type", NFXTYPE, FX_OFF, FXTYPE_NAME),
    N("fxb_mix", "FX B Dry/Wet", 0, 127, 40),
    {"fxb_p1", "FX B Param 1", 0, 255, 100, K_FXP1, nullptr},
    N("fxb_p2", "FX B Param 2", 0, 127, 64),
    B("fxb_sync", "FX B Sync", 0),
    C("fxb_div", "FX B Sync Time", NFXDIV, 3, FXDIV_NAME),
    B("arp_on", "Arp On/Off", 0),
    C("arp_oct", "Arp Octaves", 3, 0, ARPOCT),
    C("arp_mode", "Arp Mode", NARPMODE, AM_UP, ARPMODE_NAME),
    B("seq_on", "Seq Play", 0),
    N("distortion", "Distortion", 0, 127, 0),
    N("vintage", "Vintage", 0, 127, 0),
    N("pan_spread", "Pan Spread", 0, 127, 0),
    N("pgm_vol", "Program Volume", 0, 127, 100),
    N("pb_range", "Pitch Wheel Range", 1, 12, 2),
    B("split_on", "Low Split", 0),
    {"split_point", "Split Point", 0, 127, 48, K_NOTE, nullptr},
    C("split_oct", "Split Octave", 4, 2, SPLITOCT),
    N("split_glide", "Split Glide", 0, 127, 0),
    N("split_vol", "Split Volume", 0, 127, 100),
    B("split_extra", "Split +1 Voice", 1),
};

/* PITCH, osc 2 and 3: the middle (188, where the factory programs keep it) is in tune; a few steps either side are
 * cents (the factory's 193 and 182 a slight beat), further out semitones, up to +7 at the top and a fifth down at
 * 120; under 120 the knob goes on down, to two octaves under */
float pitchSemis(int v)
{
    if (v >= 188) {
        float x = (float)(v - 188) / 66.0f;
        return 7.0f * std::pow(x, 1.7f);
    }
    if (v >= 120) {
        float x = (float)(188 - v) / 68.0f;
        return -7.0f * std::pow(x, 1.7f);
    }
    return -7.0f - 17.0f * (float)(120 - v) / 120.0f;
}

float lfoHzOf(int v) { return 0.03f * std::exp2((float)v * (14.5f / 255.0f)); }

static const char *const LFOSYNC_NAME[16] = {"8 bars", "4 bars", "2 bars", "1 bar", "1/2 dot", "1/2",   "1/4 dot", "1/4",
                                             "1/4 T",  "1/8 dot", "1/8",  "1/8 T", "1/16",    "1/16 T", "1/32", "1/64"};

void paramText(int p, int v, char *out, int n, bool synced)
{
    const ParamDef &d = PARAMS[p];
    switch (d.kind) {
    case K_BIP: std::snprintf(out, (size_t)n, v > 127 ? "+%d" : "%d", v - 127); break;
    case K_CHOICE:
    case K_ONOFF: std::snprintf(out, (size_t)n, "%s", d.choice[v < d.lo ? d.lo : v > d.hi ? d.hi : v]); break;
    case K_OCT:
        if (p == P_O3_OCT)
            std::snprintf(out, (size_t)n, v == 0 ? "LO" : v > 3 ? "+%d" : "%d", v - 3);
        else
            std::snprintf(out, (size_t)n, v > 2 ? "+%d" : "%d", v - 2);
        break;
    case K_PITCH: {
        float s = pitchSemis(v);
        if (std::fabs(s) < 0.995f)
            std::snprintf(out, (size_t)n, "%+d ct", (int)std::lround(s * 100.0f));
        else
            std::snprintf(out, (size_t)n, "%+.1f st", s);
        break;
    }
    case K_LFOFREQ:
        if (synced)
            std::snprintf(out, (size_t)n, "%s", LFOSYNC_NAME[v * 16 / 256]);
        else {
            float hz = lfoHzOf(v);
            std::snprintf(out, (size_t)n, hz < 10.0f ? "%.2f Hz" : "%.0f Hz", hz);
        }
        break;
    case K_NOTE: {
        static const char *const NT[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        std::snprintf(out, (size_t)n, "%s%d", NT[(v % 12 + 12) % 12], v / 12 - 2);
        break;
    }
    default: std::snprintf(out, (size_t)n, "%d", v);
    }
}

void paramShort(int p, int v, char *out, int n)
{
    const ParamDef &d = PARAMS[p];
    if (p == P_FXA_TYPE || p == P_FXB_TYPE) {
        std::snprintf(out, (size_t)n, "%s", FXTYPE_SHORT[v < 0 ? 0 : v >= NFXTYPE ? NFXTYPE - 1 : v]);
        return;
    }
    switch (d.kind) {
    case K_BIP: std::snprintf(out, (size_t)n, "%d", v - 127); break;
    case K_ONOFF: std::snprintf(out, (size_t)n, "%s", v ? "ON" : "OFF"); break;
    case K_OCT:
        if (p == P_O3_OCT && v == 0)
            std::snprintf(out, (size_t)n, "LO");
        else
            std::snprintf(out, (size_t)n, "%d", v - (p == P_O3_OCT ? 3 : 2));
        break;
    case K_PITCH: std::snprintf(out, (size_t)n, "%d", v - 188); break;
    case K_NOTE: {
        static const char *const NT[12] = {"C", "C.", "D", "D.", "E", "F", "F.", "G", "G.", "A", "A.", "B"};
        std::snprintf(out, (size_t)n, "%s%d", NT[(v % 12 + 12) % 12], v / 12 - 2);
        break;
    }
    default: std::snprintf(out, (size_t)n, "%d", v);
    }
}

void seqClear(SeqData &s) { std::memset(&s, 0, sizeof s); }

void programInit(Program &p)
{
    std::memset(&p, 0, sizeof p);
    std::snprintf(p.name, NAME_LEN, "Basic Program");
    for (int i = 0; i < NPARAM; i++)
        p.p[i] = (int16_t)PARAMS[i].def;
    seqClear(p.seq);
    for (auto &c : p.chord)
        c = -1;
}
} // namespace tg
