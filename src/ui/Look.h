/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
/* The look of a Trigon-6 (from REEVES' parts): a black powder-coated panel with white legends and white section frames
 * (the name set in the frame's top line), black knurled knobs with a chrome ring, a dark cap and a white line, white
 * scale ticks around them, small dark buttons that light red, small red LEDs, red seven-segment displays, maple end
 * cheeks. The fonts are embedded (Fira Sans, Anonymous Pro: SIL OFL, fonts/). */
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>

namespace rvui
{
namespace col
{
const juce::Colour panelTop{0xff19191b}, panelBottom{0xff111112}, frame{0xffe9e8e4}, legend{0xfff4f3ef},
    dim{0xff9a9a96}, redLed{0xffff2d1c}, oledBg{0xff030406}, oled{0xffd8ecff}, oledDim{0xff5b6b7a};
}

enum class Face { label, title, logo, mono };

/* the fonts and the tag: they live while an editor is open (a SharedResourcePointer in each) */
struct UiResources
{
    UiResources() = default;
    ~UiResources();
};
juce::Font font(Face f, float height);
float textWidth(const juce::Font &, const juce::String &);

class Look : public juce::LookAndFeel_V4
{
public:
    Look();
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font &) override;
    juce::Font getPopupMenuFont() override { return font(Face::label, 15.0f); }
    void drawPopupMenuBackground(juce::Graphics &, int w, int h) override;
};

/* a knob in r (square): pos 0..1; bipolar: '-' and '+' at the ends of its scale */
void drawKnob(juce::Graphics &, juce::Rectangle<float> r, float pos, bool bipolar, bool endless = false, bool pointer = true);
/* its white line alone (a knob's body is drawn once and kept: only the line moves) */
void drawKnobPointer(juce::Graphics &, juce::Rectangle<float> r, float pos, bool endless);
/* a Rev2 button: dark with a fine grid, lit red when on */
void drawButton(juce::Graphics &, juce::Rectangle<float> r, bool on, bool down, bool over);
/* a small square LED */
void drawLed(juce::Graphics &, juce::Point<float> c, float size, float level);
void drawSection(juce::Graphics &, juce::Rectangle<float> r, const juce::String &title);
void drawLegend(juce::Graphics &, const juce::String &, juce::Rectangle<float>, float size = 12.0f,
                juce::Justification j = juce::Justification::centred);
/* the panel: black steel under a fine stippled powder coat (sc: the pixels a panel unit, for the coat's density) */
void drawPanelFace(juce::Graphics &, juce::Rectangle<float> r, float sc);
void drawPanelEdge(juce::Graphics &, juce::Rectangle<float> r);
/* a few years of use over the print (px: a screen pixel in panel units; played: the knobs worn round most) */
void drawWear(juce::Graphics &, juce::Rectangle<float> r, float px, const std::vector<juce::Point<float>> &played);
void drawScrew(juce::Graphics &, juce::Point<float> c, float r);
void drawWood(juce::Graphics &, juce::Rectangle<float> r, int seed);
/* the Trigon's maple end cheeks: pale, a fine straight grain, lacquered */
void drawMaple(juce::Graphics &, juce::Rectangle<float> r, int seed);
/* a red seven-segment display of n characters (its dark glass, the unlit segments faintly there, a dot after a char
 * when the text has a '.') */
void drawSeg7(juce::Graphics &, juce::Rectangle<float> r, const juce::String &text, int n, float level = 1.0f);
void drawTag(juce::Graphics &, juce::Rectangle<float> r);
} // namespace rvui
