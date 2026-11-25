// SPDX-License-Identifier: MPL-2.0
// Copyright (C) 2026 swtdr34mz (TREEGONE)
//
// TREEGONE for the MPC: the same engine as the VST3 and AU (dsp/: Engine, Params, Presets), hosted by the MPC Plugin
// SDK's framework (mpc_plugin.h). mpc/build.py copies the engine's files beside this one before the SDK builds it.
//
// The sound: 6 voices (or 12), three oscillators each with sync, osc 3's LO range, noise, FDBK <> DRIVE, the three
// filters (LP1 OTA 4-pole, LP2 ladder 4/2-pole, SVF 2-pole OTA) at 4x HIGH or 2x ECO, two ADSR, the LFO, aftertouch,
// poly mod, distortion, two effects, glide, unison with chord memory, arp and the poly sequencer; on the MPC's tempo,
// locked to its bar while it plays.
//
// Parameters (their ids never change; new ones only at the end):
//   0           VOLUME (the host nudges parameter 0 on every prepare: harmless here, it ramps)
//   1..102      the program's 102 parameters, in dsp/Params.h's order
//   103         VOICES (6 or 12)
//   104         QUALITY (ECO: the filters at 2x; HIGH: at 4x)
//   105         HOLD
// The program's sequence and chord memory ride in the state (saveExtraState); a program change brings its own.
//
// MIDI in: notes and velocity, pitch bend, channel and poly pressure, CC 1 mod wheel, CC 64 sustain, CC 120 all sound
// off, CC 121 reset controllers, CC 123 all notes off.

#include "mpc_plugin.h"
#include "Engine.h"
#include "Presets.h"
#include "treegone_names.h"

#include <cmath>
#include <cstring>

namespace {
using namespace tg;

enum {
    kVolume = 0,
    kP = 1,
    kVoices = kP + NPARAM,
    kQuality,
    kHold,
    kNumParams
};
static_assert(kNumParams == 106, "the parameters' order is permanent");
static_assert(sizeof(SHORT) / sizeof(SHORT[0]) == NPARAM, "a short name for each parameter");

class TreegoneMpc : public mpc::Plugin
{
public:
    TreegoneMpc() : mpc::Plugin(makeInfo())
    {
        addParam(mpc::floatParam("vol", "Volume", 0.0f, 1.0f, 0.7f, "", 2));
        for (int p = 0; p < NPARAM; p++) {
            const ParamDef &d = PARAMS[p];
            if (d.kind == K_CHOICE || d.kind == K_ONOFF) {
                std::vector<std::string> ch;
                for (int k = d.lo; k <= d.hi; k++)
                    ch.push_back(d.choice[k]);
                addParam(mpc::choiceParam(d.id, SHORT[p], ch, d.def - d.lo));
            } else {
                addParam(mpc::intParam(d.id, SHORT[p], d.lo, d.hi, d.def));
            }
        }
        addParam(mpc::choiceParam("voices", "Voices", {"6 Voice", "12 Voice"}, 0));
        addParam(mpc::choiceParam("quality", "Quality", {"ECO", "HIGH"}, 0));
        addParam(mpc::boolParam("hold", "Hold", false));

        /* the factory programs: VOLUME and the sound; VOICES, QUALITY and HOLD they leave as they are */
        for (int k = 0; k < NFACTORY; k++) {
            std::vector<float> v;
            v.push_back(0.7f);
            for (int p = 0; p < NPARAM; p++) {
                int val = p == P_SEQ_ON ? 0 : FACTORY[k].p[p];   /* (the sequencer waits for PLAY) */
                const ParamDef &d = PARAMS[p];
                v.push_back((float)(d.kind == K_CHOICE || d.kind == K_ONOFF ? val - d.lo : val));
            }
            addProgram(FACTORY[k].name, v);
        }
        engine.prepare(44100.0);         // (the MPC's rate: made here, on the UI thread, never in process)
        rate = 44100.0;
        cur = FACTORY[0];
    }

    void prepare(double sampleRate, int) override
    {
        if (sampleRate != rate) {
            engine.prepare(sampleRate);
            rate = sampleRate;
        }
        forceSend();
    }

    void reset() override
    {
        engine.panic();
        if (stagedReady.exchange(false)) {
            cur.seq = staged.seq;
            std::memcpy(cur.chord, staged.chord, sizeof cur.chord);
            std::memcpy(cur.name, staged.name, sizeof cur.name);
            lastProgram = currentProgram();
        } else if (currentProgram() != lastProgram) {
            int k = currentProgram();
            if (k >= 0 && k < NFACTORY) {
                cur.seq = FACTORY[k].seq;
                std::memcpy(cur.chord, FACTORY[k].chord, sizeof cur.chord);
                std::memcpy(cur.name, FACTORY[k].name, sizeof cur.name);
            }
            lastProgram = k;
        }
        forceSend();
    }

    void process(mpc::ProcessContext &ctx) override
    {
        const int n = ctx.numFrames;
        float *L = ctx.outputs[0], *R = ctx.numOutputs > 1 ? ctx.outputs[1] : ctx.outputs[0];
        sync();
        double bpm = ctx.transport.valid && ctx.transport.tempoValid ? ctx.transport.bpm : 120.0;
        bool playing = ctx.transport.valid && ctx.transport.playing && ctx.transport.ppqValid;
        engine.host(playing, playing ? ctx.transport.ppqPos : 0.0, bpm, true);
        int done = 0;
        for (int i = 0; i < ctx.numMidiEvents; i++) {
            const mpc::MidiEvent &e = ctx.midi[i];
            int at = mpc::clamp(e.sampleOffset, 0, n);
            if (at > done) {
                engine.render(L + done, R + done, at - done);
                done = at;
            }
            midi(e);
        }
        if (n > done)
            engine.render(L + done, R + done, n - done);
        float v = (float)get(kVolume), g = v * v * 2.0f;
        if (lastGain < 0.0f)
            lastGain = g;
        float step = (g - lastGain) / (float)(n > 0 ? n : 1), gg = lastGain;
        for (int s = 0; s < n; s++) {
            gg += step;
            L[s] *= gg;
            if (R != L)
                R[s] *= gg;
        }
        lastGain = g;
    }

