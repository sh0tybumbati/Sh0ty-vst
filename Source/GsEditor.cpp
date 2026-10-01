#include "GsEditor.h"

namespace street
{
const juce::Colour pink   { 0xffff2e93 };
const juce::Colour cyan   { 0xff19e6ff };
const juce::Colour yellow { 0xffffe14a };
const juce::Colour orange { 0xffff7a1a };
const juce::Colour green  { 0xff4dff88 };
const juce::Colour ink    { 0xff111015 };
const juce::Colour white  { 0xfff5f2e8 };
const juce::Colour tape   { 0xffe9dcb4 };

juce::Font marker (float size, bool italic = false, float kern = 0.04f)
{
    juce::Font f (juce::FontOptions (size, italic ? (juce::Font::bold | juce::Font::italic) : juce::Font::bold));
    f.setExtraKerningFactor (kern);
    return f;
}

// soft neon glow: a few wide, faint strokes under a crisp one
void glowStroke (juce::Graphics& g, const juce::Path& p, juce::Colour c, float w)
{
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (c.withAlpha (0.10f * (float) (4 - i)));
        g.strokePath (p, juce::PathStrokeType (w + (float) i * 4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// spray cloud: lots of tiny dots, denser at the centre
void spray (juce::Graphics& g, juce::Random& rng, juce::Point<float> c, float radius, juce::Colour col, int n)
{
    for (int i = 0; i < n; ++i)
    {
        const float a = rng.nextFloat() * juce::MathConstants<float>::twoPi;
        const float r = radius * std::sqrt (rng.nextFloat()) * rng.nextFloat();
        const auto p = c.getPointOnCircumference (r, a);
        g.setColour (col.withAlpha (0.10f + 0.5f * rng.nextFloat() * (1.f - r / radius)));
        const float d = 0.6f + rng.nextFloat() * 1.8f;
        g.fillEllipse (p.x, p.y, d, d);
    }
}

void bolt (juce::Graphics& g, juce::Point<float> c, float s, juce::Colour col)
{
    juce::Path p;
    p.startNewSubPath (c.x + 0.2f * s, c.y - s);
    p.lineTo (c.x - 0.5f * s, c.y + 0.1f * s);
    p.lineTo (c.x - 0.05f * s, c.y + 0.1f * s);
    p.lineTo (c.x - 0.25f * s, c.y + s);
    p.lineTo (c.x + 0.5f * s, c.y - 0.2f * s);
    p.lineTo (c.x + 0.05f * s, c.y - 0.2f * s);
    p.closeSubPath();
    g.setColour (ink); g.strokePath (p, juce::PathStrokeType (3.f, juce::PathStrokeType::mitered));
    g.setColour (col); g.fillPath (p);
}

void star (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour col)
{
    juce::Path p; p.addStar (c, 5, r * 0.45f, r, -juce::MathConstants<float>::halfPi);
    g.setColour (ink); g.strokePath (p, juce::PathStrokeType (3.f, juce::PathStrokeType::mitered));
    g.setColour (col); g.fillPath (p);
}
} // namespace street

void StreetLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float startAngle, float endAngle, juce::Slider& s)
{
    using namespace street;
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = startAngle + pos * (endAngle - startAngle);
    const bool small = s.getProperties().getWithDefault ("small", false);
    const juce::Colour accent ((juce::uint32) (int) s.getProperties().getWithDefault ("accent", (int) pink.getARGB()));

    // paint-stroke value arc with neon glow
    const float ar = r * 0.9f;
    juce::Path track, value;
    track.addCentredArc (c.x, c.y, ar, ar, 0.f, startAngle, endAngle, true);
    value.addCentredArc (c.x, c.y, ar, ar, 0.f, startAngle, ang, true);
    g.setColour (ink.withAlpha (0.55f));
    g.strokePath (track, juce::PathStrokeType (small ? 5.f : 7.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (pos > 0.002f) glowStroke (g, value, accent, small ? 3.5f : 5.f);

    // black spray-can-cap knob: ribbed rim, accent disc, white pointer
    const float kr = r * 0.7f;
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillEllipse (c.x - kr + 1.5f, c.y - kr + 3.5f, kr * 2, kr * 2);
    g.setColour (ink); g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    const int ribs = small ? 20 : 28;
    g.setColour (juce::Colour (0xff3a3842));
    for (int i = 0; i < ribs; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / (float) ribs;
        g.drawLine ({ c.getPointOnCircumference (kr * 0.84f, a), c.getPointOnCircumference (kr * 0.98f, a) }, 1.6f);
    }
    const float dr = kr * 0.68f;
    g.setGradientFill (juce::ColourGradient (accent.brighter (0.25f), c.x - dr, c.y - dr, accent.darker (0.35f), c.x + dr, c.y + dr, false));
    g.fillEllipse (c.x - dr, c.y - dr, dr * 2, dr * 2);
    g.setColour (ink); g.drawEllipse (c.x - dr, c.y - dr, dr * 2, dr * 2, 2.f);
    juce::Path bar; bar.addRoundedRectangle (-2.4f, -dr * 0.92f, 4.8f, dr * 0.7f, 2.f);
    g.setColour (white);
    g.fillPath (bar, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
    g.setColour (ink); g.strokePath (bar, juce::PathStrokeType (1.2f), juce::AffineTransform::rotation (ang).translated (c.x, c.y));
}

void StreetLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool)
{
    using namespace street;
    const auto r = b.getLocalBounds().toFloat().reduced (2.f);
    const float d = juce::jmin (r.getWidth(), r.getHeight());
    const auto rc = r.withSizeKeepingCentre (d, d);
    const bool on = b.getToggleState();
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillEllipse (rc.translated (2.f, 4.f));
    g.setColour (ink); g.fillEllipse (rc);
    const auto face = rc.reduced (3.5f);
    g.setGradientFill (on ? juce::ColourGradient (pink.brighter (0.3f), face.getX(), face.getY(), pink.darker (0.2f), face.getRight(), face.getBottom(), false)
                          : juce::ColourGradient (juce::Colour (0xff6b2a47), face.getX(), face.getY(), juce::Colour (0xff3a1426), face.getRight(), face.getBottom(), false));
    g.fillEllipse (face);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.5f : 0.15f)); g.fillEllipse (face.getX() + d * 0.16f, face.getY() + d * 0.08f, d * 0.3f, d * 0.16f);
    g.setColour (over ? yellow : ink); g.drawEllipse (face, over ? 3.f : 2.f);
    g.setColour (on ? ink : white.withAlpha (0.8f));
    g.setFont (marker (11.f, false, 0.1f));
    g.drawText (on ? "ON" : "OFF", b.getLocalBounds(), juce::Justification::centred);
}

Sh0tyGS424Editor::Sh0tyGS424Editor (Sh0tyGS424Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    using namespace street;
    setLookAndFeel (&laf);
    addKnob (volume, "volume", "VOLUME", yellow, false);
    addKnob (gain2,  "gain2",  "GAIN 2", pink,   false);
    addKnob (gain1,  "gain1",  "GAIN 1", cyan,   false);
    addKnob (bass,   "bass",   "BASS",   orange, false);
    addKnob (treble, "treble", "TREBLE", green,  false);
    addKnob (mix,    "mix",    "MIX",    white,  true);
    addKnob (trim,   "trim",   "TRIM",   white,  true);

    footswitch.setClickingTogglesState (true);
    footswitch.setTooltip ("Bypass footswitch");
    footswitch.onStateChange = [this] { repaint(); };
    addAndMakeVisible (footswitch);
    footAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "on", footswitch);

    setSize (320, 548);
    startTimerHz (30);
}

Sh0tyGS424Editor::~Sh0tyGS424Editor() { stopTimer(); setLookAndFeel (nullptr); }

void Sh0tyGS424Editor::addKnob (Knob& k, const juce::String& id, const juce::String& text, juce::Colour accent, bool small)
{
    k.slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setRotaryParameters (juce::degreesToRadians (-140.f), juce::degreesToRadians (140.f), true);
    k.slider.getProperties().set ("small", small);
    k.slider.getProperties().set ("accent", (int) accent.getARGB());
    k.slider.setTooltip (text);
    addAndMakeVisible (k.slider);
    k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);
    tags.push_back ({ text, {}, 0.f, small });
}

