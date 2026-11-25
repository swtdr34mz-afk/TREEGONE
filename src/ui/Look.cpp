/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (TREEGONE) */
#include "Look.h"
#include "BinaryData.h"
#include <map>

namespace rvui
{
/* the fonts and the tag: plain pointers, made when first drawn and freed when the last editor closes. Never function
 * statics: those are freed while the host unloads the DLL, under Windows' loader lock (PROPHET 67's lesson) */
struct Store
{
    juce::Typeface::Ptr t[4];
    juce::Image tag;
    std::map<int, juce::Image> coat;     /* the powder coat's tile, per pixel density */
};
static Store *store = nullptr;
static Store &get()
{
    if (store == nullptr)
    {
        store = new Store();
        store->t[0] = juce::Typeface::createSystemTypefaceFor(BinaryData::FiraSansSemiBold_ttf, BinaryData::FiraSansSemiBold_ttfSize);
        store->t[1] = juce::Typeface::createSystemTypefaceFor(BinaryData::FiraSansBold_ttf, BinaryData::FiraSansBold_ttfSize);
        store->t[2] = juce::Typeface::createSystemTypefaceFor(BinaryData::FiraSansExtraBold_ttf, BinaryData::FiraSansExtraBold_ttfSize);
        store->t[3] = juce::Typeface::createSystemTypefaceFor(BinaryData::AnonymousProBold_ttf, BinaryData::AnonymousProBold_ttfSize);
        store->tag = juce::ImageFileFormat::loadFrom(BinaryData::swtdr34mztag_png, BinaryData::swtdr34mztag_pngSize);
    }
    return *store;
}
UiResources::~UiResources()
{
    /* (the last editor closed: the fonts, the coat and the tag let go here, on the message thread, never when the
     * host unloads the plugin: on Windows that runs under the loader's lock, and freeing fonts there hung FL Studio) */
    delete store;
    store = nullptr;
    juce::Typeface::clearTypefaceCache();
    juce::ImageCache::releaseUnusedImages();
}

juce::Font font(Face f, float height) { return juce::Font(juce::FontOptions(get().t[(int)f]).withHeight(height)); }

float textWidth(const juce::Font &f, const juce::String &t)
{
    juce::GlyphArrangement ga;
    ga.addLineOfText(f, t, 0.0f, 0.0f);
    return ga.getBoundingBox(0, -1, true).getWidth();
}

Look::Look()
{
    using namespace juce;
    setColour(ResizableWindow::backgroundColourId, col::panelBottom);
    setColour(PopupMenu::backgroundColourId, Colour(0xff1d1e21));
    setColour(PopupMenu::textColourId, col::legend);
    setColour(PopupMenu::highlightedBackgroundColourId, Colour(0xffb3241a));
    setColour(PopupMenu::highlightedTextColourId, Colours::white);
    setColour(PopupMenu::headerTextColourId, col::dim);
    setColour(AlertWindow::backgroundColourId, Colour(0xff1d1e21));
    setColour(AlertWindow::textColourId, col::legend);
    setColour(TextEditor::backgroundColourId, Colour(0xff0b0b0c));
    setColour(TextEditor::textColourId, col::legend);
    setColour(TextButton::buttonColourId, Colour(0xff2a2b2e));
}

juce::Typeface::Ptr Look::getTypefaceForFont(const juce::Font &) { return get().t[0]; }

void Look::drawPopupMenuBackground(juce::Graphics &g, int w, int h)
{
    g.fillAll(findColour(juce::PopupMenu::backgroundColourId));
    g.setColour(col::frame.withAlpha(0.6f));
    g.drawRect(0, 0, w, h, 1);
}

/* ------------------------------------------------------------------------------- the knob --- */
/* PROPHET 67's knob, seen from above: white scale ticks around it, the black ribbed body, a chrome ring, a dark
 * spun-metal cap and its white line */
void drawKnob(juce::Graphics &g, juce::Rectangle<float> r, float pos, bool bipolar, bool endless, bool pointer)
{
    using namespace juce;
    const float d = jmin(r.getWidth(), r.getHeight()), R = d * 0.5f;
    const Point<float> c = r.getCentre();
    const float a0 = -2.4f, a1 = 2.4f;
    const float ang = endless ? pos * MathConstants<float>::twoPi : a0 + pos * (a1 - a0);
    const float rb = R * 0.70f;

    if (!endless) {                      /* the scale */
        g.setColour(col::legend);
        for (int i = 0; i <= 10; i++) {
            float a = a0 + (a1 - a0) * (float)i / 10.0f;
            g.drawLine(Line<float>(c.getPointOnCircumference(R * 0.84f, a), c.getPointOnCircumference(R * (i == 5 && bipolar ? 1.0f : 0.96f), a)),
                       jmax(1.0f, d * 0.03f));
        }
        if (bipolar) {
            g.setFont(font(Face::title, jmax(8.0f, d * 0.2f)));
            auto pm = c.getPointOnCircumference(R * 0.9f, a0 - 0.42f), pp = c.getPointOnCircumference(R * 0.9f, a1 + 0.42f);
            g.drawText("-", Rectangle<float>(12.0f, 12.0f).withCentre(pm), Justification::centred, false);
            g.drawText("+", Rectangle<float>(12.0f, 12.0f).withCentre(pp), Justification::centred, false);
        }
    }
    g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.75f), c.x, c.y + rb * 0.3f, Colours::transparentBlack, c.x, c.y + rb * 1.5f, true));
    g.fillEllipse(Rectangle<float>(rb * 2.7f, rb * 2.7f).withCentre(c.translated(rb * 0.08f, rb * 0.22f)));
    auto body = Rectangle<float>(rb * 2.0f, rb * 2.0f).withCentre(c);
    g.setColour(Colour(0xff070708));
    g.fillEllipse(body);
    const int ribs = 36;
    for (int i = 0; i < ribs; i++) {
        float a = MathConstants<float>::twoPi * (float)i / (float)ribs;
        float lit = 0.5f + 0.5f * std::cos(a + 0.9f);
        g.setColour(Colours::white.withAlpha(0.04f + 0.10f * lit));
        g.drawLine(Line<float>(c.getPointOnCircumference(rb * 0.90f, a), c.getPointOnCircumference(rb * 0.995f, a)), jmax(0.8f, d * 0.018f));
    }
    const float ro = rb * 0.89f, ri = rb * 0.70f;
    const int slices = 48;
    for (int i = 0; i < slices; i++) {
        float a = MathConstants<float>::twoPi * (float)i / (float)slices, a2 = a + MathConstants<float>::twoPi / (float)slices + 0.01f;
        float k = 0.5f + 0.5f * std::cos(2.0f * (a + 0.75f));
        float v = 0.38f + 0.55f * std::pow(k, 1.6f);
        Path p;
        p.addCentredArc(c.x, c.y, ro, ro, 0.0f, a, a2, true);
        p.lineTo(c.getPointOnCircumference(ri, a2));
        p.addCentredArc(c.x, c.y, ri, ri, 0.0f, a2, a, false);
        p.closeSubPath();
        g.setColour(Colour::fromFloatRGBA(v, v, v * 1.02f, 1.0f));
        g.fillPath(p);
    }
    g.setColour(Colours::black.withAlpha(0.6f));
    g.drawEllipse(Rectangle<float>(ro * 2.0f, ro * 2.0f).withCentre(c), 0.8f);
    g.setGradientFill(ColourGradient(Colour(0xff2a2a2d), c.x - ri * 0.5f, c.y - ri * 0.6f, Colour(0xff0d0d0f), c.x + ri * 0.6f, c.y + ri * 0.7f, false));
    g.fillEllipse(Rectangle<float>(ri * 2.0f, ri * 2.0f).withCentre(c));
    g.setColour(Colours::white.withAlpha(0.06f));
    g.drawEllipse(Rectangle<float>(ri * 1.5f, ri * 1.5f).withCentre(c), 0.6f);
    g.setColour(Colours::black.withAlpha(0.8f));
    g.drawEllipse(Rectangle<float>(ri * 2.0f, ri * 2.0f).withCentre(c), 0.8f);
    if (pointer)
        drawKnobPointer(g, r, pos, endless);
}

