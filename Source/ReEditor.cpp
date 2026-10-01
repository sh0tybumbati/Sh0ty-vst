#include "ReEditor.h"

namespace vault
{
const juce::Colour steel     { 0xff2c3328 };
const juce::Colour steelDark { 0xff1b2019 };
const juce::Colour steelLite { 0xff56624d };
const juce::Colour cream     { 0xffe6dab0 };
const juce::Colour phosphor  { 0xff39ff84 };
const juce::Colour phosDim   { 0xff0f5a2c };
const juce::Colour amber     { 0xffffa21f };
const juce::Colour vaultBlue { 0xff2763c4 };
const juce::Colour hazard    { 0xffe8c21a };
const juce::Colour rust      { 0xff7a3b1c };
const juce::Colour red       { 0xffe03a24 };

juce::Font font (float size, bool bold = true, float kern = 0.08f)
{
    juce::Font f (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    f.setExtraKerningFactor (kern);
    return f;
}
void text (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, float size, juce::Colour c,
           juce::Justification j = juce::Justification::centred, float kern = 0.08f)
{
    g.setColour (c); g.setFont (font (size, true, kern)); g.drawText (t, r, j, false);
}

// mode dial angles: 12 detents over a 300 degree sweep
constexpr float kStart = juce::degreesToRadians (-150.f), kEnd = juce::degreesToRadians (150.f);

void rivet (juce::Graphics& g, float x, float y, float r = 3.2f)
{
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillEllipse (x - r + 0.8f, y - r + 1.2f, 2 * r, 2 * r);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffa4ad98), x - r * 0.5f, y - r * 0.6f, juce::Colour (0xff3b4236), x + r, y + r, true));
    g.fillEllipse (x - r, y - r, 2 * r, 2 * r);
}

juce::Path star (juce::Point<float> c, float rOut, float rIn, int points, float rot = 0.f)
{
    juce::Path p;
    for (int i = 0; i < points * 2; ++i)
    {
        const float a = rot + (float) i * juce::MathConstants<float>::pi / (float) points - juce::MathConstants<float>::halfPi;
        const auto pt = c.getPointOnCircumference (i % 2 == 0 ? rOut : rIn, a);
        if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    p.closeSubPath();
    return p;
}

void trefoil (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour col)
{
    g.setColour (col);
    for (int i = 0; i < 3; ++i)
    {
        const float a0 = juce::degreesToRadians (-30.f + 120.f * (float) i - 90.f + 0.f);
        juce::Path blade;
        blade.addPieSegment (c.x - r, c.y - r, 2 * r, 2 * r, a0 + juce::MathConstants<float>::halfPi, a0 + juce::MathConstants<float>::halfPi + juce::degreesToRadians (60.f), 0.28f);
        g.fillPath (blade);
    }
    g.fillEllipse (c.x - r * 0.16f, c.y - r * 0.16f, r * 0.32f, r * 0.32f);
}

void hazardStripe (juce::Graphics& g, juce::Rectangle<float> r)
{
    juce::Graphics::ScopedSaveState s (g);
    g.reduceClipRegion (r.toNearestInt());
    g.setColour (juce::Colour (0xff15170f)); g.fillRect (r);
    g.setColour (hazard);
    for (float x = r.getX() - r.getHeight(); x < r.getRight(); x += r.getHeight() * 1.6f)
    {
        juce::Path p; p.addQuadrilateral (x, r.getBottom(), x + r.getHeight() * 0.8f, r.getBottom(), x + r.getHeight() * 1.8f, r.getY(), x + r.getHeight(), r.getY());
        g.fillPath (p);
    }
}
} // namespace vault

