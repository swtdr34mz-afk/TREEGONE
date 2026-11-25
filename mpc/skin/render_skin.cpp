/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
/* The MPC skin's pictures: six pages laid out as the Trigon-6's panel sections are (the function keys):
 *
 *   OSC      OSCILLATORS 1 2 3 (the octave LEDs round their knobs, the waves' buttons, osc 3's saw / rev saw)
 *   FLT/AMP  FILTER (with TREEGONE's TYPE and SVF MODE) / AMPLIFIER
 *   LFO/PM   LFO / POLYMOD
 *   FX       EFFECTS: ON/OFF, the A and B displays, A/B, TYPE / EFFECT PARAMETERS (1, its display, 2, SYNC, DRY<>WET)
 *   AT/ARP   AFTER TOUCH | ARPEGGIATOR / CLOCK
 *   PLAY     the bottom strip (HOLD, GLIDE RATE, GLIDE, UNISON, DISTORTION, VINTAGE, PAN SPREAD, PGM VOLUME, P WHL
 *            RANGE, KEY MODE) / GLIDE, UNISON, VOICES / LOW SPLIT
 *
 * TREEGONE's own art (its panel coat, buttons, LEDs, displays, legends), its pots: black, a white dot, a white scale.
 * The panel with what never changes (the coat, the titles, the lines, the legends, the
 * pots' scales) is one picture; each control is cut out at every value it can take: a pot's film strip, a button's
 * look at each value, an LED's lit and unlit look, a display's film strip. manifest.json says what each is, for
 * make_skin.py.
 *
 *   treegone_skin_render <out folder> [--preview] */
#include "Params.h"
#include "ui/Look.h"
#include <juce_gui_basics/juce_gui_basics.h>

using namespace juce;
using namespace rvui;
using namespace tg;