void drawKnobPointer(juce::Graphics &g, juce::Rectangle<float> r, float pos, bool endless)
{
    using namespace juce;
    const float d = jmin(r.getWidth(), r.getHeight()), ri = d * 0.5f * 0.70f * 0.70f;
    const Point<float> c = r.getCentre();
    const float ang = endless ? pos * MathConstants<float>::twoPi : -2.4f + pos * 4.8f;
    auto p0 = c.getPointOnCircumference(ri * 0.15f, ang), p1 = c.getPointOnCircumference(ri * 0.96f, ang);
    g.setColour(Colour(0xfff4f2ec));
    g.drawLine({p0, p1}, jmax(1.5f, d * 0.045f));
}

/* ---------------------------------------------------------------------------- the button --- */
void drawButton(juce::Graphics &g, juce::Rectangle<float> r, bool on, bool down, bool over)
{
    using namespace juce;
    if (on) {
        g.setGradientFill(ColourGradient(Colour(0xffff2a10).withAlpha(0.5f), r.getCentreX(), r.getCentreY(), Colours::transparentBlack,
                                         r.getRight() + r.getWidth() * 0.45f, r.getCentreY(), true));
        g.fillEllipse(r.expanded(r.getWidth() * 0.5f, r.getHeight() * 1.1f));
    }
    g.setColour(Colours::black.withAlpha(0.75f));
    g.fillRoundedRectangle(r.translated(0.6f, 1.8f).expanded(0.6f), 2.5f);
    g.setColour(Colour(0xff020202));
    g.fillRoundedRectangle(r, 2.5f);
    auto face = r.reduced(1.8f).translated(0.0f, down ? 0.8f : 0.0f);
    if (on) {
        ColourGradient lit(Colour(0xffffa070), face.getCentreX(), face.getCentreY(), Colour(0xffd81c0a), face.getRight(), face.getBottom(), true);
        lit.addColour(0.5, Colour(0xffff4a1c));
        g.setGradientFill(lit);
    } else
        g.setGradientFill(ColourGradient(Colour(0xff2c2c30), face.getX(), face.getY(), Colour(0xff151517), face.getX(), face.getBottom(), false));
    g.fillRoundedRectangle(face, 1.8f);
    /* the fine grid of a Rev2 button's face */
    g.setColour(on ? Colour(0x40a00a00) : Colour(0x30000000));
    for (float x = face.getX() + 2.0f; x < face.getRight() - 1.0f; x += 2.6f)
        g.drawVerticalLine((int)x, face.getY() + 1.5f, face.getBottom() - 1.5f);
    for (float y = face.getY() + 2.0f; y < face.getBottom() - 1.0f; y += 2.6f)
        g.drawHorizontalLine((int)y, face.getX() + 1.5f, face.getRight() - 1.5f);
    g.setColour(Colours::white.withAlpha(on ? 0.3f : 0.09f));
    g.drawLine(face.getX() + 2.0f, face.getY() + 0.7f, face.getRight() - 2.0f, face.getY() + 0.7f, 1.0f);
    if (over && !down) {
        g.setColour(Colours::white.withAlpha(0.05f));
        g.fillRoundedRectangle(face, 1.8f);
    }
}