// ---------------------------------------------------------------------------------------------------------------------
void VaultLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider& s)
{
    using namespace vault;
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto c = b.getCentre();
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const float ang = startAngle + pos * (endAngle - startAngle);
    const bool big = s.getProperties().getWithDefault ("kind", "knob").toString() == "mode";
    const juce::Colour accent ((juce::uint32) (int) s.getProperties().getWithDefault ("accent", (int) amber.getARGB()));

    if (! big)   // scale ticks
    {
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (float) i / 10.f * (endAngle - startAngle);
            const bool major = i % 5 == 0;
            g.setColour (cream.withAlpha (major ? 0.95f : 0.65f));
            g.drawLine (juce::Line<float> (c.getPointOnCircumference (r * (major ? 0.80f : 0.84f), a), c.getPointOnCircumference (r * 0.96f, a)), major ? 2.2f : 1.4f);
        }
    }

    const float br = r * (big ? 0.92f : 0.70f);

    // shadow on the plate
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (c.x - br + 2.f, c.y - br + 4.f, br * 2, br * 2);

    // chrome skirt
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe9ece4), c.x - br, c.y - br, juce::Colour (0xff3d423a), c.x + br, c.y + br, false));
    g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);
    // knurling around the skirt
    const int flutes = big ? 36 : 28;
    for (int i = 0; i < flutes; ++i)
    {
        const float a = (float) i / (float) flutes * juce::MathConstants<float>::twoPi;
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawLine (juce::Line<float> (c.getPointOnCircumference (br * 0.90f, a), c.getPointOnCircumference (br * 0.995f, a)), 1.1f);
    }
    // bakelite cap
    const float cr = br * 0.80f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff46463e), c.x - cr * 0.5f, c.y - cr * 0.7f, juce::Colour (0xff10100d), c.x + cr, c.y + cr, true));
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
    // brushed aluminium centre
    const float ar = cr * 0.58f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff2f3ee), c.x - ar * 0.6f, c.y - ar * 0.7f, juce::Colour (0xff6c7168), c.x + ar, c.y + ar, true));
    g.fillEllipse (c.x - ar, c.y - ar, ar * 2, ar * 2);
    g.setColour (juce::Colours::black.withAlpha (0.12f));
    for (int i = -3; i <= 3; ++i) g.drawEllipse (c.x - ar * (0.2f + 0.12f * (float) std::abs (i)), c.y - ar * (0.2f + 0.12f * (float) std::abs (i)), ar * 0.4f + ar * 0.24f * (float) std::abs (i), ar * 0.4f + ar * 0.24f * (float) std::abs (i), 0.6f);

    // pointer: an amber / phosphor wedge on the cap and a lit dot on the skirt
    juce::Path wedge;
    wedge.addTriangle (-2.4f, -cr * 0.52f, 2.4f, -cr * 0.52f, 0.f, -cr * 0.98f);
    g.setColour (accent);
    g.fillPath (wedge, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
    const auto tip = c.getPointOnCircumference (br * 0.93f, ang);
    g.setColour (accent.withAlpha (0.3f)); g.fillEllipse (tip.x - 4.2f, tip.y - 4.2f, 8.4f, 8.4f);
    g.setColour (accent.brighter (0.4f)); g.fillEllipse (tip.x - 1.8f, tip.y - 1.8f, 3.6f, 3.6f);
}

void VaultLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos,
                                         float, float, juce::Slider::SliderStyle, juce::Slider&)
{
    using namespace vault;
    const auto slot = juce::Rectangle<float> ((float) x + 8.f, (float) y + (float) h * 0.5f - 4.f, (float) w - 16.f, 8.f);
    g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillRoundedRectangle (slot, 4.f);
    g.setColour (steelLite.withAlpha (0.6f)); g.drawRoundedRectangle (slot, 4.f, 1.f);
    const float lw = 20.f, lh = (float) h - 4.f;
    const juce::Rectangle<float> lever (sliderPos - lw * 0.5f, (float) y + 2.f, lw, lh);
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (lever.translated (1.5f, 2.5f), 4.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff0f2ea), lever.getX(), lever.getY(), juce::Colour (0xff5a6055), lever.getRight(), lever.getBottom(), false));
    g.fillRoundedRectangle (lever, 4.f);
    g.setColour (amber); g.fillRect (lever.getCentreX() - 1.5f, lever.getY() + 4.f, 3.f, lever.getHeight() - 8.f);
}

void VaultLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    using namespace vault;
    const auto r = b.getLocalBounds().toFloat().reduced (2.f);
    const juce::String kind = b.getProperties().getWithDefault ("kind", "cancel").toString();
    const bool on = b.getToggleState();
    if (kind == "cancel")           // a fat armed button: red glow when the echo is cancelled
    {
        const auto c = r.getCentre(); const float rad = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillEllipse (c.x - rad + 1.5f, c.y - rad + 3.f, rad * 2, rad * 2);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe9ece4), c.x - rad, c.y - rad, juce::Colour (0xff3d423a), c.x + rad, c.y + rad, false));
        g.fillEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2);
        const float cr = rad * 0.78f;
        g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xffff7a5c) : juce::Colour (0xff6a2a1c), c.x - cr * 0.4f, c.y - cr * 0.6f,
                                                 on ? juce::Colour (0xff9a1608) : juce::Colour (0xff2a0f0a), c.x + cr, c.y + cr, true));
        g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
        if (on) { g.setColour (red.withAlpha (0.25f)); g.fillEllipse (c.x - rad * 1.35f, c.y - rad * 1.35f, rad * 2.7f, rad * 2.7f); }
        if (over) { g.setColour (juce::Colours::white.withAlpha (0.08f)); g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2); }
        return;
    }
    // power: a rocker switch with a lit "ON" half
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (r.translated (1.5f, 3.f), 6.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a6055), r.getX(), r.getY(), juce::Colour (0xff1f241c), r.getRight(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 6.f);
    const auto half = r.reduced (3.f).withWidth ((r.getWidth() - 6.f) * 0.5f);
    const auto onHalf = half.translated (half.getWidth(), 0.f);
    g.setColour (on ? juce::Colour (0xff1c7a3e) : juce::Colour (0xff121511)); g.fillRoundedRectangle (onHalf, 4.f);
    g.setColour (on ? juce::Colour (0xff0b0e0a) : juce::Colour (0xff6a7060)); g.fillRoundedRectangle (half, 4.f);
    text (g, "OFF", half, 10.f, on ? steelLite : juce::Colour (0xffe8e8dc));
    text (g, "ON", onHalf, 10.f, on ? juce::Colour (0xffd9ffe6) : steelLite);
    if (over) { g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRoundedRectangle (r, 6.f); }
}

