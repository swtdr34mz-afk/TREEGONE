/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
/* The editor: the Trigon-6's front panel without its keys, drawn at 2200 x 660 and scaled to the window (drag the
 * corner). Its sections as the Trigon has them: VOLUME and AFTERTOUCH, LFO and POLYMOD, OSCILLATORS, FILTER (with
 * TREEGONE's TYPE: LP1 OTA, LP2 LADDER, SVF and the SVF's MODE) and AMPLIFIER, CLOCK, EFFECTS, ARPEGGIATOR,
 * SEQUENCER; under them TRANSPOSE, HOLD, GLIDE, UNISON, the program display with BANK, TENS and the ten number
 * buttons, WRITE, GLOBALS, LOAD SYX, and DISTORTION, VINTAGE, PAN SPREAD, PGM VOLUME, P WHL RANGE, KEY MODE.
 *
 * The red displays work as the Trigon's: the program's number, the value of a knob while it turns, the tempo, the
 * effects. A line beside the program display (TREEGONE's) spells out the program's name and what was touched. A
 * Trigon .syx file dropped on the panel (or LOAD SYX) brings its programs in. */
#pragma once
#include "PluginProcessor.h"
#include "ui/Look.h"

class TreegonePanel : public juce::Component, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
    static constexpr int W = 2200, H = 660;
    explicit TreegonePanel(TreegoneProcessor &);
    ~TreegonePanel() override;
    void paint(juce::Graphics &) override;

    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void filesDropped(const juce::StringArray &files, int, int) override;

    TreegoneProcessor &proc;

    /* what the displays show */
    int shownParam = -1;                        /* a parameter in hand (-1: none) */
    juce::RangedAudioParameter *shownGlobal = nullptr;
    double shownAt = 0.0;
    juce::String message;                       /* a line of news (a .syx loaded) */
    double messageAt = 0.0;
    void touch(int p);
    void touchGlobal(juce::RangedAudioParameter *);
    void say(const juce::String &m);
    bool showingValue() const;

    void programMenu(juce::Rectangle<int> screen);
    void globalsMenu(juce::Rectangle<int> screen);
    void choiceMenu(int p, juce::Rectangle<int> screen);
    void writeDialog();
    void loadSyx();
    void selectDigit(int d);
    void stepSource();
    void stepBank();
    void stepTens();

    struct Live {
        virtual ~Live() = default;
        virtual void poll() = 0;
    };

private:
    void timerCallback() override;
    void build();
    void paintBackground(juce::Graphics &, float sc);

    rvui::Look look;
    std::vector<std::unique_ptr<juce::Component>> parts;
    std::vector<Live *> live;
    juce::Image background;
    float backgroundScale = 0.0f;
    std::vector<double> taps;
    juce::Component::SafePointer<juce::AlertWindow> dialog;
    std::unique_ptr<juce::FileChooser> chooser;
    template <class T, class... A> T *add(A &&...a);
};

class TreegoneEditor : public juce::AudioProcessorEditor
{
public:
    explicit TreegoneEditor(TreegoneProcessor &);
    void resized() override;

private:
    TreegoneProcessor &proc;
    juce::SharedResourcePointer<rvui::UiResources> resources;
    TreegonePanel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TreegoneEditor)
};
