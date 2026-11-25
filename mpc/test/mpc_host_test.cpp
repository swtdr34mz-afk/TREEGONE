/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
/* A test host for the MPC build's native library: it loads it as the MPC does (its host contract, the SDK's
 * docs/host-contract.md: open twice, parameter 0 nudged on every prepare, processing in place, any block of 32 to 512
 * frames, MIDI before each block, the transport, chunks saved and restored) and checks what a plugin on the MPC must
 * do: answer every query, never produce a NaN or a click, let every note go, survive programs, state and a parameter
 * storm, and how much CPU it costs.
 *
 *   mpc_host_test <treegone.so> [out.wav]      exit 0 when every check passes
 *
 * Built against the SDK's VST2 ABI header only (framework/mpc_vst2_abi.h, MIT). */
#include "mpc_vst2_abi.h"
#include <dlfcn.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {
VstTimeInfo ti;
int automated = 0;                       /* the parameters the plugin told the host of (audioMasterAutomate) */
VstIntPtr host(AEffect *, VstInt32 op, VstInt32, VstIntPtr, void *ptr, float)
{
    switch (op) {
    case audioMasterAutomate: automated++; return 0;
    case audioMasterVersion: return 2400;
    case audioMasterGetTime: return (VstIntPtr)&ti;
    case audioMasterGetSampleRate: return 44100;
    case audioMasterGetBlockSize: return 512;
    case audioMasterGetVendorString: if (ptr) std::strcpy((char *)ptr, "MPC 3"); return 1;
    case audioMasterGetProductString: if (ptr) std::strcpy((char *)ptr, "MPC 3"); return 1;
    case audioMasterCanDo: return 1;
    default: return 0;
    }
}

int fails = 0, checks = 0;
void check(bool ok, const std::string &what)
{
    checks++;
    if (!ok)
        fails++;
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", what.c_str());
}

struct Plug {
    AEffect *e;
    VstIntPtr d(VstInt32 op, VstInt32 idx = 0, VstIntPtr val = 0, void *ptr = nullptr, float opt = 0.0f)
    {
        return e->dispatcher(e, op, idx, val, ptr, opt);
    }
    std::string str(VstInt32 op, VstInt32 idx = 0)
    {
        char b[256] = {};
        d(op, idx, 0, b);
        return b;
    }
};

/* the MPC's prepare: rate, block, resume, parameter 0 to 1 and straight back */
void prepare(Plug &p, int block)
{
    p.d(effSetSampleRate, 0, 0, nullptr, 44100.0f);
    p.d(effSetBlockSize, 0, block);
    p.d(effMainsChanged, 0, 1);
    float v = p.e->getParameter(p.e, 0);
    p.e->setParameter(p.e, 0, 1.0f);
    p.e->setParameter(p.e, 0, v);
}

struct Ev {
    int at;
    uint8_t b0, b1, b2;
};

/* renders a phrase in blocks (sizes cycled), in place (the MPC's buffers are both input and output) */
std::vector<float> render(Plug &p, int frames, std::vector<Ev> evs, std::vector<int> blocks, double *cpu = nullptr,
                          bool playing = true, float bpm = 110.0f)
{
    std::vector<float> outL(frames), outR(frames);
    std::vector<float> bl(512), br(512);
    std::sort(evs.begin(), evs.end(), [](const Ev &a, const Ev &b) { return a.at < b.at; });
    size_t ei = 0, bi = 0;
    int pos = 0;
    double t = 0;
    while (pos < frames) {
        int n = std::min(blocks[bi++ % blocks.size()], frames - pos);
        std::vector<VstMidiEvent> me;
        while (ei < evs.size() && evs[ei].at < pos + n) {
            VstMidiEvent m = {};
            m.type = kVstMidiType;
            m.byteSize = sizeof(VstMidiEvent);
            m.deltaFrames = std::max(0, evs[ei].at - pos);
            m.midiData[0] = (char)evs[ei].b0;
            m.midiData[1] = (char)evs[ei].b1;
            m.midiData[2] = (char)evs[ei].b2;
            me.push_back(m);
            ei++;
        }
        std::vector<char> buf(sizeof(VstEvents) + sizeof(VstEvent *) * (me.size() + 2));
        VstEvents *ve = (VstEvents *)buf.data();
        ve->numEvents = (VstInt32)me.size();
        for (size_t i = 0; i < me.size(); i++)
            ve->events[i] = (VstEvent *)&me[i];
        p.d(effProcessEvents, 0, 0, ve);
        ti.flags = kVstTempoValid | kVstPpqPosValid | (playing ? kVstTransportPlaying : 0);
        ti.tempo = bpm;
        ti.sampleRate = 44100;
        ti.samplePos = pos;
        ti.ppqPos = pos / 44100.0 * bpm / 60.0;
        for (int i = 0; i < n; i++)
            bl[i] = br[i] = 0.0f;
        float *io[2] = {bl.data(), br.data()};
        auto t0 = std::chrono::steady_clock::now();
        p.e->processReplacing(p.e, io, io, n);
        t += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        for (int i = 0; i < n; i++) {
            outL[pos + i] = bl[i];
            outR[pos + i] = br[i];
        }
        pos += n;
    }
    if (cpu)
        *cpu = t / (frames / 44100.0) * 100.0;
    std::vector<float> st(2 * frames);
    for (int i = 0; i < frames; i++)
        st[2 * i] = outL[i], st[2 * i + 1] = outR[i];
    return st;
}

