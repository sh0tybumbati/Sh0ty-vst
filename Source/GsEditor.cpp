#include "GsEditor.h"

namespace cassette
{
const juce::Colour caseDark { 0xff26282c };
const juce::Colour panel    { 0xffdcd3bd };
const juce::Colour panelLo  { 0xffc4b99f };
const juce::Colour ink      { 0xff2a2724 };
const juce::Colour orange   { 0xffe8742a };
const juce::Colour yellow   { 0xfff0b429 };
const juce::Colour teal     { 0xff2f8f8a };
const juce::Colour red      { 0xffc9402f };
const juce::Colour cream    { 0xfff4ecd8 };

juce::Font sans (float size, float kern = 0.1f, bool bold = true)
{
    juce::Font f (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    f.setExtraKerningFactor (kern);
    return f;
}

void screw (juce::Graphics& g, juce::Point<float> c)
{
    g.setColour (juce::Colour (0xff9a9486)); g.fillEllipse (c.x - 5.f, c.y - 5.f, 10.f, 10.f);
    g.setColour (ink.withAlpha (0.7f)); g.drawEllipse (c.x - 5.f, c.y - 5.f, 10.f, 10.f, 1.f);
    g.drawLine (c.x - 3.f, c.y + 1.5f, c.x + 3.f, c.y - 1.5f, 1.6f);
}
} // namespace cassette

void CassetteLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                            float startAngle, float endAngle, juce::Slider& s)
{
    using namespace cassette;
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = startAngle + pos * (endAngle - startAngle);
    const bool small = s.getProperties().getWithDefault ("small", false);

    // tick dots
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.f * (endAngle - startAngle);
        const auto p = c.getPointOnCircumference (r * 0.97f, a);
        const float d = (i % 5 == 0) ? 4.f : 2.4f;
        g.setColour (i % 5 == 0 ? orange : ink.withAlpha (0.8f));
        g.fillEllipse (p.x - d * 0.5f, p.y - d * 0.5f, d, d);
    }

    const float kr = r * 0.78f;
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (c.x - kr + 1.5f, c.y - kr + 3.f, kr * 2, kr * 2);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff7d7f84), c.x - kr, c.y - kr, juce::Colour (0xff2b2d31), c.x + kr, c.y + kr, false));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    // fluted rim
    const int flutes = small ? 24 : 34;
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    for (int i = 0; i < flutes; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / (float) flutes;
        g.drawLine ({ c.getPointOnCircumference (kr * 0.86f, a), c.getPointOnCircumference (kr * 0.99f, a) }, 1.2f);
    }
    const float cr = kr * 0.74f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a5c61), c.x - cr, c.y - cr, juce::Colour (0xff1d1f22), c.x + cr, c.y + cr, false));
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
    // orange pointer, as on the reference pedal
    g.setColour (orange);
    juce::Path bar; bar.addRoundedRectangle (-2.2f, -cr * 0.96f, 4.4f, cr * 0.62f, 2.f);
    g.fillPath (bar, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
}

void CassetteLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool)
{
    using namespace cassette;
    const auto r = b.getLocalBounds().toFloat().reduced (2.f);
    const float d = juce::jmin (r.getWidth(), r.getHeight());
    const auto rc = r.withSizeKeepingCentre (d, d);
    const bool on = b.getToggleState();
    g.setColour (juce::Colours::black.withAlpha (0.4f)); g.fillEllipse (rc.translated (1.5f, 3.f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe6e8ea), rc.getX(), rc.getY(), juce::Colour (0xff7b7e83), rc.getRight(), rc.getBottom(), false));
    g.fillEllipse (rc);                                    // chrome footswitch cap
    const auto face = rc.reduced (d * 0.16f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (on ? 0xfff2f3f4 : 0xffb9bcc0), face.getX(), face.getY(), juce::Colour (0xff6a6d72), face.getRight(), face.getBottom(), false));
    g.fillEllipse (face);
    g.setColour (over ? orange : ink.withAlpha (0.6f)); g.drawEllipse (face, 1.6f);
    g.setColour (ink.withAlpha (0.55f));
    juce::Font f = sans (9.f, 0.2f); g.setFont (f);
    g.drawText (on ? "ON" : "OFF", b.getLocalBounds(), juce::Justification::centred);
}

Sh0tyGS424Editor::Sh0tyGS424Editor (Sh0tyGS424Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    addKnob (volume, "volume", "VOLUME", false);
    addKnob (gain2,  "gain2",  "GAIN 2", false);
    addKnob (gain1,  "gain1",  "GAIN 1", false);
    addKnob (bass,   "bass",   "BASS",   false);
    addKnob (treble, "treble", "TREBLE", false);
    addKnob (mix,    "mix",    "MIX",    true);
    addKnob (trim,   "trim",   "TRIM",   true);

    footswitch.setClickingTogglesState (true);
    footswitch.setTooltip ("Bypass footswitch");
    footswitch.onStateChange = [this] { repaint(); };
    addAndMakeVisible (footswitch);
    footAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "on", footswitch);

    setSize (320, 548);
    startTimerHz (30);
}