void drawLed(juce::Graphics &g, juce::Point<float> c, float s, float level)
{
    s *= 1.15f;
    using namespace juce;
    level = jlimit(0.0f, 1.0f, level);
    auto body = Rectangle<float>(s * 1.5f, s).withCentre(c);
    if (level > 0.02f) {
        g.setGradientFill(ColourGradient(Colour(0xffff3a1a).withAlpha(0.6f * level), c.x, c.y, Colours::transparentBlack, c.x + s * 2.2f, c.y, true));
        g.fillEllipse(Rectangle<float>(s * 4.4f, s * 3.6f).withCentre(c));
    }
    g.setColour(Colours::black.withAlpha(0.9f));
    g.fillRect(body.expanded(1.0f));
    g.setGradientFill(ColourGradient(Colour(0xff4a120e).interpolatedWith(Colour(0xffffb070), level), c.x, c.y,
                                     Colour(0xff2a0a08).interpolatedWith(Colour(0xffff2a10), level), body.getRight(), body.getBottom(), true));
    g.fillRect(body);
}

/* ------------------------------------------------------------------- sections and the face --- */
void drawLegend(juce::Graphics &g, const juce::String &t, juce::Rectangle<float> r, float size, juce::Justification j)
{
    g.setColour(col::legend);
    g.setFont(font(Face::title, size).withExtraKerningFactor(0.02f));
    g.drawFittedText(t, r.toNearestInt(), j, 2, 0.75f);
}

void drawSection(juce::Graphics &g, juce::Rectangle<float> r, const juce::String &title)
{
    using namespace juce;
    auto f = font(Face::title, 16.5f).withExtraKerningFactor(0.05f);
    float tw = textWidth(f, title), cx = r.getCentreX(), gl = cx - tw * 0.5f - 7.0f, gr = cx + tw * 0.5f + 7.0f, rad = 10.0f;
    Path p;
    p.startNewSubPath(gr, r.getY());
    p.lineTo(r.getRight() - rad, r.getY());
    p.quadraticTo(r.getRight(), r.getY(), r.getRight(), r.getY() + rad);
    p.lineTo(r.getRight(), r.getBottom() - rad);
    p.quadraticTo(r.getRight(), r.getBottom(), r.getRight() - rad, r.getBottom());
    p.lineTo(r.getX() + rad, r.getBottom());
    p.quadraticTo(r.getX(), r.getBottom(), r.getX(), r.getBottom() - rad);
    p.lineTo(r.getX(), r.getY() + rad);
    p.quadraticTo(r.getX(), r.getY(), r.getX() + rad, r.getY());
    if (title.isNotEmpty())
        p.lineTo(gl, r.getY());
    else
        p.closeSubPath();
    g.setColour(col::frame);
    g.strokePath(p, PathStrokeType(1.6f));
    if (title.isNotEmpty()) {
        g.setColour(col::legend);
        g.setFont(f);
        g.drawText(title, Rectangle<float>(gl, r.getY() - 11.0f, gr - gl, 22.0f), Justification::centred, false);
    }
}