namespace {
constexpr int TW = 1280, TH = 628;
const Colour INK(0xffe9e9e9), LINE(0xffbdbdbd), VALUE(0xffff3a22);
const String FXSEL = "fxsel";            /* the FX page's A/B: the plugin's FX EDIT */

int paramIndex(const String &id)
{
    for (int i = 0; i < NPARAM; i++)
        if (id == PARAMS[i].id)
            return i;
    return -1;
}
struct Range {
    int lo, hi;
    StringArray names;
};
Range rangeOf(const String &id)
{
    int p = paramIndex(id);
    if (p >= 0) {
        const ParamDef &d = PARAMS[p];
        Range r{d.lo, d.hi, {}};
        if (d.kind == K_CHOICE || d.kind == K_ONOFF)
            for (int k = d.lo; k <= d.hi; k++)
                r.names.add(d.choice[k]);
        return r;
    }
    if (id == "voices")
        return {0, 1, {"6 Voice", "12 Voice"}};
    if (id == "quality")
        return {0, 1, {"ECO", "HIGH"}};
    if (id == "hold")
        return {0, 1, {"Off", "On"}};
    if (id == "vol")
        return {0, 127, {}};
    if (id == FXSEL)
        return {0, 1, {"A", "B"}};
    std::fprintf(stderr, "unknown %s\n", id.toRawUTF8());
    std::exit(1);
}
int stepsOf(const String &id)
{
    auto r = rangeOf(id);
    return r.hi - r.lo + 1;
}
bool bipolarOf(const String &id)
{
    int p = paramIndex(id);
    return p >= 0 && (PARAMS[p].kind == K_BIP || PARAMS[p].kind == K_PITCH);
}
int defaultOf(const String &id)
{
    int p = paramIndex(id);
    if (p >= 0)
        return PARAMS[p].def - PARAMS[p].lo;
    return id == "vol" ? 100 : id == "quality" ? 1 : 0;
}
String valueText(const String &id, int v)
{
    int p = paramIndex(id);
    char t[32];
    paramText(p, PARAMS[p].lo + v, t, (int)sizeof t, false);
    return t;
}
String shortText(const String &id, int v)
{
    int p = paramIndex(id);
    char t[32];
    paramShort(p, PARAMS[p].lo + v, t, (int)sizeof t);
    return t;
}

/* ------------------------------------------------------------------------------- the art --- */
Font bold(float size) { return font(Face::title, size); }
void text(Graphics &g, const String &t, Rectangle<float> r, float size, Justification j = Justification::centred, Colour c = INK)
{
    g.setColour(c);
    g.setFont(bold(size));
    g.drawText(t, r, j, false);
}
/* a pot's scale (on the panel): 11 ticks; bipolar: 0, - and + */
void potScale(Graphics &g, float cx, float cy, float d, bool bipolar)
{
    float a0 = -2.45f, a1 = 2.45f;
    g.setColour(INK);
    for (int i = 0; i <= 10; i++) {
        float a = a0 + (a1 - a0) * (float)i / 10.0f;
        Point<float> u(std::sin(a), -std::cos(a));
        float r0 = d * 0.60f, r1 = d * 0.72f;
        g.drawLine(cx + u.x * r0, cy + u.y * r0, cx + u.x * r1, cy + u.y * r1, jmax(1.6f, d * 0.026f));
    }
    if (bipolar) {
        text(g, "0", Rectangle<float>(40, 17).withCentre({cx, cy - d * 0.84f}), 13.0f);
        text(g, "-", Rectangle<float>(40, 19).withCentre({cx - d * 0.62f, cy + d * 0.64f}), 15.0f);
        text(g, "+", Rectangle<float>(40, 18).withCentre({cx + d * 0.62f, cy + d * 0.64f}), 14.0f);
    }
}
/* the pot itself: matte black, a little domed, a white dot near its rim */
Rectangle<float> potRect(float cx, float cy, float d) { return Rectangle<float>(d * 1.28f, d * 1.30f).withCentre({cx, cy + d * 0.05f}); }
void potBody(Graphics &g, float cx, float cy, float d, float p)
{
    float a0 = -2.45f, a1 = 2.45f;
    auto r = Rectangle<float>(d, d).withCentre({cx, cy});
    g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.7f), cx, cy + d * 0.08f, Colours::transparentBlack, cx + d * 0.62f, cy + d * 0.08f, true));
    g.fillEllipse(r.expanded(d * 0.12f).translated(0, d * 0.06f));
    g.setGradientFill(ColourGradient(Colour(0xff2e2e31), cx - d * 0.35f, cy - d * 0.45f, Colour(0xff060606), cx + d * 0.3f, cy + d * 0.5f, false));
    g.fillEllipse(r);
    g.setColour(Colours::black);
    g.drawEllipse(r.reduced(0.5f), 1.2f);
    auto cap = r.reduced(d * 0.1f);
    g.setGradientFill(ColourGradient(Colour(0xff2a2a2c), cx - d * 0.15f, cy - d * 0.35f, Colour(0xff0c0c0d), cx + d * 0.2f, cy + d * 0.35f, true));
    g.fillEllipse(cap);
    g.setColour(Colours::white.withAlpha(0.06f));
    g.drawEllipse(cap, 1.0f);
    float a = a0 + (a1 - a0) * p;
    Point<float> u(std::sin(a), -std::cos(a));
    g.setColour(Colour(0xfff6f6f6));
    g.fillEllipse(Rectangle<float>(d * 0.13f, d * 0.13f).withCentre({cx + u.x * d * 0.30f, cy + u.y * d * 0.30f}));
}
/* a button (TREEGONE's), its glow kept inside its picture */
Rectangle<float> capRect(float cx, float cy, float s) { return Rectangle<float>(s * 1.7f, s * 0.95f).withCentre({cx, cy}); }
void buttonBody(Graphics &g, Rectangle<float> r, bool on, Rectangle<float> room)
{
    if (on) {
        auto c = r.getCentre();
        g.setGradientFill(ColourGradient(Colour(0xffff2a10).withAlpha(0.5f), c.x, c.y, Colours::transparentBlack, room.getRight() - 1.0f, c.y, true));
        g.fillEllipse(room.reduced(1.0f));
    }
    g.setColour(Colours::black.withAlpha(0.75f));
    g.fillRoundedRectangle(r.translated(0.6f, 1.8f).expanded(0.6f), 2.5f);
    g.setColour(Colour(0xff020202));
    g.fillRoundedRectangle(r, 2.5f);
    auto face = r.reduced(1.8f);
    if (on) {
        ColourGradient lit(Colour(0xffffa070), face.getCentreX(), face.getCentreY(), Colour(0xffd81c0a), face.getRight(), face.getBottom(), true);
        lit.addColour(0.5, Colour(0xffff4a1c));
        g.setGradientFill(lit);
    } else
        g.setGradientFill(ColourGradient(Colour(0xff2c2c30), face.getX(), face.getY(), Colour(0xff151517), face.getX(), face.getBottom(), false));
    g.fillRoundedRectangle(face, 1.8f);
    g.setColour(on ? Colour(0x40a00a00) : Colour(0x30000000));
    for (float x = face.getX() + 2.0f; x < face.getRight() - 1.0f; x += 2.6f)
        g.drawVerticalLine((int)x, face.getY() + 1.5f, face.getBottom() - 1.5f);
    for (float y = face.getY() + 2.0f; y < face.getBottom() - 1.0f; y += 2.6f)
        g.drawHorizontalLine((int)y, face.getX() + 1.5f, face.getRight() - 1.5f);
    g.setColour(Colours::white.withAlpha(on ? 0.3f : 0.09f));
    g.drawLine(face.getX() + 2.0f, face.getY() + 0.7f, face.getRight() - 2.0f, face.getY() + 0.7f, 1.0f);
}
void led(Graphics &g, float cx, float cy, bool on) { drawLed(g, {cx, cy}, 9.0f, on ? 1.0f : 0.0f); }
void wave(Graphics &g, float cx, float cy, int kind)
{
    Path p;
    float w = 10.0f, h = 9.0f;
    if (kind == 0) {          /* triangle */
        p.startNewSubPath(cx - w, cy + h * 0.5f);
        p.lineTo(cx, cy - h * 0.5f);
        p.lineTo(cx + w, cy + h * 0.5f);
    } else if (kind == 1) {   /* saw */
        p.startNewSubPath(cx - w, cy + h * 0.5f);
        p.lineTo(cx - w, cy - h * 0.5f);
        p.lineTo(cx + w, cy + h * 0.5f);
    } else if (kind == 2) {   /* rev saw */
        p.startNewSubPath(cx - w, cy + h * 0.5f);
        p.lineTo(cx + w, cy - h * 0.5f);
        p.lineTo(cx + w, cy + h * 0.5f);
    } else {                  /* pulse */
        p.startNewSubPath(cx - w, cy + h * 0.5f);
        p.lineTo(cx - w * 0.5f, cy + h * 0.5f);
        p.lineTo(cx - w * 0.5f, cy - h * 0.5f);
        p.lineTo(cx + w * 0.5f, cy - h * 0.5f);
        p.lineTo(cx + w * 0.5f, cy + h * 0.5f);
        p.lineTo(cx + w, cy + h * 0.5f);
    }
    g.setColour(INK);
    g.strokePath(p, PathStrokeType(1.8f, PathStrokeType::mitered));
}