// ---------------------------------------------------------------------------------------------------------------------
namespace
{
struct Spot { const char* id; const char* label; int cx, cy, d; };
constexpr Spot kSpots[] = {
    { "mic1",      "MIC VOLUME",         62, 214, 64 },
    { "mic2",      "MIC VOLUME",        162, 214, 64 },
    { "inst",      "INSTRUMENT VOL.",   262, 214, 64 },
    { "mode",      "",                  465, 236, 96 },
    { "bass",      "BASS",              690, 108, 64 },
    { "treble",    "TREBLE",            805, 108, 64 },
    { "reverb",    "REVERB VOLUME",     920, 108, 64 },
    { "rate",      "REPEAT RATE",       690, 194, 64 },
    { "intensity", "INTENSITY",         805, 194, 64 },
    { "echo",      "ECHO VOLUME",       920, 194, 64 },
};
const juce::Rectangle<int> kVu   { 74, 78, 226, 70 };
const juce::Rectangle<int> kCrt  { 338, 78, 254, 70 };
const juce::Point<int>     kPeak { 38, 108 };
const juce::Point<int>     kModeC { 465, 236 };
const juce::Rectangle<int> kCancel { 662, 252, 56, 44 }, kOut { 750, 262, 110, 28 }, kPower { 892, 252, 64, 44 };
const juce::Point<int>     kPowerLamp { 872, 274 };
} // namespace

Sh0tyRE201Editor::Sh0tyRE201Editor (Sh0tyRE201Processor& p) : juce::AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    Knob* knobs[] = { &mic1, &mic2, &inst, &mode, &bass, &treble, &reverb, &rate, &intensity, &echo };
    for (size_t i = 0; i < std::size (knobs); ++i) addKnob (*knobs[i], kSpots[i].id, vault::amber);
    mode.slider.getProperties().set ("kind", "mode");
    mode.slider.getProperties().set ("accent", (int) vault::phosphor.getARGB());
    mode.slider.setRotaryParameters (vault::kStart, vault::kEnd, true);

    outLevel.setSliderStyle (juce::Slider::LinearHorizontal);
    outLevel.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (outLevel);
    outAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "outlevel", outLevel);

    cancel.getProperties().set ("kind", "cancel"); power.getProperties().set ("kind", "power");
    cancel.setClickingTogglesState (true); power.setClickingTogglesState (true);
    addAndMakeVisible (cancel); addAndMakeVisible (power);
    cancelAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "cancel", cancel);
    powerAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "on", power);
    cancel.setTooltip ("Echo cancel (the foot switch): silences the echo and its repeats; the spring keeps going");
    power.setTooltip ("Power: off removes the echo and reverb and leaves the dry signal");

    setSize (1000, 360);
    renderBackground();
    startTimerHz (30);
}

Sh0tyRE201Editor::~Sh0tyRE201Editor() { stopTimer(); setLookAndFeel (nullptr); }

void Sh0tyRE201Editor::addKnob (Knob& k, const juce::String& id, juce::Colour accent)
{
    k.slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setRotaryParameters (juce::degreesToRadians (-140.f), juce::degreesToRadians (140.f), true);
    k.slider.getProperties().set ("accent", (int) accent.getARGB());
    addAndMakeVisible (k.slider);
    k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);
}

void Sh0tyRE201Editor::resized()
{
    Knob* knobs[] = { &mic1, &mic2, &inst, &mode, &bass, &treble, &reverb, &rate, &intensity, &echo };
    for (size_t i = 0; i < std::size (knobs); ++i)
        knobs[i]->slider.setBounds (kSpots[i].cx - kSpots[i].d / 2, kSpots[i].cy - kSpots[i].d / 2, kSpots[i].d, kSpots[i].d);
    outLevel.setBounds (kOut); cancel.setBounds (kCancel); power.setBounds (kPower);
}

void Sh0tyRE201Editor::timerCallback()
{
    ++frame;
    const float db = 20.f * std::log10 (proc.vuLevel.load() + 1e-5f);
    const float target = juce::jlimit (0.f, 1.f, (db + 30.f) / 33.f);
    needle += (target - needle) * 0.3f;
    lamp += (proc.peakLamp.load() - lamp) * 0.5f;
    flicker = 0.5f + 0.5f * std::sin ((float) frame * 0.9f) * std::sin ((float) frame * 0.37f);
    if (value ("on") > 0.5f) reelAngle += 0.05f + 0.18f * value ("rate");
    repaint();
}