void Sh0tyGS424Editor::timerCallback()
{
    const float db = juce::Decibels::gainToDecibels (proc.meterLevel.load(), -60.f);
    const float target = juce::jlimit (0.f, 1.f, (db + 40.f) / 43.f);
    needle += (target - needle) * (target > needle ? 0.55f : 0.12f);
    reelAngle += 0.03f + needle * 0.35f;
    repaint (meterArea);
    repaint (tapeArea.expanded (6));
}

void Sh0tyGS424Editor::resized()
{
    const int big = 80, sm = 50;
    Knob* ks[] = { &volume, &gain2, &gain1, &bass, &treble, &mix, &trim };
    const juce::Rectangle<int> rs[] = {
        { 20, 100, big, big }, { 120, 100, big, big }, { 220, 100, big, big },
        { 24, 312, big, big }, { 216, 312, big, big }, { 52, 436, sm, sm }, { 218, 436, sm, sm } };
    const float rot[] = { -0.05f, 0.04f, -0.03f, 0.05f, -0.04f, -0.06f, 0.05f };
    for (int i = 0; i < 7; ++i)
    {
        ks[i]->slider.setBounds (rs[i]);
        tags[(size_t) i].centre = { (float) rs[i].getCentreX(), (float) rs[i].getBottom() + (tags[(size_t) i].small ? 11.f : 12.f) };
        tags[(size_t) i].rotation = rot[i];
    }
    footswitch.setBounds (160 - 32, 426, 64, 64);
    meterArea = { 44, 212, 232, 80 };
    tapeArea  = { 112, 316, 96, 68 };
    renderBackground();
}