double rms(const std::vector<float> &s, int a, int b)
{
    double x = 0;
    for (int i = 2 * a; i < 2 * b; i++)
        x += (double)s[i] * s[i];
    return std::sqrt(x / std::max(1, 2 * (b - a)));
}
bool finite(const std::vector<float> &s)
{
    for (float v : s)
        if (!std::isfinite(v))
            return false;
    return true;
}
double peak(const std::vector<float> &s)
{
    double m = 0;
    for (float v : s)
        m = std::max(m, (double)std::fabs(v));
    return m;
}
/* the largest jump of the second difference against the signal's own level: a click stands out */
double clickRatio(const std::vector<float> &s)
{
    double mx = 0, sum = 0;
    int n = (int)s.size() / 2;
    for (int i = 2; i < n; i++) {
        double d = s[2 * i] - 2 * s[2 * i - 2] + s[2 * i - 4];
        mx = std::max(mx, std::fabs(d));
        sum += std::fabs(d);
    }
    return mx / (sum / std::max(1, n - 2) + 1e-9);
}

void wav(const char *fn, const std::vector<float> &s)
{
    FILE *f = std::fopen(fn, "wb");
    if (!f)
        return;
    int bytes = (int)s.size() * 4;
    auto w32 = [&](int v) { std::fwrite(&v, 4, 1, f); };
    auto w16 = [&](short v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f);
    w32(36 + bytes);
    std::fwrite("WAVEfmt ", 1, 8, f);
    w32(16);
    w16(3);
    w16(2);
    w32(44100);
    w32(44100 * 8);
    w16(8);
    w16(32);
    std::fwrite("data", 1, 4, f);
    w32(bytes);
    std::fwrite(s.data(), 4, s.size(), f);
    std::fclose(f);
}

std::vector<Ev> phrase(int len)
{
    std::vector<Ev> e;
    int chord[] = {48, 55, 60, 64};
    for (int k : chord)
        e.push_back({0, 0x90, (uint8_t)k, 100});
    for (int k : chord)
        e.push_back({len / 3, 0x80, (uint8_t)k, 0});
    int mel[] = {72, 74, 76, 79, 77, 76, 74, 72};
    for (int i = 0; i < 8; i++) {
        int t = len / 3 + 4000 + i * 5000;
        e.push_back({t, 0x90, (uint8_t)mel[i], (uint8_t)(60 + i * 6)});
        e.push_back({t + 4000, 0x80, (uint8_t)mel[i], 0});
    }
    for (int i = 0; i <= 20; i++)
        e.push_back({len / 2 + i * 300, 0xE0, 0, (uint8_t)(64 + i * 3)});   /* bend up */
    for (int i = 0; i <= 20; i++)
        e.push_back({len / 2 + i * 300, 0xB0, 1, (uint8_t)(i * 6)});       /* mod wheel */
    e.push_back({len * 2 / 3, 0xE0, 0, 64});
    e.push_back({len * 2 / 3, 0xB0, 1, 0});
    return e;
}
} // namespace