/* ------------------------------------------------------------------- the pages' controls --- */
struct Ctl {
    enum Kind { knob, button, leds, display } kind;
    String id;
    int fx = -1;                         /* on the FX page: shown with A (0) or B (1) only */
    Rectangle<float> rect, frame;        /* a knob's picture and where it is touched (with its legend) */
    std::function<void(Graphics &, int)> paint;            /* at a value */
    std::function<int(int)> next;                          /* a button: the value a tap gives */
    std::vector<Rectangle<float>> rows;                    /* LEDs: each value's */
    std::function<void(Graphics &, int, bool)> paintRow;   /* LEDs: a value's, lit or not */
};
struct Page {
    String name, title;
    std::vector<Ctl> ctls;
};

/* A page being built: what never changes is drawn on the panel (g), the controls are listed */
struct B {
    Graphics &g;
    Page &pg;
    float KD = 88.0f, BS = 36.0f;

    void title(const String &t, float cx, float y, float size = 26.0f)
    {
        g.setColour(INK);
        g.setFont(bold(size).withExtraKerningFactor(0.14f));
        g.drawText(t, Rectangle<float>(cx - 400, y, 800, size + 6), Justification::centred, false);
    }
    void titleL(const String &t, float x, float y, float size = 24.0f)
    {
        g.setColour(INK);
        g.setFont(bold(size).withExtraKerningFactor(0.14f));
        g.drawText(t, Rectangle<float>(x, y, 600, size + 6), Justification::centredLeft, false);
    }
    void hline(float x0, float x1, float y)
    {
        g.setColour(LINE);
        g.fillRect(x0, y, x1 - x0, 1.6f);
    }
    void vline(float x, float y0, float y1)
    {
        g.setColour(LINE.withAlpha(0.6f));
        g.fillRect(x, y0, 1.4f, y1 - y0);
    }
    void lineTitle(const String &t, float x0, float x1, float y)
    {
        auto f = bold(15.0f).withExtraKerningFactor(0.06f);
        float w = GlyphArrangement::getStringWidth(f, t) + 28.0f, cx = (x0 + x1) * 0.5f;
        hline(x0, cx - w * 0.5f, y);
        hline(cx + w * 0.5f, x1, y);
        g.setColour(INK);
        g.setFont(f);
        g.drawText(t, Rectangle<float>(cx - w * 0.5f, y - 10, w, 20), Justification::centred, false);
    }
    Rectangle<float> legend(const String &t, float cx, float y, float size = 17.0f)
    {
        auto r = Rectangle<float>(cx - 80, y, 160, size + 4);
        text(g, t, r, size);
        return r;
    }
    void small(const String &t, float cx, float cy, float size = 13.0f)
    {
        text(g, t, Rectangle<float>(160, size + 4).withCentre({cx, cy}), size);
    }
    void smallL(const String &t, float x, float cy, float size = 15.5f)
    {
        text(g, t, Rectangle<float>(x, cy - (size + 4) * 0.5f, 160, size + 4), size, Justification::centredLeft);
    }
    Ctl &add(Ctl c)
    {
        pg.ctls.push_back(std::move(c));
        return pg.ctls.back();
    }

