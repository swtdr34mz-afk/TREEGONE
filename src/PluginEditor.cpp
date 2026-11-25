/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
#include "PluginEditor.h"

using namespace rvui;
using namespace tg;
using Panel = TreegonePanel;

namespace
{
constexpr float KD = 60.0f;                      /* a knob's size */
constexpr float KS = 50.0f;                      /* a small knob's */
constexpr float BW = 30.0f, BH = 19.0f;          /* a button's */
constexpr float X0 = 30.0f;                      /* the panel's left edge (the maple before it) */

/* ------------------------------------------------------------------------------- a knob --- */
class Knob : public juce::Component, public Panel::Live
{
public:
    using Target = std::function<juce::RangedAudioParameter *()>;
    Knob(Target t, std::function<void(juce::RangedAudioParameter *)> touched) : target(std::move(t)), onTouch(std::move(touched)) {}

    void paint(juce::Graphics &g) override
    {
        float sc = g.getInternalContext().getPhysicalPixelScaleFactor();
        auto r = getLocalBounds().toFloat();
        if (body.isNull() || sc != bodyScale || bipolar != bodyBipolar) {
            bodyScale = sc;
            bodyBipolar = bipolar;
            body = juce::Image(juce::Image::ARGB, juce::jmax(1, (int)std::ceil(r.getWidth() * sc)), juce::jmax(1, (int)std::ceil(r.getHeight() * sc)), true);
            juce::Graphics bg(body);
            bg.addTransform(juce::AffineTransform::scale(sc));
            drawKnob(bg, r, 0.0f, bipolar, false, false);
        }
        g.drawImage(body, r, juce::RectanglePlacement::stretchToFit);
        drawKnobPointer(g, r, shownPos, false);
    }
    void poll() override
    {
        auto *p = target();
        float pos = p ? p->getValue() : 0.0f;
        bool bip = false;
        if (auto *ip = dynamic_cast<juce::AudioParameterInt *>(p))
            bip = ip->getRange().getStart() == 0 && ip->getRange().getEnd() == 254;
        if (pos != shownPos || bip != bipolar) {
            shownPos = pos;
            bipolar = bip;
            repaint();
        }
    }
    void mouseDown(const juce::MouseEvent &e) override
    {
        startY = e.position.y;
        if (auto *p = target()) {
            if (auto *ip = dynamic_cast<juce::AudioParameterInt *>(p))
                startV = ip->get();
            else
                startV = (int)std::lround(p->getValue() * 1000.0f);
            p->beginChangeGesture();
            if (onTouch)
                onTouch(p);
        }
    }
    void mouseDrag(const juce::MouseEvent &e) override
    {
        float dy = startY - e.position.y;
        bool fine = e.mods.isShiftDown();
        auto *p = target();
        if (!p)
            return;
        if (onTouch)
            onTouch(p);
        if (auto *ip = dynamic_cast<juce::AudioParameterInt *>(p)) {
            int lo = ip->getRange().getStart(), hi = ip->getRange().getEnd(), range = hi - lo;
            float px = juce::jlimit(1.0f, 22.0f, 230.0f / (float)juce::jmax(1, range)) * (fine ? 4.0f : 1.0f);
            p->setValueNotifyingHost(p->convertTo0to1((float)juce::jlimit(lo, hi, startV + (int)std::lround(dy / px))));
        } else {
            float v = juce::jlimit(0.0f, 1.0f, (float)startV / 1000.0f + dy / (fine ? 900.0f : 220.0f));
            p->setValueNotifyingHost(v);
        }
    }
    void mouseUp(const juce::MouseEvent &) override
    {
        if (auto *p = target())
            p->endChangeGesture();
    }
    void mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &w) override
    {
        int d = w.deltaY > 0 ? 1 : w.deltaY < 0 ? -1 : 0;
        auto *p = target();
        if (d == 0 || !p)
            return;
        if (onTouch)
            onTouch(p);
        p->beginChangeGesture();
        if (auto *ip = dynamic_cast<juce::AudioParameterInt *>(p)) {
            int range = ip->getRange().getLength();
            int step = range > 200 ? 2 : 1;
            p->setValueNotifyingHost(p->convertTo0to1((float)juce::jlimit(ip->getRange().getStart(), ip->getRange().getEnd(), ip->get() + d * step)));
        } else
            p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, p->getValue() + 0.01f * (float)d));
        p->endChangeGesture();
    }
    void mouseDoubleClick(const juce::MouseEvent &) override
    {
        if (auto *p = target()) {
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->getDefaultValue());
            p->endChangeGesture();
        }
    }

private:
    Target target;
    std::function<void(juce::RangedAudioParameter *)> onTouch;
    bool bipolar = false, bodyBipolar = false;
    float shownPos = -1.0f, startY = 0.0f, bodyScale = 0.0f;
    juce::Image body;
    int startV = 0;
};

/* ------------------------------------------------------------------------------ a button --- */
class Btn : public juce::Component, public Panel::Live
{
public:
    Btn(std::function<void()> press, std::function<bool()> on) : onPress(std::move(press)), isOn(std::move(on))
    {
        setRepaintsOnMouseActivity(true);
    }
    std::function<void(juce::Rectangle<int>)> onRight;
    void setPress(std::function<void()> f) { onPress = std::move(f); }
    void paint(juce::Graphics &g) override
    {
        float sc = g.getInternalContext().getPhysicalPixelScaleFactor();
        int k = (lit ? 1 : 0) | (down ? 2 : 0) | (isMouseOver() ? 4 : 0);
        auto r = getLocalBounds().toFloat();
        auto big = r.expanded(r.getWidth() * 0.6f, r.getHeight() * 1.2f);
        if (sc != cacheScale) {
            for (auto &c : cache)
                c = juce::Image();
            cacheScale = sc;
        }
        if (cache[k].isNull()) {
            cache[k] = juce::Image(juce::Image::ARGB, juce::jmax(1, (int)std::ceil(big.getWidth() * sc)), juce::jmax(1, (int)std::ceil(big.getHeight() * sc)), true);
            juce::Graphics cg(cache[k]);
            cg.addTransform(juce::AffineTransform::scale(sc).translated(-big.getX() * sc, -big.getY() * sc));
            drawButton(cg, r.reduced(1.0f), lit, down, isMouseOver());
        }
        g.drawImage(cache[k], big, juce::RectanglePlacement::stretchToFit);
    }
    void poll() override
    {
        bool l = isOn ? isOn() : false;
        if (l != lit) {
            lit = l;
            repaint();
        }
    }
    void mouseDown(const juce::MouseEvent &e) override
    {
        if (e.mods.isPopupMenu() && onRight) {
            onRight(getScreenBounds());
            return;
        }
        down = true;
        repaint();
        if (onPress)
            onPress();
        poll();
    }
    void mouseUp(const juce::MouseEvent &) override
    {
        down = false;
        repaint();
    }

private:
    std::function<void()> onPress;
    std::function<bool()> isOn;
    bool lit = false, down = false;
    juce::Image cache[8];
    float cacheScale = 0.0f;
};

