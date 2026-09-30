#include "FuzzEditor.h"

namespace deco
{
const juce::Colour ink    { 0xff0c1416 };   // near-black teal
const juce::Colour panel  { 0xff13242a };
const juce::Colour gold   { 0xffd4a94a };
const juce::Colour goldHi { 0xfff2d98a };
const juce::Colour goldLo { 0xff8a6a24 };
const juce::Colour cream  { 0xffefe4c4 };

// Stepped (ziggurat) corner rectangle.
juce::Path stepped (juce::Rectangle<float> r, float s)
{
    juce::Path p;
    p.startNewSubPath (r.getX() + 2 * s, r.getY());
    p.lineTo (r.getRight() - 2 * s, r.getY());
    p.lineTo (r.getRight() - 2 * s, r.getY() + s);
    p.lineTo (r.getRight() - s,     r.getY() + s);
    p.lineTo (r.getRight() - s,     r.getY() + 2 * s);
    p.lineTo (r.getRight(),         r.getY() + 2 * s);
    p.lineTo (r.getRight(),         r.getBottom() - 2 * s);
    p.lineTo (r.getRight() - s,     r.getBottom() - 2 * s);
    p.lineTo (r.getRight() - s,     r.getBottom() - s);
    p.lineTo (r.getRight() - 2 * s, r.getBottom() - s);
    p.lineTo (r.getRight() - 2 * s, r.getBottom());
    p.lineTo (r.getX() + 2 * s,     r.getBottom());
    p.lineTo (r.getX() + 2 * s,     r.getBottom() - s);
    p.lineTo (r.getX() + s,         r.getBottom() - s);
    p.lineTo (r.getX() + s,         r.getBottom() - 2 * s);
    p.lineTo (r.getX(),             r.getBottom() - 2 * s);
    p.lineTo (r.getX(),             r.getY() + 2 * s);
    p.lineTo (r.getX() + s,         r.getY() + 2 * s);
    p.lineTo (r.getX() + s,         r.getY() + s);
    p.lineTo (r.getX() + 2 * s,     r.getY() + s);
    p.closeSubPath();
    return p;
}

void spaced (juce::Graphics& g, const juce::String& t, juce::Rectangle<int> r, float size, float kern, bool bold = true)
{
    juce::Font f (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    f.setExtraKerningFactor (kern);
    g.setFont (f);
    g.drawText (t, r, juce::Justification::centred);
}

void diamond (juce::Graphics& g, juce::Point<float> c, float r)
{
    juce::Path d;
    d.addQuadrilateral (c.x, c.y - r, c.x + r * 0.7f, c.y, c.x, c.y + r, c.x - r * 0.7f, c.y);
    g.fillPath (d);
}
} // namespace deco

void DecoLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider& s)
{
    using namespace deco;
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = startAngle + pos * (endAngle - startAngle);
    const bool small = s.getProperties().getWithDefault ("small", false);

    // gold tick fan
    g.setColour (gold);
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.f * (endAngle - startAngle);
        const bool major = (i % 5) == 0;
        g.drawLine ({ c.getPointOnCircumference (r * 0.94f, a),
                      c.getPointOnCircumference (r * (major ? 1.14f : 1.06f), a) }, major ? 2.f : 1.f);
    }

    const float kr = r * 0.82f;
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (c.x - kr + 1.5f, c.y - kr + 3.f, kr * 2, kr * 2);

    // gold bezel
    juce::ColourGradient bz (goldHi, c.x - kr, c.y - kr, goldLo, c.x + kr, c.y + kr, false);
    g.setGradientFill (bz);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);

    // black body with fluted (deco) rings
    const float br = kr * 0.86f;
    juce::ColourGradient body (juce::Colour (0xff2a3438), c.x - br, c.y - br, juce::Colour (0xff050809), c.x + br, c.y + br, false);
    g.setGradientFill (body);
    g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);
    g.setColour (gold.withAlpha (0.65f));
    g.drawEllipse (c.x - br * 0.72f, c.y - br * 0.72f, br * 1.44f, br * 1.44f, 1.f);
    g.setColour (gold.withAlpha (0.35f));
    const int flutes = small ? 16 : 24;
    for (int i = 0; i < flutes; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / (float) flutes;
        g.drawLine ({ c.getPointOnCircumference (br * 0.74f, a), c.getPointOnCircumference (br * 0.98f, a) }, 1.f);
    }

    // cream pointer with a gold diamond tip
    g.setColour (cream);
    g.drawLine ({ c.getPointOnCircumference (br * 0.12f, ang), c.getPointOnCircumference (br * 0.7f, ang) }, small ? 2.f : 3.f);
    g.setColour (goldHi);
    diamond (g, c.getPointOnCircumference (br * 0.83f, ang), small ? 3.f : 4.5f);
    g.setColour (gold);
    g.fillEllipse (c.x - 3.f, c.y - 3.f, 6.f, 6.f);
}