void Sh0tyGS424Editor::renderBackground()
{
    using namespace street;
    const int W = getWidth(), H = getHeight(), sc = 2;
    background = juce::Image (juce::Image::ARGB, W * sc, H * sc, true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale ((float) sc));
    juce::Random rng (424);

    // --- brick wall ---
    g.fillAll (juce::Colour (0xff1b1822));
    const float bh = 20.f, bw = 46.f;
    for (int row = 0; row * (int) bh < H + 20; ++row)
        for (float x = (row % 2) ? -bw * 0.5f : 0.f; x < (float) W; x += bw)
        {
            const float v = rng.nextFloat();
            g.setColour (juce::Colour (0xff2d2433).interpolatedWith (juce::Colour (0xff3c2c36), v).withMultipliedBrightness (0.8f + 0.4f * rng.nextFloat()));
            g.fillRect (x + 1.5f, (float) row * bh + 1.5f, bw - 3.f, bh - 3.f);
            g.setColour (juce::Colours::white.withAlpha (0.03f)); g.fillRect (x + 1.5f, (float) row * bh + 1.5f, bw - 3.f, 2.f);
        }
    for (int i = 0; i < 90; ++i)   // grime
    {
        const juce::Point<float> c (rng.nextFloat() * (float) W, rng.nextFloat() * (float) H);
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.18f), c.x, c.y, juce::Colours::transparentBlack, c.x + 26.f, c.y, true));
        g.fillEllipse (c.x - 26.f, c.y - 26.f, 52.f, 52.f);
    }

    // --- overspray clouds + splatter ---
    spray (g, rng, { 60.f, 60.f },  120.f, pink, 2600);
    spray (g, rng, { 270.f, 230.f }, 130.f, cyan, 2600);
    spray (g, rng, { 90.f, 460.f }, 130.f, yellow, 2000);
    spray (g, rng, { 260.f, 470.f }, 90.f, orange, 1200);
    for (int i = 0; i < 60; ++i)
    {
        const auto cols = std::array<juce::Colour, 4> { pink, cyan, yellow, green };
        g.setColour (cols[(size_t) rng.nextInt (4)].withAlpha (0.7f));
        const float d = 1.5f + rng.nextFloat() * 3.5f;
        g.fillEllipse (rng.nextFloat() * (float) W, rng.nextFloat() * (float) H, d, d);
    }

    // --- graffiti title: block-letter extrusion, thick outline, neon fill, highlight, drips ---
    juce::GlyphArrangement ga;
    ga.addLineOfText (marker (50.f, true, 0.02f), "GS-424", 22.f, 62.f);
    juce::Path letters; ga.createPath (letters);
    for (int i = 9; i >= 1; --i)
    {
        g.setColour (i > 4 ? juce::Colour (0xff1a1230) : juce::Colour (0xff4a1d63));
        g.fillPath (letters, juce::AffineTransform::translation ((float) i * 0.9f, (float) i * 0.9f));
    }
    g.setColour (ink); g.strokePath (letters, juce::PathStrokeType (7.f, juce::PathStrokeType::mitered));
    g.setGradientFill (juce::ColourGradient (yellow, 22.f, 20.f, pink, 22.f, 66.f, false));
    g.fillPath (letters);
    g.setColour (cyan); g.strokePath (letters, juce::PathStrokeType (1.6f));
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    for (int i = 0; i < 6; ++i) g.drawLine (30.f + (float) i * 30.f, 26.f, 34.f + (float) i * 30.f, 22.f, 2.f);
    const float dripX[] = { 40.f, 82.f, 130.f, 171.f, 205.f };
    for (float dx : dripX)
    {
        const float len = 5.f + rng.nextFloat() * 10.f;
        g.setColour (ink); g.fillRoundedRectangle (dx - 3.4f, 60.f, 6.8f, len + 5.f, 3.4f); g.fillEllipse (dx - 4.8f, 60.f + len, 9.6f, 9.6f);
        g.setColour (pink); g.fillRoundedRectangle (dx - 2.2f, 58.f, 4.4f, len + 5.f, 2.2f); g.fillEllipse (dx - 3.4f, 61.f + len, 6.8f, 6.8f);
    }
    g.setColour (white); g.setFont (marker (10.f, true, 0.35f));
    g.drawText ("GAIN STAGE", 24, 84, 150, 12, juce::Justification::centredLeft);

    // --- doodles + radiating burst around the status LED ---
    const juce::Point<float> led (W - 50.f, 40.f);
    for (int i = 0; i < 14; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 14.f;
        g.setColour (yellow); g.drawLine ({ led.getPointOnCircumference (22.f, a), led.getPointOnCircumference (30.f + (i % 2) * 6.f, a) }, 2.4f);
    }
    bolt (g, { 26.f, 214.f }, 12.f, yellow);
    star (g, { (float) W - 24.f, 308.f }, 10.f, cyan);
    star (g, { 28.f, 410.f }, 8.f, pink);
    bolt (g, { (float) W - 28.f, 420.f }, 11.f, orange);

    // --- meter housing (slots) ---
    {
        const auto r = meterArea.toFloat();
        g.setColour (ink); g.fillRoundedRectangle (r.expanded (4.f), 8.f);
        g.setColour (cyan.withAlpha (0.8f)); g.drawRoundedRectangle (r.expanded (4.f), 8.f, 2.f);
        g.setColour (juce::Colour (0xff0a0a10)); g.fillRoundedRectangle (r, 6.f);
        g.setColour (white.withAlpha (0.85f)); g.setFont (marker (8.f, false, 0.1f));
        const char* t[] = { "-40", "-30", "-20", "-10", "0", "+3" };
        const float f[] = { 0.f, 0.23f, 0.465f, 0.7f, 0.93f, 1.f };
        for (int i = 0; i < 6; ++i)
            g.drawText (t[i], juce::Rectangle<float> (r.getX() + 10.f + f[i] * (r.getWidth() - 20.f) - 14.f, r.getBottom() - 15.f, 28.f, 10.f), juce::Justification::centred);
        g.setColour (cyan); g.setFont (marker (9.f, true, 0.2f)); g.drawText ("LEVEL", r.getX() + 8.f, r.getY() + 4.f, 60.f, 12.f, juce::Justification::centredLeft);
    }

    // --- masking-tape knob labels ---
    for (auto& t : tags)
    {
        const float w = t.small ? 52.f : 74.f, h = t.small ? 15.f : 19.f;
        juce::Graphics::ScopedSaveState ss (g);
        g.addTransform (juce::AffineTransform::rotation (t.rotation, t.centre.x, t.centre.y));
        juce::Path tp;
        const float x0 = t.centre.x - w * 0.5f, y0 = t.centre.y - h * 0.5f;
        tp.startNewSubPath (x0, y0);
        for (int i = 0; i <= 4; ++i) tp.lineTo (x0 + w * (float) i / 4.f, y0 + ((i % 2) ? 1.f : -0.4f));
        tp.lineTo (x0 + w + 2.f, y0 + h * 0.3f); tp.lineTo (x0 + w - 1.f, y0 + h * 0.6f); tp.lineTo (x0 + w + 1.5f, y0 + h);
        for (int i = 4; i >= 0; --i) tp.lineTo (x0 + w * (float) i / 4.f, y0 + h + ((i % 2) ? -1.f : 0.6f));
        tp.lineTo (x0 - 2.f, y0 + h * 0.65f); tp.lineTo (x0 + 1.f, y0 + h * 0.35f);
        tp.closeSubPath();
        g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillPath (tp, juce::AffineTransform::translation (1.5f, 2.f));
        g.setColour (tape); g.fillPath (tp);
        g.setColour (juce::Colours::white.withAlpha (0.25f)); g.fillRect (x0, y0 + 1.f, w, 2.f);
        g.setColour (ink); g.setFont (marker (t.small ? 9.5f : 12.5f, false, 0.12f));
        g.drawText (t.text, juce::Rectangle<float> (x0, y0, w, h), juce::Justification::centred);
    }

    // --- footer tag ---
    g.setColour (ink); g.setFont (marker (18.f, true, 0.02f));
    g.drawText ("SH0TY", 0, H - 36, W, 22, juce::Justification::centred);
    g.setColour (cyan); g.setFont (marker (18.f, true, 0.02f));
    g.drawText ("SH0TY", 0, H - 38, W, 22, juce::Justification::centred);
    juce::Path swoosh; swoosh.startNewSubPath (110.f, (float) H - 12.f); swoosh.quadraticTo (160.f, (float) H - 4.f, 212.f, (float) H - 15.f);
    glowStroke (g, swoosh, pink, 2.2f);
}