    /* a pot: its scale and legend on the panel, its body a film strip */
    Ctl &knob(float cx, float cy, float d, const String &id, const String &leg, int fx = -1)
    {
        bool bip = bipolarOf(id);
        if (fx <= 0) {
            potScale(g, cx, cy, d, bip);
            if (leg.isNotEmpty())
                legend(leg, cx, cy + d * 0.72f + 6.0f);
        }
        int steps = stepsOf(id);
        Ctl c{Ctl::knob, id, fx};
        c.rect = potRect(cx, cy, d);
        c.frame = Rectangle<float>(d * 1.44f, d * 1.44f).withCentre({cx, cy}).getUnion(Rectangle<float>(cx - d * 0.7f, cy, d * 1.4f, d * 0.72f + 26.0f));
        c.paint = [=](Graphics &gg, int v) { potBody(gg, cx, cy, d, steps > 1 ? (float)v / (float)(steps - 1) : 0.0f); };
        return add(c);
    }
    /* its value in red under its legend (a display of its own) */
    void value(float cx, float y, const String &id, int fx = -1)
    {
        Ctl c{Ctl::display, id, fx};
        c.rect = Rectangle<float>(cx - 75, y, 150, 20);
        c.paint = [=](Graphics &gg, int v) {
            gg.setColour(VALUE);
            gg.setFont(font(Face::mono, 15.5f));
            gg.drawText(valueText(id, v), Rectangle<float>(cx - 75, y, 150, 20), Justification::centred, false);
        };
        add(c);
    }
    void kv(float cx, float cy, float d, const String &id, const String &leg)
    {
        knob(cx, cy, d, id, leg);
        value(cx, cy + d * 0.72f + 25.0f, id);
    }
    /* a button: a switch (a tap: on / off) or a step through the values (cycle) */
    Ctl &button(float cx, float cy, const String &id, const String &leg, bool cycle = false, int fx = -1, float s = 0.0f)
    {
        if (s <= 0.0f)
            s = BS;
        if (leg.isNotEmpty() && fx <= 0)
            legend(leg, cx, cy + s * 0.5f + 8.0f, 15.5f);
        auto cap = capRect(cx, cy, s);
        int steps = stepsOf(id);
        Ctl c{Ctl::button, id, fx};
        c.rect = cap.expanded(s * 0.42f, s * 0.5f);
        auto room = c.rect;
        c.paint = [=](Graphics &gg, int v) { buttonBody(gg, cap, !cycle && v > 0, room); };
        c.next = [=](int v) { return cycle ? (v + 1) % steps : (v ? 0 : 1); };
        return add(c);
    }
    /* LEDs, one a value: each a radio button (where each is, the name's place beside it) */
    Ctl &leds(const String &id, std::vector<Point<float>> at, std::vector<Rectangle<float>> rows, int fx = -1)
    {
        Ctl c{Ctl::leds, id, fx};
        c.rows = rows;
        c.paintRow = [=](Graphics &gg, int k, bool on) { led(gg, at[(size_t)k].x, at[(size_t)k].y, on); };
        return add(c);
    }
    /* a list: LED, name; downwards */
    Ctl &ledList(const String &id, float x, float y, float rh, const StringArray &names, float w = 120.0f, float size = 15.5f, int fx = -1)
    {
        std::vector<Point<float>> at;
        std::vector<Rectangle<float>> rows;
        for (int i = 0; i < names.size(); i++) {
            float cy = y + rh * (float)i;
            if (fx <= 0)
                smallL(names[i], x + 14.0f, cy, size);
            at.push_back({x, cy});
            rows.push_back(Rectangle<float>(x - 10.0f, cy - rh * 0.5f, w + 24.0f, rh));
        }
        return leds(id, at, rows, fx);
    }
    void seg(Rectangle<float> r, const String &id, int n, std::function<String(int)> txt, int fx = -1)
    {
        Ctl c{Ctl::display, id, fx};
        c.rect = r;
        c.paint = [=](Graphics &gg, int v) { drawSeg7(gg, r, txt(v), n); };
        add(c);
    }

    /* the octave pot with its LEDs round it (-2..+2; osc 3: LO too) */
    void octKnob(float cx, float cy, float d, const String &id, bool lo)
    {
        const char *nm[5] = {"-2", "-1", "0", "+1", "+2"};
        std::vector<Point<float>> at;
        std::vector<Rectangle<float>> rows;
        if (lo) {
            Point<float> p(cx - d * 0.74f, cy + d * 0.42f);
            small("LO", cx - d * 0.92f, cy + d * 0.60f, 11.5f);
            at.push_back(p);
            rows.push_back(Rectangle<float>(22, 22).withCentre(p));
        }
        for (int i = 0; i < 5; i++) {
            float a = -1.25f + 2.5f * (float)i / 4.0f;
            Point<float> u(std::sin(a), -std::cos(a));
            Point<float> p(cx + u.x * d * 0.74f, cy + u.y * d * 0.74f);
            small(nm[i], cx + u.x * d * 0.95f, cy + u.y * d * 0.93f, 12.0f);
            at.push_back(p);
            rows.push_back(Rectangle<float>(22, 22).withCentre(p));
        }
        knob(cx, cy, d * 0.80f, id, "");
        legend("OCTAVE", cx, cy + d * 0.58f + 6.0f);
        leds(id, at, rows);
    }
};

