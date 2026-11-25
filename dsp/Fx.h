/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
/* One of the Trigon's two effects (A, then B in series), each with DRY<>WET and two parameters:
 *   DDL DELAY (clean, full band, a little ping-pong), BBD DELAY (dark and darker with time, saturating, its clock's
 *     wobble): time, feedback; SYNC: the time a note of the clock (SYNC TIME)
 *   CHORUS, PHASER 1 (four stages, gentle), PHASER 2 (six, with feedback), PHASER 3 (after Oberheim's: twelve stages,
 *     deep), FLANGER 1 (through zero, high feedback), FLANGER 2 (classic): rate, depth
 *   RING MOD (after Oberheim's): frequency; tracking (over 64 the carrier follows the lowest note held)
 *   ROOM, HALL, PLATE, SPRING (B only): decay, tone
 * Everything glides (no zipper, no click), a new type crossfades in. */
#pragma once
#include "Dsp.h"
#include "Params.h"
#include <algorithm>
#include <vector>

namespace tg
{
class Fx
{
public:
    void prepare(float sampleRate)
    {
        sr = sampleRate;
        dlN = 1;
        while (dlN < (int)(sr * 1.6f) + 8)
            dlN <<= 1;
        dl[0].assign((size_t)dlN, 0.0f);
        dl[1].assign((size_t)dlN, 0.0f);
        mdN = 1;
        while (mdN < (int)(sr * 0.05f))
            mdN <<= 1;
        md[0].assign((size_t)mdN, 0.0f);
        md[1].assign((size_t)mdN, 0.0f);
        for (int i = 0; i < 8; i++)
            rvLine[i].assign((size_t)((int)(RV_LEN[i] * sr / 44100.0f * 2.4f) + 8), 0.0f);
        for (int i = 0; i < 6; i++)
            rvAp[i].assign((size_t)((int)(RV_AP[i] * sr / 44100.0f * 1.5f) + 4), 0.0f);
        preN = 1;
        while (preN < (int)(sr * 0.08f))
            preN <<= 1;
        pre.assign((size_t)preN, 0.0f);
        for (int c = 0; c < 2; c++)
            spLine[c].assign((size_t)((int)(sr * 0.09f) + 8), 0.0f);
        clear();
    }
    void clear()
    {
        for (auto &b : dl)
            std::fill(b.begin(), b.end(), 0.0f);
        for (auto &b : md)
            std::fill(b.begin(), b.end(), 0.0f);
        for (auto &b : rvLine)
            std::fill(b.begin(), b.end(), 0.0f);
        for (auto &b : rvAp)
            std::fill(b.begin(), b.end(), 0.0f);
        for (auto &b : spLine)
            std::fill(b.begin(), b.end(), 0.0f);
        std::fill(pre.begin(), pre.end(), 0.0f);
        std::memset(rvW, 0, sizeof rvW);
        std::memset(rvApW, 0, sizeof rvApW);
        std::memset(rvLp, 0, sizeof rvLp);
        std::memset(ph12, 0, sizeof ph12);
        std::memset(bbdLp, 0, sizeof bbdLp);
        std::memset(fbS, 0, sizeof fbS);
        std::memset(spAp, 0, sizeof spAp);
        std::memset(spW, 0, sizeof spW);
        std::memset(spLp, 0, sizeof spLp);
        std::memset(spHp, 0, sizeof spHp);
        dlW = mdW = preW = 0;
        tail = 0.0f;
    }
    /* each chunk: the type, mix 0..127, p1 0..255, p2 0..127, synced (delays) with its division, the tempo, the
     * lowest note's pitch (Hz: RING MOD's tracking; 0 none) */
    void set(int type_, float mix_, float p1_, float p2_, bool synced_, int div_, float bpm_, float trackHz_)
    {
        wantType = type_;
        tMix = clampf(mix_, 0.0f, 127.0f) / 127.0f;
        tP1 = clampf(p1_, 0.0f, 255.0f);
        tP2 = clampf(p2_, 0.0f, 127.0f);
        synced = synced_;
        div = div_ < 0 ? 0 : div_ >= NFXDIV ? NFXDIV - 1 : div_;
        bpm = bpm_ < 20.0f ? 120.0f : bpm_;
        trackHz = trackHz_;
        if (!init) {
            type = wantType;
            sMix = tMix;
            sP1 = tP1;
            sP2 = tP2;
            xf = type != FX_OFF ? 1.0f : 0.0f;
            init = true;
        }
    }
    bool active() const { return type != FX_OFF || xf > 0.0f || wantType != FX_OFF; }