Sh0tyGS424Editor::~Sh0tyGS424Editor() { stopTimer(); setLookAndFeel (nullptr); }

void Sh0tyGS424Editor::addKnob (Knob& k, const juce::String& id, const juce::String& text, bool small)
{
    k.slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setRotaryParameters (juce::degreesToRadians (-140.f), juce::degreesToRadians (140.f), true);
    k.slider.getProperties().set ("small", small);
    addAndMakeVisible (k.slider);
    k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (cassette::sans (small ? 10.f : 12.f, 0.15f));
    k.label.setColour (juce::Label::textColourId, cassette::ink);
    addAndMakeVisible (k.label);
}

void Sh0tyGS424Editor::timerCallback()
{
    const float lvl = proc.meterLevel.load();
    const float db = juce::Decibels::gainToDecibels (lvl, -60.f);
    const float target = juce::jlimit (0.f, 1.f, (db + 40.f) / 43.f);
    needle += (target - needle) * (target > needle ? 0.5f : 0.1f);
    reelAngle += 0.03f + needle * 0.35f;
    repaint (vuArea);
    repaint (tapeArea);
}

void Sh0tyGS424Editor::resized()
{
    const int big = 80, sm = 50;
    auto place = [] (Knob& k, juce::Rectangle<int> r, int labelH)
    {
        k.slider.setBounds (r);
        k.label.setBounds (r.getX() - 8, r.getBottom() - 1, r.getWidth() + 16, labelH);
    };
    place (volume, { 20, 96, big, big }, 18);
    place (gain2,  { 120, 96, big, big }, 18);
    place (gain1,  { 220, 96, big, big }, 18);
    place (bass,   { 24, 302, big, big }, 18);
    place (treble, { 216, 302, big, big }, 18);
    place (mix,    { 52, 428, sm, sm }, 16);
    place (trim,   { 218, 428, sm, sm }, 16);
    footswitch.setBounds (160 - 31, 418, 62, 62);
    vuArea   = { 44, 204, 232, 82 };
    tapeArea = { 112, 308, 96, 68 };
}