// ---------------------------------------------------------------------------------------------------------------------
void Sh0tyRE201Editor::renderBackground()
{
    using namespace vault;
    const float S = 2.f;
    const int W = getWidth(), H = getHeight();
    background = juce::Image (juce::Image::ARGB, (int) (W * S), (int) (H * S), true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (S));
    const auto all = juce::Rectangle<float> (0, 0, (float) W, (float) H);

    // weathered olive plate
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3b4535), 0, 0, steelDark, 0, (float) H, false));
    g.fillRect (all);
    juce::Random rnd (201);
    for (int i = 0; i < 2600; ++i)    // brushed grain
    {
        const float x = rnd.nextFloat() * (float) W, y = rnd.nextFloat() * (float) H, l = 8.f + rnd.nextFloat() * 70.f;
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.015f + rnd.nextFloat() * 0.03f));
        g.drawHorizontalLine ((int) y, x, x + l);
    }
    for (int i = 0; i < 60; ++i)      // paint chips and rust
    {
        const float x = rnd.nextFloat() * (float) W, y = rnd.nextFloat() * (float) H, r = 1.f + rnd.nextFloat() * 4.f;
        g.setColour (rust.withAlpha (0.12f + rnd.nextFloat() * 0.2f)); g.fillEllipse (x, y, r * 1.6f, r);
    }
    for (int i = 0; i < 14; ++i)      // scratches
    {
        const float x = rnd.nextFloat() * (float) W, y = rnd.nextFloat() * (float) H;
        g.setColour (juce::Colour (0xffb7bfa8).withAlpha (0.10f));
        g.drawLine (x, y, x + 20.f + rnd.nextFloat() * 60.f, y + (rnd.nextFloat() - 0.5f) * 10.f, 0.8f);
    }

    // header band: cream enamel with vault blue stripe
    const auto head = juce::Rectangle<float> (10, 8, (float) W - 20, 46);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeee3bd), 0, head.getY(), juce::Colour (0xffc7ba8c), 0, head.getBottom(), false));
    g.fillRoundedRectangle (head, 8.f);
    g.setColour (juce::Colour (0xff1c2118)); g.drawRoundedRectangle (head, 8.f, 2.f);
    g.setColour (vaultBlue); g.fillRect (head.getX() + 8, head.getBottom() - 9.f, head.getWidth() - 16, 4.f);
    g.setColour (hazard);    g.fillRect (head.getX() + 8, head.getBottom() - 13.f, head.getWidth() - 16, 2.f);
    text (g, "SPACE ECHO", { 30, 10, 330, 34 }, 30.f, juce::Colour (0xff1a1d15), juce::Justification::centredLeft, 0.06f);
    const auto plate = juce::Rectangle<float> (292, 14, 96, 26);
    g.setColour (juce::Colour (0xff1a1d15)); g.fillRoundedRectangle (plate, 5.f);
    text (g, "RE-201", plate, 19.f, phosphor, juce::Justification::centred, 0.1f);
    text (g, "TAPE ECHO & SPRING REVERB UNIT", { 404, 14, 330, 24 }, 12.f, juce::Colour (0xff3b4130), juce::Justification::centredLeft, 0.14f);
    // brand: atomic starburst + SH0TY-TEC
    {
        const juce::Point<float> c (806, 31);
        g.setColour (juce::Colour (0xff1a1d15)); g.fillPath (star (c, 21.f, 8.f, 8, 0.3f));
        g.setColour (hazard); g.fillPath (star (c, 15.f, 6.f, 8, 0.3f));
        g.setColour (juce::Colour (0xff1a1d15)); g.fillEllipse (c.x - 5.f, c.y - 5.f, 10.f, 10.f);
        text (g, "SH0TY-TEC", { 836, 8, 150, 30 }, 20.f, juce::Colour (0xff1a1d15), juce::Justification::centredLeft, 0.08f);
        text (g, juce::String::fromUTF8 ("INDUSTRIES  \xC2\xB7  EST. 2077"), { 838, 31, 150, 12 }, 8.f, juce::Colour (0xff3b4130), juce::Justification::centredLeft, 0.16f);
    }

    // section panels
    auto panel = [&] (juce::Rectangle<float> r, const char* title)
    {
        g.setColour (juce::Colours::black.withAlpha (0.28f)); g.fillRoundedRectangle (r, 9.f);
        g.setColour (steelLite.withAlpha (0.5f)); g.drawRoundedRectangle (r, 9.f, 1.4f);
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.drawRoundedRectangle (r.translated (0, 1.f), 9.f, 1.f);
        if (title[0] != 0)
        {
            const auto t = juce::Rectangle<float> (r.getX() + 12, r.getY() - 7, juce::String (title).length() * 7.4f + 14.f, 14.f);
            g.setColour (steelDark); g.fillRect (t);
            text (g, title, t, 10.f, cream.withAlpha (0.9f), juce::Justification::centred, 0.18f);
        }
    };
    panel ({ 14, 64, 300, 248 }, "INPUT");
    panel ({ 326, 64, 276, 248 }, "MODE SELECTOR");
    panel ({ 614, 64, 372, 248 }, "ECHO  /  REVERB");

    // knob labels
    for (auto& s : kSpots)
        if (s.label[0] != 0)
            text (g, s.label, juce::Rectangle<float> ((float) s.cx - 56, (float) s.cy + (float) s.d * 0.5f + 1.f, 112, 14), 9.f, cream, juce::Justification::centred, 0.14f);
    text (g, "ECHO CANCEL", { (float) kCancel.getX() - 20, (float) kCancel.getBottom() + 0.f, (float) kCancel.getWidth() + 40, 12 }, 8.f, cream, juce::Justification::centred, 0.14f);
    text (g, "OUTPUT LEVEL", { (float) kOut.getX(), 246.f, (float) kOut.getWidth(), 12 }, 8.f, cream, juce::Justification::centred, 0.14f);
    for (int i = 0; i < 3; ++i)
        text (g, i == 0 ? "H" : i == 1 ? "M" : "L", { (float) kOut.getX() + 8.f + (float) i * ((float) kOut.getWidth() - 16.f) * 0.5f - 8.f, (float) kOut.getBottom() + 2.f, 16, 12 }, 9.f, amber, juce::Justification::centred, 0.f);
    text (g, "POWER", { (float) kPower.getX() - 8, (float) kPower.getBottom(), (float) kPower.getWidth() + 16, 12 }, 8.f, cream, juce::Justification::centred, 0.14f);
    text (g, "PEAK", { (float) kPeak.x - 24, (float) kPeak.y + 16.f, 48, 11 }, 8.f, cream, juce::Justification::centred, 0.14f);
    text (g, "LEVEL", { (float) kPeak.x - 24, (float) kPeak.y + 25.f, 48, 11 }, 8.f, cream, juce::Justification::centred, 0.14f);

    // mode dial numbers are drawn live (the current one lights up); the arc for the "scale" is static
    {
        const auto c = juce::Point<float> ((float) kModeC.x, (float) kModeC.y);
        g.setColour (cream.withAlpha (0.25f));
        juce::Path arc; arc.addCentredArc (c.x, c.y, 57.f, 57.f, 0.f, kStart - 0.1f, kEnd + 0.1f, true);
        g.strokePath (arc, juce::PathStrokeType (1.2f));
        text (g, "1-7  ECHO", { 336, 292, 80, 12 }, 7.5f, cream.withAlpha (0.8f), juce::Justification::centredLeft, 0.14f);
        text (g, "8-11  ECHO + REVERB", { 502, 292, 96, 12 }, 7.5f, amber.withAlpha (0.9f), juce::Justification::centredRight, 0.1f);
        text (g, "REV ONLY", { c.x - 26, c.y + 50, 52, 10 }, 7.f, hazard, juce::Justification::centred, 0.1f);
    }

    // tape transport bed (reels and heads are drawn live)
    {
        const auto r = juce::Rectangle<float> (26, 266, 276, 40);
        g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (r, 8.f);
        g.setColour (steelLite.withAlpha (0.4f)); g.drawRoundedRectangle (r, 8.f, 1.f);
    }

    // jack strip along the bottom, like the front of the real unit
    {
        const auto strip = juce::Rectangle<float> (10, 318, (float) W - 20, 30);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb9bdb0), 0, strip.getY(), juce::Colour (0xff7d8277), 0, strip.getBottom(), false));
        g.fillRoundedRectangle (strip, 6.f);
        g.setColour (juce::Colour (0xff1a1d15)); g.drawRoundedRectangle (strip, 6.f, 1.6f);
        struct Jack { const char* name; float x; float w; };
        const Jack jacks[] = { { "MIC 1", 22, 100 }, { "MIC 2", 134, 100 }, { "INSTRUMENT", 246, 150 }, { "FROM P.A.", 408, 120 },
                               { "OUTPUT", 640, 150 }, { "FOOT SW", 840, 140 } };
        for (auto& j : jacks)
        {
            g.setColour (juce::Colour (0xff1a1d15).withAlpha (0.35f)); g.drawRoundedRectangle (j.x, strip.getY() + 3, j.w, strip.getHeight() - 6, 4.f, 1.f);
            text (g, j.name, { j.x + 4, strip.getY() + 3, j.w * 0.55f, strip.getHeight() - 6 }, 9.f, juce::Colour (0xff1a1d15), juce::Justification::centredLeft, 0.12f);
            const auto jc = juce::Point<float> (j.x + j.w - 22.f, strip.getCentreY());
            g.setColour (juce::Colour (0xff25291f)); g.fillEllipse (jc.x - 9.f, jc.y - 9.f, 18.f, 18.f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffdfe3d8), jc.x - 6.f, jc.y - 6.f, juce::Colour (0xff6a6f63), jc.x + 6.f, jc.y + 6.f, false));
            g.fillEllipse (jc.x - 6.f, jc.y - 6.f, 12.f, 12.f);
            g.setColour (juce::Colour (0xff0b0c09)); g.fillEllipse (jc.x - 3.f, jc.y - 3.f, 6.f, 6.f);
        }
    }

    hazardStripe (g, { 10, 350, (float) W - 20, 7 });

    // rivets
    for (float x : { 18.f, (float) W - 18.f }) for (float y : { 62.f, 312.f }) rivet (g, x, y);
    for (float x : { 20.f, (float) W - 20.f }) for (float y : { 14.f, 48.f }) rivet (g, x, y, 2.6f);
    rivet (g, 320.f, 66.f, 2.4f); rivet (g, 320.f, 308.f, 2.4f); rivet (g, 608.f, 66.f, 2.4f); rivet (g, 608.f, 308.f, 2.4f);

    // outer frame
    g.setColour (juce::Colour (0xff0f110c)); g.drawRect (all, 3.f);
    g.setColour (steelLite.withAlpha (0.35f)); g.drawRect (all.reduced (3.f), 1.f);
}