    void process(float *L, float *R, int n)
    {
        const float kp = 1.0f - std::exp(-1.0f / (0.02f * sr));
        const float kx = 1.0f / (0.012f * sr);
        for (int i = 0; i < n; i++) {
            if (wantType != type && xf > 0.0f) {
                xf -= kx;
                if (xf <= 0.0f) {
                    xf = 0.0f;
                    type = wantType;
                    clearState();
                }
            } else if (wantType == type && type != FX_OFF && xf < 1.0f)
                xf = minf(1.0f, xf + kx);
            if (wantType != type && xf <= 0.0f) {
                type = wantType;
                clearState();
            }
            sMix += (tMix - sMix) * kp;
            sP1 += (tP1 - sP1) * kp;
            sP2 += (tP2 - sP2) * kp;
            if (type == FX_OFF || xf <= 0.0f)
                continue;
            float x0 = L[i], x1 = R[i], wl = x0, wr = x1;
            run(x0, x1, wl, wr);
            /* DRY<>WET: the echoes and rooms equal power (they are not the dry), the rest a straight crossfade (they
             * are mostly the dry again: no boost in the middle) */
            float m = sMix * xf, gd, gw;
            if (type == FX_DDL || type == FX_BBD || type >= FX_ROOM) {
                gd = std::cos(m * 1.5708f);
                gw = std::sin(m * 1.5708f);
            } else {
                gd = 1.0f - m;
                gw = m;
            }
            L[i] = x0 * gd + wl * gw;
            R[i] = x1 * gd + wr * gw;
        }
        for (int c = 0; c < 2; c++) {
            bbdLp[c] = flushDn(bbdLp[c]);
            fbS[c] = flushDn(fbS[c]);
            spLp[c] = flushDn(spLp[c]);
            spHp[c] = flushDn(spHp[c]);
            for (auto &v : ph12[c])
                v = flushDn(v);
        }
        for (float &v : rvLp)
            v = flushDn(v);
    }

private:
    static constexpr float RV_LEN[8] = {1557, 1617, 1491, 1422, 1277, 1356, 1188, 1116};
    static constexpr float RV_AP[6] = {142, 107, 379, 277, 211, 163};
    static constexpr float DIV_BEATS[NFXDIV] = {4.0f, 2.0f, 1.5f, 1.0f, 0.75f, 2.0f / 3.0f, 0.5f, 1.0f / 3.0f, 0.25f};

    float sr = 44100.0f;
    bool init = false, synced = false;
    int wantType = FX_OFF, type = FX_OFF, div = 3;
    float tMix = 0, tP1 = 0, tP2 = 0, sMix = 0, sP1 = 0, sP2 = 0, xf = 0, bpm = 120, trackHz = 0, tail = 0;
    float dlTime = 0.3f;

    std::vector<float> dl[2], md[2], rvLine[8], rvAp[6], pre, spLine[2];
    int dlN = 1, dlW = 0, mdN = 1, mdW = 0, preN = 1, preW = 0;
    int rvW[8] = {}, rvApW[6] = {}, spW[2] = {};
    float rvLp[8] = {}, rvMph[8] = {};
    float lfoPh = 0.0f, ph12[2][12] = {}, fbS[2] = {}, bbdLp[2] = {}, bbdWob = 0.0f, ringPh = 0.0f;
    float spAp[2][16] = {}, spLp[2] = {}, spHp[2] = {};
    uint32_t seed = 0x13579BDFu;