/* ---------------------------------------------------------------------------------- the coat --- */
/* The panel's finish (SOLARIS-23's): a black textured powder coat on steel, a dense, even field of tiny round bumps,
 * each about a panel unit across and lit from the upper left (a bright crown, a dark foot), the pigment's own grain
 * between them: at arm's length a soft matte black metal, close up a fine pebbling. Made at the pixel density it is
 * shown at (an 8K screen gets its own), kept per density, tiling seamlessly. */
static juce::Image makeCoat(float sc)
{
    using namespace juce;
    const int T = 512;
    std::vector<float> h((size_t)(T * T), 0.0f);
    Random rnd(7719);
    const float spacing = jmax(1.6f, 1.05f * sc);
    const int n = (int)((float)T / spacing);
    const float cell = (float)T / (float)n;
    for (int gy = 0; gy < n; gy++)
        for (int gx = 0; gx < n; gx++) {
            float cx = ((float)gx + 0.2f + 0.6f * rnd.nextFloat()) * cell, cy = ((float)gy + 0.2f + 0.6f * rnd.nextFloat()) * cell;
            float rad = cell * (0.42f + 0.18f * rnd.nextFloat()), amp = 0.7f + 0.5f * rnd.nextFloat();
            int x0 = (int)std::floor(cx - rad * 1.8f), x1 = (int)std::ceil(cx + rad * 1.8f);
            int y0 = (int)std::floor(cy - rad * 1.8f), y1 = (int)std::ceil(cy + rad * 1.8f);
            for (int y = y0; y <= y1; y++)
                for (int x = x0; x <= x1; x++) {
                    float dx = ((float)x - cx) / rad, dy = ((float)y - cy) / rad, e = dx * dx + dy * dy;
                    if (e < 3.2f)
                        h[(size_t)(((y % T + T) % T) * T + ((x % T + T) % T))] += amp * std::exp(-e);
                }
        }
    Image im(Image::ARGB, T, T, true);
    Image::BitmapData bd(im, Image::BitmapData::writeOnly);
    const float LX = -0.55f, LY = -0.68f, LZ = 0.48f, ln = std::sqrt(LX * LX + LY * LY + LZ * LZ);
    const float lx = LX / ln, ly = LY / ln, lz = LZ / ln, k = 0.9f;
    const float soft = jlimit(0.55f, 1.0f, sc * 0.5f);   /* (at one pixel a unit the bumps are pixels: quieter) */
    for (int y = 0; y < T; y++)
        for (int x = 0; x < T; x++) {
            float dx = h[(size_t)(y * T + (x + 1) % T)] - h[(size_t)(y * T + (x + T - 1) % T)];
            float dy = h[(size_t)(((y + 1) % T) * T + x)] - h[(size_t)(((y + T - 1) % T) * T + x)];
            float nx = -dx * k, ny = -dy * k, nl = std::sqrt(nx * nx + ny * ny + 1.0f);
            float v = (nx * lx + ny * ly + lz) / nl - lz + (rnd.nextFloat() - 0.5f) * 0.05f;
            Colour c = v > 0.0f ? Colour(0xffe8e6e0).withAlpha(soft * jmin(0.08f, v * 0.22f)) : Colours::black.withAlpha(soft * jmin(0.45f, -v * 0.8f));
            bd.setPixelColour(x, y, c);
        }
    return im;
}

