#include "BluesEditor.h"

namespace nouveau
{
const juce::Colour parch   { 0xffe9dcbb };
const juce::Colour parchLo { 0xffd6c59a };
const juce::Colour sage    { 0xff6f8f6a };
const juce::Colour sageLo  { 0xff47654a };
const juce::Colour teal    { 0xff1f5a70 };
const juce::Colour tealLo  { 0xff0f2f3d };
const juce::Colour bronze  { 0xffb8863a };
const juce::Colour bronzeHi{ 0xffe6c274 };
const juce::Colour bronzeLo{ 0xff6b4a1c };
const juce::Colour ink     { 0xff2b2416 };

// Lens-shaped leaf from `base` pointing at `angle` (radians, 0 = up, clockwise).
juce::Path leaf (juce::Point<float> base, float angle, float len, float wid)
{
    const auto tip = base.getPointOnCircumference (len, angle);
    const auto l = base.getPointOnCircumference (len * 0.5f, angle - 0.5f);
    const auto r = base.getPointOnCircumference (len * 0.5f, angle + 0.5f);
    const auto lc = juce::Point<float> (l.x + std::cos (angle) * wid * 0.f, l.y);
    juce::ignoreUnused (lc);
    const auto nrm = juce::Point<float> (std::cos (angle), std::sin (angle));   // perpendicular-ish
    const auto mid = base.getPointOnCircumference (len * 0.55f, angle);
    juce::Path p;
    p.startNewSubPath (base);
    p.cubicTo (mid.x - nrm.x * wid, mid.y - nrm.y * wid, mid.x - nrm.x * wid * 0.6f, mid.y - nrm.y * wid * 0.6f, tip.x, tip.y);
    p.cubicTo (mid.x + nrm.x * wid * 0.6f, mid.y + nrm.y * wid * 0.6f, mid.x + nrm.x * wid, mid.y + nrm.y * wid, base.x, base.y);
    p.closeSubPath();
    juce::ignoreUnused (r);
    return p;
}

// Spiral "whiplash" curl starting at c, turns inward.
juce::Path curl (juce::Point<float> c, float r0, float startAngle, float dir, float turns = 1.6f)
{
    juce::Path p;
    const int n = 60;
    for (int i = 0; i <= n; ++i)
    {
        const float t = (float) i / (float) n;
        const float a = startAngle + dir * t * turns * juce::MathConstants<float>::twoPi;
        const float r = r0 * (1.f - 0.85f * t);
        const auto pt = c.getPointOnCircumference (r, a);
        if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    return p;
}

// Vine with alternating leaves along a cubic S-curve.
void vine (juce::Graphics& g, juce::Point<float> a, juce::Point<float> c1, juce::Point<float> c2,
           juce::Point<float> b, bool mirrorLeaves, float leafLen)
{
    juce::Path stem; stem.startNewSubPath (a); stem.cubicTo (c1, c2, b);
    g.setColour (sageLo);
    g.strokePath (stem, juce::PathStrokeType (2.2f));
    juce::Path::Iterator dummy (stem); juce::ignoreUnused (dummy);
    for (int i = 1; i <= 6; ++i)
    {
        const float t = (float) i / 7.f, u = 1.f - t;
        const juce::Point<float> pt (u*u*u*a.x + 3*u*u*t*c1.x + 3*u*t*t*c2.x + t*t*t*b.x,
                                     u*u*u*a.y + 3*u*u*t*c1.y + 3*u*t*t*c2.y + t*t*t*b.y);
        const juce::Point<float> d (3*u*u*(c1.x-a.x) + 6*u*t*(c2.x-c1.x) + 3*t*t*(b.x-c2.x),
                                    3*u*u*(c1.y-a.y) + 6*u*t*(c2.y-c1.y) + 3*t*t*(b.y-c2.y));
        const float ang = std::atan2 (d.x, -d.y);
        const float side = ((i % 2) == 0) != mirrorLeaves ? 1.f : -1.f;
        const auto lf = leaf (pt, ang + side * 1.0f, leafLen * (1.f - 0.06f * (float) i), leafLen * 0.22f);
        g.setColour (sage);
        g.fillPath (lf);
        g.setColour (sageLo);
        g.strokePath (lf, juce::PathStrokeType (0.8f));
    }
}

juce::Font serif (float size, bool italic = true)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultSerifFontName(), size,
                                          italic ? (juce::Font::bold | juce::Font::italic) : juce::Font::bold));
}
} // namespace nouveau

void NouveauLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                           float startAngle, float endAngle, juce::Slider& s)
{
    using namespace nouveau;
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = startAngle + pos * (endAngle - startAngle);
    const bool small = s.getProperties().getWithDefault ("small", false);

    // petal scale around the knob
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.f * (endAngle - startAngle);
        const auto base = c.getPointOnCircumference (r * 0.9f, a);
        g.setColour (bronzeHi);
        g.fillPath (leaf (base, a, r * (small ? 0.22f : 0.26f), r * 0.07f));
    }

    const float kr = r * 0.8f;
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillEllipse (c.x - kr + 1.5f, c.y - kr + 3.f, kr * 2, kr * 2);
    juce::ColourGradient bz (bronzeHi, c.x - kr, c.y - kr, bronzeLo, c.x + kr, c.y + kr, false);
    g.setGradientFill (bz);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);

    // scalloped (petal) rim
    const int petals = small ? 10 : 14;
    for (int i = 0; i < petals; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * ((float) i + 0.5f) / (float) petals;
        g.setColour (bronzeLo.withAlpha (0.55f));
        g.drawLine ({ c.getPointOnCircumference (kr * 0.86f, a), c.getPointOnCircumference (kr * 0.99f, a) }, 1.2f);
    }

    const float br = kr * 0.84f;
    juce::ColourGradient body (juce::Colour (0xff2c7f98), c.x - br, c.y - br, tealLo, c.x + br, c.y + br, false);
    g.setGradientFill (body);
    g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);

    // engraved lily: 5 petals pointing outward
    g.setColour (bronzeHi.withAlpha (0.35f));
    for (int i = 0; i < 5; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 5.f;
        g.fillPath (leaf (c, a, br * 0.72f, br * 0.2f));
    }

    g.setColour (parch);
    g.drawLine ({ c.getPointOnCircumference (br * 0.15f, ang), c.getPointOnCircumference (br * 0.78f, ang) }, small ? 2.2f : 3.2f);
    g.setColour (bronzeHi);
    g.fillEllipse (c.x - 3.5f, c.y - 3.5f, 7.f, 7.f);
    g.setColour (bronzeLo);
    g.drawEllipse (c.x - 3.5f, c.y - 3.5f, 7.f, 7.f, 1.f);
}

Sh0tyBD2Editor::Sh0tyBD2Editor (Sh0tyBD2Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    addKnob (level, "level", "LEVEL", false);
    addKnob (gain,  "gain",  "GAIN",  false);
    addKnob (tone,  "tone",  "TONE",  false);
    addKnob (mix,   "mix",   "MIX",   true);
    addKnob (trim,  "trim",  "TRIM",  true);
    setSize (300, 540);
}

Sh0tyBD2Editor::~Sh0tyBD2Editor() { setLookAndFeel (nullptr); }

void Sh0tyBD2Editor::addKnob (Knob& k, const juce::String& id, const juce::String& text, bool small)
{
    k.slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setRotaryParameters (juce::degreesToRadians (-140.f), juce::degreesToRadians (140.f), true);
    k.slider.getProperties().set ("small", small);
    k.slider.setTooltip (text);
    addAndMakeVisible (k.slider);
    k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    auto f = nouveau::serif (small ? 11.f : 14.f, false);
    f.setExtraKerningFactor (0.18f);
    k.label.setFont (f);
    k.label.setColour (juce::Label::textColourId, small ? nouveau::parch : nouveau::bronzeHi);
    addAndMakeVisible (k.label);
}