    void clearState()
    {
        float keep = tail;
        clear();
        tail = keep;
    }
    inline float rd(const std::vector<float> &b, int N, int w, float d) const
    {
        float p = (float)w - d;
        while (p < 0.0f)
            p += (float)N;
        int i0 = (int)p;
        float f = p - (float)i0;
        float a = b[(size_t)(i0 & (N - 1))], c = b[(size_t)((i0 + 1) & (N - 1))];
        return a + (c - a) * f;
    }
    float delaySeconds(float maxT)
    {
        if (synced) {
            float t = DIV_BEATS[div] * 60.0f / bpm;
            while (t > maxT)
                t *= 0.5f;
            return t;
        }
        return 0.004f + std::pow(sP1 / 255.0f, 1.3f) * (maxT - 0.004f);
    }
    /* the reverbs: eight lines with a Householder mix, damped (TONE), gently moving; size and diffusion per room */
    void reverb(float x0, float x1, float &yl, float &yr, float size, float tMin, float tMax, float diff, float preS, float bright)
    {
        const float isr = 1.0f / sr;
        float t60 = tMin + std::pow(sP1 / 255.0f, 1.7f) * (tMax - tMin);
        float damp = (900.0f + bright) * std::exp2(sP2 / 127.0f * 3.8f);
        float kd = 1.0f - std::exp(-2.0f * PI * minf(damp, 0.45f * sr) * isr);
        float in = 0.3f * (x0 + x1);
        /* the pre-delay */
        pre[(size_t)preW] = in;
        in = rd(pre, preN, preW, maxf(1.0f, preS * sr));
        preW = (preW + 1) & (preN - 1);
        for (int i = 0; i < 6; i++) {                        /* input diffusion */
            auto &b = rvAp[i];
            int N = (int)b.size(), w = rvApW[i];
            float z = b[(size_t)w], y = -diff * in + z;
            b[(size_t)w] = in + diff * y;
            rvApW[i] = (w + 1) % N;
            in = y;
        }
        float o[8], sum = 0.0f;
        for (int i = 0; i < 8; i++) {
            auto &b = rvLine[i];
            int N = (int)b.size();
            rvMph[i] += (0.25f + 0.06f * (float)i) * isr;
            if (rvMph[i] >= 1.0f)
                rvMph[i] -= 1.0f;
            float len = RV_LEN[i] * sr / 44100.0f * size;
            float d = minf(len + 5.0f * std::sin(2.0f * PI * rvMph[i]), (float)N - 3.0f);
            float p = (float)rvW[i] - d;
            while (p < 0.0f)
                p += (float)N;
            int i0 = (int)p;
            float f = p - (float)i0, a = b[(size_t)(i0 % N)], c = b[(size_t)((i0 + 1) % N)];
            float y = a + (c - a) * f;
            float g = std::pow(10.0f, -3.0f * (d / sr) / t60);
            rvLp[i] += (y - rvLp[i]) * kd;
            o[i] = rvLp[i] * g;
            sum += o[i];
        }
        sum *= 0.25f;
        for (int i = 0; i < 8; i++) {
            auto &b = rvLine[i];
            int N = (int)b.size();
            float v = o[i] - sum + ((i & 1) ? in : in * 0.9f);
            b[(size_t)rvW[i]] = v > 3.0f ? 3.0f : v < -3.0f ? -3.0f : v;
            rvW[i] = (rvW[i] + 1) % N;
        }
        yl = 0.62f * (o[0] + o[2] + o[4] + o[6]);
        yr = 0.62f * (o[1] + o[3] + o[5] + o[7]);
    }

