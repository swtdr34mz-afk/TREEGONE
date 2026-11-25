/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
/* TREEGONE's engine tests, without JUCE: every factory program at three rates (finite, heard, not too loud), every
 * filter type with its resonance up (it sings), every effect, unison and the chord, the arp, the sequencer, 12
 * voices, any buffer size, the SysEx reader (a program packed as the Trigon packs it, read back). Optional: a .syx
 * file's programs, each played (engine_tests file.syx), and WAVs of the factory programs (engine_tests --wav dir). */
#include "Engine.h"
#include "Presets.h"
#include "Sysex.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

using namespace tg;

static int fails = 0;
#define CHECK(c, ...)                                                                                                  \
    do {                                                                                                               \
        if (!(c)) {                                                                                                    \
            std::printf("FAIL %s:%d: ", __FILE__, __LINE__);                                                           \
            std::printf(__VA_ARGS__);                                                                                  \
            std::printf("\n");                                                                                         \
            fails++;                                                                                                   \
        }                                                                                                              \
    } while (0)

struct Stats {
    float peak = 0.0f, rms = 0.0f;
    bool finite = true;
};

static Stats play(Engine &e, double sr, const std::vector<int> &notes, float secs, float hold, int block = 256,
                  std::vector<float> *keep = nullptr)
{
    Stats st;
    int n = (int)(secs * sr), off = (int)(hold * sr), done = 0;
    std::vector<float> L((size_t)block), R((size_t)block);
    double acc = 0.0;
    bool released = false;
    for (int k : notes)
        e.noteOn(k, 0.8f);
    while (done < n) {
        int b = std::min(block, n - done);
        if (!released && done >= off) {
            for (int k : notes)
                e.noteOff(k);
            released = true;
        }
        e.render(L.data(), R.data(), b);
        for (int i = 0; i < b; i++) {
            if (!std::isfinite(L[(size_t)i]) || !std::isfinite(R[(size_t)i]))
                st.finite = false;
            st.peak = std::max(st.peak, std::max(std::fabs(L[(size_t)i]), std::fabs(R[(size_t)i])));
            acc += (double)L[(size_t)i] * L[(size_t)i] + (double)R[(size_t)i] * R[(size_t)i];
            if (keep) {
                keep->push_back(L[(size_t)i]);
                keep->push_back(R[(size_t)i]);
            }
        }
        done += b;
    }
    st.rms = (float)std::sqrt(acc / (2.0 * n));
    return st;
}

static void wav(const std::string &path, const std::vector<float> &lr, int sr)
{
    std::ofstream f(path, std::ios::binary);
    int n = (int)lr.size();
    auto u32 = [&](uint32_t v) { f.write((const char *)&v, 4); };
    auto u16 = [&](uint16_t v) { f.write((const char *)&v, 2); };
    f.write("RIFF", 4);
    u32(36 + (uint32_t)n * 2);
    f.write("WAVEfmt ", 8);
    u32(16);
    u16(1);
    u16(2);
    u32((uint32_t)sr);
    u32((uint32_t)sr * 4);
    u16(4);
    u16(16);
    f.write("data", 4);
    u32((uint32_t)n * 2);
    for (float x : lr) {
        int v = (int)std::lround(std::max(-1.0f, std::min(1.0f, x)) * 32767.0f);
        u16((uint16_t)(int16_t)v);
    }
}

/* a program packed as the Trigon packs it */
static std::vector<unsigned char> packTrigon(const unsigned char *b)
{
    std::vector<unsigned char> m = {0xF0, 0x01, 0x39, 0x02, 0x00, 0x05};
    for (int i = 0; i < 1024; i += 7) {
        unsigned char msb = 0;
        int n = std::min(7, 1024 - i);
        for (int j = 0; j < n; j++)
            msb |= (unsigned char)(((b[i + j] >> 7) & 1) << j);
        m.push_back(msb);
        for (int j = 0; j < n; j++)
            m.push_back(b[i + j] & 0x7F);
    }
    m.push_back(0xF7);
    return m;
}