// ---------------------------------------------------------------------------------------------------------------------
void Sh0tyRE201Editor::paintCrt (juce::Graphics& g, juce::Rectangle<float> r)
{
    using namespace vault;
    const bool on = value ("on") > 0.5f;
    const int m = juce::jlimit (1, re201::kModes, (int) std::lround (value ("mode")));
    const auto mi = re201::modeInfo (m);
    const float t1 = re201::headOneDelay (value ("rate"));

    // bezel + glass
    g.setColour (juce::Colour (0xff0c0e0a)); g.fillRoundedRectangle (r.expanded (4.f), 12.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff062010), r.getCentreX(), r.getCentreY(), juce::Colour (0xff010a05), r.getX(), r.getY(), true));
    g.fillRoundedRectangle (r, 9.f);
    juce::Graphics::ScopedSaveState ss (g);
    juce::Path clip; clip.addRoundedRectangle (r, 9.f); g.reduceClipRegion (clip);

    if (on)
    {
        const float glow = 0.82f + 0.18f * flicker;
        auto line = [&] (const juce::String& s, float y, float size, juce::Colour c, juce::Justification j = juce::Justification::centredLeft, float x0 = 14.f)
        {
            const auto box = juce::Rectangle<float> (r.getX() + x0, y, r.getWidth() - x0 - 10.f, size + 4.f);
            text (g, s, box, size, c.withAlpha (0.20f * glow), j, 0.10f);                    // bloom
            text (g, s, box.translated (0.6f, 0.f), size, c.withAlpha (0.2f * glow), j, 0.10f);
            text (g, s, box, size, c.withAlpha (glow), j, 0.10f);
        };
        static const char* names[] = { "HEAD 3", "HEAD 2", "HEAD 1", "HEADS 2+3", "HEADS 1+2", "HEADS 1+3", "HEADS 1+2+3",
                                       "HEAD 3 + SPRING", "HEAD 2 + SPRING", "HEAD 1 + SPRING", "ALL HEADS + SPRING", "SPRING ONLY" };
        line (juce::String::formatted ("MODE %02d", m), r.getY() + 5.f, 17.f, phosphor);
        line (names[m - 1], r.getY() + 5.f, 12.f, phosphor, juce::Justification::centredRight);
        line (mi.echo ? (mi.reverb ? "TAPE ECHO + SPRING REVERB" : "TAPE ECHO")  : "SPRING REVERB", r.getY() + 23.f, 9.f, phosphor.withMultipliedBrightness (0.8f));
        // head timing read-out: each head a cell, bright when it is playing
        for (int h = 0; h < 3; ++h)
        {
            const float x = r.getX() + 12.f + (float) h * ((r.getWidth() - 24.f) / 3.f);
            const auto cell = juce::Rectangle<float> (x, r.getY() + 39.f, (r.getWidth() - 24.f) / 3.f - 6.f, 26.f);
            const bool act = mi.head[h] && mi.echo;
            g.setColour (act ? phosphor.withAlpha (0.16f * glow) : phosDim.withAlpha (0.2f)); g.fillRoundedRectangle (cell, 3.f);
            g.setColour (act ? phosphor.withAlpha (0.9f * glow) : phosDim.withAlpha (0.7f)); g.drawRoundedRectangle (cell, 3.f, 1.f);
            text (g, juce::String::formatted ("H%d", h + 1), cell.withHeight (12.f).translated (0, 2.f), 9.f, act ? phosphor : phosDim.brighter (0.2f), juce::Justification::centred, 0.1f);
            text (g, juce::String::formatted ("%d MS", (int) std::lround (t1 * re201::kHeadRatio[h] * 1000.f)), cell.withTrimmedTop (12.f), 11.f,
                  act ? phosphor.withAlpha (glow) : phosDim.brighter (0.2f), juce::Justification::centred, 0.06f);
        }
    }
    else
    {
        text (g, "STANDBY", r, 16.f, phosDim.brighter (0.5f).withAlpha (0.6f + 0.3f * flicker), juce::Justification::centred, 0.3f);
    }
    if (on && value ("cancel") > 0.5f && (frame / 8) % 2 == 0)
        text (g, "ECHO CANCEL", r.withHeight (18.f).translated (0, r.getHeight() - 19.f), 9.f, amber, juce::Justification::centred, 0.2f);

    // scan lines and vignette
    g.setColour (juce::Colours::black.withAlpha (0.28f));
    for (float y = r.getY(); y < r.getBottom(); y += 3.f) g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(), juce::Colours::black.withAlpha (0.55f), r.getX(), r.getY(), true));
    g.fillRect (r);
    g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.35f).reduced (6.f, 2.f), 7.f);   // glass sheen
}