/* ----------------------------------------------------------------------------- LEDs --- */
class Leds : public juce::Component, public Panel::Live
{
public:
    Leds(int n, std::function<float(int)> level, std::function<void(int)> pick = {}, bool horizontal = false)
        : n(n), level(std::move(level)), pick(std::move(pick)), horizontal(horizontal), shown((size_t)n, -1.0f)
    {
    }
    void paint(juce::Graphics &g) override
    {
        for (int i = 0; i < n; i++)
            drawLed(g, centre(i), 4.6f, shown[(size_t)i]);
    }
    void poll() override
    {
        bool ch = false;
        for (int i = 0; i < n; i++) {
            float v = std::round(juce::jlimit(0.0f, 1.0f, level(i)) * 16.0f) / 16.0f;
            if (v != shown[(size_t)i]) {
                shown[(size_t)i] = v;
                ch = true;
            }
        }
        if (ch)
            repaint();
    }
    void mouseDown(const juce::MouseEvent &e) override
    {
        if (!pick)
            return;
        float best = 1e9f;
        int bi = 0;
        for (int i = 0; i < n; i++) {
            float d = centre(i).getDistanceFrom(e.position);
            if (d < best) {
                best = d;
                bi = i;
            }
        }
        pick(bi);
    }

private:
    juce::Point<float> centre(int i) const
    {
        if (horizontal)
            return {((float)i + 0.5f) * (float)getWidth() / (float)n, (float)getHeight() * 0.5f};
        return {6.0f, ((float)i + 0.5f) * (float)getHeight() / (float)n};
    }
    int n;
    std::function<float(int)> level;
    std::function<void(int)> pick;
    bool horizontal;
    std::vector<float> shown;
};

/* ------------------------------------------------------------------- a seven-segment display --- */
class Seg : public juce::Component, public Panel::Live
{
public:
    Seg(int n, std::function<juce::String()> text, std::function<float()> level = {}) : n(n), text(std::move(text)), level(std::move(level)) {}
    std::function<void(const juce::MouseEvent &)> onClick;
    std::function<void(int)> onWheel;
    void paint(juce::Graphics &g) override { drawSeg7(g, getLocalBounds().toFloat(), shown, n, shownLevel); }
    void poll() override
    {
        auto t = text();
        float l = level ? level() : 1.0f;
        if (t != shown || l != shownLevel) {
            shown = t;
            shownLevel = l;
            repaint();
        }
    }
    void mouseDown(const juce::MouseEvent &e) override
    {
        if (onClick)
            onClick(e);
    }
    void mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &w) override
    {
        int d = w.deltaY > 0 ? 1 : w.deltaY < 0 ? -1 : 0;
        if (d && onWheel)
            onWheel(d);
    }

private:
    int n;
    std::function<juce::String()> text;
    std::function<float()> level;
    juce::String shown = "~";
    float shownLevel = -1.0f;
};

/* ------------------------------------------------- the name line: a red dot-matrix strip (TREEGONE's) --- */
class NameLine : public juce::Component, public Panel::Live
{
public:
    NameLine(Panel &p) : panel(p) {}
    std::function<void(const juce::MouseEvent &)> onClick;
    void poll() override
    {
        juce::String a, b;
        what(a, b);
        if (a != top || b != bottom) {
            top = a;
            bottom = b;
            repaint();
        }
    }
    void paint(juce::Graphics &g) override
    {
        using namespace juce;
        auto r = getLocalBounds().toFloat();
        g.setColour(Colour(0xff0b0303));
        g.fillRoundedRectangle(r, 2.0f);
        g.setColour(Colours::black);
        g.drawRoundedRectangle(r, 2.0f, 1.2f);
        auto in = r.reduced(9.0f, 4.0f);
        Colour on(0xffff3a1e);
        auto line = [&](const String &s, Rectangle<float> a, float h, float alpha) {
            g.setFont(font(Face::mono, h));
            g.setColour(on.withAlpha(0.25f * alpha));
            for (auto o : {Point<float>(-0.8f, 0.0f), Point<float>(0.8f, 0.0f), Point<float>(0.0f, 0.8f)})
                g.drawText(s, a.translated(o.x, o.y), Justification::centredLeft, true);
            g.setColour(on.interpolatedWith(Colour(0xffffb090), 0.2f).withAlpha(alpha));
            g.drawText(s, a, Justification::centredLeft, true);
        };
        line(top, in.removeFromTop(in.getHeight() * 0.5f), 17.0f, 1.0f);
        line(bottom, in, 15.0f, 0.8f);
        /* the matrix's grid over it */
        g.setColour(Colour(0xff0b0303).withAlpha(0.45f));
        for (float y = r.getY() + 2.0f; y < r.getBottom(); y += 2.0f)
            g.drawHorizontalLine((int)y, r.getX() + 2.0f, r.getRight() - 2.0f);
        g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.06f), r.getX(), r.getY(), Colours::transparentWhite, r.getX(), r.getCentreY(), false));
        g.fillRoundedRectangle(r.reduced(1.0f), 2.0f);
    }
    void mouseDown(const juce::MouseEvent &e) override
    {
        if (onClick)
            onClick(e);
    }

private:
    void what(juce::String &a, juce::String &b) const
    {
        auto &proc = panel.proc;
        int pg = proc.getCurrentProgram();
        int src = proc.sourceOf(pg);
        juce::String where = src == 0 ? "TREEGONE " + juce::String(pg).paddedLeft('0', 3)
                             : src == 1 ? "TRIGON " + proc.trigonPlace(pg)
                                        : "USER " + juce::String(proc.numberInSource(pg) + 1).paddedLeft('0', 3);
        a = proc.programName().toUpperCase();
        if (juce::Time::getMillisecondCounterHiRes() - panel.messageAt < 4000.0 && panel.message.isNotEmpty()) {
            b = panel.message.toUpperCase();
            return;
        }
        if (panel.showingValue()) {
            if (panel.shownGlobal) {
                b = panel.shownGlobal->getName(30).toUpperCase() + ": " + panel.shownGlobal->getCurrentValueAsText();
                return;
            }
            int p = panel.shownParam;
            int v = proc.value(p);
            char t[48];
            paramText(p, v, t, sizeof t, p == P_LFO_FREQ && proc.value(P_LFO_SYNC));
            b = juce::String(PARAMS[p].name).toUpperCase() + ": " + juce::String(t);
            return;
        }
        b = where;
    }
    Panel &panel;
    juce::String top, bottom;
};
} // namespace

/* ================================================================================= the panel === */
template <class T, class... A> T *Panel::add(A &&...a)
{
    auto c = std::make_unique<T>(std::forward<A>(a)...);
    T *raw = c.get();
    addAndMakeVisible(raw);
    if (auto *l = dynamic_cast<Live *>(raw))
        live.push_back(l);
    parts.push_back(std::move(c));
    return raw;
}

Panel::TreegonePanel(TreegoneProcessor &p) : proc(p)
{
    setOpaque(true);
    setLookAndFeel(&look);
    build();
    startTimerHz(30);
}
Panel::~TreegonePanel()
{
    stopTimer();
    if (dialog)
        dialog->exitModalState(0);
    delete dialog.getComponent();
    setLookAndFeel(nullptr);
}

void Panel::touch(int p)
{
    shownParam = p;
    shownGlobal = nullptr;
    shownAt = juce::Time::getMillisecondCounterHiRes();
}
void Panel::touchGlobal(juce::RangedAudioParameter *q)
{
    shownGlobal = q;
    shownParam = -1;
    shownAt = juce::Time::getMillisecondCounterHiRes();
}
void Panel::say(const juce::String &m)
{
    message = m;
    messageAt = juce::Time::getMillisecondCounterHiRes();
}
bool Panel::showingValue() const
{
    return (shownParam >= 0 || shownGlobal) && juce::Time::getMillisecondCounterHiRes() - shownAt < 2500.0;
}

