/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
#include "Engine.h"
#include <algorithm>
#include <vector>

namespace tg
{
/* ------------------------------------------------------------------ tables --- */
const float *blepTables()
{
    /* PROPHET 67's: a Kaiser-windowed sinc (cutoff 0.9 of Nyquist, beta 8) over 16 samples, integrated once (the
     * band-limited step) and its residual once more (the ramp's), 1024 points a sample. Made once, shared */
    static const std::vector<float> t = [] {
        const int H = BL_H, OS = BL_OS, N = BL_N;
        const double fc = 0.9, beta = 8.0, pi = 3.14159265358979323846;
        auto i0 = [](double x) {
            double s = 1, term = 1;
            for (int k = 1; k < 60; k++) {
                term *= (x / (2 * k)) * (x / (2 * k));
                s += term;
            }
            return s;
        };
        std::vector<double> h(N), B(N), R(N);
        for (int i = 0; i < N; i++) {
            double x = -H + (double)i / OS, r = x / H;
            double w = std::fabs(r) <= 1 ? i0(beta * std::sqrt(1 - r * r)) / i0(beta) : 0;
            double sc = x == 0 ? 1.0 : std::sin(pi * fc * x) / (pi * fc * x);
            h[i] = fc * sc * w;
        }
        double acc = 0;
        B[0] = 0;
        for (int i = 1; i < N; i++)
            B[i] = (acc += 0.5 * (h[i - 1] + h[i]) / OS);
        double total = B[2 * H * OS];
        for (int i = 0; i < N; i++)
            B[i] /= total;
        acc = 0;
        R[0] = 0;
        for (int i = 1; i < N; i++) {
            double x0 = -H + (double)(i - 1) / OS, x1 = -H + (double)i / OS;
            double e0 = B[i - 1] - (x0 >= 0 ? 1.0 : 0.0), e1 = B[i] - (x1 >= 0 ? 1.0 : 0.0);
            if (x0 < 0 && x1 >= 0)
                e0 = B[i - 1];
            R[i] = (acc += 0.5 * (e0 + e1) / OS);
        }
        double drift = R[2 * H * OS];
        std::vector<float> out(2 * N);
        for (int i = 0; i < N; i++) {
            out[i] = (float)B[i];
            out[N + i] = (float)(R[i] - drift * std::min(1.0, (double)i / (2 * H * OS)));
        }
        out[N - 1] = 1.0f;
        out[2 * N - 1] = 0.0f;
        return out;
    }();
    return t.data();
}

namespace
{
/* each unison voice's place in the detune */
const float U_DET[MAXV] = {0.0f, -1.0f, 1.0f, -0.5f, 0.5f, -0.75f, 0.75f, -0.25f, 0.25f, -0.88f, 0.88f, -0.38f};
/* PAN SPREAD: each voice's side and how far */
const float PAN_SLOT[MAXV] = {-1.0f, 1.0f, -0.55f, 0.55f, -0.8f, 0.8f, -0.35f, 0.35f, -0.9f, 0.9f, -0.65f, 0.65f};
/* the LFO synced: its cycle in quarters (8 bars .. 1/64) */
const float LFO_SYNC_Q[16] = {32, 16, 8, 4, 3, 2, 1.5f, 1, 2.0f / 3.0f, 0.75f, 0.5f, 1.0f / 3.0f, 0.25f, 1.0f / 6.0f, 0.125f, 0.0625f};
/* the clock's divisions: steps a beat, swing */
const float DIV_SPB[NDIVIDE] = {0.5f, 1.0f, 4.0f / 3.0f, 2.0f, 2.0f, 3.0f, 4.0f, 4.0f, 6.0f, 8.0f};
const float DIV_SWING[NDIVIDE] = {0, 0, 0, 0, 0.3333f, 0, 0, 0.3333f, 0, 0};

enum { K_ARP = 1000, K_UNI = 2000, K_SEQ = 3000, K_BASS = 4000 };
constexpr float VOICE_GAIN = 0.30f;

inline float envSec(float v) { return v <= 0.0f ? 0.0006f : 0.0006f * std::exp2(v * (15.5f / 127.0f)); }   /* 0.6 ms .. 27 s */
inline float glideSec(float v) { return 0.0008f * std::exp2(v * (13.6f / 127.0f)); }
inline float bip(int v) { return ((float)v - 127.0f) / 127.0f; }
inline float levelG(int v) { return std::pow((float)v / 127.0f, 1.6f); }
inline float osc3Semis(int oct)                   /* osc 3's OCTAVE: LO, -2 .. +2 */
{
    return oct == 0 ? -96.0f : (float)(oct - 3) * 12.0f;
}
} // namespace

/* ------------------------------------------------------------------- setup --- */
Engine::Engine()
{
    programInit(prog);
    tab = blepTables();
    uint32_t s = 0xC0FFEE11u;
    for (int i = 0; i <= MAXV; i++) {
        V[i].seed = 0x1234567u + 7919u * (uint32_t)i;
        V[i].panSlot = i < MAXV ? PAN_SLOT[i] : 0.0f;
        for (auto &r : V[i].vr)
            r = rndF(s);                          /* (each voice's tolerances: the same every time) */
        for (auto &o : V[i].o)
            o.ph = 0.5f * (rndF(s) + 1.0f);
    }
}

void Engine::prepare(double sampleRate)
{
    sr = (float)sampleRate;
    for (auto &v : V) {
        uint32_t s = v.seed;
        float ps = v.panSlot, vr[6];
        std::memcpy(vr, v.vr, sizeof vr);
        v = Voice();
        v.seed = s;
        v.panSlot = ps;
        std::memcpy(v.vr, vr, sizeof vr);
    }
    fxA.prepare(sr);
    fxB.prepare(sr);
    nKeys = 0;
    nB = 0;
    arpN = 0;
    arpI = -1;
    arpLatched = false;
    anyHeld = false;
    for (auto &s : sounding)
        s = 0;
    dcK = 1.0f - 2.0f * PI * 8.0f / sr;
    dcL = dcR = dcxL = dcxR = 0.0f;
    distLp[0] = distLp[1] = 0.0f;
    for (float &c : ctl)
        c = 0.0f;
    gpos = CH;
}

void Engine::setProgram(const Program &p)
{
    if (playing)
        seqPlay(false);
    prog = p;
    prog.p[P_SEQ_ON] = 0;
    rec = false;
}
void Engine::setParam(int p, int v)
{
    if (p < 0 || p >= NPARAM)
        return;
    int old = prog.p[p];
    prog.p[p] = (int16_t)clampf((float)v, (float)PARAMS[p].lo, (float)PARAMS[p].hi);
    if (p == P_SPLIT_ON && old && !prog.p[p])
        bassStop();
}
void Engine::setVoices(int n)
{
    n = n <= 6 ? 6 : 12;
    if (n == nVoices)
        return;
    nVoices = n;
    for (int i = n; i < MAXV; i++)
        if (V[i].on)
            voiceKill(V[i]);
}

/* ------------------------------------------------------------------- notes --- */
void Engine::noteOn(int note, float vel)
{
    if (prog.p[P_SPLIT_ON] && note < prog.p[P_SPLIT_POINT] && bassKey(note, vel, true))
        return;
    key(note, vel, true);
}
void Engine::noteOff(int note)
{
    if (bassKey(note, 0.0f, false))
        return;
    key(note, 0.0f, false);
}

/* ------------------------------------------------------------------- LOW SPLIT --- */
/* Under the split point: one voice of the program's sound, monophonic, the last key played; legato with its own
 * glide while a key is still held (without glide each key strikes again) */
bool Engine::bassKey(int note, float vel, bool down)
{
    int at = -1;
    for (int i = 0; i < nB; i++)
        if (bKeys[i] == note)
            at = i;
    if (down) {
        if (at >= 0)
            return true;
        bool held = nB > 0;
        if (nB < NKEYS) {
            bKeys[nB] = (int8_t)note;
            bVel[nB++] = vel;
        }
        bassPlay(note, vel, held);
        return true;
    }
    if (at < 0)
        return false;
    for (int i = at; i < nB - 1; i++) {
        bKeys[i] = bKeys[i + 1];
        bVel[i] = bVel[i + 1];
    }
    nB--;
    Voice &v = V[MAXV];
    if (nB > 0) {
        if (v.note != bKeys[nB - 1] + (prog.p[P_SPLIT_OCT] - 2) * 12)
            bassPlay(bKeys[nB - 1], bVel[nB - 1], true);
    } else if (v.on && v.gate) {
        if (susOn || holdOn)
            v.sus = true;
        else
            releaseVoice(v);
        v.gate = false;
    }
    return true;
}

void Engine::bassPlay(int note, float vel, bool legato)
{
    const int16_t *P = prog.p;
    Voice &v = V[MAXV];
    float target = (float)(note + (P[P_SPLIT_OCT] - 2) * 12);
    bool fresh = !v.on;
    bool glide = P[P_SPLIT_GLIDE] > 0 && legato && !fresh && v.gate;
    v.on = true;
    v.key = K_BASS;
    v.note = (int)target;
    if (!glide)
        v.vel = vel;
    v.uniDet = 0.0f;
    v.gate = true;
    v.sus = false;
    v.fade = 1.0f;
    v.pend = false;
    v.age = ++ageN;
    v.gmul = (float)P[P_SPLIT_VOL] / 100.0f;
    v.t = target;
    if (glide)
        v.gr = 12.0f / glideSec((float)P[P_SPLIT_GLIDE]);
    else {
        v.p = target;
        v.gr = 0.0f;
        v.fe.st = ES_ATT;
        v.ae.st = ES_ATT;
        v.kick = 1.0f;
    }
    if (fresh) {
        v.smOk = false;
        v.fe.v = v.ae.v = 0.0f;
        v.fbOut = 0.0f;
        for (float &f : v.f)
            f = 0.0f;
        v.ffb = 0.0f;
    }
}

void Engine::bassStop()
{
    nB = 0;
    Voice &v = V[MAXV];
    if (v.on) {
        v.gate = v.sus = false;
        releaseVoice(v);
    }
}

void Engine::key(int note, float vel, bool down)
{
    const int16_t *P = prog.p;
    int at = -1;
    for (int i = 0; i < nKeys; i++)
        if (keys[i] == note)
            at = i;
    if (down) {
        if (at >= 0)
            return;
        bool wasHeld = nKeys > 0;
        if (nKeys < NKEYS) {
            keys[nKeys] = (int8_t)note;
            keyVel[nKeys++] = vel;
        }
        if (rec) {                               /* RECORD: into the sequence (and heard) */
            SeqData &s = prog.seq;
            if (recFresh) {
                for (int k = 0; k < PSEQ_NOTES; k++)
                    s.note[recStep][k] = PN_REST;
                recFresh = false;
            }
            for (int k = 0; k < PSEQ_NOTES; k++)
                if (s.note[recStep][k] == PN_REST) {
                    s.note[recStep][k] = (uint8_t)(note + 2);
                    s.vel[recStep][k] = (uint8_t)clampf(vel * 127.0f, 1.0f, 127.0f);
                    break;
                }
            recHeld++;
        }
        if (P[P_ARP_ON] && !playing) {
            if (arpLatched && !wasHeld)
                arpLatched = false;              /* (held: a new chord replaces it) */
            arpRebuild();
            if (arpI < 0 && !(hostSync && hostPlaying)) {
                pos = 0.0;
                stepIdx = 0;
                stepHigh = true;
                arpStep();
            }
            return;
        }
        anyHeld = wasHeld;
        if (P[P_UNISON])
            unisonUpdate(!wasHeld);
        else
            playNote(note, note, vel);
    } else {
        if (at < 0)
            return;
        for (int i = at; i < nKeys - 1; i++) {
            keys[i] = keys[i + 1];
            keyVel[i] = keyVel[i + 1];
        }
        nKeys--;
        if (rec && recHeld > 0 && --recHeld == 0) {
            recStep = (recStep + 1) % PSEQ_STEPS;
            prog.seq.length = (uint8_t)std::max<int>(prog.seq.length, recStep == 0 ? PSEQ_STEPS : recStep);
            recFresh = true;
        }
        if (P[P_ARP_ON] && !playing) {
            if (holdOn || susOn) {
                arpLatched = true;
                return;
            }
            arpRebuild();
            return;
        }
        if (P[P_UNISON])
            unisonUpdate(false);
        else
            stopNote(note);
    }
}

void Engine::playNote(int k, int note, float vel)
{
    /* the arp's and the sequencer's notes with UNISON on: each a stack of its own */
    const int16_t *P = prog.p;
    int n = 1;
    float det = 0.0f;
    if (P[P_UNISON] && k >= K_ARP) {
        n = std::max(1, std::min<int>(P[P_UNI_VOICES] >= 6 ? 1 : P[P_UNI_VOICES] + 1, cap()));
        det = (float)P[P_UNI_DETUNE] * 2.5f;
        uniN = n;
    }
    for (int j = 0; j < n; j++) {
        int i = findVoice();
        if (i < 0)
            return;
        startVoice(V[i], k, note, vel, n > 1 ? U_DET[j] * det : 0.0f, true);
    }
}

void Engine::stopNote(int k)
{
    for (auto &v : V) {
        if (v.pend && v.pkey == k) {
            v.pend = false;
            continue;
        }
        if (v.key == k && v.gate) {
            if (susOn || holdOn)
                v.sus = true;
            else
                releaseVoice(v);
            v.gate = false;
        }
    }
}

int Engine::findVoice()
{
    /* a free one (the longest silent), else the oldest let go, else the oldest */
    int c = cap(), best = -1;
    uint32_t bestAge = 0xFFFFFFFFu;
    for (int i = 0; i < c; i++)
        if (!V[i].on && !V[i].pend && V[i].age < bestAge) {
            best = i;
            bestAge = V[i].age;
        }
    if (best >= 0)
        return best;
    for (int pass = 0; pass < 2 && best < 0; pass++) {
        bestAge = 0xFFFFFFFFu;
        for (int i = 0; i < c; i++) {
            Voice &v = V[i];
            if (pass == 0 && (v.gate || v.sus))
                continue;
            if (v.age < bestAge) {
                best = i;
                bestAge = v.age;
            }
        }
    }
    return best;
}

void Engine::startVoice(Voice &v, int k, int note, float vel, float det, bool retrig)
{
    const int16_t *P = prog.p;
    if (v.on && v.key != k && retrig && v.ae.v > 0.001f && v.ae.st != ES_IDLE) {
        /* taken while it sounds: out in 4 ms, then this note */
        v.pend = true;
        v.pnote = note;
        v.pkey = k;
        v.pvel = vel;
        v.pdet = det;
        if (v.fade >= 1.0f)
            v.fade = 0.9999f;
        v.gate = false;
        v.age = ++ageN;
        return;
    }
    bool fresh = !v.on;
    v.on = true;
    v.key = k;
    v.note = note;
    v.vel = vel;
    v.uniDet = det;
    v.gate = true;
    v.sus = false;
    v.age = ++ageN;
    v.fade = 1.0f;
    v.pend = false;
    /* glide: from where this voice was (polyphonic portamento); the legato modes only while a key is held */
    bool legatoMode = P[P_GLIDE_MODE] == GM_RATE_LEGATO || P[P_GLIDE_MODE] == GM_TIME_LEGATO;
    bool glideOn = P[P_GLIDE] && (!legatoMode || anyHeld);
    float from = fresh && v.note < 0 ? (float)note : v.p;
    if (k == K_UNI || k >= K_ARP)
        from = lastPitch;
    v.t = (float)note;
    if (glideOn) {
        float T = glideSec((float)P[P_GLIDE_RATE]);
        bool timeMode = P[P_GLIDE_MODE] >= GM_TIME;
        float dist = std::fabs((float)note - from);
        v.p = from;
        v.gr = timeMode ? maxf(dist, 0.01f) / T : 12.0f / T;
    } else {
        v.p = (float)note;
        v.gr = 0.0f;
    }
    lastPitch = (float)note;
    if (retrig || fresh) {
        v.fe.st = ES_ATT;
        v.ae.st = ES_ATT;
        v.kick = 1.0f;                           /* (the gate's edge: what pings a singing filter) */
    }
    if (fresh) {
        v.smOk = false;
        v.fe.v = v.ae.v = 0.0f;
        v.fbOut = 0.0f;
        for (float &f : v.f)
            f = 0.0f;
        v.ffb = 0.0f;
    }
}

void Engine::releaseVoice(Voice &v)
{
    if (v.fe.st != ES_IDLE)
        v.fe.st = ES_REL;
    if (v.ae.st != ES_IDLE)
        v.ae.st = ES_REL;
    v.sus = false;
}

void Engine::voiceKill(Voice &v)
{
    v.pend = false;
    v.gate = v.sus = false;
    if (v.fade >= 1.0f)
        v.fade = 0.9999f;
}

void Engine::unisonUpdate(bool newKey)
{
    const int16_t *P = prog.p;
    if (nKeys == 0) {
        stopNote(K_UNI);
        anyHeld = false;
        return;
    }
    int km = P[P_KEY_MODE], pick = nKeys - 1;
    for (int i = 0; i < nKeys; i++) {
        if ((km == KM_LOW || km == KM_LOW_R) && keys[i] < keys[pick])
            pick = i;
        if ((km == KM_HIGH || km == KM_HIGH_R) && keys[i] > keys[pick])
            pick = i;
    }
    int note = keys[pick];
    float vel = keyVel[pick];
    bool retrig = newKey || km >= KM_LOW_R;
    int c = cap(), offs[MAXV], n = 0;
    bool chord = P[P_UNI_VOICES] >= 6;
    if (chord) {
        for (int i = 0; i < NCHORD && n < c; i++)
            if (prog.chord[i] >= 0)
                offs[n++] = prog.chord[i];
        if (n == 0)
            offs[n++] = 0;
    } else {
        n = std::min<int>(P[P_UNI_VOICES] + 1, c);
        for (int i = 0; i < n; i++)
            offs[i] = 0;
    }
    float det = (float)P[P_UNI_DETUNE] * 2.5f;   /* cents a step */
    int kk = 0;
    for (int i = 0; i < c && kk < n; i++) {
        Voice &v = V[i];
        if (v.on && v.key == K_UNI && (v.gate || v.sus) && !retrig) {
            float tgt = (float)(note + offs[kk]);
            v.t = tgt;
            if (P[P_GLIDE]) {
                float T = glideSec((float)P[P_GLIDE_RATE]);
                v.gr = P[P_GLIDE_MODE] >= GM_TIME ? std::fabs(tgt - v.p) / T : 12.0f / T;
            } else {
                v.p = tgt;
                v.gr = 0.0f;
            }
            v.note = note + offs[kk];
            v.gate = true;
            v.sus = false;
            kk++;
        }
    }
    uniN = n;
    if (kk == 0) {
        for (int i = 0; i < c; i++)
            if (V[i].key == K_UNI && i >= n) {
                V[i].key = -1;
                if (V[i].gate || V[i].sus)
                    releaseVoice(V[i]);
                V[i].gate = V[i].sus = false;
            }
        anyHeld = !newKey;
        for (int j = 0; j < n && j < c; j++)
            startVoice(V[j], K_UNI, note + offs[j], vel, chord ? 0.0f : (n > 1 ? U_DET[j] * det : 0.0f), true);
    }
    lastPitch = (float)note;
}

void Engine::sustain(bool on)
{
    susOn = on;
    if (!on && !holdOn) {
        for (auto &v : V)
            if (v.sus) {
                releaseVoice(v);
                v.sus = false;
            }
        if (arpLatched) {
            arpLatched = false;
            arpRebuild();
        }
    }
}
void Engine::hold(bool on)
{
    holdOn = on;
    if (!on)
        sustain(susOn);
}
void Engine::bend(float v) { bendV = clampf(v, -1.0f, 1.0f); }
void Engine::modWheel(float v) { mwV = clampf(v, 0.0f, 1.0f); }
void Engine::pressure(float v) { prV = clampf(v, 0.0f, 1.0f); }

void Engine::allOff()
{
    nKeys = 0;
    nB = 0;
    arpN = 0;
    arpI = -1;
    arpLatched = false;
    anyHeld = false;
    recHeld = 0;
    for (auto &s : sounding)
        s = 0;
    for (auto &v : V) {
        v.gate = v.sus = false;
        v.pend = false;
        releaseVoice(v);
    }
}
void Engine::panic()
{
    allOff();
    for (auto &v : V) {
        v.fe = Env();
        v.ae = Env();
        v.on = false;
        v.key = -1;
        v.fade = 1.0f;
    }
    fxA.clear();
    fxB.clear();
    playing = false;
}

void Engine::host(bool hp, double ppq, double bpm, bool sync)
{
    hostSync = sync;
    if (bpm > 10.0)
        hostBpm = bpm;
    if (hp && !hostPlaying)
        stepIdx = -1;
    hostPlaying = hp;
    hostPpq = ppq;
    if (sync && hp)
        pos = ppq;
}

/* ------------------------------------------------------------------- the sequencer --- */
void Engine::seqPlay(bool on)
{
    if (on == playing)
        return;
    playing = on;
    prog.p[P_SEQ_ON] = on ? 1 : 0;
    if (on) {
        pStep = -1;
        if (!(hostSync && hostPlaying)) {
            pos = 0.0;
            stepIdx = 0;
            stepHigh = true;
            polyStep();
        }
    } else {
        for (int k = 0; k < PSEQ_NOTES; k++)
            if (sounding[k]) {
                stopNote(K_SEQ + k);
                sounding[k] = 0;
            }
        pStep = -1;
    }
}
void Engine::seqRecord(bool on)
{
    rec = on;
    if (on) {
        recStep = 0;
        recHeld = 0;
        recFresh = true;
        prog.seq.length = 0;
        for (auto &st : prog.seq.note)
            for (auto &n : st)
                n = PN_REST;
    }
}
void Engine::seqRest()
{
    if (!rec)
        return;
    for (int k = 0; k < PSEQ_NOTES; k++)
        prog.seq.note[recStep][k] = PN_REST;
    recStep = (recStep + 1) % PSEQ_STEPS;
    prog.seq.length = (uint8_t)std::max<int>(prog.seq.length, recStep == 0 ? PSEQ_STEPS : recStep);
    recFresh = true;
}
void Engine::seqTie()
{
    if (!rec)
        return;
    for (int k = 0; k < PSEQ_NOTES; k++)
        prog.seq.note[recStep][k] = k == 0 ? PN_TIE : PN_REST;
    recStep = (recStep + 1) % PSEQ_STEPS;
    prog.seq.length = (uint8_t)std::max<int>(prog.seq.length, recStep == 0 ? PSEQ_STEPS : recStep);
    recFresh = true;
}
void Engine::seqBack()
{
    if (rec && recStep > 0) {
        recStep--;
        recFresh = true;
    }
}
void Engine::unisonChord()
{
    if (nKeys == 0)
        return;
    int8_t sorted[NKEYS];
    int n = nKeys;
    for (int i = 0; i < n; i++)
        sorted[i] = keys[i];
    std::sort(sorted, sorted + n);
    for (int i = 0; i < NCHORD; i++)
        prog.chord[i] = i < n ? (int8_t)(sorted[i] - sorted[0]) : (int8_t)-1;
}

int Engine::seqStep() const { return playing ? pStep : (rec ? recStep : -1); }
bool Engine::clockBeat() const { return pos - std::floor(pos) < 0.5; }

void Engine::clockChunk()
{
    const int16_t *P = prog.p;
    float dt = (float)CH / sr;
    bpmNow = hostSync ? (float)hostBpm : (float)P[P_BPM];
    pos += (double)(dt * bpmNow / 60.0f);
    int dv = P[P_DIVIDE];
    float spb = DIV_SPB[dv], sw = DIV_SWING[dv];
    double x = pos * spb;
    long s = (long)std::floor(x);
    double f = x - (double)s;
    long eff = s;
    if ((s & 1) && f < sw)
        eff = s - 1;
    double start = (double)eff + ((eff & 1) ? sw : 0.0), len = (eff & 1) ? 1.0 - sw : 1.0 + sw;
    if (eff < stepIdx - 1 || eff > stepIdx + 64)
        stepIdx = eff - 1;
    bool arp = P[P_ARP_ON] && !playing && arpN > 0;
    if (eff != stepIdx) {
        stepIdx = eff;
        stepHigh = true;
        if (arp)
            arpStep();
        if (playing)
            polyStep();
    } else if (stepHigh && x - start >= len * 0.5) {
        stepHigh = false;                       /* the gate's second half: the arp's note let go, the step's */
        if (arpPlaying >= 0) {
            stopNote(K_ARP);
            arpPlaying = -1;
        }
        if (playing && pStep >= 0) {
            const SeqData &sd = prog.seq;
            int nx = sd.length ? (pStep + 1) % sd.length : 0;
            if (sd.note[nx][0] != PN_TIE)
                for (int k = 0; k < PSEQ_NOTES; k++)
                    if (sounding[k]) {
                        stopNote(K_SEQ + k);
                        sounding[k] = 0;
                    }
        }
    }
}

void Engine::arpRebuild()
{
    if (arpLatched && nKeys == 0)
        return;
    if (arpLatched && nKeys > 0) {
        for (int i = 0; i < nKeys; i++) {
            bool in = false;
            for (int j = 0; j < arpN; j++)
                in |= arpNotes[j] == keys[i];
            if (!in && arpN < NKEYS) {
                arpNotes[arpN] = keys[i];
                arpVel[arpN++] = keyVel[i];
            }
        }
        return;
    }
    arpN = nKeys;
    for (int i = 0; i < nKeys; i++) {
        arpNotes[i] = keys[i];
        arpVel[i] = keyVel[i];
    }
    if (arpN == 0) {
        arpI = -1;
        if (arpPlaying >= 0) {
            stopNote(K_ARP);
            arpPlaying = -1;
        }
    }
}

void Engine::arpStep()
{
    const int16_t *P = prog.p;
    if (arpN == 0)
        return;
    if (arpPlaying >= 0) {
        stopNote(K_ARP);
        arpPlaying = -1;
    }
    int8_t order[NKEYS];
    float ov[NKEYS];
    int n = arpN;
    for (int i = 0; i < n; i++) {
        order[i] = arpNotes[i];
        ov[i] = arpVel[i];
    }
    int mode = P[P_ARP_MODE];
    if (mode != AM_ASSIGN)
        for (int i = 1; i < n; i++)
            for (int j = i; j > 0 && order[j] < order[j - 1]; j--) {
                std::swap(order[j], order[j - 1]);
                std::swap(ov[j], ov[j - 1]);
            }
    int total = n * (P[P_ARP_OCT] + 1);
    switch (mode) {
    case AM_DOWN: arpI = arpI <= 0 ? total - 1 : arpI - 1; break;
    case AM_UPDOWN:
        if (total == 1)
            arpI = 0;
        else if (arpI < 0) {
            arpI = 0;
            arpDir = 1;
        } else {
            arpI += arpDir;
            if (arpI >= total) {
                arpI = total - 2;
                arpDir = -1;
            } else if (arpI < 0) {
                arpI = 1;
                arpDir = 1;
            }
        }
        break;
    case AM_RANDOM: arpI = (int)(rndU(noiseSeed) % (uint32_t)total); break;
    default: arpI = arpI + 1 >= total ? 0 : arpI + 1;
    }
    if (arpI >= total || arpI < 0)
        arpI = 0;
    int o = arpI / n, k = arpI % n;
    int note = order[k] + 12 * o;
    while (note > 127)
        note -= 12;
    arpPlaying = note;
    anyHeld = false;
    playNote(K_ARP, note, ov[k]);
}

void Engine::polyStep()
{
    const SeqData &s = prog.seq;
    if (s.length == 0)
        return;
    pStep = (pStep + 1) % s.length;
    const uint8_t *st = s.note[pStep];
    if (st[0] == PN_TIE)
        return;
    for (int k = 0; k < PSEQ_NOTES; k++)
        if (sounding[k]) {
            stopNote(K_SEQ + k);
            sounding[k] = 0;
        }
    /* the sequence transposes with the key held (from middle C), as the Trigon's does */
    int tr = nKeys > 0 ? keys[nKeys - 1] - 60 : 0;
    for (int k = 0; k < PSEQ_NOTES; k++)
        if (st[k] >= 2) {
            int note = std::min(127, std::max(0, st[k] - 2 + tr));
            playNote(K_SEQ + k, note, s.vel[pStep][k] / 127.0f);
            sounding[k] = st[k];
        }
}

/* ---------------------------------------------------------------- the envelopes --- */
/* this chunk's target and rate for an envelope (base: its ATTACK), the attack an analog one's curve (charging toward
 * past the top, so it ends sharp), decay and release exponential; tscale: VINTAGE's spread */
float Engine::envTarget(Env &e, int base, float tscale, float *tgt, bool *att) const
{
    const int16_t *Q = prog.p + base;
    *att = false;
    if (e.st == ES_ATT && e.v >= 0.9999f) {
        e.v = 1.0f;
        e.st = ES_DEC;
    }
    float sus = (float)Q[2] / 127.0f;
    switch (e.st) {
    case ES_ATT: {
        float T = envSec((float)Q[0]) * tscale;
        *tgt = 1.3f;
        *att = true;
        return 1.0f - std::exp(-1.466f / (T * sr));
    }
    case ES_DEC: {
        float T = envSec((float)Q[1]) * tscale;
        *tgt = sus;
        return 1.0f - std::exp(-4.6f / (T * sr));
    }
    case ES_REL: {
        float T = envSec((float)Q[3]) * tscale;
        *tgt = 0.0f;
        return 1.0f - std::exp(-4.6f / (T * sr));
    }
    default: *tgt = 0.0f; return 1.0f;
    }
}

/* --------------------------------------------------------------- the voice --- */
void Engine::voiceChunk(Voice &v, float *Lo, float *Ro)
{
    const int16_t *P = prog.p;
    const float dt = (float)CH / sr, isr = 1.0f / sr;
    const float vint = (float)P[P_VINTAGE] / 127.0f;

    /* VINTAGE: each voice's own tuning, cutoff and timing, and a slow wander (always a little: it is analog) */
    float wander = (0.7f + 11.0f * vint) * 0.01f;            /* semitones */
    if (--v.drn <= 0) {
        for (int i = 0; i < 3; i++)
            v.dt[i] = rndF(v.seed) * wander;
        v.drn = (int)((0.25f + 0.5f * (float)(rndU(v.seed) & 1023u) / 1023.0f) / dt);
    }
    {
        float kd = 1.0f - std::exp(-dt / 0.3f);
        for (int i = 0; i < 3; i++)
            v.dr[i] += (v.dt[i] - v.dr[i]) * kd;
    }
    float vdet = 0.22f * std::pow(vint, 1.3f);               /* (semitones at most) */
    float tsc = 1.0f + 0.3f * vint * v.vr[4];

    /* glide */
    if (v.gr > 0.0f) {
        float st = v.gr * dt;
        v.p = v.p < v.t ? minf(v.p + st, v.t) : maxf(v.p - st, v.t);
    } else
        v.p = v.t;

    /* pitch, in semitones (from middle C's octave), before what moves per sample */
    float bendS = ctl[0] * (float)P[P_PB_RANGE];
    float uni = v.uniDet * 0.01f;
    float s1 = v.p + (float)(P[P_O1_OCT] - 2) * 12.0f + bendS + uni + v.dr[0] + vdet * v.vr[0];
    float s2 = v.p + (float)(P[P_O2_OCT] - 2) * 12.0f + pitchSemis(P[P_O2_PITCH]) + bendS - uni * 0.7f + v.dr[1] + vdet * v.vr[1];
    float k3 = P[P_O3_KEY] ? v.p : 60.0f;
    float s3 = k3 + osc3Semis(P[P_O3_OCT]) + pitchSemis(P[P_O3_PITCH]) + (P[P_O3_KEY] ? bendS + uni * 0.4f : 0.0f) + v.dr[2] +
               vdet * v.vr[2];

    /* the controls: aftertouch (bipolar amount) and the LFO's depth (the mod wheel raises it to the top) */
    float at = ctl[2] * bip(P[P_AT_AMT]);
    float lfoD = (float)P[P_LFO_AMT] / 255.0f;
    lfoD = clampf(lfoD + ctl[1] * (1.0f - lfoD) + (P[P_AT_LFO] ? at : 0.0f), 0.0f, 1.0f);
    float lfoSemi = 24.0f * lfoD * lfoD, lfoPw = 0.45f * lfoD, lfoOct = 5.0f * lfoD, lfoAmp = lfoD;
    float atSemi = 12.0f * at;
    s1 += P[P_AT_F1] ? atSemi : 0.0f;
    s2 += P[P_AT_F2] ? atSemi : 0.0f;
    s3 += P[P_AT_F3] ? atSemi : 0.0f;
    const float lf1 = P[P_LFO_F1] ? lfoSemi : 0.0f, lf2 = P[P_LFO_F2] ? lfoSemi : 0.0f, lf3 = P[P_LFO_F3] ? lfoSemi : 0.0f;
    const float lp1 = P[P_LFO_PW1] ? lfoPw : 0.0f, lp2 = P[P_LFO_PW2] ? lfoPw : 0.0f, lp3 = P[P_LFO_PW3] ? lfoPw : 0.0f;
    const float lfc = P[P_LFO_FILT] ? lfoOct : 0.0f, lfa = P[P_LFO_AMP] ? lfoAmp : 0.0f;
    /* POLY MOD: two sources (bipolar, on a curve: small amounts fine), their destinations */
    float a3 = bip(P[P_PM_OSC3]), ae = bip(P[P_PM_FENV]);
    a3 *= std::fabs(a3);
    ae *= std::fabs(ae);
    const float pf1 = P[P_PM_F1] ? 48.0f : 0.0f, pf2 = P[P_PM_F2] ? 48.0f : 0.0f, pf3 = P[P_PM_F3] ? 48.0f : 0.0f;
    const float pp1 = P[P_PM_PW1] ? 0.45f : 0.0f, pp2 = P[P_PM_PW2] ? 0.45f : 0.0f;
    const float pfc = P[P_PM_FILT] ? 6.0f : 0.0f, pfb = P[P_PM_FDBK] ? 0.9f : 0.0f;
    const bool pmOn = (a3 != 0.0f || ae != 0.0f) && (pf1 + pf2 + pf3 + pp1 + pp2 + pfc + pfb) > 0.0f;

    /* the waves */
    Wave w1, w2, w3;
    float pwb[3] = {(float)P[P_O1_PW] / 255.0f, (float)P[P_O2_PW] / 255.0f, (float)P[P_O3_PW] / 255.0f};
    for (int i = 0; i < 3; i++)
        pwb[i] = 0.5f + (pwb[i] - 0.5f) * 0.92f + 0.05f * vint * v.vr[5];
    const int saw3 = P[P_O3_SAW] == 1 ? 1 : P[P_O3_SAW] == 2 ? -1 : 0;
    const float n1 = (float)(P[P_O1_TRI] + P[P_O1_SAW] + P[P_O1_PULSE]);
    const float n2 = (float)(P[P_O2_TRI] + P[P_O2_SAW] + P[P_O2_PULSE]);
    const float n3 = (float)(P[P_O3_TRI] + (saw3 != 0) + P[P_O3_PULSE]);
    /* (several waves at once: louder, not as loud as their sum) */
    auto waveG = [](float n) { return n <= 1.0f ? 1.0f : 1.0f / std::sqrt(n * 0.8f); };

    /* the envelopes' rates this chunk */
    float fet, aet;
    bool fea, aea;
    float fek = envTarget(v.fe, P_F_ATT, tsc, &fet, &fea);
    float aek = envTarget(v.ae, P_A_ATT, tsc, &aet, &aea);

    /* the filter */
    int ft = P[P_F_TYPE];
    float vel = v.vel;
    float famt = bip(P[P_F_ENV_AMT]) * 8.0f * (P[P_F_VEL] ? 0.25f + 0.75f * vel : 1.0f);
    float ktr = P[P_F_KEY] == 2 ? 1.0f : P[P_F_KEY] == 1 ? 0.5f : 0.0f;
    float cutOct = (float)P[P_CUTOFF] * (10.0f / 164.0f) + ktr * (v.p - 60.0f) / 12.0f + 0.35f * vint * v.vr[3] +
                   (P[P_AT_FILT] ? 4.0f * at : 0.0f);
    float reso = (float)P[P_RESO] / 255.0f;
    /* FDBK <> DRIVE */
    int fd = P[P_FDBK_DRIVE];
    float fbAmt = fd < 127 ? (float)(127 - fd) / 127.0f : 0.0f, drive = fd > 127 ? (float)(fd - 127) / 127.0f : 0.0f;
    /* the amplifier */
    float avs = P[P_A_VEL] ? 0.15f + 0.85f * vel : 1.0f;
    if (P[P_AT_AMP])
        avs *= clampf(1.0f + at, 0.0f, 2.0f);
    float pgm = (float)P[P_PGM_VOL] / 100.0f * VOICE_GAIN * (v.key == K_BASS ? v.gmul : 1.0f);
    if ((v.key == K_UNI || (v.key >= K_ARP && v.key != K_BASS && P[P_UNISON])) && uniN > 1)
        pgm *= 1.0f / std::sqrt((float)uniN);
    float pan = clampf(v.panSlot * (float)P[P_PAN_SPREAD] / 127.0f, -1.0f, 1.0f);

    float tgt[NSM];
    tgt[SM_G1] = 0.5f * levelG(P[P_O1_LEVEL]) * waveG(n1);
    tgt[SM_G2] = 0.5f * levelG(P[P_O2_LEVEL]) * waveG(n2);
    tgt[SM_G3] = 0.5f * levelG(P[P_O3_LEVEL]) * waveG(n3);
    tgt[SM_GN] = 0.55f * std::pow((float)P[P_NOISE] / 127.0f, 2.0f) * std::sqrt(sr / 44100.0f);
    tgt[SM_CUT] = cutOct;
    tgt[SM_ENV] = famt;
    tgt[SM_RES] = reso;
    tgt[SM_FB] = fbAmt;
    tgt[SM_DRV] = 1.0f + 5.0f * drive * std::sqrt(drive);
    tgt[SM_AMP] = avs;
    tgt[SM_PGM] = pgm;
    tgt[SM_PAN] = pan;
    tgt[SM_PW1] = pwb[0];
    tgt[SM_PW2] = pwb[1];
    tgt[SM_PW3] = pwb[2];
    tgt[SM_MODE] = (float)P[P_SVF_MODE] / 127.0f;
    if (!v.smOk) {
        for (int i = 0; i < NSM; i++)
            v.sm[i] = tgt[i];
        v.smOk = true;
    }
    float cur[NSM], stp[NSM];
    const float inv = 1.0f / (float)CH;
    for (int i = 0; i < NSM; i++) {
        cur[i] = v.sm[i];
        stp[i] = (tgt[i] - v.sm[i]) * inv;
        v.sm[i] = tgt[i];
    }
    /* the base increments (the oscillators' Hz at their base pitch; modulation multiplies per sample) */
    const float inc1 = hzOf(s1) * isr, inc2 = hzOf(s2) * isr, inc3 = hzOf(s3) * isr;
    const float steal = 1.0f / (0.004f * sr);
    int os = quality ? 4 : 2;
    if (sr > 80000.0f)
        os /= 2;
    if (sr > 160000.0f)
        os = 1;
    const float fos = sr * (float)os, wmax = 0.42f * fos, wk = 2.0f * PI / fos;
    const bool sync = P[P_SYNC] != 0, pole2 = P[P_POLE2] != 0;
    const bool on1 = n1 > 0.0f, on2 = n2 > 0.0f;
    const float kickAmt = 0.8f * reso * reso * reso, kickDecay = std::exp(-1.0f / (0.0012f * sr));

    /* the three filters, one oversampled step each:
     *   LADDER (LP2): four transistor-pair stages, each saturating on its input and its own charge (Huovilainen's),
     *     the resonance from the last stage, averaged over two samples; 2-POLE taps the second stage (the loop stays
     *     the four's: it sings, and at high resonance turns half band-pass, the Trigon's). Part of the bass the
     *     resonance takes given back.
     *   LP1 OTA: a Curtis-style 4-pole, four OTA stages each saturating (REEVES').
     *   SVF: a 2-pole state variable of OTAs, its band saturating; MODE from low-pass through band-pass to high-pass */
    auto ladder = [&v, pole2](float x, float g, float k) {
        float fb = 0.5f * (v.f[3] + v.ffb);
        v.ffb = v.f[3];
        float u = softT(x - k * fb);
        float t0 = softT(v.f[0]), t1 = softT(v.f[1]), t2 = softT(v.f[2]), t3 = softT(v.f[3]);
        v.f[0] += g * (u - t0);
        v.f[1] += g * (t0 - t1);
        v.f[2] += g * (t1 - t2);
        v.f[3] += g * (t2 - t3);
        return pole2 ? v.f[1] : v.f[3];
    };
    auto otaLp = [&v](float x, float g, float k) {
        float fb = 0.5f * (v.f[3] + v.ffb);
        v.ffb = v.f[3];
        float u = sat(x - 4.0f * k * fb);
        v.f[0] += g * ota(u - v.f[0]);
        v.f[1] += g * ota(v.f[0] - v.f[1]);
        v.f[2] += g * ota(v.f[1] - v.f[2]);
        v.f[3] += g * ota(v.f[2] - v.f[3]);
        return v.f[3];
    };
    auto svf = [&v](float x, float g, float kd, float m) {
        float a1 = 1.0f / (1.0f + g * (g + kd)), a2 = g * a1;
        float in = sat(x);
        float v3 = in - v.f[1], v1 = a1 * v.f[0] + a2 * v3, v2 = v.f[1] + g * v1;
        v.f[0] = 3.0f * softT((2.0f * v1 - v.f[0]) * (1.0f / 3.0f));
        v.f[1] = 2.0f * v2 - v.f[1];
        float lp = v2, bp = v1, hp = in - kd * v1 - v2;
        return m < 0.5f ? lp + (bp * 1.4f - lp) * (2.0f * m) : bp * 1.4f + (hp - bp * 1.4f) * (2.0f * m - 1.0f);
    };

    for (int s = 0; s < CH; s++) {
        for (int i = 0; i < NSM; i++)
            cur[i] += stp[i];
        /* the envelopes */
        v.fe.v = fea ? minf(v.fe.v + (fet - v.fe.v) * fek, 1.0f) : v.fe.v + (fet - v.fe.v) * fek;
        v.ae.v = aea ? minf(v.ae.v + (aet - v.ae.v) * aek, 1.0f) : v.ae.v + (aet - v.ae.v) * aek;
        if (fea && v.fe.v >= 0.9999f) {
            v.fe.st = ES_DEC;
            fek = envTarget(v.fe, P_F_ATT, tsc, &fet, &fea);
        }
        if (aea && v.ae.v >= 0.9999f) {
            v.ae.st = ES_DEC;
            aek = envTarget(v.ae, P_A_ATT, tsc, &aet, &aea);
        }
        float ae = v.ae.v;
        if (v.fade < 1.0f) {                     /* taken: out on a smooth curve, then the note it was taken for */
            v.fade -= steal;
            if (v.fade <= 0.0f) {
                v.fade = 1.0f;
                v.fe = Env();
                v.ae = Env();
                if (v.pend) {
                    v.pend = false;
                    v.on = false;
                    startVoice(v, v.pkey, v.pnote, v.pvel, v.pdet, true);
                    fek = envTarget(v.fe, P_F_ATT, tsc, &fet, &fea);
                    aek = envTarget(v.ae, P_A_ATT, tsc, &aet, &aea);
                } else {
                    v.on = false;
                    v.key = -1;
                }
                continue;
            }
            float f = v.fade;
            ae *= f * f * (3.0f - 2.0f * f);
        }
        const float lfo = lfoBuf[s];
        const float fenv = v.fe.v;
        /* osc 3 first: it is a source */
        float pm = pmOn ? a3 * v.o3last + ae * fenv : 0.0f;
        int wr = 0;
        float wd = 0.0f;
        float m3 = lf3 * lfo + pf3 * pm;
        w3.set3(P[P_O3_TRI] != 0, saw3, P[P_O3_PULSE] != 0, cur[SM_PW3] + lp3 * lfo);
        float i3 = m3 != 0.0f ? inc3 * fexp2(m3 * (1.0f / 12.0f)) : inc3;
        float o3 = v.o[2].tick(tab, w3, minf(i3, 0.45f), -1.0f, wr, wd);
        v.o3last = o3 * (n3 > 1.0f ? 1.0f / n3 : 1.0f);
        if (pmOn)
            pm = a3 * v.o3last + ae * fenv;
        /* osc 1, then osc 2 (synced to it) */
        float m1 = lf1 * lfo + pf1 * pm, m2 = lf2 * lfo + pf2 * pm;
        w1.set3(P[P_O1_TRI] != 0, P[P_O1_SAW], P[P_O1_PULSE] != 0, cur[SM_PW1] + lp1 * lfo + pp1 * pm);
        w2.set3(P[P_O2_TRI] != 0, P[P_O2_SAW], P[P_O2_PULSE] != 0, cur[SM_PW2] + lp2 * lfo + pp2 * pm);
        float i1 = m1 != 0.0f ? inc1 * fexp2(m1 * (1.0f / 12.0f)) : inc1;
        float i2 = m2 != 0.0f ? inc2 * fexp2(m2 * (1.0f / 12.0f)) : inc2;
        int wr1 = 0;
        float wd1 = 0.0f;
        float o1 = v.o[0].tick(tab, w1, minf(i1, 0.45f), -1.0f, wr1, wd1);
        float o2 = v.o[1].tick(tab, w2, minf(i2, 0.45f), sync && wr1 ? wd1 : -1.0f, wr, wd);
        if (!on1)
            o1 = 0.0f;
        if (!on2)
            o2 = 0.0f;
        float x = cur[SM_G1] * o1 + cur[SM_G2] * o2 + cur[SM_G3] * o3;
        x += (cur[SM_GN] + 2e-5f) * rndF(v.seed);   /* (and the circuit's own hiss) */
        if (v.kick > 1e-4f) {                    /* the gate's thump: a ladder near its singing starts at once (the
                                                  * Trigon's filter drums: KICK IT, OLD TOMS) */
            x += v.kick * kickAmt;
            v.kick *= kickDecay;
        }
        /* FDBK: the voice's output back in (saturating); DRIVE: harder into the filter */
        float fbk = clampf(cur[SM_FB] + pfb * pm, 0.0f, 1.2f);
        x = x * cur[SM_DRV] + fbk * 1.7f * softT(v.fbOut * 1.4f);
        /* the cutoff, per sample (the envelope, the LFO, poly mod at audio rate) */
        float oct = cur[SM_CUT] + cur[SM_ENV] * fenv + lfc * lfo + pfc * pm;
        float hz = minf(20.0f * fexp2(clampf(oct, -1.5f, 10.6f)), wmax);
        float res = cur[SM_RES], y;
        if (ft == FT_LADDER) {
            float g = 1.0f - fexp2(-wk * hz * 1.4427f * 1.08f);
            float k = 4.3f * res;
            float xi = x * 0.85f;
            if (os == 4) {
                float a, b, a0, a1, b0, b1;
                v.os1.up(xi, a, b);
                v.os2.up(a, a0, a1);
                v.os2.up(b, b0, b1);
                a0 = ladder(a0, g, k);
                a1 = ladder(a1, g, k);
                b0 = ladder(b0, g, k);
                b1 = ladder(b1, g, k);
                y = v.os1.down(v.os2.down(a0, a1), v.os2.down(b0, b1));
            } else if (os == 2) {
                float a, b;
                v.os1.up(xi, a, b);
                y = v.os1.down(ladder(a, g, k), ladder(b, g, k));
            } else
                y = ladder(xi, g, k);
            y *= (1.0f + 0.18f * k) * 1.2f;
        } else if (ft == FT_OTA) {
            float g = 1.0f - fexp2(-wk * hz * 1.4427f * 1.06f);
            float k = res * 1.22f;
            if (os == 4) {
                float a, b, a0, a1, b0, b1;
                v.os1.up(x, a, b);
                v.os2.up(a, a0, a1);
                v.os2.up(b, b0, b1);
                a0 = otaLp(a0, g, k);
                a1 = otaLp(a1, g, k);
                b0 = otaLp(b0, g, k);
                b1 = otaLp(b1, g, k);
                y = v.os1.down(v.os2.down(a0, a1), v.os2.down(b0, b1));
            } else if (os == 2) {
                float a, b;
                v.os1.up(x, a, b);
                y = v.os1.down(otaLp(a, g, k), otaLp(b, g, k));
            } else
                y = otaLp(x, g, k);
            y *= 1.0f + 0.45f * k;
        } else {
            float g = tanW(minf(PI * hz / fos, 1.2f)), kd = 2.0f - 2.12f * std::pow(res, 0.8f), m = cur[SM_MODE];
            if (os == 4) {
                float a, b, a0, a1, b0, b1;
                v.os1.up(x, a, b);
                v.os2.up(a, a0, a1);
                v.os2.up(b, b0, b1);
                a0 = svf(a0, g, kd, m);
                a1 = svf(a1, g, kd, m);
                b0 = svf(b0, g, kd, m);
                b1 = svf(b1, g, kd, m);
                y = v.os1.down(v.os2.down(a0, a1), v.os2.down(b0, b1));
            } else if (os == 2) {
                float a, b;
                v.os1.up(x, a, b);
                y = v.os1.down(svf(a, g, kd, m), svf(b, g, kd, m));
            } else
                y = svf(x, g, kd, m);
            y *= 1.0f / (1.0f + 0.5f * res);
        }
        /* the amplifier (the LFO's AMP: a tremolo down from the top) */
        y *= 1.0f / (1.0f + 0.12f * (cur[SM_DRV] - 1.0f));   /* (driven: more saturated, not much louder) */
        float amp = ae * cur[SM_AMP] * (1.0f - lfa * 0.5f * (1.0f - lfo));
        float out = y * amp;
        v.fbOut = out;
        out *= cur[SM_PGM];
        float p0 = cur[SM_PAN];
        float gl = minf(1.0f, 1.0f - p0), gr = minf(1.0f, 1.0f + p0);
        Lo[s] += out * gl;
        Ro[s] += out * gr;
    }
    /* finished: its amp envelope at its end, its key let go */
    if (v.ae.st == ES_REL && v.ae.v < 3.2e-4f && !v.pend) {
        v.fe = Env();
        v.ae = Env();
        v.on = false;
    }
    for (float &f : v.f)
        f = flushDn(f);
    v.ffb = flushDn(v.ffb);
    v.fbOut = flushDn(v.fbOut);
    v.os1.flush();
    v.os2.flush();
}

/* ------------------------------------------------------------------ render --- */
void Engine::chunk()
{
    const int16_t *P = prog.p;
    const float dt = (float)CH / sr;
    {
        const float in[3] = {bendV, mwV, prV}, kc = 1.0f - std::exp(-dt / 0.005f);
        for (int i = 0; i < 3; i++)
            ctl[i] = flushDn(ctl[i] + (in[i] - ctl[i]) * kc);
    }
    clockChunk();
    /* the LFO, per sample (it runs to audio rate) */
    {
        float hz;
        bool synced = P[P_LFO_SYNC] != 0;
        if (synced) {
            int i = std::min(15, std::max(0, P[P_LFO_FREQ] * 16 / 256));
            hz = bpmNow / 60.0f / LFO_SYNC_Q[i];
        } else
            hz = lfoHzOf(P[P_LFO_FREQ]);
        float inc = hz / sr;
        int sh = P[P_LFO_SHAPE];
        for (int s = 0; s < CH; s++) {
            lfoPh += inc;
            if (lfoPh >= 1.0f) {
                lfoPh -= std::floor(lfoPh);
                lfoSh = rndF(lfoSeed);
            }
            float ph = lfoPh, y;
            switch (sh) {
            case LS_TRI: y = ph < 0.25f ? 4.0f * ph : ph < 0.75f ? 2.0f - 4.0f * ph : 4.0f * ph - 4.0f; break;
            case LS_SAW: y = 2.0f * ph - 1.0f; break;
            case LS_REVSAW: y = 1.0f - 2.0f * ph; break;
            case LS_SQUARE: y = ph < 0.5f ? 1.0f : -1.0f; break;
            default: y = lfoSh;
            }
            lfoBuf[s] = y;
        }
        lfoShow = lfoBuf[CH - 1];
    }
    /* the voices */
    float bl[CH] = {}, br[CH] = {};
    float trackLo = 1e9f;
    for (int i = 0; i <= MAXV; i++) {
        Voice &v = V[i];
        if (!v.on && !v.pend)
            continue;
        voiceChunk(v, bl, br);
        if (v.on && v.gate)
            trackLo = minf(trackLo, (float)v.note);
    }
    /* the DC let go (8 Hz) */
    for (int i = 0; i < CH; i++) {
        float xl = bl[i], xr = br[i];
        dcL = xl - dcxL + dcK * dcL;
        dcR = xr - dcxR + dcK * dcR;
        dcxL = xl;
        dcxR = xr;
        bl[i] = dcL;
        br[i] = dcR;
    }
    dcL = flushDn(dcL);
    dcR = flushDn(dcR);
    /* the stereo analog distortion: a diode pair's curve (a little uneven: its even harmonics), the tone of a small
     * amp, as loud in as out */
    if (P[P_DISTORTION] > 0) {
        float a = (float)P[P_DISTORTION] / 127.0f;
        float g = 1.0f + 40.0f * a * a, wet = minf(1.0f, a * 10.0f), mk = 1.0f / (1.0f + 0.55f * std::sqrt(g - 1.0f));
        float k = 1.0f - std::exp(-2.0f * PI * (7000.0f - 3500.0f * a) / sr);
        const float bias = 0.18f, b0 = softT(bias);
        for (int i = 0; i < CH; i++)
            for (int c = 0; c < 2; c++) {
                float &x = c ? br[i] : bl[i];
                float y = (softT(x * g * 1.6f + bias) - b0) * mk * 0.85f;
                distLp[c] += (y - distLp[c]) * k;
                x = x * (1.0f - wet) + distLp[c] * wet;
            }
    }
    /* the effects: A, then B (the bypass: both out) */
    if (P[P_FX_ON]) {
        float at = ctl[2] * bip(P[P_AT_AMT]) * 127.0f;
        float tr = trackLo < 1e8f ? hzOf(trackLo) : 0.0f;
        fxA.set(P[P_FXA_TYPE], (float)P[P_FXA_MIX] + (P[P_AT_FXA] ? at : 0.0f), (float)P[P_FXA_P1], (float)P[P_FXA_P2],
                P[P_FXA_SYNC] != 0, P[P_FXA_DIV], bpmNow, tr);
        fxB.set(P[P_FXB_TYPE], (float)P[P_FXB_MIX] + (P[P_AT_FXB] ? at : 0.0f), (float)P[P_FXB_P1], (float)P[P_FXB_P2],
                P[P_FXB_SYNC] != 0, P[P_FXB_DIV], bpmNow, tr);
    } else {
        fxA.set(FX_OFF, 0.0f, 0.0f, 0.0f, false, 0, bpmNow, 0.0f);
        fxB.set(FX_OFF, 0.0f, 0.0f, 0.0f, false, 0, bpmNow, 0.0f);
    }
    fxA.process(bl, br, CH);
    fxB.process(bl, br, CH);
    /* the output stage: clean to full scale, then rounding off (a hot program never clips hard) */
    for (int i = 0; i < CH; i++) {
        auto stage = [](float x) {
            float a = std::fabs(x);
            return a <= 1.0f ? x : (x > 0.0f ? 1.0f : -1.0f) * (1.0f + 0.5f * softT((a - 1.0f) * 2.0f));
        };
        outL[i] = stage(bl[i]);
        outR[i] = stage(br[i]);
    }
}

void Engine::render(float *L, float *R, int n)
{
    while (n > 0) {
        if (gpos >= CH) {
            chunk();
            gpos = 0;
        }
        int k = std::min(CH - gpos, n);
        std::memcpy(L, outL + gpos, sizeof(float) * (size_t)k);
        std::memcpy(R, outR + gpos, sizeof(float) * (size_t)k);
        gpos += k;
        L += k;
        R += k;
        n -= k;
    }
}

int Engine::voicesSounding() const
{
    int n = 0;
    for (const auto &v : V)
        n += v.on && v.ae.st != ES_IDLE;
    return n;
}
} // namespace tg