void Sh0tyRE201Editor::paintVu (juce::Graphics& g, juce::Rectangle<float> r)
{
    using namespace vault;
    g.setColour (juce::Colour (0xff0c0e0a)); g.fillRoundedRectangle (r.expanded (4.f), 8.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd4e6a8), r.getCentreX(), r.getBottom(), juce::Colour (0xff7e9a52), r.getX(), r.getY(), true));
    g.fillRoundedRectangle (r, 6.f);
    juce::Graphics::ScopedSaveState ss (g);
    juce::Path clip; clip.addRoundedRectangle (r, 6.f); g.reduceClipRegion (clip);

    const juce::Point<float> pivot (r.getCentreX(), r.getBottom() + 26.f);
    const float L = r.getHeight() + 14.f;
    auto angleFor = [] (float v) { return (v - 0.5f) * 1.45f; };
    // scale
    g.setColour (juce::Colour (0xff1c2610));
    juce::Path arc; arc.addCentredArc (pivot.x, pivot.y, L * 0.86f, L * 0.86f, 0.f, angleFor (0.f), angleFor (0.8f), true);
    g.strokePath (arc, juce::PathStrokeType (1.6f));
    g.setColour (red);
    juce::Path hot; hot.addCentredArc (pivot.x, pivot.y, L * 0.86f, L * 0.86f, 0.f, angleFor (0.8f), angleFor (1.f), true);
    g.strokePath (hot, juce::PathStrokeType (3.4f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = angleFor ((float) i / 10.f);
        g.setColour (i >= 8 ? red : juce::Colour (0xff1c2610));
        g.drawLine (juce::Line<float> (pivot.getPointOnCircumference (L * 0.86f, a), pivot.getPointOnCircumference (L * (i % 2 == 0 ? 0.76f : 0.80f), a)), i % 2 == 0 ? 1.8f : 1.1f);
    }
    text (g, "VU", { r.getCentreX() - 20, r.getY() + 12, 40, 18 }, 15.f, juce::Colour (0xff1c2610), juce::Justification::centred, 0.3f);
    text (g, "SH0TY-TEC", { r.getCentreX() - 40, r.getBottom() - 15, 80, 11 }, 7.f, juce::Colour (0xff2a3718), juce::Justification::centred, 0.2f);
    text (g, "-20", { r.getX() + 6, r.getY() + 28, 30, 10 }, 7.f, juce::Colour (0xff1c2610), juce::Justification::centredLeft, 0.05f);
    text (g, "+3", { r.getRight() - 36, r.getY() + 28, 30, 10 }, 7.f, red, juce::Justification::centredRight, 0.05f);
    // needle
    const float a = angleFor (needle);
    const auto tip = pivot.getPointOnCircumference (L * 0.9f, a), base = pivot.getPointOnCircumference (L * 0.35f, a);
    g.setColour (juce::Colours::black.withAlpha (0.25f)); g.drawLine (base.x + 2, base.y + 2, tip.x + 2, tip.y + 2, 1.8f);
    g.setColour (juce::Colour (0xffb5360f)); g.drawLine (base.x, base.y, tip.x, tip.y, 1.6f);
    // glass sheen
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.4f).reduced (4.f, 2.f), 5.f);
}