/* ------------------------------------------------------------------------------- programs --- */
void Panel::selectDigit(int d)
{
    /* the Trigon's way: BANK and TENS chosen, a number button picks the program in them */
    int cur = proc.getCurrentProgram(), src = proc.sourceOf(cur);
    int num = proc.numberInSource(cur);
    int want = (num / 10) * 10 + d;
    for (int i = 0; i < proc.getNumPrograms(); i++)
        if (proc.sourceOf(i) == src && proc.numberInSource(i) == want) {
            proc.setCurrentProgram(i);
            shownParam = -1;
            shownGlobal = nullptr;
            return;
        }
}
void Panel::stepBank()
{
    int cur = proc.getCurrentProgram(), src = proc.sourceOf(cur), num = proc.numberInSource(cur);
    /* the next bank that has programs (wrapping round) */
    for (int k = 1; k <= 10; k++) {
        int bank = (num / 100 + k) % 10, best = -1, bestNum = 100000;
        for (int i = 0; i < proc.getNumPrograms(); i++)
            if (proc.sourceOf(i) == src && proc.numberInSource(i) / 100 == bank) {
                int n = proc.numberInSource(i);
                int score = std::abs(n % 100 - num % 100);
                if (score < bestNum) {
                    bestNum = score;
                    best = i;
                }
            }
        if (best >= 0) {
            proc.setCurrentProgram(best);
            return;
        }
    }
}
void Panel::stepTens()
{
    int cur = proc.getCurrentProgram(), src = proc.sourceOf(cur), num = proc.numberInSource(cur);
    for (int k = 1; k <= 10; k++) {
        int tens = (num / 10 % 10 + k) % 10, want = num / 100 * 100 + tens * 10 + num % 10;
        int best = -1, bestD = 100000;
        for (int i = 0; i < proc.getNumPrograms(); i++)
            if (proc.sourceOf(i) == src && proc.numberInSource(i) / 10 == want / 10) {
                int dd = std::abs(proc.numberInSource(i) - want);
                if (dd < bestD) {
                    bestD = dd;
                    best = i;
                }
            }
        if (best >= 0) {
            proc.setCurrentProgram(best);
            return;
        }
    }
}
void Panel::stepSource()
{
    int src = proc.sourceOf(proc.getCurrentProgram());
    for (int k = 1; k <= 3; k++) {
        int s = (src + k) % 3, at = proc.firstOf(s);
        if (at >= 0) {
            proc.setCurrentProgram(at);
            return;
        }
    }
}

void Panel::programMenu(juce::Rectangle<int> screen)
{
    juce::PopupMenu m;
    m.addSectionHeader("TREEGONE");
    for (int i = 0; i < NFACTORY; i++)
        m.addItem(i + 1, juce::String(i).paddedLeft('0', 3) + "   " + proc.getProgramName(i), true, i == proc.getCurrentProgram());
    if (!proc.imported.empty()) {
        /* the Trigon's banks, each a submenu */
        juce::PopupMenu banks[11];
        for (int i = NFACTORY; i < NFACTORY + (int)proc.imported.size(); i++) {
            auto &im = proc.imported[(size_t)(i - NFACTORY)];
            int b = im.bank < 0 || im.bank > 9 ? 10 : im.bank;
            banks[b].addItem(i + 1, proc.trigonPlace(i) + "   " + juce::String(im.prog.name), true, i == proc.getCurrentProgram());
        }
        m.addSectionHeader("TRIGON-6 (your .syx)");
        for (int b = 0; b < 11; b++)
            if (banks[b].getNumItems() > 0)
                m.addSubMenu(b < 10 ? "Bank " + juce::String(b) : juce::String("Edit buffers"), banks[b]);
    }
    m.addSectionHeader("YOUR PROGRAMS");
    int u0 = NFACTORY + (int)proc.imported.size();
    for (int i = u0; i < proc.getNumPrograms(); i++)
        m.addItem(i + 1, "U" + juce::String(i - u0 + 1).paddedLeft('0', 2) + "   " + juce::String(proc.userPrograms[(size_t)(i - u0)].name),
                  true, i == proc.getCurrentProgram());
    if (proc.userPrograms.empty())
        m.addItem(-1, "(WRITE keeps the sound as a program of your own)", false, false);
    juce::Component::SafePointer<Panel> self(this);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(screen).withMinimumWidth(280), [self](int r) {
        if (self && r > 0) {
            self->proc.setCurrentProgram(r - 1);
            self->shownParam = -1;
            self->shownGlobal = nullptr;
        }
    });
}

void Panel::choiceMenu(int p, juce::Rectangle<int> screen)
{
    juce::PopupMenu m;
    const ParamDef &d = PARAMS[p];
    m.addSectionHeader(d.name);
    int cur = proc.value(p);
    for (int v = d.lo; v <= d.hi; v++) {
        char t[40];
        paramText(p, v, t, sizeof t);
        m.addItem(v - d.lo + 1, t, true, v == cur);
    }
    juce::Component::SafePointer<Panel> self(this);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(screen).withMinimumWidth(160), [self, p, d](int r) {
        if (self && r > 0) {
            self->proc.setValue(p, d.lo + r - 1);
            self->touch(p);
        }
    });
}

void Panel::globalsMenu(juce::Rectangle<int> screen)
{
    juce::PopupMenu m;
    auto glob = [this](const char *id) { return proc.apvts.getParameter(id); };
    bool twelve = glob("voices")->getValue() > 0.5f;
    m.addSectionHeader("GLOBALS");
    m.addItem(1, "6 voices (one Trigon-6)", true, !twelve);
    m.addItem(2, "12 voices (two, poly-chained)", true, twelve);
    m.addSeparator();
    bool high = glob("quality")->getValue() > 0.5f;
    m.addItem(3, "Quality HIGH (filters at 4x)", true, high);
    m.addItem(4, "Quality ECO (2x, less CPU)", true, !high);
    m.addItem(5, "Clock follows the host (HOST SYNC)", true, glob("host_sync")->getValue() > 0.5f);
    m.addSeparator();
    juce::PopupMenu uni, det, gm;
    for (int v = 0; v <= 6; v++)
        uni.addItem(100 + v, v < 6 ? juce::String(v + 1) + (v ? " voices" : " voice") : juce::String("Chord memory"), true, proc.value(P_UNI_VOICES) == v);
    uni.addSeparator();
    uni.addItem(150, "Keep the keys held as the chord", true, false);
    for (int v = 0; v <= 15; v++)
        det.addItem(200 + v, juce::String(v), true, proc.value(P_UNI_DETUNE) == v);
    for (int v = 0; v < NGLIDEMODE; v++) {
        char t[32];
        paramText(P_GLIDE_MODE, v, t, sizeof t);
        gm.addItem(300 + v, t, true, proc.value(P_GLIDE_MODE) == v);
    }
    m.addSubMenu("Unison voices", uni);
    m.addSubMenu("Unison detune", det);
    m.addSubMenu("Glide mode", gm);
    m.addSeparator();
    m.addItem(10, "Load Trigon-6 programs (.syx) ...");
    m.addItem(11, "Remove the loaded Trigon programs", !proc.imported.empty());
    m.addItem(12, "Show the TREEGONE folder");
    m.addSeparator();
    m.addItem(13, "Panic (all sound off)");
    juce::Component::SafePointer<Panel> self(this);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(screen).withMinimumWidth(240), [self, glob](int r) {
        if (!self || r <= 0)
            return;
        auto setG = [&](const char *id, float v) {
            auto *q = glob(id);
            q->beginChangeGesture();
            q->setValueNotifyingHost(v);
            q->endChangeGesture();
            self->touchGlobal(q);
        };
        auto &pr = self->proc;
        if (r == 1 || r == 2)
            setG("voices", r == 2 ? 1.0f : 0.0f);
        else if (r == 3 || r == 4)
            setG("quality", r == 3 ? 1.0f : 0.0f);
        else if (r == 5)
            setG("host_sync", glob("host_sync")->getValue() > 0.5f ? 0.0f : 1.0f);
        else if (r == 10)
            self->loadSyx();
        else if (r == 11) {
            pr.clearImported();
            self->say("Trigon programs removed");
        } else if (r == 12) {
            auto f = TreegoneProcessor::programFolder().getParentDirectory();
            f.createDirectory();
            f.startAsProcess();
        } else if (r == 13)
            pr.command(TreegoneProcessor::C_PANIC);
        else if (r >= 100 && r <= 106) {
            pr.setValue(P_UNI_VOICES, r - 100);
            self->touch(P_UNI_VOICES);
        } else if (r == 150) {
            pr.command(TreegoneProcessor::C_CHORD);
            self->say("Chord kept");
        } else if (r >= 200 && r < 216) {
            pr.setValue(P_UNI_DETUNE, r - 200);
            self->touch(P_UNI_DETUNE);
        } else if (r >= 300 && r < 300 + NGLIDEMODE) {
            pr.setValue(P_GLIDE_MODE, r - 300);
            self->touch(P_GLIDE_MODE);
        }
    });
}