void Sh0tyGS424Editor::paint (juce::Graphics& g)
{
    using namespace cassette;
    const float W = (float) getWidth(), H = (float) getHeight();
    g.fillAll (caseDark);

    // front panel
    const auto pan = juce::Rectangle<float> (10.f, 10.f, W - 20.f, H - 20.f);
    g.setGradientFill (juce::ColourGradient (panel, 0.f, 10.f, panelLo, 0.f, H, false));
    g.fillRoundedRectangle (pan, 18.f);
    g.setColour (juce::Colours::white.withAlpha (0.35f)); g.drawRoundedRectangle (pan.reduced (1.f), 18.f, 1.5f);
    g.setColour (ink.withAlpha (0.5f)); g.drawRoundedRectangle (pan, 18.f, 1.f);
    for (auto c : { juce::Point<float> (26.f, 26.f), { W - 26.f, 26.f }, { 26.f, H - 26.f }, { W - 26.f, H - 26.f } }) screw (g, c);

    // title + status LED
    g.setColour (ink);        g.setFont (sans (40.f, 0.02f)); g.drawText ("GS-424", 40, 18, 170, 46, juce::Justification::centredLeft);
    g.setColour (orange);     g.setFont (sans (10.f, 0.3f));  g.drawText ("GAIN STAGE", 40, 57, 150, 12, juce::Justification::centredLeft);
    const bool lit = footswitch.getToggleState();
    const juce::Point<float> led (W - 50.f, 40.f);
    if (lit)
    {
        g.setGradientFill (juce::ColourGradient (orange.withAlpha (0.6f), led.x, led.y, orange.withAlpha (0.f), led.x + 24.f, led.y, true));
        g.fillEllipse (led.x - 24.f, led.y - 24.f, 48.f, 48.f);
    }
    g.setColour (ink); g.fillEllipse (led.x - 9.f, led.y - 9.f, 18.f, 18.f);
    g.setColour (lit ? juce::Colour (0xffff9a4a) : juce::Colour (0xff4a2a14)); g.fillEllipse (led.x - 6.5f, led.y - 6.5f, 13.f, 13.f);
    if (lit) { g.setColour (juce::Colours::white.withAlpha (0.6f)); g.fillEllipse (led.x - 3.5f, led.y - 4.5f, 4.f, 3.f); }

    // four-colour tape stripe
    {
        const juce::Colour cols[] = { orange, yellow, teal, red };
        for (int i = 0; i < 4; ++i) { g.setColour (cols[i]); g.fillRect (24.f, 72.f + (float) i * 3.5f, W - 48.f, 3.f); }
    }

    // VU meter window
    {
        const auto r = vuArea.toFloat();
        g.setColour (ink); g.fillRoundedRectangle (r.expanded (3.f), 9.f);
        g.setGradientFill (juce::ColourGradient (cream, r.getX(), r.getY(), juce::Colour (0xffe3d6b4), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 7.f);
        const juce::Point<float> pivot (r.getCentreX(), r.getBottom() + 18.f);
        const float R = 88.f, a0 = juce::degreesToRadians (-46.f), a1 = juce::degreesToRadians (46.f);
        // red zone
        juce::Path zone; zone.addCentredArc (pivot.x, pivot.y, R, R, 0.f, a0 + (a1 - a0) * 0.84f, a1, true);
        g.setColour (red); g.strokePath (zone, juce::PathStrokeType (5.f));
        juce::Path arc; arc.addCentredArc (pivot.x, pivot.y, R, R, 0.f, a0, a0 + (a1 - a0) * 0.84f, true);
        g.setColour (ink); g.strokePath (arc, juce::PathStrokeType (1.5f));
        g.setFont (sans (8.f, 0.05f));
        const char* labels[] = { "-40", "-30", "-20", "-10", "", "0", "+3" };
        const float marks[]  = { 0.f, 0.23f, 0.47f, 0.7f, 0.81f, 0.93f, 1.f };
        for (int i = 0; i < 7; ++i)
        {
            const float a = a0 + (a1 - a0) * marks[i];
            g.setColour (i >= 6 ? red : ink);
            g.drawLine ({ pivot.getPointOnCircumference (R - 7.f, a), pivot.getPointOnCircumference (R + 2.f, a) }, 1.4f);
            const auto tp = pivot.getPointOnCircumference (R - 17.f, a);
            g.drawText (labels[i], juce::Rectangle<float> (tp.x - 14.f, tp.y - 6.f, 28.f, 12.f), juce::Justification::centred);
        }
        g.setColour (ink.withAlpha (0.7f)); g.setFont (sans (11.f, 0.2f));
        g.drawText ("dB", r.withTrimmedTop (r.getHeight() - 24.f).withTrimmedLeft (r.getWidth() * 0.5f - 12.f).withWidth (24.f), juce::Justification::centred);
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (vuArea);
            const float na = a0 + (a1 - a0) * needle;
            g.setColour (juce::Colours::black.withAlpha (0.25f));
            g.drawLine ({ pivot.translated (2.f, 2.f), pivot.getPointOnCircumference (R - 3.f, na).translated (2.f, 2.f) }, 2.f);
            g.setColour (red); g.drawLine ({ pivot, pivot.getPointOnCircumference (R - 3.f, na) }, 1.8f);
        }
    }

    // cassette with turning reels
    {
        const auto r = tapeArea.toFloat();
        g.setColour (ink); g.fillRoundedRectangle (r, 6.f);
        g.setColour (cream); g.fillRoundedRectangle (r.reduced (3.f), 4.f);
        g.setColour (orange); g.fillRect (r.getX() + 8.f, r.getY() + 8.f, r.getWidth() - 16.f, 12.f);   // label strip
        g.setColour (ink); g.setFont (sans (7.f, 0.2f)); g.drawText ("GS-424  C-60", juce::Rectangle<float> (r.getX() + 10.f, r.getY() + 8.f, r.getWidth() - 20.f, 12.f), juce::Justification::centredLeft);
        const auto win = juce::Rectangle<float> (r.getX() + 14.f, r.getY() + 26.f, r.getWidth() - 28.f, 28.f);
        g.setColour (ink); g.fillRoundedRectangle (win, 14.f);
        for (int s = 0; s < 2; ++s)
        {
            const juce::Point<float> c (win.getX() + 14.f + (float) s * (win.getWidth() - 28.f), win.getCentreY());
            g.setColour (cream); g.fillEllipse (c.x - 10.f, c.y - 10.f, 20.f, 20.f);
            g.setColour (ink);
            for (int k = 0; k < 6; ++k)
            {
                const float a = reelAngle * (s ? 1.f : 1.15f) + (float) k * juce::MathConstants<float>::pi / 3.f;
                g.drawLine ({ c, c.getPointOnCircumference (8.f, a) }, 1.6f);
            }
            g.drawEllipse (c.x - 10.f, c.y - 10.f, 20.f, 20.f, 1.2f);
        }
        g.setColour (juce::Colour (0xff6a4a2a)); g.drawLine (win.getX() + 24.f, win.getBottom() - 4.f, win.getRight() - 24.f, win.getBottom() - 4.f, 1.4f);
    }

    // footer
    g.setColour (ink.withAlpha (0.6f)); g.setFont (sans (9.f, 0.3f));
    g.drawText (juce::String::fromUTF8 ("TAPE-STAGE PREAMP  \xC2\xB7  SH0TY"), 0, (int) H - 36, (int) W, 14, juce::Justification::centred);
}