void Sh0tyRE201Editor::paintTransport (juce::Graphics& g)
{
    using namespace vault;
    const juce::Point<float> L (62.f, 286.f), R (196.f, 286.f);
    const float rr = 15.f;
    const int m = juce::jlimit (1, re201::kModes, (int) std::lround (value ("mode")));
    const auto mi = re201::modeInfo (m);
    // tape path: reel to reel across three heads
    g.setColour (juce::Colour (0xff6b4a2a));
    g.drawLine (L.x + rr, L.y - rr + 2.f, 74.f + rr, 273.f, 1.4f);
    juce::Path tape; tape.startNewSubPath (L.x + rr, L.y + rr - 3.f); tape.lineTo (R.x - rr, R.y + rr - 3.f);
    g.strokePath (tape, juce::PathStrokeType (1.4f));
    for (int i = 0; i < 2; ++i)
    {
        const auto c = i == 0 ? L : R;
        g.setColour (juce::Colour (0xff121410)); g.fillEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2);
        g.setColour (juce::Colour (0xff5a3d22)); g.fillEllipse (c.x - rr * 0.82f, c.y - rr * 0.82f, rr * 1.64f, rr * 1.64f);
        g.setColour (juce::Colour (0xff121410)); g.fillEllipse (c.x - rr * 0.46f, c.y - rr * 0.46f, rr * 0.92f, rr * 0.92f);
        g.setColour (juce::Colour (0xffcfd4c4));
        for (int k = 0; k < 3; ++k)
        {
            const float a = reelAngle * (i == 0 ? 1.f : 1.f) + (float) k * juce::MathConstants<float>::twoPi / 3.f;
            g.drawLine (juce::Line<float> (c, c.getPointOnCircumference (rr * 0.44f, a)), 2.f);
        }
        g.setColour (juce::Colour (0xffcfd4c4)); g.fillEllipse (c.x - 2.f, c.y - 2.f, 4.f, 4.f);
    }
    // heads: lit when the mode uses them
    for (int h = 0; h < 3; ++h)
    {
        const float x = 242.f + (float) h * 20.f;
        const bool act = mi.head[h] && mi.echo && value ("on") > 0.5f;
        g.setColour (act ? phosphor.withAlpha (0.28f) : juce::Colours::transparentBlack); g.fillEllipse (x - 7.f, 271.f, 14.f, 14.f);
        g.setColour (act ? phosphor : phosDim); g.fillEllipse (x - 3.5f, 274.5f, 7.f, 7.f);
        text (g, juce::String::formatted ("H%d", h + 1), { x - 10.f, 288.f, 20.f, 10.f }, 7.f, cream.withAlpha (0.8f), juce::Justification::centred, 0.05f);
    }
    text (g, "TAPE", { 118.f, 290.f, 32.f, 10.f }, 6.f, cream.withAlpha (0.45f), juce::Justification::centred, 0.2f);
}