void Panel::writeDialog()
{
    auto *w = new juce::AlertWindow("WRITE", "Keep the sound as a program of your own:", juce::MessageBoxIconType::NoIcon, this);
    w->setLookAndFeel(&look);
    w->addTextEditor("name", proc.programName(), "Name");
    w->addButton("Write", 1, juce::KeyPress(juce::KeyPress::returnKey));
    w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Panel> self(this);
    dialog = w;
    w->enterModalState(true, juce::ModalCallbackFunction::create([self, w](int r) {
                           if (self && r == 1) {
                               if (self->proc.writeProgram(w->getTextEditorContents("name")) >= 0)
                                   self->say("Written");
                           }
                       }),
                       true);
}

void Panel::loadSyx()
{
    chooser = std::make_unique<juce::FileChooser>("Trigon-6 programs (.syx)", juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.syx");
    juce::Component::SafePointer<Panel> self(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [self](const juce::FileChooser &fc) {
        if (!self)
            return;
        auto f = fc.getResult();
        if (f.existsAsFile()) {
            juce::String msg;
            self->proc.importSyx(f, msg);
            self->say(msg);
        }
    });
}

bool Panel::isInterestedInFileDrag(const juce::StringArray &files)
{
    for (auto &f : files)
        if (f.endsWithIgnoreCase(".syx"))
            return true;
    return false;
}
void Panel::filesDropped(const juce::StringArray &files, int, int)
{
    for (auto &f : files)
        if (f.endsWithIgnoreCase(".syx")) {
            juce::String msg;
            proc.importSyx(juce::File(f), msg);
            say(msg);
        }
}

/* ------------------------------------------------------------------------------- the build --- */
static juce::Point<float> at(float x, float y) { return {X0 + x, y}; }

void Panel::build()
{
    using juce::Rectangle;
    auto P = [this](int p) { return [this, p]() -> juce::RangedAudioParameter * { return proc.param(p); }; };
    auto touchP = [this](int p) { return [this, p](juce::RangedAudioParameter *) { touch(p); }; };
    auto place = [](juce::Component *c, float x, float y, float w, float h) {
        c->setBounds(Rectangle<float>(w, h).withCentre(at(x, y)).toNearestInt());
    };
    auto knob = [&](float x, float y, int p, float size = KD) {
        auto *k = add<Knob>(P(p), touchP(p));
        place(k, x, y, size, size);
        return k;
    };
    auto button = [&](float x, float y, std::function<void()> press, std::function<bool()> on) {
        auto *b = add<Btn>(std::move(press), std::move(on));
        place(b, x, y, BW, BH);
        return b;
    };
    auto toggle = [this](int p) {
        return [this, p] {
            proc.setValue(p, proc.value(p) ? 0 : 1);
            touch(p);
        };
    };
    auto isOn = [this](int p) { return [this, p] { return proc.value(p) != 0; }; };
    auto pbutton = [&](float x, float y, int p) { return button(x, y, toggle(p), isOn(p)); };
    /* a choice's LEDs (a column at x, from y, step apart), each a click away */
    auto choiceLeds = [&](float x, float y0, float step, int p, int n, bool horizontal = false) {
        auto *l = add<Leds>(n, [this, p](int i) { return proc.value(p) == PARAMS[p].lo + i ? 1.0f : 0.0f; },
                            [this, p](int i) {
                                proc.setValue(p, PARAMS[p].lo + i);
                                touch(p);
                            },
                            horizontal);
        if (horizontal)
            l->setBounds((int)(X0 + x), (int)(y0 - 7.0f), (int)(step * (float)n), 14);
        else
            l->setBounds((int)(X0 + x - 6.0f), (int)(y0 - step * 0.5f), 12, (int)(step * (float)n));
        return l;
    };
    auto glob = [this](const char *id) { return proc.apvts.getParameter(id); };

    /* VOLUME, AFTERTOUCH */
    {
        auto *k = add<Knob>([glob] { return glob("volume"); }, [this](juce::RangedAudioParameter *q) { touchGlobal(q); });
        place(k, 95, 95, KD, KD);
        knob(95, 228, P_AT_AMT);
        const float xs[3] = {45, 95, 145};
        const int dst[8] = {P_AT_F1, P_AT_F2, P_AT_F3, P_AT_FILT, P_AT_AMP, P_AT_LFO, P_AT_FXA, P_AT_FXB};
        for (int i = 0; i < 8; i++)
            pbutton(i < 6 ? xs[i % 3] : xs[i - 6] + 25.0f, 300.0f + 50.0f * (float)(i / 3), dst[i]);
    }
    /* LFO */
    {
        knob(270, 95, P_LFO_FREQ);
        pbutton(338, 82, P_LFO_SYNC);
        knob(410, 95, P_LFO_SHAPE);
        choiceLeds(460, 66, 14, P_LFO_SHAPE, NLFOSHAPE);
        knob(575, 95, P_LFO_AMT);
        const int dst[8] = {P_LFO_F1, P_LFO_F2, P_LFO_F3, P_LFO_PW1, P_LFO_PW2, P_LFO_PW3, P_LFO_FILT, P_LFO_AMP};
        for (int i = 0; i < 8; i++)
            pbutton(262.0f + 47.0f * (float)i, 170, dst[i]);
    }
    /* POLYMOD */
    {
        knob(282, 300, P_PM_OSC3);
        knob(372, 300, P_PM_FENV);
        const int top[4] = {P_PM_F1, P_PM_F2, P_PM_F3, P_PM_FILT}, bot[3] = {P_PM_PW1, P_PM_PW2, P_PM_FDBK};
        for (int i = 0; i < 4; i++)
            pbutton(445.0f + 50.0f * (float)i, 282, top[i]);
        for (int i = 0; i < 3; i++)
            pbutton(445.0f + 50.0f * (float)i, 350, bot[i]);
    }
    /* OSCILLATORS: three rows */
    {
        const float ry[3] = {100, 215, 330};
        const int octP[3] = {P_O1_OCT, P_O2_OCT, P_O3_OCT};
        const int tri[3] = {P_O1_TRI, P_O2_TRI, P_O3_TRI}, saw[3] = {P_O1_SAW, P_O2_SAW, P_O3_SAW}, pul[3] = {P_O1_PULSE, P_O2_PULSE, P_O3_PULSE};
        const int pw[3] = {P_O1_PW, P_O2_PW, P_O3_PW}, lvl[3] = {P_O1_LEVEL, P_O2_LEVEL, P_O3_LEVEL};
        for (int o = 0; o < 3; o++) {
            float y = ry[o];
            knob(715, y + 6, octP[o], KS);
            choiceLeds(o == 2 ? 683 : 690, y - 30, 12.5f, octP[o], o == 2 ? 6 : 5, true);
            if (o == 0)
                pbutton(800, y, P_SYNC);
            else
                knob(800, y + 4, o == 1 ? P_O2_PITCH : P_O3_PITCH);
            pbutton(872, y, tri[o]);
            if (o < 2)
                pbutton(917, y, saw[o]);
            else {
                /* osc 3's sawtooth button: off, sawtooth, reverse sawtooth (its two LEDs) */
                button(917, y, [this] {
                    proc.setValue(P_O3_SAW, (proc.value(P_O3_SAW) + 1) % 3);
                    touch(P_O3_SAW);
                }, [this] { return proc.value(P_O3_SAW) != 0; });
                add<Leds>(2, [this](int i) { return proc.value(P_O3_SAW) == i + 1 ? 1.0f : 0.0f; }, [this](int i) {
                    proc.setValue(P_O3_SAW, proc.value(P_O3_SAW) == i + 1 ? 0 : i + 1);
                    touch(P_O3_SAW);
                }, true)->setBounds((int)(X0 + 903), (int)(y - 31), 28, 12);
            }
            pbutton(962, y, pul[o]);
            knob(1035, y + 4, pw[o]);
            knob(1110, y + 4, lvl[o]);
        }
        knob(1188, ry[0] + 4, P_FDBK_DRIVE);
        knob(1188, ry[1] + 4, P_NOISE);
        pbutton(1188, ry[2], P_O3_KEY);
    }
    /* FILTER: TYPE (TREEGONE's), CUTOFF, RESONANCE, ENV AMT, SVF MODE; 2 POLE, KEYBOARD; the envelope */
    {
        button(1288, 78, [this] {
            proc.setValue(P_F_TYPE, (proc.value(P_F_TYPE) + 1) % NFILTERTYPE);
            touch(P_F_TYPE);
        }, [] { return false; });
        choiceLeds(1316, 64, 15, P_F_TYPE, NFILTERTYPE);
        knob(1440, 92, P_CUTOFF, 66.0f);
        knob(1520, 92, P_RESO);
        knob(1595, 92, P_F_ENV_AMT);
        knob(1290, 140, P_SVF_MODE, KS);
        pbutton(1660, 58, P_POLE2);
        button(1660, 128, [this] {
            proc.setValue(P_F_KEY, (proc.value(P_F_KEY) + 1) % 3);
            touch(P_F_KEY);
        }, [this] { return proc.value(P_F_KEY) != 0; });
        add<Leds>(2, [this](int i) { return proc.value(P_F_KEY) == i + 1 ? 1.0f : 0.0f; }, [this](int i) {
            proc.setValue(P_F_KEY, proc.value(P_F_KEY) == i + 1 ? 0 : i + 1);
            touch(P_F_KEY);
        }, true)->setBounds((int)(X0 + 1645), (int)(108), 30, 12);
        const int env[4] = {P_F_ATT, P_F_DEC, P_F_SUS, P_F_REL};
        for (int i = 0; i < 4; i++)
            knob(1365.0f + 75.0f * (float)i, 200, env[i]);
        pbutton(1660, 196, P_F_VEL);
    }
    /* AMPLIFIER */
    {
        const int env[4] = {P_A_ATT, P_A_DEC, P_A_SUS, P_A_REL};
        for (int i = 0; i < 4; i++)
            knob(1365.0f + 75.0f * (float)i, 338, env[i]);
        pbutton(1660, 334, P_A_VEL);
    }
    /* CLOCK: the tempo's display, DIVIDE, BPM, TAP */
    {
        auto *s = add<Seg>(3, [this] {
            if (showingValue() && shownParam == P_DIVIDE) {
                static const char *const D[NDIVIDE] = {"HAF", "QTR", "8.D", "8TH", "8 S", "8 T", "16", "16S", "16T", "32"};
                return juce::String(D[proc.value(P_DIVIDE)]);
            }
            return juce::String((int)std::lround(proc.bpmNow.load()));
        });
        place(s, 1772, 72, 76, 34);
        knob(1740, 150, P_DIVIDE, KS);
        knob(1806, 150, P_BPM, KS);
        button(1772, 214, [this] {
            double now = juce::Time::getMillisecondCounterHiRes();
            if (!taps.empty() && now - taps.back() > 2000.0)
                taps.clear();
            taps.push_back(now);
            if (taps.size() > 4)
                taps.erase(taps.begin());
            if (taps.size() >= 2) {
                double avg = (taps.back() - taps.front()) / (double)(taps.size() - 1);
                proc.setValue(P_BPM, (int)std::lround(60000.0 / avg));
                touch(P_BPM);
            }
        }, [this] { return proc.beatLed.load(); });
    }
    /* EFFECTS: ON/OFF, the two types' displays, A/B; TYPE, 1, 2, SYNC, DRY<>WET (of the one chosen) */
    {
        auto fx = [this](int a, int b) { return proc.fxEditB.load() ? b : a; };
        pbutton(1870, 70, P_FX_ON);
        for (int s = 0; s < 2; s++) {
            auto *d = add<Seg>(3, [this, s] {
                int p = s ? P_FXB_TYPE : P_FXA_TYPE;
                if (showingValue() && proc.fxEditB.load() == s && shownParam >= P_FXA_TYPE && shownParam <= P_FXB_DIV &&
                    shownParam != P_FXA_TYPE && shownParam != P_FXB_TYPE) {
                    char t[8];
                    paramShort(shownParam, proc.value(shownParam), t, sizeof t);
                    return juce::String(t);
                }
                return juce::String(FXTYPE_SHORT[proc.value(p)]);
            }, [this, s] { return proc.value(P_FX_ON) ? (proc.fxEditB.load() == s ? 1.0f : 0.55f) : 0.2f; });
            d->onClick = [this, s](const juce::MouseEvent &) { proc.fxEditB = s; };
            place(d, 1945.0f + 74.0f * (float)s, 70, 66, 30);
        }
        button(2090, 62, [this] { proc.fxEditB = proc.fxEditB.load() ? 0 : 1; }, [] { return false; });
        add<Leds>(2, [this](int i) { return proc.fxEditB.load() == i ? 1.0f : 0.0f; }, [this](int i) { proc.fxEditB = i; })
            ->setBounds((int)(X0 + 2078), 74, 12, 28);
        auto fxKnob = [&](float x, float y, std::function<int()> which) {
            auto *k = add<Knob>([this, which] { return proc.param(which()); }, [this, which](juce::RangedAudioParameter *) { touch(which()); });
            place(k, x, y, KS, KS);
        };
        fxKnob(1985, 115, [this, fx] { return fx(P_FXA_TYPE, P_FXB_TYPE); });
        /* PARAM 1: synced, a delay's time is a note of the clock */
        fxKnob(1880, 185, [this, fx] {
            int t = proc.value(fx(P_FXA_TYPE, P_FXB_TYPE));
            bool dly = t == FX_DDL || t == FX_BBD;
            return dly && proc.value(fx(P_FXA_SYNC, P_FXB_SYNC)) ? fx(P_FXA_DIV, P_FXB_DIV) : fx(P_FXA_P1, P_FXB_P1);
        });
        fxKnob(1950, 185, [fx] { return fx(P_FXA_P2, P_FXB_P2); });
        button(2018, 176, [this, fx] {
            int p = fx(P_FXA_SYNC, P_FXB_SYNC);
            proc.setValue(p, proc.value(p) ? 0 : 1);
            touch(p);
        }, [this, fx] { return proc.value(fx(P_FXA_SYNC, P_FXB_SYNC)) != 0; });
        fxKnob(2080, 185, [fx] { return fx(P_FXA_MIX, P_FXB_MIX); });
    }
    /* ARPEGGIATOR */
    {
        pbutton(1730, 330, P_ARP_ON);
        knob(1800, 340, P_ARP_OCT, KS);
        choiceLeds(1835, 322, 14, P_ARP_OCT, 3);
        knob(1885, 340, P_ARP_MODE, KS);
        choiceLeds(1920, 312, 13, P_ARP_MODE, NARPMODE);
    }
    /* SEQUENCER: PLAY, RECORD; while recording REST, TIE */
    {
        button(2025, 315, [this] {
            proc.setValue(P_SEQ_ON, proc.value(P_SEQ_ON) ? 0 : 1);
            touch(P_SEQ_ON);
        }, [this] { return proc.playingNow.load(); });
        button(2085, 315, [this] { proc.command(TreegoneProcessor::C_RECORD, proc.recording.load() ? 0 : 1); },
               [this] { return proc.recording.load(); });
        button(2025, 378, [this] { proc.command(TreegoneProcessor::C_REST); }, [] { return false; });
        button(2085, 378, [this] { proc.command(TreegoneProcessor::C_TIE); }, [] { return false; });
    }

    /* the strip: TRANSPOSE, HOLD, GLIDE RATE, GLIDE, UNISON */
    {
        auto tr = [this, glob](int d) {
            return [this, glob, d] {
                auto *q = dynamic_cast<juce::AudioParameterInt *>(glob("transpose"));
                q->beginChangeGesture();
                *q = juce::jlimit(-2, 2, q->get() + d);
                q->endChangeGesture();
                touchGlobal(q);
            };
        };
        button(45, 560, tr(-1), [] { return false; });
        button(140, 560, tr(1), [] { return false; });
        add<Leds>(5, [this, glob](int i) {
            auto *q = dynamic_cast<juce::AudioParameterInt *>(glob("transpose"));
            return q && q->get() == i - 2 ? 1.0f : 0.0f;
        }, [this, glob](int i) {
            auto *q = dynamic_cast<juce::AudioParameterInt *>(glob("transpose"));
            q->beginChangeGesture();
            *q = i - 2;
            q->endChangeGesture();
            touchGlobal(q);
        }, true)->setBounds((int)(X0 + 55), 512, 75, 14);
        button(215, 560, [this] { proc.command(TreegoneProcessor::C_HOLD, proc.holdNow.load() ? 0 : 1); }, [this] { return proc.holdNow.load(); });
        knob(295, 553, P_GLIDE_RATE, KS);
        pbutton(370, 560, P_GLIDE);
        button(440, 560, [this] {
            bool on = proc.value(P_UNISON) != 0;
            proc.setValue(P_UNISON, on ? 0 : 1);
            if (!on && proc.value(P_UNI_VOICES) >= 6)
                proc.command(TreegoneProcessor::C_CHORD);
            touch(P_UNISON);
        }, isOn(P_UNISON));
    }
    /* the programs: BANK, the display, TENS, 0..9; the name line */
    {
        auto *bank = button(545, 560, [this] { stepBank(); }, [] { return false; });
        bank->setSize(36, 30);
        bank->setCentrePosition(at(545, 560).toInt());
        auto *s = add<Seg>(3, [this] {
            if (showingValue() && shownParam >= 0) {
                char t[8];
                paramShort(shownParam, proc.value(shownParam), t, sizeof t);
                return juce::String(t);
            }
            return juce::String(proc.numberInSource(proc.getCurrentProgram()) % 1000).paddedLeft('0', 3);
        });
        place(s, 638, 556, 112, 50);
        s->onClick = [this, s](const juce::MouseEvent &) { programMenu(s->getScreenBounds()); };
        s->onWheel = [this](int d) {
            int n = proc.getNumPrograms();
            proc.setCurrentProgram(((proc.getCurrentProgram() - d) % n + n) % n);
        };
        auto *tens = button(730, 560, [this] { stepTens(); }, [] { return false; });
        tens->setSize(36, 30);
        tens->setCentrePosition(at(730, 560).toInt());
        for (int d = 0; d < 10; d++)
            button(790.0f + 44.0f * (float)d, 566, [this, d] { selectDigit(d); },
                   [this, d] { return proc.numberInSource(proc.getCurrentProgram()) % 10 == d; });
        auto *nl = add<NameLine>(*this);
        nl->setBounds(Rectangle<float>(X0 + 770, 470, 440, 50).toNearestInt());
        nl->onClick = [this, nl](const juce::MouseEvent &) { programMenu(nl->getScreenBounds()); };
    }
    /* WRITE, GLOBALS, LOAD SYX and the program sources */
    {
        button(1265, 560, [this] { writeDialog(); }, [] { return false; });
        auto *gb = button(1320, 560, [] {}, [] { return false; });
        gb->setComponentID("globals");
        auto *src = button(1375, 560, [this] { stepSource(); }, [] { return false; });
        (void)src;
        add<Leds>(3, [this](int i) { return proc.sourceOf(proc.getCurrentProgram()) == i ? 1.0f : 0.0f; }, [this](int i) {
            int a = proc.firstOf(i);
            if (a >= 0)
                proc.setCurrentProgram(a);
            else if (i == 1)
                loadSyx();
        })->setBounds((int)(X0 + 1400), 525, 12, 48);
        button(1500, 560, [this] { loadSyx(); }, [] { return false; });
    }
    /* DISTORTION, VINTAGE, PAN SPREAD, PGM VOLUME, P WHL RANGE, KEY MODE */
    {
        knob(1585, 553, P_DISTORTION, KS);
        knob(1655, 553, P_VINTAGE, KS);
        knob(1725, 553, P_PAN_SPREAD, KS);
        knob(1795, 553, P_PGM_VOL, KS);
        auto *pw = button(1850, 560, [] {}, [] { return false; });
        pw->setComponentID("pwhl");
        auto *km = button(1905, 560, [] {}, [] { return false; });
        km->setComponentID("keymode");
    }
    /* the menu buttons: opened where they are */
    for (auto &c : parts) {
        auto id = c->getComponentID();
        if (id.isEmpty())
            continue;
        auto *b = dynamic_cast<Btn *>(c.get());
        juce::Component::SafePointer<juce::Component> sp(b);
        int pid = id == "pwhl" ? P_PB_RANGE : id == "keymode" ? P_KEY_MODE : -1;
        auto open = [this, sp, pid, id] {
            if (!sp)
                return;
            if (pid >= 0)
                choiceMenu(pid, sp->getScreenBounds());
            else
                globalsMenu(sp->getScreenBounds());
        };
        b->setPress(open);
    }
}

/* ---------------------------------------------------------------------------- the background --- */
void Panel::paintBackground(juce::Graphics &g, float sc)
{
    using namespace juce;
    const float wood = 24.0f;
    auto face = Rectangle<float>(wood, 0.0f, (float)W - 2.0f * wood, (float)H);
    drawPanelFace(g, face, sc);
    drawMaple(g, {0.0f, 0.0f, wood, (float)H}, 11);
    drawMaple(g, {(float)W - wood, 0.0f, wood, (float)H}, 29);
    auto R = [](float x0, float y0, float x1, float y1) { return Rectangle<float>(X0 + x0, y0, x1 - x0, y1 - y0); };
    auto C = [](float x, float y, float w, float h) { return Rectangle<float>(w, h).withCentre(at(x, y)); };
    auto legend = [&](float x, float y, const String &t, float size = 11.0f) {
        drawLegend(g, t, C(x, y, 110, 16), size);
    };
    auto small = [&](Rectangle<float> r, const String &t, Justification j = Justification::centredLeft, float size = 9.5f) {
        g.setColour(col::legend);
        g.setFont(font(Face::label, size));
        g.drawText(t, r, j, false);
    };
    /* the frames */
    struct S {
        float x0, y0, x1, y1;
        const char *t;
    };
    const S secs[] = {{12, 160, 182, 428, "AFTER TOUCH"},  {196, 30, 630, 210, "LFO"},       {196, 225, 630, 428, "POLYMOD"},
                      {645, 30, 1228, 428, "OSCILLATORS"}, {1243, 30, 1695, 250, "FILTER"},  {1243, 265, 1695, 428, "AMPLIFIER"},
                      {1708, 30, 1840, 250, "CLOCK"},      {1853, 30, 2108, 250, "EFFECTS"}, {1708, 265, 1990, 428, "ARPEGGIATOR"},
                      {2000, 265, 2108, 428, "SEQUENCER"}};
    for (auto &s : secs)
        drawSection(g, R(s.x0, s.y0, s.x1, s.y1), s.t);
    legend(95, 40, "VOLUME", 13.0f);
    legend(95, 265, "AMOUNT");
    {
        const char *at[8] = {"FREQ 1", "FREQ 2", "FREQ 3", "FILTER", "AMP", "LFO", "FX MIX A", "FX MIX B"};
        const float xs[3] = {45, 95, 145};
        for (int i = 0; i < 8; i++)
            small(C(i < 6 ? xs[i % 3] : xs[i - 6] + 25.0f, 317.0f + 50.0f * (float)(i / 3), 60, 12), at[i], Justification::centred, 9.0f);
    }
    /* LFO */
    legend(270, 135, "FREQUENCY");
    legend(338, 100, "LFO SYNC", 9.5f);
    legend(410, 135, "SHAPE");
    for (int i = 0; i < NLFOSHAPE; i++)
        small(C(500, 66.0f + 14.0f * (float)i, 70, 13), LFOSHAPE_NAME[i]);
    legend(575, 135, "AMOUNT");
    {
        const char *d[8] = {"FREQ 1", "FREQ 2", "FREQ 3", "PW 1", "PW 2", "PW 3", "FILTER", "AMP"};
        for (int i = 0; i < 8; i++)
            small(C(262.0f + 47.0f * (float)i, 189, 50, 12), d[i], Justification::centred, 9.0f);
    }
    /* POLYMOD */
    legend(282, 340, "OSC 3 AMOUNT", 9.5f);
    legend(372, 340, "FILTER ENV", 9.5f);
    {
        const char *top[4] = {"FREQ 1", "FREQ 2", "FREQ 3", "FILTER"}, *bot[3] = {"PW 1", "PW 2", "FDBK"};
        for (int i = 0; i < 4; i++)
            small(C(445.0f + 50.0f * (float)i, 300, 50, 12), top[i], Justification::centred, 9.0f);
        for (int i = 0; i < 3; i++)
            small(C(445.0f + 50.0f * (float)i, 368, 50, 12), bot[i], Justification::centred, 9.0f);
    }
    /* OSCILLATORS */
    {
        const float ry[3] = {100, 215, 330};
        for (int o = 0; o < 3; o++) {
            float y = ry[o];
            g.setColour(col::legend);
            g.setFont(font(Face::title, 22.0f));
            g.drawText(String(o + 1), C(668, y, 20, 26), Justification::centred, false);
            legend(715, y + 42, "OCTAVE", 9.5f);
            const char *oct5[5] = {"-2", "-1", "0", "+1", "+2"}, *oct6[6] = {"LO", "-2", "-1", "0", "+1", "+2"};
            for (int i = 0; i < (o == 2 ? 6 : 5); i++)
                small(Rectangle<float>(X0 + (o == 2 ? 683.0f : 690.0f) + 12.5f * (float)i - 2.0f, y - 50.0f, 17.0f, 11.0f),
                      o == 2 ? oct6[i] : oct5[i], Justification::centred, 8.0f);
            legend(800, y + (o ? 40.0f : 22.0f), o ? "PITCH" : "SYNC OSC 2", 9.5f);
            /* the wave symbols over their buttons */
            g.setColour(col::legend);
            auto sym = [&](float x, int k) {
                float cx = X0 + x, cy = y - 22.0f, w = 9.0f, h = 6.0f;
                Path p;
                if (k == 0) {
                    p.startNewSubPath(cx - w, cy + h);
                    p.lineTo(cx, cy - h);
                    p.lineTo(cx + w, cy + h);
                } else if (k == 1) {
                    p.startNewSubPath(cx - w, cy + h);
                    p.lineTo(cx + w, cy - h);
                    p.lineTo(cx + w, cy + h);
                } else if (k == 2) {
                    p.startNewSubPath(cx - w, cy + h);
                    p.lineTo(cx - w, cy - h);
                    p.lineTo(cx + w, cy + h);
                } else {
                    p.startNewSubPath(cx - w, cy + h);
                    p.lineTo(cx - w, cy - h);
                    p.lineTo(cx, cy - h);
                    p.lineTo(cx, cy + h);
                    p.lineTo(cx + w, cy + h);
                }
                g.strokePath(p, PathStrokeType(1.6f));
            };
            sym(872, 0);
            if (o < 2)
                sym(917, 1);
            else {
                /* osc 3: sawtooth and reverse sawtooth, an LED each */
                Path p;
                float cx = X0 + 909.0f, cy = y - 40.0f;
                p.startNewSubPath(cx - 5, cy + 4);
                p.lineTo(cx + 5, cy - 4);
                p.lineTo(cx + 5, cy + 4);
                p.startNewSubPath(cx + 11, cy + 4);
                p.lineTo(cx + 11, cy - 4);
                p.lineTo(cx + 21, cy + 4);
                g.strokePath(p, PathStrokeType(1.4f));
            }
            sym(962, 3);
            legend(1035, y + 42, "PULSE WIDTH", 9.5f);
            legend(1110, y + 42, "VOLUME", 9.5f);
        }
        legend(1188, ry[0] + 42, "FDBK <> DRIVE", 9.5f);
        legend(1188, ry[1] + 42, "NOISE", 9.5f);
        legend(1188, ry[2] + 18, "KEYBOARD", 9.5f);
    }
    /* FILTER */
    {
        legend(1288, 50, "TYPE", 9.5f);
        const char *tn[3] = {"LP1 OTA 4P", "LP2 LADDER 4P", "SVF OTA 2P"};
        for (int i = 0; i < 3; i++)
            small(Rectangle<float>(X0 + 1325.0f, 57.5f + 15.0f * (float)i, 95.0f, 13.0f), tn[i], Justification::centredLeft, 9.0f);
        legend(1290, 176, "SVF MODE", 9.0f);
        small(C(1290, 112, 60, 10), "LP    BP    HP", Justification::centred, 7.5f);
        legend(1440, 136, "CUTOFF");
        legend(1520, 136, "RESONANCE");
        legend(1595, 136, "ENV AMT");
        legend(1660, 75, "2 POLE", 9.5f);
        small(Rectangle<float>(X0 + 1645.0f, 94.0f, 15.0f, 10.0f), "Half", Justification::centred, 7.5f);
        small(Rectangle<float>(X0 + 1660.0f, 94.0f, 15.0f, 10.0f), "Full", Justification::centred, 7.5f);
        legend(1660, 146, "KEYBOARD", 9.0f);
        const char *e[4] = {"ATTACK", "DECAY", "SUSTAIN", "RELEASE"};
        for (int i = 0; i < 4; i++) {
            legend(1365.0f + 75.0f * (float)i, 240, e[i]);
            legend(1365.0f + 75.0f * (float)i, 378, e[i]);
        }
        legend(1660, 214, "VELOCITY", 9.0f);
        legend(1660, 352, "VELOCITY", 9.0f);
    }
    /* CLOCK, EFFECTS, ARPEGGIATOR, SEQUENCER */
    legend(1740, 185, "DIVIDE", 9.5f);
    legend(1806, 185, "BPM", 9.5f);
    legend(1772, 232, "TAP", 9.5f);
    legend(1870, 88, "ON/OFF", 9.0f);
    small(C(1945, 95, 66, 11), "A", Justification::centred, 9.0f);
    small(C(2019, 95, 66, 11), "B", Justification::centred, 9.0f);
    small(Rectangle<float>(X0 + 2092.0f, 74.0f, 12.0f, 14.0f), "A", Justification::centredLeft, 8.5f);
    small(Rectangle<float>(X0 + 2092.0f, 88.0f, 12.0f, 14.0f), "B", Justification::centredLeft, 8.5f);
    legend(2090, 46, "A / B", 9.0f);
    legend(1985, 147, "TYPE", 9.5f);
    legend(1880, 218, "1", 10.0f);
    legend(1950, 218, "2", 10.0f);
    small(C(1915, 152, 120, 11), "EFFECT PARAMETERS", Justification::centred, 8.5f);
    legend(2018, 194, "SYNC", 9.0f);
    legend(2080, 218, "DRY <> WET", 9.0f);
    legend(1730, 348, "ON/OFF", 9.0f);
    legend(1800, 375, "OCTAVES", 9.0f);
    for (int i = 0; i < 3; i++)
        small(Rectangle<float>(X0 + 1843.0f, 315.0f + 14.0f * (float)i, 14.0f, 13.0f), String(i + 1), Justification::centredLeft, 9.0f);
    legend(1885, 375, "MODE", 9.0f);
    for (int i = 0; i < NARPMODE; i++)
        small(Rectangle<float>(X0 + 1928.0f, 305.5f + 13.0f * (float)i, 60.0f, 13.0f), ARPMODE_NAME[i], Justification::centredLeft, 8.5f);
    legend(2025, 333, "PLAY", 9.0f);
    legend(2085, 333, "RECORD", 9.0f);
    legend(2025, 396, "REST", 8.5f);
    legend(2085, 396, "TIE", 8.5f);
    /* the strip */
    g.setColour(col::frame.withAlpha(0.9f));
    g.drawLine(X0 + 12.0f, 450.0f, X0 + 2108.0f, 450.0f, 1.2f);
    legend(92, 595, "TRANSPOSE", 9.5f);
    legend(45, 578, "DOWN", 8.5f);
    legend(140, 578, "UP", 8.5f);
    {
        const char *tn[5] = {"-2", "-1", "0", "+1", "+2"};
        for (int i = 0; i < 5; i++)
            small(Rectangle<float>(X0 + 55.0f + 15.0f * (float)i, 496.0f, 15.0f, 11.0f), tn[i], Justification::centred, 8.0f);
    }
    legend(215, 578, "HOLD", 9.0f);
    legend(295, 590, "GLIDE RATE", 9.0f);
    legend(370, 578, "GLIDE", 9.0f);
    legend(440, 578, "UNISON", 9.0f);
    small(C(545, 528, 70, 11), "Decrement", Justification::centred, 8.0f);
    legend(545, 540, "BANK", 9.0f);
    legend(545, 585, "SELECT", 9.0f);
    legend(638, 590, "BANK  PROGRAM", 9.0f);
    small(C(730, 528, 70, 11), "Increment", Justification::centred, 8.0f);
    legend(730, 540, "TENS", 9.0f);
    legend(730, 585, "SELECT", 9.0f);
    for (int d = 0; d < 10; d++)
        legend(790.0f + 44.0f * (float)d, 588, String(d), 11.0f);
    legend(1265, 578, "WRITE", 9.0f);
    legend(1320, 578, "GLOBALS", 9.0f);
    legend(1375, 578, "SOURCE", 9.0f);
    {
        const char *sn[3] = {"TREEGONE", "TRIGON .SYX", "USER"};
        for (int i = 0; i < 3; i++)
            small(Rectangle<float>(X0 + 1412.0f, 525.0f + 16.0f * (float)i, 80.0f, 16.0f), sn[i], Justification::centredLeft, 8.5f);
    }
    legend(1500, 578, "LOAD SYX", 9.0f);
    legend(1585, 590, "DISTORTION", 9.0f);
    legend(1655, 590, "VINTAGE", 9.0f);
    legend(1725, 590, "PAN SPREAD", 9.0f);
    legend(1795, 590, "PGM VOLUME", 9.0f);
    legend(1850, 578, "P WHL RANGE", 8.5f);
    legend(1905, 578, "KEY MODE", 8.5f);
    /* the name: TREEGONE, as a maker sets its own; the tag beside it */
    {
        auto f = font(Face::logo, 44.0f).withExtraKerningFactor(0.12f);
        GlyphArrangement ga;
        ga.addLineOfText(f, "TREEGONE", 0.0f, 0.0f);
        Path p;
        ga.createPath(p);
        auto b = p.getBounds();
        p.applyTransform(AffineTransform::translation(X0 + 2015.0f - b.getWidth() * 0.5f - b.getX(), 500.0f - b.getY()));
        g.setColour(col::legend);
        g.fillPath(p);
        small(Rectangle<float>(X0 + 2015.0f - 130.0f, 548.0f, 260.0f, 12.0f), "6-VOICE  3-VCO  POLYPHONIC SYNTHESIZER",
              Justification::centred, 8.5f);
        drawTag(g, Rectangle<float>(X0 + 2045.0f, 582.0f, 80.0f, 40.0f));
    }
    for (float x : {(float)wood + 14.0f, (float)W - wood - 14.0f})
        for (float y : {12.0f, (float)H - 12.0f})
            drawScrew(g, {x, y}, 5.5f);
    drawPanelEdge(g, face);
    drawWear(g, face, 1.0f / sc, {at(1440, 92), at(1520, 92), at(95, 95), at(1110, 100), at(1595, 92), at(270, 95), at(638, 556)});
}

void Panel::paint(juce::Graphics &g)
{
    float sc = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (sc != backgroundScale || background.isNull()) {
        backgroundScale = sc;
        background = juce::Image(juce::Image::RGB, juce::jmax(1, (int)std::ceil(W * sc)), juce::jmax(1, (int)std::ceil(H * sc)), true);
        juce::Graphics bg(background);
        bg.addTransform(juce::AffineTransform::scale(sc));
        paintBackground(bg, sc);
    }
    g.drawImage(background, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
}

void Panel::timerCallback()
{
    for (auto *l : live)
        l->poll();
}

/* ============================================================================== the editor === */
TreegoneEditor::TreegoneEditor(TreegoneProcessor &p) : AudioProcessorEditor(&p), proc(p), panel(p)
{
    addAndMakeVisible(panel);
    panel.setBounds(0, 0, TreegonePanel::W, TreegonePanel::H);
    const double W = TreegonePanel::W, H = TreegonePanel::H;
    auto area = juce::Rectangle<int>(0, 0, 1920, 1080);
    if (auto *d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        area = d->userArea;
    double fit = juce::jmin((area.getWidth() - 40) / W, (area.getHeight() - 90) / H);
    fit = juce::jlimit(0.4, 1.6, fit);
    setResizable(true, true);
    setResizeLimits((int)(W * 0.4), (int)(H * 0.4), (int)(W * juce::jmax(0.5, fit)), (int)(H * juce::jmax(0.5, fit)));
    getConstrainer()->setFixedAspectRatio(W / H);
    double s = proc.uiScaleSet ? (double)proc.uiScale.load() : juce::jmin(fit, 1.0);
    s = juce::jlimit(0.4, fit, s);
    setSize((int)std::lround(W * s), (int)std::lround(H * s));
}

void TreegoneEditor::resized()
{
    float s = (float)getWidth() / (float)TreegonePanel::W;
    panel.setTransform(juce::AffineTransform::scale(s));
    proc.uiScale = s;
    proc.uiScaleSet = true;
}