void Sh0tyBD2Editor::resized()
{
    const int big = 84, sm = 48;
    auto place = [] (Knob& k, juce::Rectangle<int> r, int labelH)
    {
        k.slider.setBounds (r);
        k.label.setBounds (r.getX() - 6, r.getBottom() - 2, r.getWidth() + 12, labelH);
    };
    place (level, { 42,  92, big, big }, 18);
    place (gain,  { 300 - 42 - big, 92, big, big }, 18);
    place (tone,  { 150 - 38, 178, 76, 76 }, 18);
    place (mix,   { 66, 448, sm, sm }, 16);
    place (trim,  { 300 - 66 - sm, 448, sm, sm }, 16);
}

void Sh0tyBD2Editor::paint (juce::Graphics& g)
{
    using namespace nouveau;
    const float W = (float) getWidth(), H = (float) getHeight();

    // parchment ground
    g.setGradientFill (juce::ColourGradient (parch, 0.f, 0.f, parchLo, 0.f, H, false));
    g.fillAll();

    // organic double border with corner curls
    g.setColour (sage);
    g.drawRoundedRectangle (7.f, 7.f, W - 14.f, H - 14.f, 30.f, 4.f);
    g.setColour (bronze);
    g.drawRoundedRectangle (15.f, 15.f, W - 30.f, H - 30.f, 24.f, 1.3f);
    for (auto corner : { juce::Point<float> (24.f, 24.f), { W - 24.f, 24.f } })
    {
        const bool right = corner.x > W * 0.5f, bottom = corner.y > H * 0.5f;
        g.setColour (bronze);
        g.strokePath (curl (corner, 12.f, right ? (bottom ? 0.f : 3.14f) : (bottom ? 1.57f : -1.57f), (right != bottom) ? 1.f : -1.f), juce::PathStrokeType (1.6f));
    }

    // arched window with halo
    juce::Path arch;
    const float ax = 26.f, aw = W - 52.f, ay = 26.f, ar = aw * 0.5f, ab = 300.f;
    arch.startNewSubPath (ax, ay + ar);
    arch.addArc (ax, ay, aw, aw, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, false);
    arch.lineTo (ax + aw, ab);
    // ogee-curved base
    arch.cubicTo (ax + aw, ab + 14.f, ax + aw * 0.6f, ab + 4.f, ax + aw * 0.5f, ab + 16.f);
    arch.cubicTo (ax + aw * 0.4f, ab + 4.f, ax, ab + 14.f, ax, ab);
    arch.closeSubPath();

    g.setGradientFill (juce::ColourGradient (teal, W * 0.5f, 150.f, tealLo, W * 0.5f, ab + 16.f, true));
    g.fillPath (arch);
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (arch);
        const juce::Point<float> ctr (W * 0.5f, 160.f);
        for (int i = 6; i >= 1; --i)
        {
            const float rr = 22.f * (float) i;
            g.setColour (bronzeHi.withAlpha (i % 2 ? 0.10f : 0.05f));
            g.fillEllipse (ctr.x - rr, ctr.y - rr, rr * 2, rr * 2);
            g.setColour (bronzeHi.withAlpha (0.28f));
            g.drawEllipse (ctr.x - rr, ctr.y - rr, rr * 2, rr * 2, 0.8f);
        }
        // flowing vines up both sides
        vine (g, { ax + 4.f, ab + 6.f }, { ax + 40.f, 250.f }, { ax - 6.f, 190.f }, { ax + 16.f, 110.f }, false, 26.f);
        vine (g, { ax + aw - 4.f, ab + 6.f }, { ax + aw - 40.f, 250.f }, { ax + aw + 6.f, 190.f }, { ax + aw - 16.f, 110.f }, true, 26.f);
    }
    g.setColour (bronze);
    g.strokePath (arch, juce::PathStrokeType (2.2f));
    g.setColour (bronzeHi.withAlpha (0.7f));
    { juce::Path inner; inner.addPath (arch, juce::AffineTransform::scale (0.965f, 0.975f, W * 0.5f, 160.f));
      g.strokePath (inner, juce::PathStrokeType (0.9f)); }

    // CHECK jewel at the apex
    {
        const juce::Point<float> c (W * 0.5f, 60.f);
        g.setColour (bronze);
        g.fillEllipse (c.x - 13.f, c.y - 13.f, 26.f, 26.f);
        g.setColour (bronzeHi);
        g.drawEllipse (c.x - 13.f, c.y - 13.f, 26.f, 26.f, 1.6f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff7a66), c.x - 3.f, c.y - 3.f, juce::Colour (0xff8e0d12), c.x + 7.f, c.y + 7.f, true));
        g.fillEllipse (c.x - 8.5f, c.y - 8.5f, 17.f, 17.f);
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.fillEllipse (c.x - 4.f, c.y - 5.f, 5.f, 3.5f);
        g.setColour (bronzeHi);
        g.setFont (serif (9.f, false));
        g.drawText ("CHECK", (int) c.x - 40, 76, 80, 12, juce::Justification::centred);
    }

    // jack labels
    g.setColour (ink);
    g.setFont (serif (12.f, false));
    g.drawText (juce::String::fromUTF8 ("\xE2\x9D\xA7 OUTPUT"), 28, 322, 110, 16, juce::Justification::left);
    g.drawText (juce::String::fromUTF8 ("INPUT \xE2\x9D\xA7"), (int) W - 138, 322, 110, 16, juce::Justification::right);

    // title lockup with whiplash flourishes
    g.setColour (tealLo);
    g.setFont (serif (46.f));
    g.drawText ("Blues", 0, 334, (int) W, 50, juce::Justification::centred);
    g.setColour (teal);
    g.setFont (serif (40.f));
    g.drawText ("Driver", 0, 372, (int) W, 44, juce::Justification::centred);
    g.setColour (bronze);
    {
        juce::Path fl;
        fl.startNewSubPath (34.f, 388.f);
        fl.cubicTo (60.f, 368.f, 70.f, 408.f, 96.f, 390.f);
        fl.startNewSubPath (W - 34.f, 388.f);
        fl.cubicTo (W - 60.f, 368.f, W - 70.f, 408.f, W - 96.f, 390.f);
        g.strokePath (fl, juce::PathStrokeType (1.6f));
    }
    g.setColour (bronzeLo);
    g.setFont (serif (12.f, false));
    g.drawText (juce::String::fromUTF8 ("BD-2 style  \xC2\xB7  SH0TY"), 0, 418, (int) W, 14, juce::Justification::centred);

    // lower pad: teal band with a sinuous top edge and leaf motif
    {
        juce::Path pad;
        pad.startNewSubPath (26.f, 452.f);
        pad.cubicTo (80.f, 436.f, 120.f, 468.f, W * 0.5f, 452.f);
        pad.cubicTo (W - 120.f, 436.f, W - 80.f, 468.f, W - 26.f, 452.f);
        pad.lineTo (W - 26.f, H - 26.f);
        pad.lineTo (26.f, H - 26.f);
        pad.closeSubPath();
        // (knobs sit on the band, so the band starts above them)
        juce::Path band;
        band.addPath (pad);
        g.setGradientFill (juce::ColourGradient (teal, 0.f, 440.f, tealLo, 0.f, H, false));
        g.fillPath (band);
        g.setColour (bronze);
        g.strokePath (band, juce::PathStrokeType (1.6f));
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (band);
        for (int i = 0; i < 9; ++i)
        {
            const float x = 40.f + (float) i * 27.f;
            g.setColour (sage.withAlpha (0.35f));
            g.fillPath (leaf ({ x, H - 24.f }, (i % 2 ? 0.35f : -0.35f), 34.f, 7.f));
        }
    }
}