/* --------------------------------------------------------------------------------- pages --- */
void pageOsc(B &b)
{
    b.title("OSCILLATORS", 640, 14);
    float rows[3] = {150, 312, 474};
    float cNum = 52, cOct = 168, c2 = 345, w1 = 470, w2 = 570, w3 = 670, cPw = 830, cVol = 990, cLast = 1160;
    for (int o = 0; o < 3; o++) {
        float y = rows[o];
        String pre = "o" + String(o + 1) + "_";
        text(b.g, String(o + 1), Rectangle<float>(cNum - 30, y - 28, 60, 56), 40.0f);
        b.octKnob(cOct, y, b.KD - 4, pre + "oct", o == 2);
        if (o == 0)
            b.button(c2, y - 6, "sync", "SYNC OSC 2");
        else
            b.knob(c2, y, b.KD, pre + "pitch", "PITCH");
        wave(b.g, w1, y - 38, 0);
        b.button(w1, y - 6, pre + "tri", "");
        if (o < 2) {
            wave(b.g, w2, y - 38, 1);
            b.button(w2, y - 6, pre + "saw", "");
        } else {   /* osc 3: one button, saw and rev saw on its two LEDs */
            wave(b.g, w2 - 13, y - 52, 1);
            wave(b.g, w2 + 13, y - 52, 2);
            auto &c = b.button(w2, y - 6, "o3_saw", "", true);
            c.rect = c.rect.getUnion(Rectangle<float>(w2 - 24, y - 44, 48, 20));
            auto room = c.rect;
            auto cap = capRect(w2, y - 6, b.BS);
            c.paint = [=](Graphics &gg, int v) {
                buttonBody(gg, cap, false, room);
                led(gg, w2 - 13, y - 34, v == 1);
                led(gg, w2 + 13, y - 34, v == 2);
            };
        }
        wave(b.g, w3, y - 38, 3);
        b.button(w3, y - 6, pre + "pulse", "");
        b.knob(cPw, y, b.KD, pre + "pw", "PULSE WIDTH");
        b.knob(cVol, y, b.KD, pre + "level", "VOLUME");
    }
    b.knob(cLast, rows[0], b.KD, "fdbk_drive", "FDBK < > DRIVE");
    b.knob(cLast, rows[1], b.KD, "noise", "NOISE");
    b.button(cLast, rows[2] - 6, "o3_key", "KEYBOARD");
}

void pageFilter(B &b)
{
    b.title("FILTER", 640, 6);
    float r1 = 124, r2 = 280, r3 = 492;
    float cT = 95, cC = 285, cR = 445, cE = 605, cS = 765, cP = 930, cK = 1110;
    /* TREEGONE's three filters: TYPE steps through them, or a tap on one */
    b.button(cT - 50, r1 - 4, "f_type", "TYPE", true);
    b.ledList("f_type", cT + 6, r1 - 26, 26, {"LP1 OTA 4P", "LP2 LADDER 4P", "SVF OTA 2P"}, 100.0f, 14.0f);
    b.knob(cC, r1, b.KD, "cutoff", "CUTOFF");
    b.knob(cR, r1, b.KD, "reso", "RESONANCE");
    b.knob(cE, r1, b.KD, "f_env_amt", "ENV AMT");
    b.knob(cS, r1, b.KD, "svf_mode", "SVF MODE");
    b.small("LP", cS - 62, r1 - 52, 12.0f);
    b.small("BP", cS, r1 - 76, 12.0f);
    b.small("HP", cS + 62, r1 - 52, 12.0f);
    b.button(cP, r1 - 6, "pole2", "2 POLE");
    b.small("Half", cK - 22, r1 - 62, 12.0f);
    b.small("Full", cK + 22, r1 - 62, 12.0f);
    {   /* KEYBOARD: off, half, full on its two LEDs */
        auto &c = b.button(cK, r1 - 6, "f_key", "KEYBOARD", true);
        c.rect = c.rect.getUnion(Rectangle<float>(cK - 34, r1 - 54, 68, 22));
        auto room = c.rect;
        auto cap = capRect(cK, r1 - 6, b.BS);
        c.paint = [=](Graphics &gg, int v) {
            buttonBody(gg, cap, false, room);
            led(gg, cK - 22, r1 - 44, v == 1);
            led(gg, cK + 22, r1 - 44, v == 2);
        };
    }
    const char *env[4] = {"ATTACK", "DECAY", "SUSTAIN", "RELEASE"};
    const char *fid[4] = {"f_att", "f_dec", "f_sus", "f_rel"}, *aid[4] = {"a_att", "a_dec", "a_sus", "a_rel"};
    float ce[4] = {cC, cR, cE, cS};
    for (int i = 0; i < 4; i++)
        b.knob(ce[i], r2, b.KD, fid[i], env[i]);
    b.button((cP + cK) * 0.5f, r2 - 6, "f_vel", "VELOCITY");
    b.hline(30, 1250, 378);
    b.title("AMPLIFIER", 640, 384);
    for (int i = 0; i < 4; i++)
        b.knob(ce[i], r3, b.KD, aid[i], env[i]);
    b.button((cP + cK) * 0.5f, r3 - 6, "a_vel", "VELOCITY");
}

