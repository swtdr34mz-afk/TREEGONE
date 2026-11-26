/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
/* TREEGONE as a plugin (VST3, AU on the Mac) and a standalone app: the engine (dsp/Engine.h), its parameters for the
 * host (automation, saved with the project), the programs as the host's programs: TREEGONE's own, then the Trigon
 * programs the user loaded from his own .syx files (kept in Documents/TREEGONE/Imported), then the user's own
 * (WRITE: Documents/TREEGONE/Programs). MIDI in (notes, velocity, pressure, pitch bend, mod wheel, sustain), the
 * host's tempo and position for the clock. The panel's buttons that act on the engine come through a small queue. */
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "Engine.h"
#include "Presets.h"

class TreegoneProcessor : public juce::AudioProcessor
{
public:
    TreegoneProcessor();
    ~TreegoneProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "TREEGONE"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    /* The host is told a fixed number of program slots: a VST3 host (Cubase) sizes its program parameter when the
     * plugin opens, so programs loaded later (a .syx, WRITE) past that size were clamped to its last slot (the
     * factory's Basic Program) and the program buttons stuck there. The slots past the real programs are empty. */
    static constexpr int HOST_PROGRAMS = 2048;
    int getNumPrograms() override { return HOST_PROGRAMS; }
    int programCount() const { return tg::NFACTORY + (int)imported.size() + (int)userPrograms.size(); }
    int getCurrentProgram() override { return program; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String &) override {}
    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::RangedAudioParameter *param(int p) const { return params[p]; }
    int value(int p) const;
    void setValue(int p, int v);

    enum Cmd { C_PLAY, C_RECORD, C_REST, C_TIE, C_BACK, C_HOLD, C_CHORD, C_PANIC };
    void command(Cmd c, int v = 0);

    /* what the engine shows */
    std::atomic<float> voiceLevel[tg::MAXV] = {};
    std::atomic<int> seqStepNow{-1}, recStepNow{0};
    std::atomic<bool> recording{false}, playingNow{false}, beatLed{false}, holdNow{false};
    std::atomic<float> bpmNow{120.0f}, lfoNow{0.0f};
    juce::MidiKeyboardState keyboardState;
    std::atomic<float> uiScale{0.9f};
    std::atomic<bool> uiScaleSet{false};
    std::atomic<int> fxEditB{0};                /* the effects' A/B: which the knobs edit */
    std::atomic<bool> splitLearn{false};        /* LOW SPLIT's SET: the next key played is the split point */
    std::atomic<int> learnedNote{-1};

    juce::String programName() const;
    void setProgramName(const juce::String &);
    std::array<int8_t, tg::NCHORD> chordCopy() const;
    tg::SeqData seqCopy() const;

    /* where a program comes from: 0 TREEGONE's, 1 a Trigon .syx, 2 the user's own */
    int sourceOf(int index) const;
    int firstOf(int source) const;              /* (-1: none) */
    int numberInSource(int index) const;
    juce::String trigonPlace(int index) const;  /* an imported program's bank and number on the Trigon ("0-07") */

    /* the user's Trigon programs: loaded from his .syx (kept in Documents/TREEGONE/Imported) */
    struct Imported {
        tg::Program prog;
        int bank, number;
        juce::String file;
    };
    std::vector<Imported> imported;
    static juce::File importFolder();
    void loadImported();
    int importSyx(const juce::File &f, juce::String &message);   /* the programs read (0: none) */
    void clearImported();

    std::vector<tg::Program> userPrograms;
    static juce::File programFolder();
    void loadUserPrograms();
    int writeProgram(const juce::String &name);
    tg::Program currentProgram() const;

private:
    tg::Engine engine;
    juce::RangedAudioParameter *params[tg::NPARAM] = {};
    int16_t sent[tg::NPARAM] = {};
    int sentVoices = -1;
    int program = 0;
    float lastGain = -1.0f;
    int transpose = 0;

    mutable juce::SpinLock progLock;
    tg::Program pending, live;
    std::atomic<bool> pendingSet{false};

    struct Command {
        int c, v;
    };
    juce::AbstractFifo fifo{256};
    std::array<Command, 256> cmds;

    const tg::Program &programAt(int index) const;
    void handleMidi(const juce::MidiMessage &m);
    void applyProgram(const tg::Program &p, bool toHost);
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static void programToXml(const tg::Program &p, juce::XmlElement &x);
    static bool programFromXml(const juce::XmlElement &x, tg::Program &p);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TreegoneProcessor)
};
