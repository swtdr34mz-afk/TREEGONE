/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
/* The MPC skin's pictures, drawn with the VST3's own art (src/ui/Look.cpp: the powder-coated panel, the knobs, the
 * buttons, the LEDs, the section frames, the legends, the maker's tag), on the MPC's six pages (its function keys):
 *
 *   OSC      OSCILLATORS 1, 2 / 3, NOISE, FDBK <> DRIVE
 *   FILTER   FILTER (with the TYPE) / AMPLIFIER
 *   MOD      LFO / POLYMOD
 *   AT/FX    AFTER TOUCH / EFFECTS A and B
 *   PLAY     ARPEGGIATOR, CLOCK, SEQUENCER / GLIDE, UNISON, KEYS
 *   MASTER   DISTORTION, VINTAGE, PAN SPREAD, VOLUMES / VOICES, QUALITY, HOLD
 *
 * Each page is drawn at the MPC's 1280 x 628 and cut up: its background, and every control at every value it can take
 * (a knob's film strip, a button's look for each value with the value a press gives, an LED list's rows lit and
 * unlit). manifest.json says what each is, for make_skin.py.
 *
 *   treegone_skin_render <out folder> */
#include "Params.h"
#include "ui/Look.h"
#include <juce_gui_basics/juce_gui_basics.h>

using namespace juce;
using namespace rvui;
using namespace tg;

namespace {
constexpr int TW = 1280, TH = 628;
constexpr float KS = 74.0f;              /* a knob */
constexpr float BW = 52.0f, BH = 28.0f;  /* a button */
constexpr float LR = 21.0f;              /* an LED row */

/* a control on a page: which parameter (its VST id: "cutoff"; or a global one), where */
struct Ctl {
    enum Kind { knob, button, leds } kind;
    String id;                           /* PARAMS[].id; else "vol", "voices", "quality", "hold" */
    float x, y;                          /* the centre (knob, button) or the top left (LEDs) */
    String legend;
    StringArray names;                   /* LEDs: each row's name */
    float w = 0.0f;                      /* LEDs: the rows' width */
    bool cycle = false;                  /* a button that steps through the values (else a switch) */
    float rh = LR;                       /* LEDs: a row's height */
};
struct Sec {
    Rectangle<float> r;
    String title;
};
struct Page {
    String name;
    std::vector<Sec> secs;
    std::vector<Ctl> ctls;
    std::vector<std::pair<Point<float>, String>> notes;   /* small print */
};

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
    std::fprintf(stderr, "unknown %s\n", id.toRawUTF8());
    std::exit(1);
}
bool bipolarOf(const String &id)
{
    int p = paramIndex(id);
    return p >= 0 && PARAMS[p].kind == K_BIP;
}

/* ------------------------------------------------------------------------------- the pages --- */
struct Builder {
    Page pg;
    void sec(float x0, float y0, float x1, float y1, const char *t) { pg.secs.push_back({{x0, y0, x1 - x0, y1 - y0}, t}); }
    void K(float x, float y, const char *id, const char *leg) { pg.ctls.push_back({Ctl::knob, id, x, y, leg, {}, 0.0f, false}); }
    void B(float x, float y, const char *id, const char *leg, bool cycle = false) { pg.ctls.push_back({Ctl::button, id, x, y, leg, {}, 0.0f, cycle}); }
    void L(float x, float y, const char *id, float w, StringArray names = {}, float rh = LR)
    {
        auto r = rangeOf(id);
        pg.ctls.push_back({Ctl::leds, id, x, y, "", names.isEmpty() ? r.names : names, w, false, rh});
    }
    void note(float x, float y, const char *t) { pg.notes.push_back({{x, y}, t}); }
};

