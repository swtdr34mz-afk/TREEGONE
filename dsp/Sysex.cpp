/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
#include "Sysex.h"
#include <cstdio>
#include <cstring>

namespace tg
{
namespace
{
int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

/* DSI's packing: eight MIDI bytes carry seven, the first byte their top bits (bit k: byte k's) */
size_t unpack(const unsigned char *in, size_t n, unsigned char *out, size_t cap)
{
    size_t o = 0;
    for (size_t i = 0; i < n && o < cap; i += 8) {
        unsigned char msb = in[i];
        for (size_t j = 1; j < 8 && i + j < n && o < cap; j++)
            out[o++] = (unsigned char)(in[i + j] | (((msb >> (j - 1)) & 1u) << 7));
    }
    return o;
}
} // namespace

void fromTrigon(const unsigned char *b, Program &p)
{
    programInit(p);
    auto set = [&](int prm, int v) { p.p[prm] = (int16_t)clampi(v, PARAMS[prm].lo, PARAMS[prm].hi); };
    auto bit = [&](int i) { return b[i] ? 1 : 0; };
    /* OSCILLATORS */
    set(P_O1_OCT, b[0]);
    set(P_O2_OCT, b[1]);
    set(P_O3_OCT, b[2]);
    set(P_O2_PITCH, b[3]);
    set(P_O3_PITCH, b[4]);
    set(P_O1_SAW, bit(5));
    set(P_O2_SAW, bit(6));
    set(P_O3_SAW, bit(7));
    set(P_O1_TRI, bit(8));
    set(P_O2_TRI, bit(9));
    set(P_O3_TRI, bit(10));
    set(P_O1_PULSE, bit(11));
    set(P_O2_PULSE, bit(12));
    set(P_O3_PULSE, bit(13));
    set(P_O1_PW, b[14]);
    set(P_O2_PW, b[15]);
    set(P_O3_PW, b[16]);
    set(P_O1_LEVEL, b[17]);
    set(P_O2_LEVEL, b[18]);
    set(P_O3_LEVEL, b[19]);
    set(P_SYNC, bit(20));
    set(P_O3_KEY, bit(21));
    set(P_NOISE, b[22]);
    /* GLIDE, the wheel, VINTAGE, FDBK <> DRIVE */
    set(P_GLIDE_RATE, b[23]);
    set(P_GLIDE_MODE, b[24]);
    set(P_GLIDE, bit(25));
    set(P_PB_RANGE, b[26]);
    set(P_VINTAGE, b[27]);
    set(P_FDBK_DRIVE, b[28]);
    /* FILTER, AMPLIFIER (the envelopes' stages filter and amp in turn) */
    set(P_F_TYPE, FT_LADDER);
    set(P_CUTOFF, b[29]);
    set(P_RESO, b[30]);
    set(P_F_KEY, b[31]);
    set(P_F_VEL, bit(32));
    set(P_A_VEL, bit(33));
    set(P_PGM_VOL, b[34]);
    set(P_PAN_SPREAD, b[35]);
    set(P_F_ENV_AMT, b[36]);
    set(P_F_ATT, b[40]);
    set(P_A_ATT, b[41]);
    set(P_F_DEC, b[42]);
    set(P_A_DEC, b[43]);
    set(P_F_SUS, b[44]);
    set(P_A_SUS, b[45]);
    set(P_F_REL, b[46]);
    set(P_A_REL, b[47]);
    set(P_POLE2, bit(48));
    /* EFFECTS (byte 51 is the panel's A/B, which slot the knobs edit) */
    set(P_FXA_TYPE, b[49]);
    set(P_FXB_TYPE, b[50]);
    set(P_FX_ON, b[52] ? 0 : 1);
    set(P_FXA_MIX, b[53]);
    set(P_FXB_MIX, b[54]);
    set(P_FXA_P1, b[55]);
    set(P_FXB_P1, b[56]);
    set(P_FXA_P2, b[57]);
    set(P_FXB_P2, b[58]);
    set(P_FXA_SYNC, bit(59));
    set(P_FXB_SYNC, bit(60));
    set(P_FXA_DIV, b[61]);
    set(P_FXB_DIV, b[62]);
    set(P_DISTORTION, b[63]);
    /* LFO */
    set(P_LFO_FREQ, b[64]);
    set(P_LFO_SYNC, bit(66));
    set(P_LFO_SHAPE, b[67]);
    set(P_LFO_AMT, b[68]);
    for (int i = 0; i < 8; i++)
        set(P_LFO_F1 + i, bit(69 + i));
    /* AFTERTOUCH: FREQ 1-3, FILTER, AMP, LFO, FX MIX A, B */
    set(P_AT_AMT, b[77]);
    for (int i = 0; i < 8; i++)
        set(P_AT_F1 + i, bit(78 + i));
    /* POLY MOD: the filter envelope's amount, osc 3's; its buttons as the panel's rows: FREQ 1-3, FILTER, PW 1-2, FDBK */
    set(P_PM_FENV, b[86]);
    set(P_PM_OSC3, b[87]);
    set(P_PM_F1, bit(88));
    set(P_PM_F2, bit(89));
    set(P_PM_F3, bit(90));
    set(P_PM_FILT, bit(91));
    set(P_PM_PW1, bit(92));
    set(P_PM_PW2, bit(93));
    set(P_PM_FDBK, bit(94));
    /* UNISON, KEY MODE, CLOCK, ARPEGGIATOR, SEQUENCER */
    set(P_UNISON, bit(95));
    set(P_UNI_VOICES, b[96]);
    set(P_KEY_MODE, b[97]);
    set(P_BPM, b[98]);
    set(P_ARP_MODE, b[100]);
    set(P_ARP_OCT, b[101]);
    set(P_ARP_ON, bit(102));
    set(P_DIVIDE, b[103]);
    set(P_SEQ_ON, 0);
    set(P_UNI_DETUNE, b[115]);
    /* the chord memory: the root, then up to five notes over it (127: none) */
    for (int i = 0; i < NCHORD; i++)
        p.chord[i] = b[108 + i] <= 60 ? (int8_t)b[108 + i] : (int8_t)-1;
    /* the name */
    char nm[NAME_LEN] = {};
    for (int i = 0; i < 20; i++) {
        unsigned char c = b[116 + i];
        nm[i] = (char)(c >= 32 && c < 127 ? c : ' ');
    }
    int e = 19;
    while (e >= 0 && nm[e] == ' ')
        nm[e--] = 0;
    std::snprintf(p.name, NAME_LEN, "%s", nm[0] ? nm : "Trigon Program");
    /* the poly sequence: six tracks, each 64 notes then 64 velocities (128 + velocity a note, 128 a rest, 0 no step);
     * its length: the first track's steps */
    seqClear(p.seq);
    int len = 0;
    while (len < PSEQ_STEPS && b[137 + 64 + len] != 0)
        len++;
    p.seq.length = (uint8_t)len;
    for (int t = 0; t < PSEQ_NOTES; t++) {
        const unsigned char *notes = b + 137 + 128 * t, *vels = notes + 64;
        for (int s = 0; s < len; s++) {
            int v = vels[s];
            if (v > 128 && notes[s] < 126) {
                p.seq.note[s][t] = (uint8_t)(notes[s] + 2);
                p.seq.vel[s][t] = (uint8_t)clampi(v - 128, 1, 127);
            }
        }
    }
}

std::vector<SyxProgram> readSyx(const unsigned char *d, size_t n)
{
    std::vector<SyxProgram> out;
    std::vector<unsigned char> buf(1100);
    size_t i = 0;
    while (i < n) {
        if (d[i] != 0xF0) {
            i++;
            continue;
        }
        size_t e = i + 1;
        while (e < n && d[e] != 0xF7)
            e++;
        if (e >= n)
            break;
        /* F0 01 <id> 02 bank prog <data> F7, or F0 01 <id> 03 <data> F7 */
        size_t len = e - i;
        if (len > 6 && d[i + 1] == 0x01 && (d[i + 3] == 0x02 || d[i + 3] == 0x03)) {
            bool numbered = d[i + 3] == 0x02;
            size_t at = i + (numbered ? 6 : 4);
            size_t body = e - at;
            if (body >= 1160) {
                size_t got = unpack(d + at, body, buf.data(), 1024);
                if (got >= 1024) {
                    SyxProgram sp;
                    sp.bank = numbered ? d[i + 4] : -1;
                    sp.number = numbered ? d[i + 5] : -1;
                    fromTrigon(buf.data(), sp.prog);
                    out.push_back(sp);
                }
            }
        }
        i = e + 1;
    }
    return out;
}
} // namespace tg