Sh0tyFZ3Editor::Sh0tyFZ3Editor (Sh0tyFZ3Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    addKnob (level, "volume", "LEVEL", false);
    addKnob (fuzz,  "fuzz",   "FUZZ",  false);
    addKnob (tone,  "tone",   "TONE",  false);
    addKnob (mix,   "mix",    "MIX",   true);
    addKnob (trim,  "trim",   "TRIM",  true);
    setSize (300, 520);
}

Sh0tyFZ3Editor::~Sh0tyFZ3Editor() { setLookAndFeel (nullptr); }

void Sh0tyFZ3Editor::addKnob (Knob& k, const juce::String& id, const juce::String& text, bool small)
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
    juce::Font f (juce::FontOptions (small ? 10.f : 13.f, juce::Font::bold));
    f.setExtraKerningFactor (0.25f);
    k.label.setFont (f);
    k.label.setColour (juce::Label::textColourId, deco::goldHi);
    addAndMakeVisible (k.label);
}

void Sh0tyFZ3Editor::resized()
{
    const int big = 88, sm = 54;
    auto place = [] (Knob& k, juce::Rectangle<int> r, int labelH)
    {
        k.slider.setBounds (r);
        k.label.setBounds (r.getX() - 10, r.getBottom() - 1, r.getWidth() + 20, labelH);
    };
    place (level, { 30, 62, big, big }, 18);
    place (fuzz,  { 300 - 30 - big, 62, big, big }, 18);
    place (tone,  { 150 - 38, 138, 76, 76 }, 18);
    place (mix,   { 58, 424, sm, sm }, 16);
    place (trim,  { 300 - 58 - sm, 424, sm, sm }, 16);
}