/* rows: the top band's knob lines at 80 and 208, the bottom band's at 368 and 496 */
std::vector<Page> pages()
{
    std::vector<Page> ps;
    const float T1 = 80, T2 = 208, B1 = 368, B2 = 496;
    {
        Builder b;
        b.pg.name = "OSC";
        b.sec(14, 16, 1266, 284, "OSCILLATORS 1 / 2");
        b.K(80, T1, "o1_oct", "OCTAVE");
        b.B(200, T1 - 8, "sync", "SYNC OSC 2");
        b.B(330, T1 - 8, "o1_tri", "TRIANGLE");
        b.B(450, T1 - 8, "o1_saw", "SAWTOOTH");
        b.B(570, T1 - 8, "o1_pulse", "PULSE");
        b.K(720, T1, "o1_pw", "PULSE WIDTH");
        b.K(860, T1, "o1_level", "VOLUME 1");
        b.K(80, T2, "o2_oct", "OCTAVE");
        b.K(200, T2, "o2_pitch", "PITCH");
        b.B(330, T2 - 8, "o2_tri", "TRIANGLE");
        b.B(450, T2 - 8, "o2_saw", "SAWTOOTH");
        b.B(570, T2 - 8, "o2_pulse", "PULSE");
        b.K(720, T2, "o2_pw", "PULSE WIDTH");
        b.K(860, T2, "o2_level", "VOLUME 2");
        b.K(1060, T1, "fdbk_drive", "FDBK <> DRIVE");
        b.K(1060, T2, "noise", "NOISE");
        b.sec(14, 304, 1266, 572, "OSCILLATOR 3");
        b.K(80, B1, "o3_oct", "OCTAVE (LO)");
        b.K(200, B1, "o3_pitch", "PITCH");
        b.B(330, B1 - 8, "o3_tri", "TRIANGLE");
        b.B(450, B1 - 8, "o3_saw", "SAW / REV", true);
        b.L(490, B1 - 30, "o3_saw", 90);
        b.B(660, B1 - 8, "o3_pulse", "PULSE");
        b.K(790, B1, "o3_pw", "PULSE WIDTH");
        b.K(920, B1, "o3_level", "VOLUME 3");
        b.B(1060, B1 - 8, "o3_key", "KEYBOARD");
        b.note(140, B2, "OCTAVE LO: osc 3 as a slow modulator");
        ps.push_back(b.pg);
    }
    {
        Builder b;
        b.pg.name = "FILTER";
        b.sec(14, 16, 1266, 284, "FILTER");
        b.B(70, T1 - 8, "f_type", "TYPE", true);
        b.L(110, T1 - 34, "f_type", 170);
        b.K(390, T1, "cutoff", "CUTOFF");
        b.K(520, T1, "reso", "RESONANCE");
        b.K(650, T1, "f_env_amt", "ENV AMT");
        b.K(780, T1, "svf_mode", "SVF MODE");
        b.B(910, T1 - 8, "pole2", "2 POLE");
        b.B(1030, T1 - 8, "f_key", "KEYBOARD", true);
        b.L(1070, T1 - 34, "f_key", 70);
        b.K(390, T2, "f_att", "ATTACK");
        b.K(520, T2, "f_dec", "DECAY");
        b.K(650, T2, "f_sus", "SUSTAIN");
        b.K(780, T2, "f_rel", "RELEASE");
        b.B(910, T2 - 8, "f_vel", "VELOCITY");
        b.sec(14, 304, 1266, 572, "AMPLIFIER");
        b.K(390, B1, "a_att", "ATTACK");
        b.K(520, B1, "a_dec", "DECAY");
        b.K(650, B1, "a_sus", "SUSTAIN");
        b.K(780, B1, "a_rel", "RELEASE");
        b.B(910, B1 - 8, "a_vel", "VELOCITY");
        ps.push_back(b.pg);
    }
    {
        Builder b;
        b.pg.name = "MOD";
        b.sec(14, 16, 1266, 284, "LFO");
        b.K(80, T1, "lfo_freq", "FREQUENCY");
        b.B(200, T1 - 8, "lfo_sync", "LFO SYNC");
        b.K(330, T1, "lfo_shape", "SHAPE");
        b.L(380, T1 - 52, "lfo_shape", 110);
        b.K(620, T1, "lfo_amt", "AMOUNT");
        const char *ld[8][2] = {{"lfo_f1", "FREQ 1"}, {"lfo_f2", "FREQ 2"}, {"lfo_f3", "FREQ 3"}, {"lfo_pw1", "PW 1"},
                                {"lfo_pw2", "PW 2"}, {"lfo_pw3", "PW 3"}, {"lfo_filt", "FILTER"}, {"lfo_amp", "AMP"}};
        for (int i = 0; i < 8; i++)
            b.B(80.0f + 150.0f * (float)i, T2 - 8, ld[i][0], ld[i][1]);
        b.note(950, T1, "the mod wheel raises the amount");
        b.sec(14, 304, 1266, 572, "POLYMOD");
        b.K(80, B1, "pm_osc3", "OSC 3 AMOUNT");
        b.K(220, B1, "pm_fenv", "FILTER ENV");
        const char *pd[7][2] = {{"pm_f1", "FREQ 1"}, {"pm_f2", "FREQ 2"}, {"pm_f3", "FREQ 3"}, {"pm_filt", "FILTER"},
                                {"pm_pw1", "PW 1"},  {"pm_pw2", "PW 2"},  {"pm_fdbk", "FDBK"}};
        for (int i = 0; i < 7; i++)
            b.B(380.0f + 130.0f * (float)(i % 4), (i < 4 ? B1 : B2) - 8, pd[i][0], pd[i][1]);
        ps.push_back(b.pg);
    }
    {
        Builder b;
        b.pg.name = "AT/FX";
        b.sec(14, 16, 1266, 284, "AFTER TOUCH");
        b.K(80, T1, "at_amt", "AMOUNT");
        const char *ad[8][2] = {{"at_f1", "FREQ 1"}, {"at_f2", "FREQ 2"}, {"at_f3", "FREQ 3"}, {"at_filt", "FILTER"},
                                {"at_amp", "AMP"},  {"at_lfo", "LFO"},   {"at_fxa", "FX MIX A"}, {"at_fxb", "FX MIX B"}};
        for (int i = 0; i < 8; i++)
            b.B(240.0f + 140.0f * (float)(i % 4) + (i >= 4 ? 0.0f : 0.0f), (i < 4 ? T1 : T2) - 8, ad[i][0], ad[i][1]);
        b.sec(14, 304, 1266, 572, "EFFECTS");
        b.B(70, B1 - 8, "fx_on", "ON/OFF");
        b.K(220, B1, "fxa_type", "TYPE A");
        b.K(360, B1, "fxa_mix", "DRY<>WET A");
        b.K(500, B1, "fxa_p1", "A PARAM 1");
        b.K(640, B1, "fxa_p2", "A PARAM 2");
        b.B(770, B1 - 8, "fxa_sync", "A SYNC");
        b.K(900, B1, "fxa_div", "A SYNC TIME");
        b.K(220, B2, "fxb_type", "TYPE B");
        b.K(360, B2, "fxb_mix", "DRY<>WET B");
        b.K(500, B2, "fxb_p1", "B PARAM 1");
        b.K(640, B2, "fxb_p2", "B PARAM 2");
        b.B(770, B2 - 8, "fxb_sync", "B SYNC");
        b.K(900, B2, "fxb_div", "B SYNC TIME");
        b.note(1100, B1, "A, then B");
        b.note(1100, B1 + 22, "B also: Room, Hall,");
        b.note(1100, B1 + 44, "Plate, Spring");
        ps.push_back(b.pg);
    }
    {
        Builder b;
        b.pg.name = "PLAY";
        b.sec(14, 16, 560, 284, "ARPEGGIATOR");
        b.B(80, T1 - 8, "arp_on", "ON/OFF");
        b.K(200, T1, "arp_mode", "MODE");
        b.L(250, T1 - 52, "arp_mode", 110);
        b.K(460, T1, "arp_oct", "OCTAVES");
        b.B(80, T2 - 8, "hold", "HOLD");
        b.sec(572, 16, 860, 284, "CLOCK");
        b.K(716, T1, "divide", "DIVIDE");
        b.note(716, T2 - 20, "on the MPC's tempo");
        b.sec(872, 16, 1266, 284, "SEQUENCER");
        b.B(960, T1 - 8, "seq_on", "PLAY");
        b.note(1069, T2 - 20, "the program's sequence");
        b.sec(14, 304, 1266, 572, "GLIDE / UNISON / KEYS");
        b.B(80, B1 - 8, "glide", "GLIDE");
        b.K(200, B1, "glide_rate", "GLIDE RATE");
        b.K(330, B1, "glide_mode", "GLIDE MODE");
        b.B(520, B1 - 8, "unison", "UNISON");
        b.K(650, B1, "uni_voices", "VOICES");
        b.K(780, B1, "uni_detune", "DETUNE");
        b.K(910, B1, "key_mode", "KEY MODE");
        b.K(1040, B1, "pb_range", "P WHL RANGE");
        b.note(650, B2, "VOICES 'Chord': the program's chord memory");
        ps.push_back(b.pg);
    }
    {
        Builder b;
        b.pg.name = "MASTER";
        b.sec(14, 16, 1266, 284, "MASTER");
        b.K(110, T1, "distortion", "DISTORTION");
        b.K(260, T1, "vintage", "VINTAGE");
        b.K(410, T1, "pan_spread", "PAN SPREAD");
        b.K(560, T1, "pgm_vol", "PGM VOLUME");
        b.K(710, T1, "vol", "VOLUME");
        b.sec(14, 304, 1266, 572, "VOICES");
        b.L(60, B1 - 30, "voices", 140);
        b.L(300, B1 - 30, "quality", 140, {"ECO (2x)", "HIGH (4x)"});
        b.note(380, B2 - 10, "HIGH: the filters at 4x (more CPU)");
        b.note(130, B2 - 10, "12: two Trigons chained");
        ps.push_back(b.pg);
    }
    return ps;
}