    bool formatValue(int index, float plain, std::string &out) override
    {
        int v = (int)std::lround(plain);
        char t[40];
        if (index == kVolume) {
            if (plain <= 0.0001f)
                out = "-inf dB";
            else {
                std::snprintf(t, sizeof t, "%.1f dB", 20.0 * std::log10(plain * plain * 2.0));
                out = t;
            }
            return true;
        }
        if (index >= kP && index < kVoices) {
            int p = index - kP;
            const ParamDef &d = PARAMS[p];
            if (d.kind == K_CHOICE || d.kind == K_ONOFF)
                v += d.lo;
            v = mpc::clamp(v, d.lo, d.hi);
            paramText(p, v, t, sizeof t, p == P_LFO_FREQ && getInt(kP + P_LFO_SYNC) != 0);
            out = t;
            if (out.size() > 12)
                out.resize(12);
            return true;
        }
        return false;
    }

    void saveExtraState(std::vector<uint8_t> &out) override
    {
        const uint8_t magic[4] = {'T', 'G', 'N', '1'};
        out.insert(out.end(), magic, magic + 4);
        auto put = [&](const void *p, size_t n) { out.insert(out.end(), (const uint8_t *)p, (const uint8_t *)p + n); };
        put(&cur.seq, sizeof cur.seq);
        put(cur.chord, sizeof cur.chord);
        put(cur.name, sizeof cur.name);
    }
    void loadExtraState(const uint8_t *data, size_t size) override
    {
        if (size != 4 + sizeof cur.seq + sizeof cur.chord + sizeof cur.name || std::memcmp(data, "TGN1", 4) != 0)
            return;
        const uint8_t *p = data + 4;
        std::memcpy(&staged.seq, p, sizeof staged.seq);
        p += sizeof staged.seq;
        std::memcpy(staged.chord, p, sizeof staged.chord);
        p += sizeof staged.chord;
        std::memcpy(staged.name, p, sizeof staged.name);
        staged.name[NAME_LEN - 1] = 0;
        if (staged.seq.length > PSEQ_STEPS)
            staged.seq.length = PSEQ_STEPS;
        stagedReady.store(true);
    }

private:
    static mpc::PluginInfo makeInfo()
    {
        mpc::PluginInfo i;
        i.name = "TREEGONE";
        i.vendor = "swtdr34mz";
        i.uniqueId = MPC_FOURCC('T', 'g', 'n', '6');
        i.version = 1000;
        i.isInstrument = true;
        i.numInputs = 0;
        i.numOutputs = 2;
        i.tailSamples = 44100 * 10;
        return i;
    }

    void forceSend()
    {
        programSent = false;
        lastGain = -1.0f;
    }

    int value(int p) const
    {
        const ParamDef &d = PARAMS[p];
        int v = getInt(kP + p);
        if (d.kind == K_CHOICE || d.kind == K_ONOFF)
            v += d.lo;
        return mpc::clamp(v, d.lo, d.hi);
    }

    void sync()
    {
        if (!programSent) {
            for (int p = 0; p < NPARAM; p++)
                cur.p[p] = (int16_t)value(p);
            engine.setProgram(cur);
            for (int p = 0; p < NPARAM; p++)
                sent[p] = engine.program().p[p];
            programSent = true;
        }
        for (int p = 0; p < NPARAM; p++) {
            int v = value(p);
            if (v != sent[p]) {
                if (p == P_SEQ_ON)
                    engine.seqPlay(v != 0);
                engine.setParam(p, v);
                sent[p] = (int16_t)v;
                cur.p[p] = (int16_t)v;
            }
        }
        engine.setVoices(getInt(kVoices) ? 12 : 6);
        engine.setQuality(getInt(kQuality));
        bool hold = getBool(kHold);
        if (hold != holdSent) {
            engine.hold(hold);
            holdSent = hold;
        }
    }

    void midi(const mpc::MidiEvent &e)
    {
        const uint8_t st = e.status();
        if (e.isNoteOn())
            engine.noteOn(e.note(), (float)e.velocity() / 127.0f);
        else if (e.isNoteOff())
            engine.noteOff(e.note());
        else if (e.isPitchBend())
            engine.bend((float)e.pitchBend() / 8192.0f);
        else if (st == 0xA0)
            engine.pressure((float)e.data[2] / 127.0f);
        else if (e.isAftertouch())
            engine.pressure((float)e.data[1] / 127.0f);
        else if (e.isController()) {
            int cc = e.controller();
            float v = (float)e.controllerValue() / 127.0f;
            switch (cc) {
            case 1: engine.modWheel(v); break;
            case 64: engine.sustain(e.controllerValue() >= 64); break;
            case 120: engine.panic(); break;
            case 123: engine.allOff(); break;
            case 121:
                engine.bend(0.0f);
                engine.modWheel(0.0f);
                engine.pressure(0.0f);
                engine.sustain(false);
                break;
            default: break;
            }
        }
    }

    Engine engine;
    double rate = 0.0;
    Program cur, staged;
    std::atomic<bool> stagedReady{false};
    int lastProgram = 0;
    int16_t sent[NPARAM] = {};
    bool programSent = false, holdSent = false;
    float lastGain = -1.0f;
};

} // namespace

MPC_PLUGIN_ENTRY(TreegoneMpc)