void pageMod(B &b)
{
    b.title("LFO", 640, 6);
    float r1 = 112, r2 = 244;
    b.knob(150, r1, b.KD, "lfo_freq", "FREQUENCY");
    b.button(300, r1 - 14, "lfo_sync", "LFO SYNC");
    b.knob(560, r1, b.KD, "lfo_shape", "SHAPE");
    b.ledList("lfo_shape", 652, r1 - 44, 24, {"Triangle", "Sawtooth", "Rev Saw", "Square", "Random"}, 80.0f);
    b.knob(1000, r1, b.KD, "lfo_amt", "AMOUNT");
    const char *ld[8][2] = {{"lfo_f1", "FREQ 1"}, {"lfo_f2", "FREQ 2"}, {"lfo_f3", "FREQ 3"}, {"lfo_pw1", "PW 1"},
                            {"lfo_pw2", "PW 2"},  {"lfo_pw3", "PW 3"},  {"lfo_filt", "FILTER"}, {"lfo_amp", "AMP"}};
    for (int i = 0; i < 8; i++)
        b.button(150.0f + 132.0f * (float)i, r2, ld[i][0], ld[i][1]);
    b.hline(30, 1250, 304);
    b.titleL("POLYMOD", 60, 312);
    float r3 = 470;
    b.knob(150, r3, b.KD, "pm_osc3", "OSC 3 AMOUNT");
    b.knob(340, r3, b.KD, "pm_fenv", "FILTER ENV");
    const char *pd[6][2] = {{"pm_f1", "FREQ 1"}, {"pm_f2", "FREQ 2"}, {"pm_f3", "FREQ 3"}, {"pm_pw1", "PW 1"}, {"pm_pw2", "PW 2"}, {"pm_fdbk", "FDBK"}};
    for (int i = 0; i < 6; i++)
        b.button(600.0f + 150.0f * (float)(i % 3), (i < 3 ? 412.0f : 512.0f), pd[i][0], pd[i][1]);
    b.button(1100, 462, "pm_filt", "FILTER");
}

/* EFFECTS: two sub-pages, A and B (the FX key again switches); each edits its own effect, both displays shown */
void pageFx(B &b, int f)
{
    b.title(f ? "EFFECTS  B" : "EFFECTS  A", 640, 6);
    float r1 = 140;
    b.button(110, r1 - 8, "fx_on", "ON/OFF", false, -1, b.BS + 4);
    b.smallL("A", 262, r1 - 8, 15.0f);
    b.smallL("B", 572, r1 - 8, 15.0f);
    led(b.g, 250, r1 - 8, f == 0);
    led(b.g, 560, r1 - 8, f == 1);
    b.seg({290, r1 - 40, 200, 64}, "fxa_type", 3, [](int v) { return String(FXTYPE_SHORT[v]); });
    b.seg({600, r1 - 40, 200, 64}, "fxb_type", 3, [](int v) { return String(FXTYPE_SHORT[v]); });
    b.small("A / B:", 920, r1 - 20, 15.0f);
    b.small("press FX again", 920, r1 + 2, 15.0f);
    String x = f ? "fxb_" : "fxa_";
    b.knob(1120, r1 - 8, b.KD + 8, x + "type", "TYPE");
    float r2 = 400;
    b.knob(110, r2, b.KD + 8, x + "p1", "1");
    b.seg({215, r2 - 19, 160, 38}, x + "p1", 3, [x](int v) { return shortText(x + "p1", v); });
    b.seg({415, r2 - 19, 160, 38}, x + "p2", 3, [x](int v) { return shortText(x + "p2", v); });
    b.knob(650, r2, b.KD + 8, x + "p2", "2");
    b.button(810, r2 - 10, x + "sync", "SYNC");
    b.knob(960, r2, b.KD, x + "div", "SYNC TIME");
    b.value(960, r2 + b.KD * 0.72f + 25.0f, x + "div");
    b.knob(1140, r2, b.KD + 8, x + "mix", "DRY < > WET");
    b.lineTitle("EFFECT PARAMETERS", 30, 1250, 268);
}

void pageAtArp(B &b)
{
    b.title("AFTER TOUCH", 300, 8);
    b.knob(120, 250, b.KD + 6, "at_amt", "AMOUNT");
    const char *ad[8][2] = {{"at_f1", "FREQ 1"}, {"at_f2", "FREQ 2"}, {"at_f3", "FREQ 3"}, {"at_filt", "FILTER"},
                            {"at_amp", "AMP"},  {"at_lfo", "LFO"},   {"at_fxa", "FX MIX A"}, {"at_fxb", "FX MIX B"}};
    for (int i = 0; i < 8; i++) {
        int row = i / 3, col = i % 3;
        b.button(290.0f + 125.0f * (float)col + (row == 2 ? 62.0f : 0.0f), 130.0f + 125.0f * (float)row, ad[i][0], ad[i][1]);
    }
    b.vline(620, 30, 560);
    b.title("ARPEGGIATOR", 950, 8);
    float r = 150;
    b.button(700, r - 6, "arp_on", "ON/OFF");
    b.knob(840, r, b.KD, "arp_oct", "OCTAVES");
    b.ledList("arp_oct", 924, r - 24, 24, {"1", "2", "3"}, 30.0f);
    b.knob(1040, r, b.KD, "arp_mode", "MODE");
    b.ledList("arp_mode", 1124, r - 48, 24, {"Up", "Down", "Up/Down", "Random", "Assign"}, 80.0f);
    b.hline(650, 1250, 300);
    b.title("CLOCK", 950, 312);
    float r2 = 450;
    b.kv(780, r2, b.KD, "divide", "DIVIDE");
    b.button(950, r2 - 6, "hold", "HOLD");
    b.small("Tempo: MPC", 1120, r2 - 6, 14.0f);
}