/* ----------------------------------------------------------------------------- drawing --- */
void drawPage(Graphics &g, const Page &pg, int layer)
{
    auto r = Rectangle<float>(0.0f, 0.0f, (float)TW, (float)TH);
    drawPanelFace(g, r, 1.0f);
    for (auto &s : pg.secs)
        drawSection(g, s.r, s.title);
    for (auto &c : pg.ctls) {
        if (c.kind == Ctl::knob)
            drawLegend(g, c.legend, Rectangle<float>(c.x - 70.0f, c.y + KS * 0.5f + 4.0f, 140.0f, 20.0f), 16.0f);
        else if (c.kind == Ctl::button)
            drawLegend(g, c.legend, Rectangle<float>(c.x - 70.0f, c.y + BH * 0.5f + 6.0f, 140.0f, 20.0f), 15.0f);
        else {
            g.setColour(col::legend);
            g.setFont(font(Face::title, 15.0f));
            for (int i = 0; i < c.names.size(); i++)
                g.drawText(c.names[i], Rectangle<float>(c.x + 20.0f, c.y + c.rh * (float)i, c.w, c.rh), Justification::centredLeft, false);
        }
    }
    g.setColour(col::legend);
    g.setFont(font(Face::label, 14.0f));
    for (auto &[p, t] : pg.notes)
        g.drawText(t, Rectangle<float>(p.x - 160.0f, p.y - 9.0f, 320.0f, 18.0f), Justification::centred, false);
    /* the bottom strip: the name, the page, the layer; the maker's tag */
    g.setFont(font(Face::logo, 20.0f));
    g.drawText("TREEGONE", Rectangle<float>(18.0f, 586.0f, 160.0f, 30.0f), Justification::centredLeft, false);
    g.setFont(font(Face::title, 16.0f));
    if (layer >= 0)   /* (the layer's name: a picture of its own over the one background both layers share) */
        g.drawText(pg.name, Rectangle<float>(180.0f, 586.0f, 400.0f, 30.0f), Justification::centredLeft, false);
    drawTag(g, Rectangle<float>(1280.0f - 18.0f - 200.0f, 628.0f - 14.0f - 24.0f, 200.0f, 24.0f));
    drawWear(g, r, 1.0f, {});
}