int param(Plug &p, const char *name)      /* a parameter's index, by its name */
{
    for (int i = 0; i < p.e->numParams; i++)
        if (p.str(effGetParamName, i) == name)
            return i;
    std::printf("no parameter %s\n", name);
    std::exit(1);
}
void setPlain(Plug &p, int i, float plain, float lo, float hi) { p.e->setParameter(p.e, i, (plain - lo) / (hi - lo)); }

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <plugin.so> [out.wav]\n", argv[0]);
        return 2;
    }
    void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!lib) {
        std::fprintf(stderr, "%s\n", dlerror());
        return 2;
    }
    auto entry = (AEffect * (*)(audioMasterCallback)) dlsym(lib, "VSTPluginMain");
    check(entry != nullptr, "VSTPluginMain exported");
    if (!entry)
        return 1;
    Plug p{entry(host)};
    check(p.e && p.e->magic == kEffectMagic && p.e->object && p.e->resvd2 == 0, "the effect: magic, object, resvd2");
    check(p.e->flags & effFlagsIsSynth, "an instrument");
    check(p.e->numOutputs == 2 && p.e->numInputs == 0, "0 in, 2 out");
    check(!(p.e->flags & effFlagsHasEditor), "no editor (the MPC draws the skin)");
    std::printf("  %s by %s, %d parameters, %d programs\n", p.str(effGetEffectName).c_str(), p.str(effGetVendorString).c_str(),
                p.e->numParams, p.e->numPrograms);
    check(p.str(effGetEffectName) == "TREEGONE" && p.str(effGetVendorString) == "swtdr34mz", "names as in plugin.json");
    check(p.e->numParams == 112, "112 parameters (their order permanent)");

    p.d(effSetSampleRate, 0, 0, nullptr, 44100.0f);
    p.d(effSetBlockSize, 0, 512);
    p.d(effOpen);
    p.d(effOpen);
    bool names = true;
    std::vector<std::string> seen;
    for (int i = 0; i < p.e->numParams; i++) {
        std::string n = p.str(effGetParamName, i), v = p.str(effGetParamDisplay, i);
        if (n.empty() || n.size() > 12 || v.size() > 12 || std::find(seen.begin(), seen.end(), n) != seen.end()) {
            std::printf("    parameter %d: '%s' = '%s'\n", i, n.c_str(), v.c_str());
            names = false;
        }
        seen.push_back(n);
        p.d(effCanBeAutomated, i);
    }
    check(names, "every parameter named (unique, 12 characters at most), every value shown");
    p.d(effSetProgram, 0, 0);
    bool pn = true;
    for (int i = 0; i < p.e->numPrograms; i++) {
        char b[64] = {};
        pn &= p.d(effGetProgramNameIndexed, i, 0, b) == 1 && b[0] && std::strlen(b) <= 24;
    }
    check(pn && p.e->numPrograms == 24, "24 programs, each named");
    prepare(p, 512);

    const int LEN = 44100 * 5;
    bool allok = true;
    double worstCpu = 0;
    std::vector<float> keep;
    for (int pr = 0; pr < 24; pr++) {
        p.d(effSetProgram, 0, pr);
        double cpu = 0;
        auto s = render(p, LEN, phrase(LEN), {32, 64, 96, 128, 192, 256, 512}, &cpu);
        worstCpu = std::max(worstCpu, cpu);
        bool ok = finite(s) && peak(s) < 2.0 && rms(s, 0, LEN) > 1e-4;
        if (!ok) {
            char b[64] = {};
            p.d(effGetProgramNameIndexed, pr, 0, b);
            std::printf("    program %d %s: finite %d peak %.2f rms %.5f\n", pr, b, (int)finite(s), peak(s), rms(s, 0, LEN));
        }
        allok &= ok;
        if (pr == 0)
            keep = s;
    }
    check(allok, "all 24 programs sound, finite, under 2.0, in blocks of 32 to 512");
    if (argc > 2)
        wav(argv[2], keep);

    auto fresh = [&](std::vector<int> blocks) {
        Plug q{entry(host)};
        q.d(effSetSampleRate, 0, 0, nullptr, 44100.0f);
        q.d(effSetBlockSize, 0, 512);
        q.d(effOpen);
        q.d(effSetProgram, 0, 1);
        prepare(q, 512);
        auto out = render(q, LEN, phrase(LEN), blocks);
        q.d(effMainsChanged, 0, 0);
        q.d(effClose);
        return out;
    };
    auto a = fresh({512});
    auto b = fresh({32, 192, 64, 96});
    double dd = 0, ss = 0;
    for (size_t i = 0; i < a.size(); i++)
        dd += (double)(a[i] - b[i]) * (a[i] - b[i]), ss += (double)a[i] * a[i];
    check(dd <= ss * 1e-12, "the same at any block size");

    /* the poly sequencer: the program's own sequence plays on PLAY, on the MPC's tempo; and stops */
    {
        p.d(effSetProgram, 0, 11);       /* (Seq Pulse) */
        render(p, 4410, {}, {256});
        int play = param(p, "Seq Play");
        p.e->setParameter(p.e, play, 1.0f);
        auto s = render(p, 44100 * 4, {}, {256});
        p.e->setParameter(p.e, play, 0.0f);
        auto t = render(p, 44100 * 4, {}, {256});
        check(rms(s, 4410, 44100 * 4) > 1e-3 && rms(t, 44100 * 3, 44100 * 4) < 1e-4, "the poly sequence plays on PLAY and stops");
    }

    /* state: the parameters and the sequences saved and restored */
    {
        p.d(effSetProgram, 0, 11);       /* (Seq Pulse) */
        render(p, 4410, {}, {256});
        void *chunk = nullptr;
        VstIntPtr sz = p.d(effGetChunk, 0, 0, &chunk);
        std::vector<uint8_t> saved((uint8_t *)chunk, (uint8_t *)chunk + (sz > 0 ? sz : 0));
        std::vector<float> before(p.e->numParams);
        for (int i = 0; i < p.e->numParams; i++)
            before[i] = p.e->getParameter(p.e, i);
        p.d(effSetProgram, 0, 0);        /* (a program without a sequence) */
        render(p, 4410, {}, {256});
        p.d(effSetChunk, 0, (VstIntPtr)saved.size(), saved.data());
        bool same = sz > 0;
        for (int i = 0; i < p.e->numParams; i++)
            same &= std::fabs(p.e->getParameter(p.e, i) - before[i]) < 1e-6f;
        render(p, 4410, {}, {256});
        int play = param(p, "Seq Play");
        p.e->setParameter(p.e, play, 1.0f);
        auto s = render(p, 44100 * 3, {}, {256});
        p.e->setParameter(p.e, play, 0.0f);
        render(p, 44100 * 3, {}, {256});
        check(same && rms(s, 4410, 44100 * 3) > 1e-3, "state saved and restored with its sequence (" + std::to_string(saved.size()) + " bytes)");
    }

    /* a storm */
    {
        uint32_t r = 12345;
        auto rnd = [&] { r ^= r << 13; r ^= r >> 17; r ^= r << 5; return r; };
        std::vector<Ev> e;
        for (int t = 0; t < 44100 * 8; t += 200 + (int)(rnd() % 3000)) {
            int k = rnd() % 10;
            uint8_t n = (uint8_t)(24 + rnd() % 80);
            if (k < 4) e.push_back({t, 0x90, n, (uint8_t)(1 + rnd() % 127)});
            else if (k < 7) e.push_back({t, 0x80, n, 0});
            else if (k == 7) e.push_back({t, 0xB0, 64, (uint8_t)(rnd() & 127)});
            else if (k == 8) e.push_back({t, 0xA0, n, (uint8_t)(rnd() & 127)});
            else e.push_back({t, 0xD0, (uint8_t)(rnd() & 127), 0});
        }
        for (int k = 0; k < 128; k++)
            e.push_back({44100 * 8, 0x80, (uint8_t)k, 0});
        e.push_back({44100 * 8, 0xB0, 64, 0});
        p.d(effSetProgram, 0, 23);
        for (int q = 0; q < 300; q++)
            {
                int i = (int)(rnd() % 108);       /* (the program's parameters: not VOICES, QUALITY, HOLD) */
                p.e->setParameter(p.e, i < 102 ? 1 + i : 106 + i - 102, (float)(rnd() % 1000) / 999.0f);
            }
        auto st = render(p, 44100 * 9, e, {256, 33, 512});
        check(finite(st) && peak(st) < 6.0, "a storm of parameters and MIDI: finite and bounded");
        p.d(effSetProgram, 0, 23);
        p.e->setParameter(p.e, 0, 0.7f);
        auto tail = render(p, 44100 * 16, {{0, 0xB0, 123, 0}}, {512});
        check(rms(tail, 44100 * 15, 44100 * 16) < 1e-5, "every voice ends after ALL NOTES OFF");
    }

    /* QUALITY both ways, VOICES 6 and 12, each filter */
    {
        std::vector<Ev> e;
        for (int k : {48, 52, 55, 60})
            e.push_back({0, 0x90, (uint8_t)k, 100});
        for (int k : {48, 52, 55, 60})
            e.push_back({44100 * 2, 0x80, (uint8_t)k, 0});
        int qi = param(p, "Quality"), vi = param(p, "Voices"), ft = param(p, "Filt Type");
        bool ok = true;
        for (int q = 0; q < 2; q++)
            for (int v = 0; v < 2; v++)
                for (int m = 0; m < 3; m++) {
                    p.d(effSetProgram, 0, 0);
                    p.e->setParameter(p.e, qi, (float)q);
                    p.e->setParameter(p.e, vi, (float)v);
                    p.e->setParameter(p.e, ft, (float)m / 2.0f);
                    auto s = render(p, 44100 * 3, e, {128});
                    ok &= finite(s) && rms(s, 4410, 44100 * 2) > 1e-3 && peak(s) < 2.0;
                }
        render(p, 44100 * 6, {}, {512});
        check(ok, "QUALITY, VOICES and the three filters every way: all play");
    }

    /* the arp on the MPC's tempo, its notes ending when the keys go */
    {
        p.d(effSetProgram, 0, 10);       /* (Arp Machine) */
        auto s = render(p, 44100 * 6, {{100, 0x90, 60, 100}, {100, 0x90, 64, 100}, {100, 0x90, 67, 100}, {44100 * 3, 0x80, 60, 0},
                                       {44100 * 3, 0x80, 64, 0}, {44100 * 3, 0x80, 67, 0}}, {256});
        check(rms(s, 4410, 44100 * 3) > 1e-3 && rms(s, 44100 * 5, 44100 * 6) < 1e-3, "the arp plays on the tempo and stops");
    }

    /* LOW SPLIT: a bass voice under the split point, on top of six */
    {
        p.d(effSetProgram, 0, 0);
        render(p, 4410, {}, {256});
        int on = param(p, "Split On"), pt = param(p, "Split Point");
        p.e->setParameter(p.e, on, 1.0f);
        p.e->setParameter(p.e, pt, 48.0f / 127.0f);
        std::vector<Ev> e;
        for (int k : {36, 60, 64, 67})
            e.push_back({0, 0x90, (uint8_t)k, 100});
        for (int k : {36, 60, 64, 67})
            e.push_back({44100 * 2, 0x80, (uint8_t)k, 0});
        auto s = render(p, 44100 * 4, e, {128});
        p.e->setParameter(p.e, on, 0.0f);
        auto t = render(p, 44100 * 3, {}, {256});
        check(finite(s) && rms(s, 4410, 44100 * 2) > 1e-3 && peak(s) < 2.0 && rms(t, 44100 * 2, 44100 * 3) < 1e-4,
              "LOW SPLIT plays under the split point and lets go");
    }

    /* CPU: twelve voices (six notes, unison off), ECO and HIGH (this computer; the MPC's ARM is slower) */
    {
        std::vector<Ev> e;
        for (int k : {36, 43, 48, 55, 60, 64, 67, 72, 76, 79, 84, 88})
            e.push_back({0, 0x90, (uint8_t)k, 100});
        int qi = param(p, "Quality"), vi = param(p, "Voices");
        double c[2];
        for (int q = 0; q < 2; q++) {
            p.d(effSetProgram, 0, 0);    /* (Trigon Brass: three oscillators, a hall) */
            p.e->setParameter(p.e, vi, 1.0f);
            p.e->setParameter(p.e, qi, (float)q);
            render(p, 44100 * 4, e, {256}, &c[q]);
        }
        std::printf("  CPU, 12 voices (this computer): ECO %.1f %%, HIGH %.1f %% of a core; the programs' worst: %.1f %%\n",
                    c[0], c[1], worstCpu);
    }

    p.d(effMainsChanged, 0, 0);
    p.d(effClose);
    std::printf("\n%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