void drawPanelFace(juce::Graphics &g, juce::Rectangle<float> r, float sc)
{
    using namespace juce;
    /* the steel's black, a little warmer and cooler in large soft patches (the coat thicker and thinner) */
    g.setColour(Colour(0xff141416));
    g.fillRect(r);
    Random rnd(1923);
    for (int i = 0; i < (int)(r.getWidth() * r.getHeight() / 30000.0f); i++) {
        float x = r.getX() + rnd.nextFloat() * r.getWidth(), y = r.getY() + rnd.nextFloat() * r.getHeight();
        float s = 40.0f + rnd.nextFloat() * 180.0f;
        Colour c = rnd.nextFloat() < 0.5f ? Colour(0xff26262a) : Colour(0xff0a0a0b);
        ColourGradient cg(c.withAlpha(0.08f), x, y, c.withAlpha(0.0f), x + s, y, true);
        g.setGradientFill(cg);
        g.fillEllipse(x - s, y - s, s * 2.0f, s * 2.0f);
    }
    /* the coat, at this density */
    {
        auto &cache = get().coat;
        int key = jlimit(4, 32, (int)std::lround(sc * 4.0f));
        auto it = cache.find(key);
        if (it == cache.end())
            it = cache.emplace(key, makeCoat((float)key / 4.0f)).first;
        Graphics::ScopedSaveState ss(g);
        g.reduceClipRegion(r.toNearestInt());
        float k = 4.0f / (float)key;
        g.setFillType(FillType(it->second, AffineTransform::scale(k).translated(r.getX(), r.getY())));
        g.fillRect(r);
    }
    /* the light across the sheet: brighter at the upper left, a soft sheen across the middle */
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.035f), r.getX(), r.getY(), Colours::black.withAlpha(0.16f), r.getRight(),
                                     r.getBottom(), false));
    g.fillRect(r);
    ColourGradient sheen(Colours::white.withAlpha(0.0f), r.getX(), r.getY() + r.getHeight() * 0.1f, Colours::white.withAlpha(0.0f),
                         r.getRight(), r.getBottom(), false);
    sheen.addColour(0.42, Colours::white.withAlpha(0.025f));
    g.setGradientFill(sheen);
    g.fillRect(r);
}

void drawPanelEdge(juce::Graphics &g, juce::Rectangle<float> r)
{
    using namespace juce;
    for (int i = 0; i < 4; i++) {        /* the sheet's folded edge: lit along the top, in shadow along the bottom */
        float a = 0.18f - 0.04f * (float)i;
        auto e = r.reduced((float)i + 0.5f);
        g.setColour(Colour(0xffdedcd6).withAlpha(a));
        g.drawLine(e.getX(), e.getY(), e.getRight(), e.getY(), 1.0f);
        g.setColour(Colours::black.withAlpha(a * 2.4f));
        g.drawLine(e.getX(), e.getBottom(), e.getRight(), e.getBottom(), 1.0f);
    }
}

/* A few years of use, over the print: the coat rubbed glossy round the knobs played most, light scuffs, fine
 * scratches (most along the hand's way across the panel), a few chips at the edges, the print worn thin in pin-holes.
 * Gentle: a cared-for instrument, not a relic. px: one screen pixel in panel units (hairlines a pixel wide). Fixed
 * seeds: the same panel every time. */
