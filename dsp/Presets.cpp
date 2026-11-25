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
        /* The effects kept as a good mix keeps them: most programs dry or with one short colour, a room or a hall only
         * where the sound wants air, set low, darkened and not too long */
        /* three saws, the ladder, a brass swell that opens with the hand; a small room */
        p[i++] = make("Trigon Brass", {{P_O1_LEVEL, 112}, {P_O2_LEVEL, 108}, {P_O2_PITCH, BEAT}, {P_O3_LEVEL, 96},
                                       {P_O3_PITCH, BEAT_DN}, {P_CUTOFF, 50}, {P_RESO, 28}, {P_F_ENV_AMT, 206}, {P_F_ATT, 36},
                                       {P_F_DEC, 74}, {P_F_SUS, 58}, {P_F_REL, 60}, {P_A_ATT, 8}, {P_A_SUS, 122}, {P_A_REL, 56},
                                       {P_F_KEY, 1}, {P_VINTAGE, 24}, {P_FDBK_DRIVE, 146}, {P_PAN_SPREAD, 64},
                                       {P_AT_AMT, 175}, {P_AT_FILT, 1}, {P_FXB_TYPE, FX_ROOM}, {P_FXB_MIX, 16},
                                       {P_FXB_P1, 110}, {P_FXB_P2, 50}});
        /* a fat mono lead: unison, legato glide, a little feedback, a dark echo; vibrato on the wheel and the hand */
        p[i++] = make("Creamy Lead", {{P_O2_LEVEL, 115}, {P_O2_PITCH, BEAT}, {P_O3_LEVEL, 100}, {P_O3_OCT, 2},
                                      {P_O1_LEVEL, 120}, {P_CUTOFF, 66}, {P_RESO, 70}, {P_F_ENV_AMT, 172}, {P_F_DEC, 68},
                                      {P_F_SUS, 52}, {P_A_REL, 36}, {P_F_KEY, 1}, {P_UNISON, 1}, {P_UNI_VOICES, 3},
                                      {P_UNI_DETUNE, 4}, {P_GLIDE, 1}, {P_GLIDE_RATE, 52}, {P_GLIDE_MODE, GM_RATE_LEGATO},
                                      {P_FDBK_DRIVE, 98}, {P_LFO_FREQ, 140}, {P_LFO_F1, 1}, {P_LFO_F2, 1}, {P_LFO_F3, 1},
                                      {P_AT_AMT, 196}, {P_AT_LFO, 1}, {P_FXA_TYPE, FX_BBD}, {P_FXA_MIX, 20}, {P_FXA_SYNC, 1},
                                      {P_FXA_DIV, 4}, {P_FXA_P2, 38}, {P_KEY_MODE, KM_LAST}, {P_VINTAGE, 18}});
        /* the ladder's bass: saw and a square under it, driven; dry */
        p[i++] = make("Ladder Bass", {{P_O1_OCT, 1}, {P_O2_OCT, 0}, {P_O2_SAW, 0}, {P_O2_PULSE, 1}, {P_O2_LEVEL, 98},
                                      {P_CUTOFF, 38}, {P_RESO, 92}, {P_F_ENV_AMT, 204}, {P_F_DEC, 50}, {P_F_SUS, 12},
                                      {P_F_REL, 38}, {P_A_DEC, 70}, {P_A_SUS, 100}, {P_A_REL, 26}, {P_F_KEY, 1},
                                      {P_FDBK_DRIVE, 186}, {P_UNISON, 1}, {P_UNI_VOICES, 1}, {P_UNI_DETUNE, 2},
                                      {P_KEY_MODE, KM_LAST_R}, {P_VINTAGE, 12}});
        /* fifths, slow, wide: a chorus, a darkened hall behind */
        p[i++] = make("Fifth Cathedral", {{P_O2_LEVEL, 100}, {P_O2_PITCH, FIFTH}, {P_O3_LEVEL, 86}, {P_O3_OCT, 4},
                                          {P_O3_PITCH, BEAT}, {P_CUTOFF, 70}, {P_RESO, 36}, {P_F_ENV_AMT, 160}, {P_F_ATT, 86},
                                          {P_F_DEC, 94}, {P_F_SUS, 72}, {P_F_REL, 92}, {P_A_ATT, 80}, {P_A_REL, 92},
                                          {P_VINTAGE, 40}, {P_PAN_SPREAD, 104}, {P_FXA_TYPE, FX_CHORUS}, {P_FXA_MIX, 34},
                                          {P_FXA_P1, 30}, {P_FXA_P2, 70}, {P_FXB_TYPE, FX_HALL}, {P_FXB_MIX, 22},
                                          {P_FXB_P1, 135}, {P_FXB_P2, 44}});
        /* the OTA low-pass, pulses swept by the LFO; chorus, a short hall */
        p[i++] = make("OTA Strings", {{P_F_TYPE, FT_OTA}, {P_O1_SAW, 0}, {P_O1_PULSE, 1}, {P_O2_SAW, 0}, {P_O2_PULSE, 1},
                                      {P_O2_LEVEL, 108}, {P_O2_PITCH, BEAT}, {P_O2_PW, 100}, {P_O3_LEVEL, 76}, {P_O3_OCT, 4},
                                      {P_CUTOFF, 96}, {P_RESO, 20}, {P_F_ENV_AMT, 150}, {P_F_ATT, 66}, {P_F_DEC, 88}, {P_F_SUS, 80},
                                      {P_F_REL, 86}, {P_A_ATT, 68}, {P_A_REL, 88}, {P_LFO_FREQ, 100}, {P_LFO_AMT, 140},
                                      {P_LFO_PW1, 1}, {P_LFO_PW2, 1}, {P_PAN_SPREAD, 96}, {P_VINTAGE, 30}, {P_FXA_TYPE, FX_CHORUS},
                                      {P_FXA_MIX, 36}, {P_FXA_P1, 26}, {P_FXA_P2, 64}, {P_FXB_TYPE, FX_HALL}, {P_FXB_MIX, 18},
                                      {P_FXB_P1, 120}, {P_FXB_P2, 42}});
        /* the SVF as a band-pass: a hollow pluck into a dotted echo */
        p[i++] = make("SVF Pluck", {{P_F_TYPE, FT_SVF}, {P_SVF_MODE, 64}, {P_O1_TRI, 1}, {P_O2_LEVEL, 70}, {P_O2_OCT, 3},
                                    {P_CUTOFF, 68}, {P_RESO, 150}, {P_F_ENV_AMT, 214}, {P_F_DEC, 44}, {P_F_REL, 44},
                                    {P_A_DEC, 62}, {P_A_SUS, 0}, {P_A_REL, 56}, {P_F_KEY, 2}, {P_FXA_TYPE, FX_DDL},
                                    {P_FXA_MIX, 26}, {P_FXA_SYNC, 1}, {P_FXA_DIV, 4}, {P_FXA_P2, 52}, {P_PAN_SPREAD, 76}});
        /* hard sync, the filter envelope sweeping osc 2 (POLY MOD) */
        p[i++] = make("Sync Sweeper", {{P_SYNC, 1}, {P_O1_LEVEL, 0}, {P_O2_LEVEL, 120}, {P_O2_PITCH, 210}, {P_PM_FENV, 225},
                                       {P_PM_F2, 1}, {P_CUTOFF, 92}, {P_RESO, 40}, {P_F_ENV_AMT, 168}, {P_F_DEC, 78},
                                       {P_F_SUS, 30}, {P_A_REL, 38}, {P_O3_LEVEL, 68}, {P_O3_OCT, 2}, {P_FXA_TYPE, FX_BBD},
                                       {P_FXA_MIX, 18}, {P_FXA_P1, 120}, {P_FXA_P2, 36}});
        /* osc 3 into osc 1's pitch at audio rate: bells, a plate kept short */
        p[i++] = make("Poly Mod Bells", {{P_O1_SAW, 0}, {P_O1_TRI, 1}, {P_O3_SAW, 0}, {P_O3_TRI, 1}, {P_O3_OCT, 5},
                                         {P_O3_PITCH, 230}, {P_PM_OSC3, 205}, {P_PM_F1, 1}, {P_CUTOFF, 116},
                                         {P_F_ENV_AMT, 160}, {P_F_DEC, 74}, {P_A_DEC, 86}, {P_A_SUS, 0}, {P_A_REL, 82},
                                         {P_FXB_TYPE, FX_PLATE}, {P_FXB_MIX, 20}, {P_FXB_P1, 110}, {P_FXB_P2, 62},
                                         {P_PAN_SPREAD, 88}});
        /* the feedback loop: a growl, the 2-pole ladder half band-pass; dry */
        p[i++] = make("Feedback Growl", {{P_O1_OCT, 1}, {P_O2_LEVEL, 100}, {P_O2_OCT, 1}, {P_O2_PITCH, BEAT}, {P_FDBK_DRIVE, 20},
                                         {P_POLE2, 1}, {P_CUTOFF, 54}, {P_RESO, 160}, {P_F_ENV_AMT, 184}, {P_F_DEC, 64},
                                         {P_F_SUS, 40}, {P_A_REL, 28}, {P_UNISON, 1}, {P_UNI_VOICES, 2}, {P_UNI_DETUNE, 5},
                                         {P_DISTORTION, 22}});
        /* an old keyboard: soft waves, a lot of VINTAGE, the deep phaser, a small room */
        p[i++] = make("Vintage Keys", {{P_O1_SAW, 0}, {P_O1_TRI, 1}, {P_O1_PULSE, 1}, {P_O1_PW, 40}, {P_O2_SAW, 0}, {P_O2_TRI, 1},
                                       {P_O2_LEVEL, 80}, {P_O2_OCT, 3}, {P_CUTOFF, 82}, {P_F_ENV_AMT, 170}, {P_F_DEC, 78},
                                       {P_F_SUS, 20}, {P_A_DEC, 94}, {P_A_SUS, 30}, {P_A_REL, 52}, {P_VINTAGE, 90},
                                       {P_FXA_TYPE, FX_PHASER3}, {P_FXA_MIX, 42}, {P_FXA_P1, 34}, {P_FXA_P2, 80},
                                       {P_FXB_TYPE, FX_ROOM}, {P_FXB_MIX, 14}, {P_FXB_P1, 100}, {P_FXB_P2, 52}});
        /* the arpeggiator over two octaves, a synced bucket-brigade echo */
        p[i++] = make("Arp Machine", {{P_ARP_ON, 1}, {P_ARP_OCT, 1}, {P_ARP_MODE, AM_UPDOWN}, {P_DIVIDE, DV_16}, {P_O1_SAW, 0},
                                      {P_O1_PULSE, 1}, {P_O1_PW, 70}, {P_O2_LEVEL, 90}, {P_O2_PITCH, BEAT}, {P_CUTOFF, 58},
                                      {P_RESO, 110}, {P_F_ENV_AMT, 200}, {P_F_DEC, 40}, {P_A_DEC, 50}, {P_A_SUS, 40},
                                      {P_A_REL, 38}, {P_FXA_TYPE, FX_BBD}, {P_FXA_MIX, 30}, {P_FXA_SYNC, 1}, {P_FXA_DIV, 4},
                                      {P_FXA_P2, 56}, {P_PAN_SPREAD, 86}});
        /* a sequence (PLAY), the low-pass opening with the LFO */
        p[i++] = make("Seq Pulse", {{P_O1_OCT, 1}, {P_O2_LEVEL, 100}, {P_O2_PULSE, 1}, {P_O2_SAW, 0}, {P_CUTOFF, 44},
                                    {P_RESO, 140}, {P_F_ENV_AMT, 194}, {P_F_DEC, 42}, {P_A_DEC, 55}, {P_A_SUS, 30},
                                    {P_A_REL, 30}, {P_LFO_FREQ, 60}, {P_LFO_AMT, 60}, {P_LFO_FILT, 1}, {P_FXA_TYPE, FX_DDL},
                                    {P_FXA_MIX, 22}, {P_FXA_SYNC, 1}, {P_FXA_DIV, 4}, {P_FXA_P2, 50}});
        seq(p[i - 1], {48, 48, 60, 48, 51, 48, 58, 48, 48, 55, 48, 60, 63, 48, 58, 55});
        /* UNISON with a chord kept: one key, a minor ninth's voicing */
        p[i] = make("Chord Memory", {{P_UNISON, 1}, {P_UNI_VOICES, 6}, {P_O2_LEVEL, 90}, {P_O2_PITCH, BEAT}, {P_CUTOFF, 60},
                                     {P_RESO, 60}, {P_F_ENV_AMT, 198}, {P_F_DEC, 58}, {P_F_SUS, 20}, {P_A_DEC, 70}, {P_A_SUS, 60},
                                     {P_A_REL, 48}, {P_FXA_TYPE, FX_CHORUS}, {P_FXA_MIX, 30}, {P_FXA_P1, 30}, {P_FXA_P2, 60},
                                     {P_FXB_TYPE, FX_ROOM}, {P_FXB_MIX, 12}, {P_FXB_P1, 90}, {P_FXB_P2, 50}});
        chord(p[i++], {0, 3, 7, 10, 14});
        /* the SVF high-pass: air and noise, a gentle flanger, a darkened hall */
        p[i++] = make("High Air", {{P_F_TYPE, FT_SVF}, {P_SVF_MODE, 127}, {P_O2_LEVEL, 100}, {P_O2_PITCH, BEAT}, {P_NOISE, 40},
                                   {P_CUTOFF, 82}, {P_RESO, 110}, {P_F_ENV_AMT, 100}, {P_F_ATT, 88}, {P_F_DEC, 98},
                                   {P_F_SUS, 60}, {P_A_ATT, 78}, {P_A_REL, 96}, {P_LFO_FREQ, 70}, {P_LFO_AMT, 70},
                                   {P_LFO_FILT, 1}, {P_FXA_TYPE, FX_FLANGER2}, {P_FXA_MIX, 28}, {P_FXA_P1, 18},
                                   {P_FXA_P2, 80}, {P_FXB_TYPE, FX_HALL}, {P_FXB_MIX, 24}, {P_FXB_P1, 150}, {P_FXB_P2, 48},
                                   {P_PAN_SPREAD, 100}});
        /* osc 3 at LO, slow, into the filter: a wobble each voice its own; dry */
        p[i++] = make("LO Wobble", {{P_O1_OCT, 1}, {P_O2_LEVEL, 110}, {P_O2_OCT, 1}, {P_O2_PITCH, BEAT}, {P_O3_OCT, 0},
                                    {P_O3_KEY, 0}, {P_O3_PITCH, 240}, {P_O3_SAW, 0}, {P_O3_TRI, 1}, {P_PM_OSC3, 220},
                                    {P_PM_FILT, 1}, {P_CUTOFF, 54}, {P_RESO, 150}, {P_F_ENV_AMT, 150}, {P_F_DEC, 80},
                                    {P_A_REL, 38}, {P_FDBK_DRIVE, 168}});
        /* a pulse into the spring tank (the effect is the sound here) */
        p[i++] = make("Spring Twang", {{P_O1_SAW, 0}, {P_O1_PULSE, 1}, {P_O1_PW, 50}, {P_CUTOFF, 88}, {P_RESO, 50},
                                       {P_F_ENV_AMT, 188}, {P_F_DEC, 50}, {P_A_DEC, 70}, {P_A_SUS, 10}, {P_A_REL, 56},
                                       {P_FXB_TYPE, FX_SPRING}, {P_FXB_MIX, 34}, {P_FXB_P1, 110}, {P_FXB_P2, 56}});
        /* triangles through the ring modulator, tracking the notes; a short plate */
        p[i++] = make("Ring Clang", {{P_O1_SAW, 0}, {P_O1_TRI, 1}, {P_O2_TRI, 1}, {P_O2_SAW, 0}, {P_O2_LEVEL, 90}, {P_O2_OCT, 3},
                                     {P_O2_PITCH, 200}, {P_CUTOFF, 126}, {P_F_ENV_AMT, 150}, {P_F_DEC, 70}, {P_A_DEC, 84},
                                     {P_A_SUS, 0}, {P_A_REL, 72}, {P_FXA_TYPE, FX_RING}, {P_FXA_MIX, 56}, {P_FXA_P1, 170},
                                     {P_FXA_P2, 100}, {P_FXB_TYPE, FX_PLATE}, {P_FXB_MIX, 16}, {P_FXB_P1, 90}, {P_FXB_P2, 60}});
        /* everything driven: DRIVE, DISTORTION; dry */
        p[i++] = make("Driven Saws", {{P_O2_LEVEL, 120}, {P_O2_PITCH, BEAT}, {P_O3_LEVEL, 108}, {P_O3_PITCH, BEAT_DN},
                                      {P_FDBK_DRIVE, 254}, {P_CUTOFF, 78}, {P_RESO, 80}, {P_F_ENV_AMT, 178}, {P_F_DEC, 68},
                                      {P_F_SUS, 50}, {P_A_REL, 32}, {P_DISTORTION, 55}, {P_PAN_SPREAD, 60}});
        /* three saws wide open, the through-zero flanger: the classic sweep, a little hall */
        p[i++] = make("Flanged Pad", {{P_O2_LEVEL, 108}, {P_O2_PITCH, BEAT}, {P_O3_LEVEL, 96}, {P_O3_OCT, 4}, {P_CUTOFF, 104},
                                      {P_RESO, 30}, {P_F_ENV_AMT, 140}, {P_F_ATT, 70}, {P_F_DEC, 90}, {P_F_SUS, 70},
                                      {P_A_ATT, 72}, {P_A_REL, 90}, {P_FXA_TYPE, FX_FLANGER1}, {P_FXA_MIX, 44},
                                      {P_FXA_P1, 22}, {P_FXA_P2, 92}, {P_FXB_TYPE, FX_HALL}, {P_FXB_MIX, 16}, {P_FXB_P1, 120},
                                      {P_FXB_P2, 46}, {P_PAN_SPREAD, 104}, {P_VINTAGE, 28}});
        /* a Moog-style bass: three oscillators an octave apart, legato glide; dry */
        p[i++] = make("Three Osc Bass", {{P_O1_OCT, 1}, {P_O2_OCT, 0}, {P_O2_LEVEL, 115}, {P_O3_OCT, 2}, {P_O3_LEVEL, 88},
                                         {P_O3_PITCH, BEAT}, {P_CUTOFF, 34}, {P_RESO, 60}, {P_F_ENV_AMT, 200}, {P_F_DEC, 58},
                                         {P_F_SUS, 25}, {P_A_REL, 24}, {P_FDBK_DRIVE, 80}, {P_UNISON, 1}, {P_UNI_VOICES, 0},
                                         {P_GLIDE, 1}, {P_GLIDE_RATE, 40}, {P_GLIDE_MODE, GM_RATE_LEGATO}});
        /* the OTA filter screaming: resonance high, aftertouch opens it; a clean echo */
        p[i++] = make("OTA Scream", {{P_F_TYPE, FT_OTA}, {P_O1_SAW, 1}, {P_O2_LEVEL, 100}, {P_O2_PITCH, 199}, {P_CUTOFF, 72},
                                     {P_RESO, 212}, {P_F_ENV_AMT, 170}, {P_F_DEC, 78}, {P_F_SUS, 60}, {P_A_REL, 38},
                                     {P_AT_AMT, 228}, {P_AT_FILT, 1}, {P_UNISON, 1}, {P_UNI_VOICES, 3}, {P_UNI_DETUNE, 6},
                                     {P_FXA_TYPE, FX_DDL}, {P_FXA_MIX, 20}, {P_FXA_SYNC, 1}, {P_FXA_DIV, 4}, {P_FXA_P2, 42}});
        /* the random LFO: sample and hold on the cutoff, clock synced, a phaser */
        p[i++] = make("Random Steps", {{P_O1_SAW, 1}, {P_O2_LEVEL, 100}, {P_O2_OCT, 3}, {P_CUTOFF, 58}, {P_RESO, 166},
                                       {P_F_ENV_AMT, 140}, {P_LFO_SHAPE, LS_RANDOM}, {P_LFO_SYNC, 1}, {P_LFO_FREQ, 200},
                                       {P_LFO_AMT, 110}, {P_LFO_FILT, 1}, {P_A_REL, 46}, {P_FXA_TYPE, FX_PHASER2},
                                       {P_FXA_MIX, 28}, {P_FXA_P1, 40}, {P_FXA_P2, 70}});
        /* a soft triangle flute: breath noise, vibrato on the wheel, a small room */
        p[i++] = make("Tri Flute", {{P_O1_SAW, 0}, {P_O1_TRI, 1}, {P_NOISE, 26}, {P_CUTOFF, 88}, {P_F_ENV_AMT, 150},
                                    {P_F_ATT, 30}, {P_F_DEC, 70}, {P_F_SUS, 60}, {P_A_ATT, 24}, {P_A_REL, 48}, {P_LFO_FREQ, 145},
                                    {P_LFO_F1, 1}, {P_FXB_TYPE, FX_ROOM}, {P_FXB_MIX, 16}, {P_FXB_P1, 120}, {P_FXB_P2, 56},
                                    {P_GLIDE, 1}, {P_GLIDE_RATE, 30}, {P_GLIDE_MODE, GM_TIME_LEGATO}});
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