void Sh0tyRE201Editor::paint (juce::Graphics& g)
{
    using namespace vault;
    g.drawImage (background, getLocalBounds().toFloat());
    paintVu (g, kVu.toFloat());
    paintCrt (g, kCrt.toFloat());
    paintTransport (g);

    // PEAK lamp: a lens with a radiation trefoil
    {
        const auto c = kPeak.toFloat();
        g.setColour (juce::Colour (0xff0c0e0a)); g.fillEllipse (c.x - 16.f, c.y - 16.f, 32.f, 32.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour::fromFloatRGBA (0.35f + 0.65f * lamp, 0.10f + 0.05f * lamp, 0.06f, 1.f), c.x - 5.f, c.y - 6.f,
                                                 juce::Colour::fromFloatRGBA (0.12f + 0.4f * lamp, 0.03f, 0.02f, 1.f), c.x + 12.f, c.y + 12.f, true));
        g.fillEllipse (c.x - 13.f, c.y - 13.f, 26.f, 26.f);
        trefoil (g, c, 9.5f, juce::Colour (0xff130503).interpolatedWith (juce::Colour (0xffffd3a0), lamp));
        if (lamp > 0.05f) { g.setColour (red.withAlpha (0.25f * lamp)); g.fillEllipse (c.x - 22.f, c.y - 22.f, 44.f, 44.f); }
        g.setColour (juce::Colours::white.withAlpha (0.16f)); g.fillEllipse (c.x - 8.f, c.y - 11.f, 10.f, 6.f);
    }

    // mode dial numbers: the selected one glows
    {
        const int m = juce::jlimit (1, re201::kModes, (int) std::lround (value ("mode")));
        const auto c = juce::Point<float> ((float) kModeC.x, (float) kModeC.y);
        for (int i = 1; i <= re201::kModes; ++i)
        {
            const float a = kStart + (float) (i - 1) / 11.f * (kEnd - kStart);
            const auto p = c.getPointOnCircumference (70.f, a);
            const bool sel = i == m;
            const juce::Colour col = sel ? phosphor : (i >= 8 ? amber.withAlpha (0.9f) : cream.withAlpha (0.9f));
            if (sel) { g.setColour (phosphor.withAlpha (0.18f)); g.fillEllipse (p.x - 11.f, p.y - 11.f, 22.f, 22.f); }
            text (g, juce::String (i), { p.x - 11.f, p.y - 7.f, 22.f, 14.f }, sel ? 13.f : 11.f, col, juce::Justification::centred, 0.f);
            const auto t0 = c.getPointOnCircumference (55.f, a), t1 = c.getPointOnCircumference (59.f, a);
            g.setColour (cream.withAlpha (0.5f)); g.drawLine (t0.x, t0.y, t1.x, t1.y, 1.2f);
        }
    }

    // power lamp
    {
        const bool on = value ("on") > 0.5f;
        const auto c = kPowerLamp.toFloat();
        g.setColour (juce::Colour (0xff0c0e0a)); g.fillEllipse (c.x - 8.f, c.y - 8.f, 16.f, 16.f);
        g.setColour (on ? phosphor : juce::Colour (0xff12331f)); g.fillEllipse (c.x - 5.5f, c.y - 5.5f, 11.f, 11.f);
        if (on) { g.setColour (phosphor.withAlpha (0.25f)); g.fillEllipse (c.x - 13.f, c.y - 13.f, 26.f, 26.f); }
        g.setColour (juce::Colours::white.withAlpha (on ? 0.5f : 0.15f)); g.fillEllipse (c.x - 3.f, c.y - 4.f, 4.f, 3.f);
    }
}