int main(int argc, char **argv)
{
    auto eng = std::make_unique<Engine>();
    Engine &e = *eng;

    /* every factory program, three rates */
    for (double sr : {44100.0, 48000.0, 96000.0})
        for (int i = 0; i < NFACTORY; i++) {
            e.prepare(sr);
            e.setProgram(FACTORY[i]);
            if (FACTORY[i].seq.length)
                e.seqPlay(true);
            Stats s = play(e, sr, {48, 55, 60, 64}, 3.0f, 2.0f);
            CHECK(s.finite, "%s at %.0f: not finite", FACTORY[i].name, sr);
            CHECK(s.peak > 0.02f, "%s at %.0f: silent (%f)", FACTORY[i].name, sr, s.peak);
            CHECK(s.peak < 2.5f, "%s at %.0f: too loud (%f)", FACTORY[i].name, sr, s.peak);
            if (sr == 48000.0)
                std::printf("  %-18s peak %.3f  rms %.3f\n", FACTORY[i].name, s.peak, s.rms);
            e.panic();
        }

    /* each filter, resonance up, no oscillator: it sings */
    for (int ft = 0; ft < NFILTERTYPE; ft++) {
        Program p;
        programInit(p);
        p.p[P_F_TYPE] = (int16_t)ft;
        p.p[P_O1_LEVEL] = 0;
        p.p[P_CUTOFF] = 90;
        p.p[P_RESO] = 255;
        e.prepare(48000.0);
        e.setProgram(p);
        Stats s = play(e, 48000.0, {60}, 1.5f, 1.4f);
        CHECK(s.finite && s.peak < 3.0f, "filter %d: %f", ft, s.peak);
        CHECK(s.rms > 0.005f, "filter %d does not self-oscillate (rms %f)", ft, s.rms);
        std::printf("  filter %-18s self-osc rms %.3f peak %.3f\n", FILTERTYPE_NAME[ft], s.rms, s.peak);
        p.p[P_POLE2] = 1;
        e.setProgram(p);
        s = play(e, 48000.0, {60}, 0.8f, 0.7f);
        CHECK(s.finite && s.peak < 3.0f, "filter %d 2-pole: %f", ft, s.peak);
    }

    /* every effect in both slots, with a saw */
    for (int t = 0; t < NFXTYPE; t++) {
        Program p;
        programInit(p);
        p.p[P_FXB_TYPE] = (int16_t)t;
        p.p[P_FXB_MIX] = 100;
        p.p[P_FXB_P1] = 200;
        p.p[P_FXB_P2] = 120;
        if (t < NFXTYPE_A) {
            p.p[P_FXA_TYPE] = (int16_t)t;
            p.p[P_FXA_MIX] = 90;
        }
        e.prepare(44100.0);
        e.setProgram(p);
        Stats s = play(e, 44100.0, {57, 64}, 3.0f, 0.5f);
        CHECK(s.finite && s.peak < 3.0f && s.peak > 0.02f, "fx %s: peak %f", FXTYPE_NAME[t], s.peak);
    }

    /* unison and the chord memory, 12 voices, the arp, the sequencer, odd buffer sizes */
    {
        Program p = FACTORY[0];
        p.p[P_UNISON] = 1;
        p.p[P_UNI_VOICES] = 6;
        p.chord[0] = 0;
        p.chord[1] = 4;
        p.chord[2] = 7;
        e.prepare(48000.0);
        e.setVoices(12);
        e.setProgram(p);
        Stats s = play(e, 48000.0, {60, 62, 65}, 1.0f, 0.6f, 17);
        CHECK(s.finite && s.peak > 0.02f && s.peak < 2.5f, "chord unison: %f", s.peak);
        CHECK(e.voicesSounding() <= 12, "voices");
        p.p[P_UNISON] = 0;
        p.p[P_ARP_ON] = 1;
        e.setProgram(p);
        s = play(e, 48000.0, {60, 64, 67, 71, 72, 76, 79, 83}, 2.0f, 1.5f, 1);
        CHECK(s.finite && s.peak > 0.02f, "arp");
        e.setVoices(6);
        e.setProgram(FACTORY[11]);
        e.seqPlay(true);
        s = play(e, 48000.0, {}, 2.0f, 2.0f, 333);
        CHECK(s.finite && s.peak > 0.02f, "sequencer: %f", s.peak);
        e.seqPlay(false);
    }

    /* the SysEx reader: a made-up Trigon program, packed, read back */
    {
        unsigned char b[1024] = {};
        b[0] = 1;
        b[3] = 193;
        b[5] = 1;
        b[17] = 100;
        b[29] = 77;
        b[30] = 200;
        b[36] = 180;
        b[45] = 127;
        b[49] = 3;
        b[50] = 11;
        b[98] = 133;
        b[108] = 0;
        b[109] = 7;
        for (int i = 110; i < 114; i++)
            b[i] = 127;
        std::memcpy(b + 116, "Test Patch          ", 20);
        b[137] = 60;
        b[137 + 64] = 128 + 100;
        b[138] = 62;
        b[138 + 64] = 128;
        auto m = packTrigon(b);
        CHECK(m.size() == 1178, "packed size %d", (int)m.size());
        auto ps = readSyx(m.data(), m.size());
        CHECK(ps.size() == 1, "read %d", (int)ps.size());
        if (ps.size() == 1) {
            const Program &p = ps[0].prog;
            CHECK(std::string(p.name) == "Test Patch", "name '%s'", p.name);
            CHECK(p.p[P_O2_PITCH] == 193 && p.p[P_CUTOFF] == 77 && p.p[P_RESO] == 200 && p.p[P_F_ENV_AMT] == 180, "params");
            CHECK(p.p[P_FXA_TYPE] == 3 && p.p[P_FXB_TYPE] == 11 && p.p[P_BPM] == 133, "fx, bpm");
            CHECK(p.chord[1] == 7 && p.chord[2] == -1, "chord");
            CHECK(p.seq.length == 2 && p.seq.note[0][0] == 62 && p.seq.vel[0][0] == 100 && p.seq.note[1][0] == PN_REST, "seq");
            CHECK(ps[0].bank == 0 && ps[0].number == 5, "place");
        }
    }

    /* the CPU: six voices of three saws, HIGH, at 48 kHz */
    {
        e.prepare(48000.0);
        e.setQuality(1);
        e.setProgram(FACTORY[0]);
        auto t0 = std::chrono::steady_clock::now();
        play(e, 48000.0, {48, 52, 55, 60, 64, 67}, 10.0f, 9.0f);
        double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::printf("  CPU: 10 s of six voices (HIGH) in %.3f s (%.1f %% of a core)\n", s, s * 10.0);
    }

    /* a .syx file: each program read and played; --wav dir: the factory programs as WAVs */
    for (int a = 1; a < argc; a++) {
        std::string arg = argv[a];
        if (arg == "--wav" && a + 1 < argc) {
            std::string dir = argv[++a];
            for (int i = 0; i < NFACTORY; i++) {
                std::vector<float> lr;
                e.prepare(48000.0);
                e.setProgram(FACTORY[i]);
                if (FACTORY[i].seq.length)
                    e.seqPlay(true);
                play(e, 48000.0, {48, 55, 60, 64}, 4.0f, 2.5f, 256, &lr);
                e.panic();
                char nm[64];
                std::snprintf(nm, sizeof nm, "%s/%02d.wav", dir.c_str(), i + 1);
                wav(nm, lr, 48000);
            }
            continue;
        }
        std::ifstream f(arg, std::ios::binary);
        std::vector<unsigned char> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        auto ps = readSyx(d.data(), d.size());
        std::printf("  %s: %d programs\n", arg.c_str(), (int)ps.size());
        float worst = 0.0f;
        int silent = 0;
        for (auto &sp : ps) {
            e.prepare(48000.0);
            e.setProgram(sp.prog);
            Stats s = play(e, 48000.0, {48, 55, 60, 64}, 2.0f, 1.5f);
            CHECK(s.finite, "%s: not finite", sp.prog.name);
            if (s.peak > worst)
                worst = s.peak;
            if (s.peak < 0.01f)
                silent++;
            if (s.peak > 2.0f)
                std::printf("    loud: %03d %s %.2f\n", sp.bank * 100 + sp.number, sp.prog.name, s.peak);
            e.panic();
        }
        std::printf("  loudest %.2f, near silent %d\n", worst, silent);
    }

    std::printf(fails ? "%d FAILED\n" : "all passed\n", fails);
    return fails ? 1 : 0;
}