void drawWear(juce::Graphics &g, juce::Rectangle<float> r, float px, const std::vector<juce::Point<float>> &played)
{
    using namespace juce;
    Random rnd(4242);
    auto P = [&](float mx, float my) { return Point<float>(r.getX() + mx * r.getWidth(), r.getY() + my * r.getHeight()); };
    /* round the knobs played most: the coat polished in a ring, fine circular scratches from the fingers */
    for (auto c : played) {
        float rr = 30.0f + rnd.nextFloat() * 8.0f;
        ColourGradient cg(Colours::white.withAlpha(0.0f), c.x, c.y, Colours::white.withAlpha(0.0f), c.x + rr * 1.6f, c.y, true);
        cg.addColour(0.62, Colours::white.withAlpha(0.035f + 0.02f * rnd.nextFloat()));
        g.setGradientFill(cg);
        g.fillEllipse(Rectangle<float>(rr * 3.2f, rr * 3.2f).withCentre(c));
        for (int k = 0; k < 26; k++) {
            float a0 = rnd.nextFloat() * 6.283f, len = 0.3f + rnd.nextFloat() * 1.1f, rad = rr * (0.85f + rnd.nextFloat() * 0.6f);
            Path p;
            p.addCentredArc(c.x, c.y, rad, rad, 0.0f, a0, a0 + len, true);
            g.setColour(Colour(0xffcfccc4).withAlpha(0.05f + 0.07f * rnd.nextFloat()));
            g.strokePath(p, PathStrokeType(px));
        }
    }
    /* light scuffs */
    for (int i = 0; i < 9; i++) {
        auto c = P(rnd.nextFloat(), rnd.nextFloat());
        float w = 30.0f + rnd.nextFloat() * 120.0f, h = w * (0.2f + rnd.nextFloat() * 0.3f), ang = rnd.nextFloat() * 3.14f;
        Graphics::ScopedSaveState s(g);
        g.addTransform(AffineTransform::rotation(ang, c.x, c.y));
        float a = 0.025f + 0.04f * rnd.nextFloat();
        g.setGradientFill(ColourGradient(Colour(0xff8a8780).withAlpha(a), c.x, c.y, Colour(0xff8a8780).withAlpha(0.0f), c.x + w * 0.5f, c.y, true));
        g.fillEllipse(c.x - w * 0.5f, c.y - h * 0.5f, w, h);
        for (int k = 0; k < 10; k++) {
            float yy = c.y + (rnd.nextFloat() - 0.5f) * h * 0.8f, xx = c.x + (rnd.nextFloat() - 0.5f) * w * 0.6f;
            g.setColour(Colour(0xff9a968c).withAlpha(a * 0.9f));
            g.drawLine(xx - w * 0.2f, yy, xx + w * 0.2f, yy, px);
        }
    }
    /* fine scratches: a groove's shadow and its lit lip */
    for (int i = 0; i < 260; i++) {
        auto a = P(rnd.nextFloat(), rnd.nextFloat());
        float len = rnd.nextFloat() < 0.85f ? 4.0f + rnd.nextFloat() * 30.0f : 30.0f + rnd.nextFloat() * 120.0f;
        float ang = rnd.nextFloat() < 0.6f ? -0.3f + rnd.nextFloat() * 0.6f : rnd.nextFloat() * 3.14f;
        auto b = a + Point<float>(std::cos(ang), std::sin(ang)) * len;
        Path p;
        p.startNewSubPath(a);
        p.quadraticTo((a + b) * 0.5f + Point<float>(rnd.nextFloat() - 0.5f, rnd.nextFloat() - 0.5f) * len * 0.15f, b);
        float w = px * (1.0f + rnd.nextFloat() * 0.6f);
        PathStrokeType st(w, PathStrokeType::curved, PathStrokeType::rounded);
        g.setColour(Colours::black.withAlpha(0.25f));
        g.strokePath(p, st, AffineTransform::translation(w * 0.7f, w * 0.7f));
        g.setColour(Colour(0xffc4c1b8).withAlpha(len > 50.0f ? 0.035f + 0.04f * rnd.nextFloat() : 0.05f + 0.1f * rnd.nextFloat()));
        g.strokePath(p, st);
    }
    /* a few chips at the edges, bare steel with a dark lip */
    for (int i = 0; i < 26; i++) {
        float t = rnd.nextFloat();
        int side = rnd.nextInt(2);
        Point<float> c = side == 0 ? P(t, 0.0f) : P(t, 1.0f);
        c.y += (side == 0 ? 1.0f : -1.0f) * rnd.nextFloat() * 3.0f;
        Path chip;
        int n = 7 + rnd.nextInt(4);
        float s = 0.8f + rnd.nextFloat() * rnd.nextFloat() * 3.5f;
        for (int k = 0; k < n; k++) {
            float a = (float)k / (float)n * 6.283f;
            auto q = c + Point<float>(std::cos(a), std::sin(a)) * s * (0.45f + rnd.nextFloat() * 0.7f);
            if (k)
                chip.lineTo(q);
            else
                chip.startNewSubPath(q);
        }
        chip.closeSubPath();
        g.setColour(Colours::black.withAlpha(0.5f));
        g.strokePath(chip, PathStrokeType(px * 1.4f));
        g.setGradientFill(ColourGradient(Colour(0xffc8c9c4), c.x - s, c.y - s, Colour(0xff626360), c.x + s, c.y + s, false));
        g.fillPath(chip);
    }
    /* the print worn: pin-holes of the coat over it */
    const int n = (int)(r.getWidth() * r.getHeight() * 0.0025f / jmax(0.25f, px));
    for (int i = 0; i < n; i++) {
        auto c = P(rnd.nextFloat(), rnd.nextFloat());
        float s = px * (0.8f + rnd.nextFloat() * 1.4f);
        g.setColour(Colour(0xff141416).withAlpha(0.5f + 0.5f * rnd.nextFloat()));
        g.fillEllipse(c.x, c.y, s, s * (0.6f + rnd.nextFloat() * 0.8f));
    }
}

/* a panel screw: a dome of dull steel, its slot worn a little by screwdrivers */
void drawScrew(juce::Graphics &g, juce::Point<float> c, float rr)
{
    using namespace juce;
    Random rnd((int64)(c.x * 13.0f + c.y * 7.0f));
    g.setColour(Colours::black.withAlpha(0.6f));
    g.fillEllipse(Rectangle<float>(rr * 2.3f, rr * 2.3f).withCentre(c.translated(0.4f, 0.7f)));
    ColourGradient hg(Colour(0xffd6d3cb), c.x - rr * 0.5f, c.y - rr * 0.6f, Colour(0xff3c3b38), c.x + rr, c.y + rr, true);
    hg.addColour(0.35, Colour(0xff95928a));
    g.setGradientFill(hg);
    g.fillEllipse(Rectangle<float>(rr * 2.0f, rr * 2.0f).withCentre(c));
    float a = 0.4f + rnd.nextFloat();
    Path p;
    p.addRoundedRectangle(-rr * 0.75f, -rr * 0.13f, rr * 1.5f, rr * 0.26f, rr * 0.05f);
    g.setColour(Colour(0xff121212));
    g.fillPath(p, AffineTransform::rotation(a).translated(c));
    g.setColour(Colours::white.withAlpha(0.22f));
    g.strokePath(p, PathStrokeType(rr * 0.05f), AffineTransform::rotation(a).translated(c.translated(rr * 0.04f, rr * 0.06f)));
}

