/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
#include "Presets.h"
#include <cstdio>
#include <initializer_list>
#include <utility>

namespace tg
{
namespace
{
using KV = std::pair<int, int>;

Program make(const char *name, std::initializer_list<KV> kv)
{
    Program p;
    programInit(p);
    std::snprintf(p.name, NAME_LEN, "%s", name);
    for (auto &x : kv)
        p.p[x.first] = (int16_t)x.second;
    return p;
}
void chord(Program &p, std::initializer_list<int> notes)
{
    int i = 0;
    for (int n : notes)
        if (i < NCHORD)
            p.chord[i++] = (int8_t)n;
}
void seq(Program &p, std::initializer_list<int> notes, int vel = 100)
{
    int s = 0;
    for (int n : notes) {
        if (s >= PSEQ_STEPS)
            break;
        p.seq.note[s][0] = n < 0 ? PN_REST : (uint8_t)(n + 2);
        p.seq.vel[s][0] = (uint8_t)vel;
        s++;
    }
    p.seq.length = (uint8_t)s;
}

/* the pitches: PITCH for a fifth up (+7), a slight beat */
constexpr int FIFTH = 254, BEAT = 193, BEAT_DN = 183;

struct Bank {
    Program p[NFACTORY];
    Bank()
    {
        int i = 0;
        /* three saws, the ladder, a brass swell */
        p[i++] = make("Trigon Brass", {{P_O2_LEVEL, 110}, {P_O2_PITCH, BEAT}, {P_O3_LEVEL, 100}, {P_O3_PITCH, BEAT_DN},
                                       {P_O1_LEVEL, 110}, {P_CUTOFF, 58}, {P_RESO, 30}, {P_F_ENV_AMT, 200}, {P_F_ATT, 40},
                                       {P_F_DEC, 78}, {P_F_SUS, 60}, {P_F_REL, 70}, {P_A_ATT, 12}, {P_A_SUS, 120},
                                       {P_A_REL, 64}, {P_F_KEY, 1}, {P_VINTAGE, 30}, {P_FXB_TYPE, FX_HALL}, {P_FXB_MIX, 30},
                                       {P_FXB_P1, 120}, {P_PAN_SPREAD, 60}, {P_FDBK_DRIVE, 150}});
        /* a fat mono lead: unison, glide, a little feedback */
        p[i++] = make("Creamy Lead", {{P_O2_LEVEL, 115}, {P_O2_PITCH, BEAT}, {P_O3_LEVEL, 105}, {P_O3_OCT, 2},
                                      {P_O1_LEVEL, 120}, {P_CUTOFF, 70}, {P_RESO, 70}, {P_F_ENV_AMT, 175}, {P_F_DEC, 70},
                                      {P_F_SUS, 50}, {P_A_REL, 40}, {P_F_KEY, 1}, {P_UNISON, 1}, {P_UNI_VOICES, 3},
                                      {P_UNI_DETUNE, 4}, {P_GLIDE, 1}, {P_GLIDE_RATE, 55}, {P_GLIDE_MODE, GM_RATE_LEGATO},
                                      {P_FDBK_DRIVE, 95}, {P_LFO_FREQ, 140}, {P_LFO_F1, 1}, {P_LFO_F2, 1}, {P_LFO_F3, 1},
                                      {P_AT_AMT, 200}, {P_AT_LFO, 1}, {P_FXA_TYPE, FX_BBD}, {P_FXA_MIX, 28}, {P_FXA_P1, 150},
                                      {P_FXA_P2, 50}, {P_KEY_MODE, KM_LAST}});
        /* the ladder's bass: saw and a square under it, driven */
        p[i++] = make("Ladder Bass", {{P_O1_OCT, 1}, {P_O2_OCT, 0}, {P_O2_SAW, 0}, {P_O2_PULSE, 1}, {P_O2_LEVEL, 100},
                                      {P_CUTOFF, 40}, {P_RESO, 90}, {P_F_ENV_AMT, 205}, {P_F_DEC, 52}, {P_F_SUS, 10},
                                      {P_F_REL, 40}, {P_A_DEC, 70}, {P_A_SUS, 100}, {P_A_REL, 30}, {P_F_KEY, 1},
                                      {P_FDBK_DRIVE, 190}, {P_UNISON, 1}, {P_UNI_VOICES, 1}, {P_UNI_DETUNE, 2}, {P_KEY_MODE, KM_LAST_R}});
        /* fifths, slow, wide */
        p[i++] = make("Fifth Cathedral", {{P_O2_LEVEL, 100}, {P_O2_PITCH, FIFTH}, {P_O3_LEVEL, 90}, {P_O3_OCT, 4},
                                          {P_O3_PITCH, BEAT}, {P_CUTOFF, 75}, {P_RESO, 40}, {P_F_ENV_AMT, 160}, {P_F_ATT, 90},
                                          {P_F_DEC, 95}, {P_F_SUS, 70}, {P_F_REL, 95}, {P_A_ATT, 85}, {P_A_REL, 98},
                                          {P_VINTAGE, 45}, {P_PAN_SPREAD, 110}, {P_FXA_TYPE, FX_CHORUS}, {P_FXA_MIX, 50},
                                          {P_FXA_P1, 40}, {P_FXA_P2, 80}, {P_FXB_TYPE, FX_HALL}, {P_FXB_MIX, 55}, {P_FXB_P1, 200}});
        /* the OTA low-pass, pulses swept by the LFO */
        p[i++] = make("OTA Strings", {{P_F_TYPE, FT_OTA}, {P_O1_SAW, 0}, {P_O1_PULSE, 1}, {P_O2_SAW, 0}, {P_O2_PULSE, 1},
                                      {P_O2_LEVEL, 110}, {P_O2_PITCH, BEAT}, {P_O2_PW, 100}, {P_O3_LEVEL, 80}, {P_O3_OCT, 4},
                                      {P_CUTOFF, 100}, {P_RESO, 20}, {P_F_ENV_AMT, 150}, {P_F_ATT, 70}, {P_F_DEC, 90}, {P_F_SUS, 80},
                                      {P_F_REL, 90}, {P_A_ATT, 72}, {P_A_REL, 92}, {P_LFO_FREQ, 105}, {P_LFO_AMT, 150},
                                      {P_LFO_PW1, 1}, {P_LFO_PW2, 1}, {P_PAN_SPREAD, 100}, {P_FXA_TYPE, FX_CHORUS},
                                      {P_FXA_MIX, 45}, {P_FXA_P1, 30}, {P_FXA_P2, 70}, {P_FXB_TYPE, FX_HALL}, {P_FXB_MIX, 45},
                                      {P_FXB_P1, 170}});
        /* the SVF as a band-pass: a hollow pluck into echoes */
        p[i++] = make("SVF Pluck", {{P_F_TYPE, FT_SVF}, {P_SVF_MODE, 64}, {P_O1_TRI, 1}, {P_O2_LEVEL, 70}, {P_O2_OCT, 3},
                                    {P_CUTOFF, 70}, {P_RESO, 150}, {P_F_ENV_AMT, 215}, {P_F_DEC, 45}, {P_F_REL, 45},
                                    {P_A_DEC, 62}, {P_A_SUS, 0}, {P_A_REL, 58}, {P_F_KEY, 2}, {P_FXA_TYPE, FX_DDL},
                                    {P_FXA_MIX, 40}, {P_FXA_SYNC, 1}, {P_FXA_DIV, 4}, {P_FXA_P2, 70}, {P_FXB_TYPE, FX_PLATE},
                                    {P_FXB_MIX, 30}, {P_FXB_P1, 120}, {P_PAN_SPREAD, 80}});
        /* hard sync, the filter envelope sweeping osc 2 (POLY MOD) */
        p[i++] = make("Sync Sweeper", {{P_SYNC, 1}, {P_O1_LEVEL, 0}, {P_O2_LEVEL, 120}, {P_O2_PITCH, 210}, {P_PM_FENV, 225},
                                       {P_PM_F2, 1}, {P_CUTOFF, 95}, {P_RESO, 40}, {P_F_ENV_AMT, 170}, {P_F_DEC, 80},
                                       {P_F_SUS, 30}, {P_A_REL, 40}, {P_O3_LEVEL, 70}, {P_O3_OCT, 2}, {P_FXA_TYPE, FX_BBD},
                                       {P_FXA_MIX, 25}, {P_FXA_P1, 120}, {P_FXA_P2, 40}});
        /* osc 3 into osc 1's pitch at audio rate: bells */
        p[i++] = make("Poly Mod Bells", {{P_O1_SAW, 0}, {P_O1_TRI, 1}, {P_O3_SAW, 0}, {P_O3_TRI, 1}, {P_O3_OCT, 5},
                                         {P_O3_PITCH, 230}, {P_PM_OSC3, 205}, {P_PM_F1, 1}, {P_PM_FENV, 127}, {P_CUTOFF, 120},
                                         {P_F_ENV_AMT, 160}, {P_F_DEC, 75}, {P_A_DEC, 88}, {P_A_SUS, 0}, {P_A_REL, 85},
                                         {P_FXB_TYPE, FX_PLATE}, {P_FXB_MIX, 45}, {P_FXB_P1, 170}, {P_PAN_SPREAD, 90}});
        /* the feedback loop: a growl, the 2-pole ladder half band-pass */
        p[i++] = make("Feedback Growl", {{P_O1_OCT, 1}, {P_O2_LEVEL, 100}, {P_O2_OCT, 1}, {P_O2_PITCH, BEAT}, {P_FDBK_DRIVE, 20},
                                         {P_POLE2, 1}, {P_CUTOFF, 55}, {P_RESO, 160}, {P_F_ENV_AMT, 185}, {P_F_DEC, 66},
                                         {P_F_SUS, 40}, {P_A_REL, 30}, {P_UNISON, 1}, {P_UNI_VOICES, 2}, {P_UNI_DETUNE, 5},
                                         {P_DISTORTION, 25}});
        /* an old keyboard: soft waves, lots of VINTAGE, a phaser */
        p[i++] = make("Vintage Keys", {{P_O1_SAW, 0}, {P_O1_TRI, 1}, {P_O1_PULSE, 1}, {P_O1_PW, 40}, {P_O2_SAW, 0}, {P_O2_TRI, 1},
                                       {P_O2_LEVEL, 80}, {P_O2_OCT, 3}, {P_CUTOFF, 85}, {P_F_ENV_AMT, 170}, {P_F_DEC, 80},
                                       {P_F_SUS, 20}, {P_A_DEC, 95}, {P_A_SUS, 30}, {P_A_REL, 55}, {P_VINTAGE, 95},
                                       {P_FXA_TYPE, FX_PHASER3}, {P_FXA_MIX, 55}, {P_FXA_P1, 40}, {P_FXA_P2, 90},
                                       {P_FXB_TYPE, FX_ROOM}, {P_FXB_MIX, 30}, {P_FXB_P1, 120}});
        /* the arpeggiator over two octaves, a synced bucket-brigade echo */
        p[i++] = make("Arp Machine", {{P_ARP_ON, 1}, {P_ARP_OCT, 1}, {P_ARP_MODE, AM_UPDOWN}, {P_DIVIDE, DV_16}, {P_O1_SAW, 0},
                                      {P_O1_PULSE, 1}, {P_O1_PW, 70}, {P_O2_LEVEL, 90}, {P_O2_PITCH, BEAT}, {P_CUTOFF, 60},
                                      {P_RESO, 110}, {P_F_ENV_AMT, 200}, {P_F_DEC, 40}, {P_A_DEC, 50}, {P_A_SUS, 40},
                                      {P_A_REL, 40}, {P_FXA_TYPE, FX_BBD}, {P_FXA_MIX, 45}, {P_FXA_SYNC, 1}, {P_FXA_DIV, 4},
                                      {P_FXA_P2, 70}, {P_PAN_SPREAD, 90}});
        /* a sequence (PLAY), the low-pass opening with the LFO */
        p[i++] = make("Seq Pulse", {{P_O1_OCT, 1}, {P_O2_LEVEL, 100}, {P_O2_PULSE, 1}, {P_O2_SAW, 0}, {P_CUTOFF, 45},
                                    {P_RESO, 140}, {P_F_ENV_AMT, 195}, {P_F_DEC, 42}, {P_A_DEC, 55}, {P_A_SUS, 30},
                                    {P_A_REL, 30}, {P_LFO_FREQ, 60}, {P_LFO_AMT, 60}, {P_LFO_FILT, 1}, {P_FXA_TYPE, FX_DDL},
                                    {P_FXA_MIX, 30}, {P_FXA_SYNC, 1}, {P_FXA_DIV, 4}, {P_FXA_P2, 60}});
        seq(p[i - 1], {48, 48, 60, 48, 51, 48, 58, 48, 48, 55, 48, 60, 63, 48, 58, 55});
        /* UNISON with a chord kept: one key, a minor ninth's voicing */
        p[i] = make("Chord Memory", {{P_UNISON, 1}, {P_UNI_VOICES, 6}, {P_O2_LEVEL, 90}, {P_O2_PITCH, BEAT}, {P_CUTOFF, 62},
                                     {P_RESO, 60}, {P_F_ENV_AMT, 200}, {P_F_DEC, 60}, {P_F_SUS, 20}, {P_A_DEC, 70}, {P_A_SUS, 60},
                                     {P_A_REL, 50}, {P_FXA_TYPE, FX_CHORUS}, {P_FXA_MIX, 40}, {P_FXB_TYPE, FX_ROOM},
                                     {P_FXB_MIX, 30}});
        chord(p[i++], {0, 3, 7, 10, 14});
        /* the SVF high-pass: air and noise */
        p[i++] = make("High Air", {{P_F_TYPE, FT_SVF}, {P_SVF_MODE, 127}, {P_O2_LEVEL, 100}, {P_O2_PITCH, BEAT}, {P_NOISE, 50},
                                   {P_CUTOFF, 85}, {P_RESO, 120}, {P_F_ENV_AMT, 100}, {P_F_ATT, 90}, {P_F_DEC, 100},
                                   {P_F_SUS, 60}, {P_A_ATT, 80}, {P_A_REL, 100}, {P_LFO_FREQ, 70}, {P_LFO_AMT, 80},
                                   {P_LFO_FILT, 1}, {P_FXA_TYPE, FX_FLANGER2}, {P_FXA_MIX, 40}, {P_FXA_P1, 20},
                                   {P_FXA_P2, 90}, {P_FXB_TYPE, FX_HALL}, {P_FXB_MIX, 60}, {P_FXB_P1, 220}});
        /* osc 3 at LO, slow, into the filter: a wobble each voice its own */
        p[i++] = make("LO Wobble", {{P_O1_OCT, 1}, {P_O2_LEVEL, 110}, {P_O2_OCT, 1}, {P_O2_PITCH, BEAT}, {P_O3_OCT, 0},
                                    {P_O3_KEY, 0}, {P_O3_PITCH, 240}, {P_O3_SAW, 0}, {P_O3_TRI, 1}, {P_PM_OSC3, 220},
                                    {P_PM_FILT, 1}, {P_CUTOFF, 55}, {P_RESO, 150}, {P_F_ENV_AMT, 150}, {P_F_DEC, 80},
                                    {P_A_REL, 40}, {P_FDBK_DRIVE, 170}});
        /* a pulse into the spring */
        p[i++] = make("Spring Twang", {{P_O1_SAW, 0}, {P_O1_PULSE, 1}, {P_O1_PW, 50}, {P_CUTOFF, 90}, {P_RESO, 50},
                                       {P_F_ENV_AMT, 190}, {P_F_DEC, 50}, {P_A_DEC, 70}, {P_A_SUS, 10}, {P_A_REL, 60},
                                       {P_FXB_TYPE, FX_SPRING}, {P_FXB_MIX, 60}, {P_FXB_P1, 150}, {P_FXB_P2, 60}});
        /* triangles through the ring modulator, tracking the notes */
        p[i++] = make("Ring Clang", {{P_O1_SAW, 0}, {P_O1_TRI, 1}, {P_O2_TRI, 1}, {P_O2_SAW, 0}, {P_O2_LEVEL, 90}, {P_O2_OCT, 3},
                                     {P_O2_PITCH, 200}, {P_CUTOFF, 130}, {P_F_ENV_AMT, 150}, {P_F_DEC, 70}, {P_A_DEC, 85},
                                     {P_A_SUS, 0}, {P_A_REL, 75}, {P_FXA_TYPE, FX_RING}, {P_FXA_MIX, 70}, {P_FXA_P1, 170},
                                     {P_FXA_P2, 100}, {P_FXB_TYPE, FX_PLATE}, {P_FXB_MIX, 35}});
        /* everything driven: DRIVE, DISTORTION */
        p[i++] = make("Driven Saws", {{P_O2_LEVEL, 120}, {P_O2_PITCH, BEAT}, {P_O3_LEVEL, 110}, {P_O3_PITCH, BEAT_DN},
                                      {P_FDBK_DRIVE, 254}, {P_CUTOFF, 80}, {P_RESO, 80}, {P_F_ENV_AMT, 180}, {P_F_DEC, 70},
                                      {P_F_SUS, 50}, {P_A_REL, 35}, {P_DISTORTION, 60}, {P_FXB_TYPE, FX_ROOM},
                                      {P_FXB_MIX, 25}});
        /* three saws, the filter wide open, the flanger: the classic sweep */
        p[i++] = make("Flanged Pad", {{P_O2_LEVEL, 110}, {P_O2_PITCH, BEAT}, {P_O3_LEVEL, 100}, {P_O3_OCT, 4}, {P_CUTOFF, 110},
                                      {P_RESO, 30}, {P_A_ATT, 75}, {P_A_REL, 95}, {P_FXA_TYPE, FX_FLANGER1}, {P_FXA_MIX, 60},
                                      {P_FXA_P1, 25}, {P_FXA_P2, 100}, {P_FXB_TYPE, FX_HALL}, {P_FXB_MIX, 40}, {P_PAN_SPREAD, 110}});
        /* a Moog-style bass: three oscillators an octave apart, the 2-pole ladder */
        p[i++] = make("Three Osc Bass", {{P_O1_OCT, 1}, {P_O2_OCT, 0}, {P_O2_LEVEL, 115}, {P_O3_OCT, 2}, {P_O3_LEVEL, 90},
                                         {P_O3_PITCH, BEAT}, {P_CUTOFF, 35}, {P_RESO, 60}, {P_F_ENV_AMT, 200}, {P_F_DEC, 60},
                                         {P_F_SUS, 25}, {P_A_REL, 25}, {P_FDBK_DRIVE, 80}, {P_UNISON, 1}, {P_UNI_VOICES, 0},
                                         {P_GLIDE, 1}, {P_GLIDE_RATE, 40}, {P_GLIDE_MODE, GM_RATE_LEGATO}});
        /* the OTA filter screaming: resonance high, aftertouch opens it */
        p[i++] = make("OTA Scream", {{P_F_TYPE, FT_OTA}, {P_O1_SAW, 1}, {P_O2_LEVEL, 100}, {P_O2_PITCH, 199}, {P_CUTOFF, 75},
                                     {P_RESO, 215}, {P_F_ENV_AMT, 170}, {P_F_DEC, 80}, {P_F_SUS, 60}, {P_A_REL, 40},
                                     {P_AT_AMT, 230}, {P_AT_FILT, 1}, {P_UNISON, 1}, {P_UNI_VOICES, 3}, {P_UNI_DETUNE, 6},
                                     {P_FXA_TYPE, FX_DDL}, {P_FXA_MIX, 30}, {P_FXA_P1, 140}, {P_FXA_P2, 50}});
        /* the random LFO: sample and hold on the cutoff, clock synced */
        p[i++] = make("Random Steps", {{P_O1_SAW, 1}, {P_O2_LEVEL, 100}, {P_O2_OCT, 3}, {P_CUTOFF, 60}, {P_RESO, 170},
                                       {P_F_ENV_AMT, 140}, {P_LFO_SHAPE, LS_RANDOM}, {P_LFO_SYNC, 1}, {P_LFO_FREQ, 200},
                                       {P_LFO_AMT, 120}, {P_LFO_FILT, 1}, {P_A_REL, 50}, {P_FXA_TYPE, FX_PHASER2},
                                       {P_FXA_MIX, 35}, {P_FXB_TYPE, FX_ROOM}, {P_FXB_MIX, 30}});
        /* a soft triangle flute: noise breath, vibrato on the wheel */
        p[i++] = make("Tri Flute", {{P_O1_SAW, 0}, {P_O1_TRI, 1}, {P_NOISE, 30}, {P_CUTOFF, 90}, {P_F_ENV_AMT, 150},
                                    {P_F_ATT, 30}, {P_F_DEC, 70}, {P_F_SUS, 60}, {P_A_ATT, 25}, {P_A_REL, 50}, {P_LFO_FREQ, 145},
                                    {P_LFO_F1, 1}, {P_FXB_TYPE, FX_HALL}, {P_FXB_MIX, 40}, {P_GLIDE, 1}, {P_GLIDE_RATE, 30},
                                    {P_GLIDE_MODE, GM_TIME_LEGATO}});
        /* where to start */
        p[i++] = make("Basic Program", {});
    }
};
const Bank &bank()
{
    static const Bank b;
    return b;
}
} // namespace

const Program *const FACTORY = bank().p;
} // namespace tg