void pagePlay(B &b)
{
    float KD = b.KD, r1 = 100;
    b.button(58, r1 - 6, "hold", "HOLD");
    b.knob(168, r1, KD - 8, "glide_rate", "GLIDE RATE");
    b.button(276, r1 - 6, "glide", "GLIDE");
    b.button(364, r1 - 6, "unison", "UNISON");
    b.knob(484, r1, KD - 8, "distortion", "DISTORTION");
    b.knob(630, r1, KD - 8, "vintage", "VINTAGE");
    b.knob(776, r1, KD - 8, "pan_spread", "PAN SPREAD");
    b.knob(922, r1, KD - 8, "pgm_vol", "PGM VOLUME");
    b.kv(1060, r1, KD - 16, "pb_range", "P WHL RANGE");
    b.kv(1192, r1, KD - 16, "key_mode", "KEY MODE");
    b.lineTitle("GLIDE  /  UNISON  /  VOICES", 30, 1250, 222);
    float r2 = 298;
    b.kv(150, r2, KD - 12, "glide_mode", "GLIDE MODE");
    b.kv(320, r2, KD - 12, "uni_voices", "UNI VOICES");
    b.kv(490, r2, KD - 12, "uni_detune", "UNI DETUNE");
    b.button(612, r2 - 2, "voices", "VOICES", true);
    b.ledList("voices", 668, r2 - 14, 24, {"6 VOICE", "12 VOICE"}, 80.0f);
    b.button(822, r2 - 2, "quality", "QUALITY", true);
    b.ledList("quality", 878, r2 - 14, 24, {"ECO (2x)", "HIGH (4x)"}, 80.0f);
    b.knob(1120, r2, KD - 12, "vol", "VOLUME");
    b.lineTitle("LOW SPLIT", 30, 1250, 420);
    float r3 = 490;
    b.button(150, r3 - 6, "split_on", "ON/OFF");
    b.button(300, r3 - 6, "split_extra", "+1 VOICE");
    b.kv(480, r3, KD - 12, "split_point", "SPLIT POINT");
    b.kv(660, r3, KD - 12, "split_oct", "OCTAVE");
    b.knob(840, r3, KD - 12, "split_glide", "GLIDE");
    b.knob(1020, r3, KD - 12, "split_vol", "VOLUME");
}

struct Def {
    const char *name, *title;
    std::function<void(B &)> build;
};
const std::vector<Def> DEFS = {
    {"OSC", "OSCILLATORS", pageOsc},       {"FLT/AMP", "FILTER / AMP", pageFilter}, {"LFO/PM", "LFO / POLYMOD", pageMod},
    {"FX A", "EFFECTS A", [](B &b) { pageFx(b, 0); }}, {"FX B", "EFFECTS B", [](B &b) { pageFx(b, 1); }},             {"AT/ARP", "AFTER TOUCH / ARP", pageAtArp}, {"PLAY", "PLAY", pagePlay},
};

/* ------------------------------------------------------------------------------ cutting --- */
bool writePng(const Image &img, const File &f)
{
    f.deleteFile();
    FileOutputStream out(f);
    PNGImageFormat png;
    return out.openedOk() && png.writeImageToStream(img, out);
}
var rectVar(Rectangle<int> r) { return Array<var>{r.getX(), r.getY(), r.getWidth(), r.getHeight()}; }
Rectangle<int> px(Rectangle<float> r) { return r.getSmallestIntegerContainer().getIntersection({0, 0, TW, TH}); }

Image cut(const Image &bg, Rectangle<int> r, std::function<void(Graphics &)> paint)
{
    Image img(Image::RGB, r.getWidth(), r.getHeight(), true);
    Graphics g(img);
    g.drawImageAt(bg.getClippedImage(r), 0, 0);
    g.addTransform(AffineTransform::translation((float)-r.getX(), (float)-r.getY()));
    paint(g);
    return img;
}
} // namespace