void Sh0tyFZ3Editor::paint (juce::Graphics& g)
{
    using namespace deco;
    const float W = (float) getWidth(), H = (float) getHeight();
    g.fillAll (ink);

    // outer + inner stepped gold frames
    g.setColour (gold);
    g.strokePath (stepped ({ 6.f, 6.f, W - 12.f, H - 12.f }, 8.f), juce::PathStrokeType (2.5f));
    g.setColour (goldLo);
    g.strokePath (stepped ({ 14.f, 14.f, W - 28.f, H - 28.f }, 6.f), juce::PathStrokeType (1.f));

    // control panel: sunburst radiating from the top-centre
    {
        const juce::Rectangle<float> pr (22.f, 22.f, W - 44.f, 214.f);
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (stepped (pr, 6.f).createPathWithRoundedCorners (0.f).getBounds().toNearestInt());
        juce::ColourGradient pg (juce::Colour (0xff1c363d), W * 0.5f, 22.f, panel, W * 0.5f, 236.f, true);
        g.setGradientFill (pg);
        g.fillPath (stepped (pr, 6.f));
        const juce::Point<float> o (W * 0.5f, 22.f);
        for (int i = 0; i < 17; ++i)
        {
            const float a0 = juce::MathConstants<float>::pi * (0.52f + 0.96f * (float) i / 17.f);
            const float a1 = a0 + juce::MathConstants<float>::pi * 0.96f / 34.f;
            juce::Path ray;
            ray.startNewSubPath (o);
            ray.lineTo (o.getPointOnCircumference (420.f, a0));
            ray.lineTo (o.getPointOnCircumference (420.f, a1));
            ray.closeSubPath();
            g.setColour (gold.withAlpha (0.10f));
            g.fillPath (ray);
        }
        g.setColour (goldLo);
        g.strokePath (stepped (pr, 6.f), juce::PathStrokeType (1.2f));
    }

    // CHECK jewel in a gold arch
    {
        const juce::Point<float> c (W * 0.5f, 42.f);
        g.setColour (goldLo);
        g.fillEllipse (c.x - 12.f, c.y - 12.f, 24.f, 24.f);
        g.setColour (gold);
        g.drawEllipse (c.x - 12.f, c.y - 12.f, 24.f, 24.f, 2.f);
        juce::ColourGradient jw (juce::Colour (0xffff6a5a), c.x - 3.f, c.y - 3.f, juce::Colour (0xff8e0d12), c.x + 7.f, c.y + 7.f, true);
        g.setGradientFill (jw);
        g.fillEllipse (c.x - 8.f, c.y - 8.f, 16.f, 16.f);
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.fillEllipse (c.x - 4.f, c.y - 5.f, 5.f, 3.5f);
        g.setColour (goldHi);
        spaced (g, "CHECK", { (int) c.x - 40, 56, 80, 12 }, 9.f, 0.35f);
    }

    // jack labels
    g.setColour (goldHi);
    spaced (g, juce::String::fromUTF8 ("\xE2\x97\x80  OUTPUT"), { 24, 246, 110, 16 }, 11.f, 0.2f);
    spaced (g, juce::String::fromUTF8 ("INPUT  \xE2\x97\x80"), { (int) W - 134, 246, 110, 16 }, 11.f, 0.2f);

    // title lockup
    g.setColour (gold);
    g.fillRect (30.f, 286.f, W - 60.f, 1.5f);
    g.setColour (cream);
    spaced (g, "FUZZ", { 0, 288, (int) W, 70 }, 62.f, 0.28f);
    g.setColour (gold);
    g.fillRect (30.f, 362.f, W - 60.f, 1.5f);
    diamond (g, { W * 0.5f, 362.7f }, 6.f);
    g.setColour (goldHi);
    spaced (g, juce::String::fromUTF8 ("FZ-3  \xC2\xB7  SH0TY"), { 0, 368, (int) W, 18 }, 13.f, 0.4f);

    // lower pad with a fan motif
    {
        const juce::Rectangle<float> pad (22.f, 396.f, W - 44.f, H - 396.f - 22.f);
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (pad.toNearestInt());
        g.setColour (panel);
        g.fillRect (pad);
        const juce::Point<float> o (W * 0.5f, pad.getBottom() + 4.f);
        for (int i = 9; i >= 1; --i)
        {
            const float rr = 22.f * (float) i;
            g.setColour ((i % 2) ? gold.withAlpha (0.06f) : juce::Colours::black.withAlpha (0.25f));
            g.fillEllipse (o.x - rr, o.y - rr, rr * 2, rr * 2);
            g.setColour (goldLo.withAlpha (0.35f));
            g.drawEllipse (o.x - rr, o.y - rr, rr * 2, rr * 2, 0.8f);
        }
        for (int i = 0; i <= 14; ++i)
        {
            const float a = juce::MathConstants<float>::pi * (1.f + (float) i / 14.f);
            g.setColour (goldLo.withAlpha (0.3f));
            g.drawLine ({ o.getPointOnCircumference (22.f, a), o.getPointOnCircumference (200.f, a) }, 0.8f);
        }
        g.setColour (goldLo);
        g.drawRect (pad, 1.f);
    }
}