    void run(float x0, float x1, float &yl, float &yr)
    {
        const float isr = 1.0f / sr;
        switch (type) {
        case FX_DDL:
        case FX_BBD: {
            bool bbd = type == FX_BBD;
            float t = delaySeconds(bbd ? 0.8f : 1.5f);
            float kt = bbd ? 0.00012f : 0.00035f;          /* (the time glides: a tape's bend, no click) */
            kt = 1.0f - std::pow(1.0f - kt, 44100.0f * isr);
            dlTime += (t - dlTime) * kt;
            float fb = minf(sP2 / 127.0f * 1.02f, 0.985f);
            float d = dlTime * sr;
            if (bbd) {
                bbdWob += 2.0f * PI * 0.55f * isr;
                if (bbdWob > 2.0f * PI)
                    bbdWob -= 2.0f * PI;
                d *= 1.0f + 0.003f * std::sin(bbdWob);
            }
            d = clampf(d, 2.0f, (float)dlN - 4.0f);
            if (!bbd) {                                    /* clean; a little of each side's echo crosses over */
                float a = rd(dl[0], dlN, dlW, d), b = rd(dl[1], dlN, dlW, d);
                dl[0][(size_t)dlW] = x0 + softT(fb * (0.75f * a + 0.25f * b));
                dl[1][(size_t)dlW] = x1 + softT(fb * (0.75f * b + 0.25f * a));
                yl = a;
                yr = b;
            } else {
                float fc = clampf(8000.0f - dlTime * 7000.0f, 1500.0f, 8000.0f);
                float k = 1.0f - std::exp(-2.0f * PI * fc * isr);
                for (int c = 0; c < 2; c++) {
                    float y = rd(dl[c], dlN, dlW, d * (c ? 1.013f : 1.0f));
                    bbdLp[c] += (y - bbdLp[c]) * k;
                    float in = c ? x1 : x0;
                    dl[c][(size_t)dlW] = softT((in + bbdLp[c] * fb) * 0.9f) * 1.05f + 2e-6f * rndF(seed);
                    (c ? yr : yl) = bbdLp[c];
                }
            }
            dlW = (dlW + 1) & (dlN - 1);
            break;
        }
        case FX_CHORUS:
        case FX_FLANGER1:
        case FX_FLANGER2: {
            float rate = type == FX_CHORUS ? 0.05f + std::pow(sP1 / 255.0f, 2.0f) * 8.0f : 0.02f + std::pow(sP1 / 255.0f, 2.0f) * 6.0f;
            float depth = sP2 / 127.0f;
            lfoPh += rate * isr;
            if (lfoPh >= 1.0f)
                lfoPh -= 1.0f;
            float ml = 0.5f + 0.5f * std::sin(2.0f * PI * lfoPh), mr = 0.5f + 0.5f * std::sin(2.0f * PI * (lfoPh + 0.25f));
            md[0][(size_t)mdW] = x0 + (type != FX_CHORUS ? fbS[0] : 0.0f);
            md[1][(size_t)mdW] = x1 + (type != FX_CHORUS ? fbS[1] : 0.0f);
            if (type == FX_CHORUS) {                       /* two voices a side, the far one inverted (wide) */
                float base = 0.0075f * sr, sw = (0.0006f + 0.0065f * depth) * sr;
                yl = 0.7f * rd(md[0], mdN, mdW, base + sw * ml) + 0.3f * rd(md[1], mdN, mdW, base * 1.3f + sw * (1.0f - mr));
                yr = 0.7f * rd(md[1], mdN, mdW, base + sw * mr) + 0.3f * rd(md[0], mdN, mdW, base * 1.3f + sw * (1.0f - ml));
            } else {
                bool tz = type == FX_FLANGER1;
                float fb = tz ? 0.86f : 0.6f;
                float base = (tz ? 0.0002f : 0.0006f) * sr, sw = (0.0002f + 0.0065f * depth) * sr;
                float a = rd(md[0], mdN, mdW, base + sw * ml), b = rd(md[1], mdN, mdW, base + sw * mr);
                fbS[0] = softT(a * fb);
                fbS[1] = softT(b * fb);
                if (tz) {
                    a = 0.6f * (a - rd(md[0], mdN, mdW, base + sw * 0.5f));
                    b = 0.6f * (b - rd(md[1], mdN, mdW, base + sw * 0.5f));
                } else {
                    a *= 0.8f;
                    b *= 0.8f;
                }
                yl = a;
                yr = b;
            }
            mdW = (mdW + 1) & (mdN - 1);
            break;
        }
        case FX_PHASER1:
        case FX_PHASER2:
        case FX_PHASER3: {
            int stages = type == FX_PHASER1 ? 4 : type == FX_PHASER2 ? 6 : 12;
            float fb = type == FX_PHASER1 ? 0.2f : type == FX_PHASER2 ? 0.62f : 0.45f;
            float rate = 0.03f + std::pow(sP1 / 255.0f, 2.0f) * 7.0f, depth = sP2 / 127.0f;
            lfoPh += rate * isr;
            if (lfoPh >= 1.0f)
                lfoPh -= 1.0f;
            for (int c = 0; c < 2; c++) {
                float ph = lfoPh + (c ? (type == FX_PHASER3 ? 0.5f : 0.25f) : 0.0f);
                float m = 0.5f + 0.5f * std::sin(2.0f * PI * ph);
                float lo = type == FX_PHASER3 ? 90.0f : 160.0f;
                float fc = lo * std::exp2(depth * 5.0f * m + (1.0f - depth) * 2.2f);
                float t = tanW(clampf(PI * fc * isr, 0.0f, 1.2f)), a = (t - 1.0f) / (t + 1.0f);
                float x = (c ? x1 : x0) + fbS[c] * fb;
                for (int k = 0; k < stages; k++) {
                    float y = a * x + ph12[c][k];
                    ph12[c][k] = x - a * y;
                    x = y;
                }
                fbS[c] = softT(x);
                (c ? yr : yl) = x;
            }
            yl = 0.5f * (yl + x0);                          /* (the notches come with the dry: half and half) */
            yr = 0.5f * (yr + x1);
            break;
        }
        case FX_RING: {
            float f;
            if (sP2 >= 64.0f && trackHz > 0.0f)
                f = trackHz * std::exp2((sP1 - 128.0f) / 32.0f);
            else
                f = 20.0f * std::exp2(sP1 / 255.0f * 8.5f);
            ringPh += f * isr;
            if (ringPh >= 1.0f)
                ringPh -= ringPh >= 2.0f ? 2.0f : 1.0f;
            /* (Oberheim's: a sine with a touch of its own square: a little rougher) */
            float s = std::sin(2.0f * PI * ringPh), c = 0.88f * s + 0.12f * (s > 0.0f ? 1.0f : -1.0f);
            yl = x0 * c;
            yr = x1 * c;
            break;
        }
        case FX_ROOM: reverb(x0, x1, yl, yr, 0.55f, 0.2f, 3.0f, 0.62f, 0.004f, 1500.0f); break;
        case FX_HALL: reverb(x0, x1, yl, yr, 1.45f, 0.6f, 14.0f, 0.6f, 0.025f, 600.0f); break;
        case FX_PLATE: reverb(x0, x1, yl, yr, 0.9f, 0.4f, 8.0f, 0.72f, 0.001f, 3000.0f); break;
        case FX_SPRING: {
            /* a spring: a chain of allpasses (the chirp: high frequencies arrive first), two short lines feeding back
             * (the tank's echoes), band-limited as a tank is */
            float t60 = 0.6f + std::pow(sP1 / 255.0f, 1.5f) * 5.0f;
            float lpf = 2500.0f * std::exp2(sP2 / 127.0f * 1.6f);
            float kl = 1.0f - std::exp(-2.0f * PI * lpf * isr), kh = 1.0f - std::exp(-2.0f * PI * 180.0f * isr);
            for (int c = 0; c < 2; c++) {
                auto &b = spLine[c];
                int N = (int)b.size();
                float len = (c ? 0.0371f : 0.0329f) * sr;
                float y = rd2(b, N, spW[c], len);
                float g = std::pow(10.0f, -3.0f * (len / sr) / t60);
                float in = 0.5f * (x0 + x1) + y * g;
                for (int k = 0; k < 16; k++) {             /* the chirp */
                    float a = 0.62f;
                    float z = spAp[c][k], o = -a * in + z;
                    spAp[c][k] = in + a * o;
                    in = o;
                }
                spLp[c] += (in - spLp[c]) * kl;
                spHp[c] += (spLp[c] - spHp[c]) * kh;
                float out = spLp[c] - spHp[c];
                b[(size_t)spW[c]] = clampf(out, -3.0f, 3.0f);
                spW[c] = (spW[c] + 1) % N;
                (c ? yr : yl) = out * 1.4f;
            }
            break;
        }
        default: yl = x0; yr = x1;
        }
    }
    inline float rd2(const std::vector<float> &b, int N, int w, float d) const
    {
        float p = (float)w - d;
        while (p < 0.0f)
            p += (float)N;
        int i0 = (int)p;
        float f = p - (float)i0;
        return b[(size_t)(i0 % N)] * (1.0f - f) + b[(size_t)((i0 + 1) % N)] * f;
    }
};
} // namespace tg