void Sh0tyGS424Editor::paint (juce::Graphics& g)
{
    using namespace street;
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (background, getLocalBounds().toFloat());

    // status LED
    const bool lit = footswitch.getToggleState();
    const juce::Point<float> led ((float) getWidth() - 50.f, 40.f);
    if (lit)
    {
        g.setGradientFill (juce::ColourGradient (cyan.withAlpha (0.7f), led.x, led.y, cyan.withAlpha (0.f), led.x + 30.f, led.y, true));
        g.fillEllipse (led.x - 30.f, led.y - 30.f, 60.f, 60.f);
    }
    g.setColour (ink); g.fillEllipse (led.x - 11.f, led.y - 11.f, 22.f, 22.f);
    g.setColour (lit ? cyan : juce::Colour (0xff0d3a44)); g.fillEllipse (led.x - 8.f, led.y - 8.f, 16.f, 16.f);
    if (lit) { g.setColour (juce::Colours::white.withAlpha (0.8f)); g.fillEllipse (led.x - 4.f, led.y - 5.f, 5.f, 3.5f); }

    // LED bar meter
    {
        const auto r = meterArea.toFloat().reduced (8.f, 6.f).withTrimmedTop (14.f).withTrimmedBottom (14.f);
        const int n = 20;
        const float bw = r.getWidth() / (float) n;
        for (int i = 0; i < n; ++i)
        {
            const bool on = (float) (i + 1) / (float) n <= needle + 0.001f;
            const juce::Colour col = i < 12 ? green : (i < 17 ? yellow : pink);
            const float h = r.getHeight() * (0.35f + 0.65f * (float) i / (float) (n - 1));
            const auto bar = juce::Rectangle<float> (r.getX() + (float) i * bw + 1.f, r.getBottom() - h, bw - 2.5f, h);
            if (on) { g.setColour (col.withAlpha (0.25f)); g.fillRoundedRectangle (bar.expanded (2.f), 2.f); }
            g.setColour (on ? col : col.withAlpha (0.14f));
            g.fillRoundedRectangle (bar, 1.5f);
        }
    }

    // cassette sticker (die-cut, slightly crooked) with turning reels
    {
        const auto r = tapeArea.toFloat();
        juce::Graphics::ScopedSaveState ss (g);
        g.addTransform (juce::AffineTransform::rotation (-0.07f, r.getCentreX(), r.getCentreY()));
        g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (r.translated (2.f, 3.f).expanded (3.f), 8.f);
        g.setColour (white); g.fillRoundedRectangle (r.expanded (3.f), 8.f);                 // sticker border
        g.setColour (ink); g.fillRoundedRectangle (r, 6.f);
        g.setColour (yellow); g.fillRoundedRectangle (r.reduced (3.f), 4.f);
        g.setColour (pink); g.fillRect (r.getX() + 7.f, r.getY() + 8.f, r.getWidth() - 14.f, 12.f);
        g.setColour (ink); g.setFont (marker (7.5f, true, 0.12f));
        g.drawText ("GS-424  C-60", juce::Rectangle<float> (r.getX() + 10.f, r.getY() + 8.f, r.getWidth() - 20.f, 12.f), juce::Justification::centredLeft);
        const auto win = juce::Rectangle<float> (r.getX() + 13.f, r.getY() + 26.f, r.getWidth() - 26.f, 28.f);
        g.setColour (ink); g.fillRoundedRectangle (win, 14.f);
        for (int s = 0; s < 2; ++s)
        {
            const juce::Point<float> c (win.getX() + 14.f + (float) s * (win.getWidth() - 28.f), win.getCentreY());
            g.setColour (white); g.fillEllipse (c.x - 10.f, c.y - 10.f, 20.f, 20.f);
            g.setColour (ink);
            for (int k = 0; k < 6; ++k)
            {
                const float a = reelAngle * (s ? 1.f : 1.15f) + (float) k * juce::MathConstants<float>::pi / 3.f;
                g.drawLine ({ c, c.getPointOnCircumference (8.f, a) }, 1.8f);
            }
            g.drawEllipse (c.x - 10.f, c.y - 10.f, 20.f, 20.f, 1.4f);
        }
    }
}