void drawWood(juce::Graphics &g, juce::Rectangle<float> r, int seed)
{
    using namespace juce;
    ColourGradient base(Colour(0xff4a2412), r.getX(), 0.0f, Colour(0xff3c1d0f), r.getRight(), 0.0f, false);
    base.addColour(0.35, Colour(0xff7a3e1f));
    base.addColour(0.7, Colour(0xff653319));
    g.setGradientFill(base);
    g.fillRect(r);
    Random rnd(seed);
    for (int i = 0; i < 34; i++) {
        float x0 = r.getX() + rnd.nextFloat() * r.getWidth(), amp = 1.0f + rnd.nextFloat() * 3.5f,
              f = 0.004f + rnd.nextFloat() * 0.01f, ph = rnd.nextFloat() * 6.28f;
        Path p;
        p.startNewSubPath(x0, r.getY());
        for (float y = r.getY(); y <= r.getBottom(); y += 12.0f)
            p.lineTo(x0 + amp * std::sin(y * f + ph) + 1.2f * std::sin(y * f * 3.7f + ph * 2.0f), y);
        bool dark = rnd.nextFloat() < 0.7f;
        g.setColour(dark ? Colour(0xff200d05).withAlpha(0.16f + 0.2f * rnd.nextFloat())
                         : Colour(0xffa86a40).withAlpha(0.10f + 0.12f * rnd.nextFloat()));
        g.strokePath(p, PathStrokeType(0.5f + rnd.nextFloat() * 1.6f));
    }
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.10f), r.getX(), 0.0f, Colours::transparentWhite, r.getX() + r.getWidth() * 0.3f, 0.0f, false));
    g.fillRect(r);
}

void drawMaple(juce::Graphics &g, juce::Rectangle<float> r, int seed)
{
    using namespace juce;
    ColourGradient base(Colour(0xffc8a173), r.getX(), 0.0f, Colour(0xffb88f60), r.getRight(), 0.0f, false);
    base.addColour(0.4, Colour(0xffdcb98a));
    base.addColour(0.75, Colour(0xffcfa877));
    g.setGradientFill(base);
    g.fillRect(r);
    Random rnd(seed);
    for (int i = 0; i < 60; i++) {
        float x0 = r.getX() + rnd.nextFloat() * r.getWidth(), amp = 0.4f + rnd.nextFloat() * 1.2f, f = 0.002f + rnd.nextFloat() * 0.006f,
              ph = rnd.nextFloat() * 6.28f;
        Path p;
        p.startNewSubPath(x0, r.getY());
        for (float y = r.getY(); y <= r.getBottom(); y += 10.0f)
            p.lineTo(x0 + amp * std::sin(y * f + ph), y);
        bool dark = rnd.nextFloat() < 0.65f;
        g.setColour(dark ? Colour(0xff8a5e32).withAlpha(0.10f + 0.16f * rnd.nextFloat()) : Colour(0xfff2dcb4).withAlpha(0.10f + 0.12f * rnd.nextFloat()));
        g.strokePath(p, PathStrokeType(0.4f + rnd.nextFloat() * 1.2f));
    }
    /* the lacquer's sheen, the edges rounded off */
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.22f), r.getX() + r.getWidth() * 0.3f, 0.0f, Colours::transparentWhite,
                                     r.getRight(), 0.0f, false));
    g.fillRect(r);
    g.setColour(Colours::black.withAlpha(0.35f));
    g.drawLine(r.getRight() - 0.5f, r.getY(), r.getRight() - 0.5f, r.getBottom(), 1.0f);
    g.drawLine(r.getX() + 0.5f, r.getY(), r.getX() + 0.5f, r.getBottom(), 1.0f);
}