Image snapBackground(const Page &pg, int layer)
{
    Image img(Image::RGB, TW, TH, true);
    Graphics g(img);
    drawPage(g, pg, layer);
    return img;
}
const Rectangle<int> LABEL{160, 586, 400, 30};

bool writePng(const Image &img, const File &f)
{
    f.deleteFile();
    FileOutputStream out(f);
    PNGImageFormat png;
    return out.openedOk() && png.writeImageToStream(img, out);
}
var rectVar(Rectangle<int> r) { return Array<var>{r.getX(), r.getY(), r.getWidth(), r.getHeight()}; }

/* the control alone over its patch of the background, at a value */
Image drawCtl(const Image &bg, const Ctl &c, Rectangle<int> r, int index, int steps, int row = -1)
{
    Image img(Image::RGB, r.getWidth(), r.getHeight(), true);
    Graphics g(img);
    g.drawImageAt(bg.getClippedImage(r), 0, 0);
    g.addTransform(AffineTransform::translation((float)-r.getX(), (float)-r.getY()));
    if (c.kind == Ctl::knob) {
        float pos = steps > 1 ? (float)index / (float)(steps - 1) : 0.0f;
        drawKnob(g, Rectangle<float>(KS, KS).withCentre({c.x, c.y}), pos, bipolarOf(c.id));
    } else if (c.kind == Ctl::button) {
        bool lit = !c.cycle && index > 0;
        drawButton(g, Rectangle<float>(BW, BH).withCentre({c.x, c.y}), lit, false, false);
    } else {
        drawLed(g, {c.x + 8.0f, c.y + c.rh * ((float)row + 0.5f)}, 9.0f, index == row ? 1.0f : 0.0f);
    }
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
    for (auto &pg : pages()) {
        String tag = pg.name.toLowerCase().retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789");
        Image bgA = snapBackground(pg, -1);
        writePng(bgA, dir.getChildFile(tag + "_panel.png"));
        writePng(snapBackground(pg, 0).getClippedImage(LABEL), dir.getChildFile(tag + "_label_a.png"));
        writePng(snapBackground(pg, 1).getClippedImage(LABEL), dir.getChildFile(tag + "_label_b.png"));
        Array<var> list;
        int n = 0;
        for (auto &c : pg.ctls) {
            auto rg = rangeOf(c.id);
            int steps = rg.hi - rg.lo + 1;
            auto *o = new DynamicObject();
            o->setProperty("id", c.id);
            o->setProperty("steps", steps);
            Array<var> names;
            for (auto &t : rg.names)
                names.add(t);
            o->setProperty("names", names);
            String base = tag + "_" + String(n++).paddedLeft('0', 3) + "_" + c.id.retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789_");
            if (c.kind == Ctl::knob) {
                auto r = Rectangle<float>(KS + 6.0f, KS + 6.0f).withCentre({c.x, c.y}).toNearestInt();
                int frames = jmax(2, jmin(steps, 128, 9800 / r.getHeight()));
                Image strip(Image::RGB, r.getWidth(), r.getHeight() * frames, true);
                {
                    Graphics sg(strip);
                    for (int f = 0; f < frames; f++) {
                        int idx = frames == steps ? f : (int)std::lround((double)f * (steps - 1) / (frames - 1));
                        sg.drawImageAt(drawCtl(bgA, c, r, idx, steps), 0, f * r.getHeight());
                    }
                }
                writePng(strip, dir.getChildFile(base + "_knob.png"));
                auto fr = r.getUnion(Rectangle<float>(c.x - 64.0f, c.y + KS * 0.5f, 128.0f, 26.0f).toNearestInt());
                o->setProperty("kind", "knob");
                o->setProperty("bipolar", bipolarOf(c.id));
                o->setProperty("rect", rectVar(r));
                o->setProperty("frame", rectVar(fr));
                o->setProperty("frames", frames);
                o->setProperty("strip", base + "_knob.png");
            } else if (c.kind == Ctl::button) {
                auto r = Rectangle<float>(BW + 6.0f, BH + 6.0f).withCentre({c.x, c.y}).toNearestInt();
                o->setProperty("kind", "button");
                o->setProperty("rect", rectVar(r));
                Array<var> rs;
                for (int i = 0; i < steps; i++) {
                    auto *ro = new DynamicObject();
                    ro->setProperty("a", i);
                    ro->setProperty("b", i);
                    ro->setProperty("next", c.cycle ? (i + 1) % steps : (i ? 0 : 1));
                    String f = base + "_button_" + String(i) + ".png";
                    writePng(drawCtl(bgA, c, r, i, steps), dir.getChildFile(f));
                    ro->setProperty("image", f);
                    rs.add(var(ro));
                }
                o->setProperty("ranges", rs);
            } else {
                o->setProperty("kind", "leds");
                Array<var> rows;
                for (int k = 0; k < c.names.size(); k++) {
                    auto r = Rectangle<float>(c.x, c.y + c.rh * (float)k, 20.0f + c.w, c.rh).toNearestInt();
                    auto *ro = new DynamicObject();
                    ro->setProperty("rect", rectVar(r));
                    String on = base + "_led" + String(k) + "_on.png", off = base + "_led" + String(k) + "_off.png";
                    writePng(drawCtl(bgA, c, r, k, steps, k), dir.getChildFile(on));
                    writePng(drawCtl(bgA, c, r, -1, steps, k), dir.getChildFile(off));
                    ro->setProperty("on", on);
                    ro->setProperty("off", off);
                    rows.add(var(ro));
                }
                o->setProperty("rows", rows);
            }
            list.add(var(o));
        }
        auto *po = new DynamicObject();
        po->setProperty("name", pg.name);
        po->setProperty("background", tag + "_panel.png");
        po->setProperty("labelA", tag + "_label_a.png");
        po->setProperty("labelB", tag + "_label_b.png");
        po->setProperty("labelRect", rectVar(LABEL));
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