int main(int argc, char **argv)
{
    ScopedJuceInitialiser_GUI gui;
    SharedResourcePointer<UiResources> res;
    if (argc < 2) {
        std::fprintf(stderr, "usage: treegone_skin_render <out folder>\n");
        return 2;
    }
    File dir = File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    dir.deleteRecursively();
    dir.createDirectory();
    Array<var> pageList;
    for (auto &d : DEFS) {
        Page pg;
        pg.name = d.name;
        pg.title = d.title;
        String tag = pg.name.toLowerCase().retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789");
        /* the panel: the coat, then what never changes, the bottom strip */
        Image bg(Image::RGB, TW, TH, true);
        {
            Graphics g(bg);
            auto r = Rectangle<float>(0, 0, (float)TW, (float)TH);
            drawPanelFace(g, r, 1.0f);   /* (the coat's rough surface alone: no scratches) */
            B b{g, pg};
            d.build(b);
            g.setColour(INK);
            g.setFont(font(Face::logo, 20.0f));
            g.drawText("TREEGONE", Rectangle<float>(20, 592, 260, 28), Justification::centredLeft, false);
            g.setFont(font(Face::title, 15.0f));
            g.setColour(INK.withAlpha(0.7f));
            g.drawText(pg.title, Rectangle<float>(200, 592, 300, 28), Justification::centredLeft, false);
            drawTag(g, Rectangle<float>(1280 - 18 - 200, 628 - 12 - 24, 200, 24));
        }
        writePng(bg, dir.getChildFile(tag + "_panel.png"));

        Array<var> list;
        Image pv = bg.createCopy();   /* (a look at the page with every control at its default, for checking) */
        Graphics pg2(pv);
        int n = 0;
        for (auto &c : pg.ctls) {
            int steps = stepsOf(c.id);
            auto *o = new DynamicObject();
            o->setProperty("id", c.id);
            o->setProperty("fx", c.fx);
            o->setProperty("steps", steps);
            Array<var> names;
            for (auto &t : rangeOf(c.id).names)
                names.add(t);
            o->setProperty("names", names);
            String base = tag + "_" + String(n++).paddedLeft('0', 3) + "_" + c.id.retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789_");
            int dv = jlimit(0, steps - 1, defaultOf(c.id));
            if (c.kind == Ctl::knob || c.kind == Ctl::display) {
                auto r = px(c.rect);
                int frames = jmax(2, jmin(steps, 9800 / r.getHeight()));
                if (frames < steps && c.kind == Ctl::display)
                    std::fprintf(stderr, "%s: a display of %d frames for %d values\n", c.id.toRawUTF8(), frames, steps);
                frames = jmin(frames, c.kind == Ctl::knob ? 128 : 512);
                Image strip(Image::RGB, r.getWidth(), r.getHeight() * frames, true);
                {
                    Graphics sg(strip);
                    for (int f = 0; f < frames; f++) {
                        int idx = frames == steps ? f : (int)std::lround((double)f * (steps - 1) / (frames - 1));
                        sg.drawImageAt(cut(bg, r, [&](Graphics &g) { c.paint(g, idx); }), 0, f * r.getHeight());
                    }
                }
                String f = base + (c.kind == Ctl::knob ? "_knob.png" : "_display.png");
                writePng(strip, dir.getChildFile(f));
                o->setProperty("kind", c.kind == Ctl::knob ? "knob" : "display");
                o->setProperty("bipolar", bipolarOf(c.id));
                o->setProperty("rect", rectVar(r));
                o->setProperty("frame", rectVar(px(c.frame.getUnion(c.rect))));
                o->setProperty("frames", frames);
                o->setProperty("strip", f);
                if (c.fx <= 0)
                    c.paint(pg2, dv);
            } else if (c.kind == Ctl::button) {
                auto r = px(c.rect);
                o->setProperty("kind", "button");
                o->setProperty("rect", rectVar(r));
                Array<var> rs;
                for (int i = 0; i < steps; i++) {
                    auto *ro = new DynamicObject();
                    ro->setProperty("a", i);
                    ro->setProperty("b", i);
                    ro->setProperty("next", c.next(i));
                    String f = base + "_button_" + String(i) + ".png";
                    writePng(cut(bg, r, [&](Graphics &g) { c.paint(g, i); }), dir.getChildFile(f));
                    ro->setProperty("image", f);
                    rs.add(var(ro));
                }
                o->setProperty("ranges", rs);
                if (c.fx <= 0)
                    c.paint(pg2, dv);
            } else {
                o->setProperty("kind", "leds");
                Array<var> rows;
                for (int k = 0; k < (int)c.rows.size(); k++) {
                    auto r = px(c.rows[(size_t)k]);
                    auto *ro = new DynamicObject();
                    ro->setProperty("rect", rectVar(r));
                    String on = base + "_led" + String(k) + "_on.png", off = base + "_led" + String(k) + "_off.png";
                    writePng(cut(bg, r, [&](Graphics &g) { c.paintRow(g, k, true); }), dir.getChildFile(on));
                    writePng(cut(bg, r, [&](Graphics &g) { c.paintRow(g, k, false); }), dir.getChildFile(off));
                    ro->setProperty("on", on);
                    ro->setProperty("off", off);
                    rows.add(var(ro));
                    if (c.fx <= 0)
                        c.paintRow(pg2, k, k == dv);
                }
                o->setProperty("rows", rows);
            }
            list.add(var(o));
        }
        writePng(pv, dir.getChildFile("preview_" + tag + ".png"));
        auto *po = new DynamicObject();
        po->setProperty("name", pg.name);
        po->setProperty("background", tag + "_panel.png");
        po->setProperty("controls", list);
        pageList.add(var(po));
        std::printf("%-8s %d controls\n", pg.name.toRawUTF8(), (int)pg.ctls.size());
    }
    auto *m = new DynamicObject();
    m->setProperty("pages", pageList);
    dir.getChildFile("manifest.json").replaceWithText(JSON::toString(var(m)));
    std::printf("manifest.json in %s\n", dir.getFullPathName().toRawUTF8());
    return 0;
}