/* the segments: a b c d e f g as bits 0..6 */
static int segOf(juce::juce_wchar ch)
{
    switch (juce::CharacterFunctions::toUpperCase(ch)) {
    case '0': case 'O': case 'D': return 0x3F;
    case '1': case 'I': return 0x06;
    case '2': case 'Z': return 0x5B;
    case '3': return 0x4F;
    case '4': return 0x66;
    case '5': case 'S': return 0x6D;
    case '6': case 'G': return 0x7D;
    case '7': return 0x07;
    case '8': case 'B': return 0x7F;
    case '9': return 0x6F;
    case 'A': return 0x77;
    case 'C': return 0x39;
    case 'E': return 0x79;
    case 'F': return 0x71;
    case 'H': case 'X': return 0x76;
    case 'J': return 0x1E;
    case 'K': return 0x75;
    case 'L': return 0x38;
    case 'M': case 'N': return 0x37;
    case 'P': return 0x73;
    case 'Q': return 0x67;
    case 'R': return 0x50;
    case 'T': return 0x78;
    case 'U': case 'V': case 'W': return 0x3E;
    case 'Y': return 0x6E;
    case '-': return 0x40;
    case '_': return 0x08;
    case '=': return 0x48;
    default: return 0;
    }
}

void drawSeg7(juce::Graphics &g, juce::Rectangle<float> r, const juce::String &text, int n, float level)
{
    using namespace juce;
    g.setColour(Colour(0xff0b0303));
    g.fillRoundedRectangle(r, 2.0f);
    g.setColour(Colours::black);
    g.drawRoundedRectangle(r, 2.0f, 1.2f);
    /* the characters, right-aligned as the Trigon's */
    std::vector<int> segs;
    std::vector<bool> dots;
    for (int i = 0; i < text.length(); i++) {
        auto c = text[i];
        if (c == '.' && !segs.empty()) {
            dots.back() = true;
            continue;
        }
        segs.push_back(segOf(c));
        dots.push_back(false);
    }
    while ((int)segs.size() < n) {
        segs.insert(segs.begin(), 0);
        dots.insert(dots.begin(), false);
    }
    while ((int)segs.size() > n) {
        segs.pop_back();
        dots.pop_back();
    }
    auto in = r.reduced(r.getHeight() * 0.16f, r.getHeight() * 0.14f);
    float cw = in.getWidth() / (float)n, h = in.getHeight(), w = jmin(cw * 0.72f, h * 0.55f), t = jmax(1.6f, h * 0.11f);
    Colour on = Colour(0xffff2a14), off = Colour(0xff2a0805);
    for (int i = 0; i < n; i++) {
        float x = in.getX() + cw * (float)i + (cw - w) * 0.5f, y = in.getY(), sk = h * 0.08f;
        auto seg = [&](int k) {
            /* each segment a slanted bar (the display leans a little) */
            Point<float> a, b;
            float m = y + h * 0.5f;
            switch (k) {
            case 0: a = {x + t * 0.6f, y}; b = {x + w - t * 0.6f, y}; break;
            case 1: a = {x + w, y + t * 0.6f}; b = {x + w, m - t * 0.5f}; break;
            case 2: a = {x + w, m + t * 0.5f}; b = {x + w, y + h - t * 0.6f}; break;
            case 3: a = {x + t * 0.6f, y + h}; b = {x + w - t * 0.6f, y + h}; break;
            case 4: a = {x, m + t * 0.5f}; b = {x, y + h - t * 0.6f}; break;
            case 5: a = {x, y + t * 0.6f}; b = {x, m - t * 0.5f}; break;
            default: a = {x + t * 0.6f, m}; b = {x + w - t * 0.6f, m}; break;
            }
            auto lean = [&](Point<float> p) { return Point<float>(p.x + sk * (1.0f - (p.y - y) / h), p.y); };
            return Line<float>(lean(a), lean(b));
        };
        for (int k = 0; k < 7; k++) {
            bool lit = (segs[(size_t)i] >> k) & 1;
            auto ln = seg(k);
            if (lit && level > 0.0f) {
                g.setColour(on.withAlpha(0.22f * level));
                g.drawLine(ln, t * 2.6f);
                g.setColour(on.interpolatedWith(Colour(0xffffb090), 0.25f).withMultipliedAlpha(level));
                g.drawLine(ln, t);
            } else {
                g.setColour(off);
                g.drawLine(ln, t);
            }
        }
        if (dots[(size_t)i]) {
            g.setColour(on.withMultipliedAlpha(level));
            g.fillEllipse(x + w + t * 0.6f, y + h - t * 0.6f, t * 1.2f, t * 1.2f);
        }
    }
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.07f), r.getX(), r.getY(), Colours::transparentWhite, r.getX(),
                                     r.getCentreY(), false));
    g.fillRoundedRectangle(r.reduced(1.0f), 2.0f);
}

void drawTag(juce::Graphics &g, juce::Rectangle<float> r)
{
    const juce::Image &tag = get().tag;
    if (tag.isNull())
        return;
    auto fit = juce::RectanglePlacement(juce::RectanglePlacement::centred).appliedTo(tag.getBounds().toFloat(), r);
    g.setColour(col::legend);
    g.drawImage(tag, fit, juce::RectanglePlacement::stretchToFit, true);
}
} // namespace rvui
