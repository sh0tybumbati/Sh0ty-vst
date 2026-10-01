#include "GsEditor.h"

namespace street
{
const juce::Colour wall   { 0xffd9d6cf };
const juce::Colour ink    { 0xff131313 };
const juce::Colour pink   { 0xffff3d7f };

juce::Font stencil (float size, float kern = 0.06f, bool italic = false)
{
    juce::Font f (juce::FontOptions (size, italic ? (juce::Font::bold | juce::Font::italic) : juce::Font::bold));
    f.setExtraKerningFactor (kern);
    return f;
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

    // hairline track, solid ink value arc
    const float ar = r * 0.92f;
    juce::Path track, value;
    track.addCentredArc (c.x, c.y, ar, ar, 0.f, startAngle, endAngle, true);
    value.addCentredArc (c.x, c.y, ar, ar, 0.f, startAngle, ang, true);
    g.setColour (ink.withAlpha (0.22f));
    g.strokePath (track, juce::PathStrokeType (1.5f));
    if (pos > 0.002f) { g.setColour (ink); g.strokePath (value, juce::PathStrokeType (small ? 3.f : 4.f, juce::PathStrokeType::curved, juce::PathStrokeType::butt)); }

    // flat black knob, thin light ring, pink pointer
    const float kr = r * 0.72f;
    g.setColour (juce::Colours::black.withAlpha (0.25f)); g.fillEllipse (c.x - kr + 1.5f, c.y - kr + 3.f, kr * 2, kr * 2);
    g.setColour (ink); g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    g.setColour (wall.withAlpha (0.5f)); g.drawEllipse (c.x - kr * 0.8f, c.y - kr * 0.8f, kr * 1.6f, kr * 1.6f, 1.f);
    juce::Path bar; bar.addRoundedRectangle (-2.3f, -kr * 0.82f, 4.6f, kr * 0.62f, 2.f);
    g.setColour (pink);
    g.fillPath (bar, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
}

void StreetLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool)
{
    using namespace street;
    const auto r = b.getLocalBounds().toFloat().reduced (3.f);
    const float d = juce::jmin (r.getWidth(), r.getHeight());
    const auto rc = r.withSizeKeepingCentre (d, d);
    const bool on = b.getToggleState();
    g.setColour (on ? pink : wall.brighter (0.15f)); g.fillEllipse (rc);
    g.setColour (ink); g.drawEllipse (rc, over ? 4.f : 3.f);
    g.setColour (ink); g.setFont (stencil (11.f, 0.12f));
    g.drawText (on ? "ON" : "OFF", b.getLocalBounds(), juce::Justification::centred);
}

Sh0tyGS424Editor::Sh0tyGS424Editor (Sh0tyGS424Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    addKnob (volume, "volume", "VOLUME", street::ink, false);
    addKnob (gain2,  "gain2",  "GAIN 2", street::ink, false);
    addKnob (gain1,  "gain1",  "GAIN 1", street::ink, false);
    addKnob (bass,   "bass",   "BASS",   street::ink, false);
    addKnob (treble, "treble", "TREBLE", street::ink, false);
    addKnob (mix,    "mix",    "MIX",    street::ink, true);
    addKnob (trim,   "trim",   "TRIM",   street::ink, true);

    footswitch.setClickingTogglesState (true);
    footswitch.setTooltip ("Bypass footswitch");
    footswitch.onStateChange = [this] { repaint(); };
    addAndMakeVisible (footswitch);
    footAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "on", footswitch);

    setSize (320, 548);
    startTimerHz (30);
}

Sh0tyGS424Editor::~Sh0tyGS424Editor() { stopTimer(); setLookAndFeel (nullptr); }

void Sh0tyGS424Editor::addKnob (Knob& k, const juce::String& id, const juce::String& text, juce::Colour, bool small)
{
    k.slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setRotaryParameters (juce::degreesToRadians (-140.f), juce::degreesToRadians (140.f), true);
    k.slider.getProperties().set ("small", small);
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
    repaint (tapeArea);
}

void Sh0tyGS424Editor::resized()
{
    const int big = 80, sm = 50;
    Knob* ks[] = { &volume, &gain2, &gain1, &bass, &treble, &mix, &trim };
    const juce::Rectangle<int> rs[] = {
        { 20, 112, big, big }, { 120, 112, big, big }, { 220, 112, big, big },
        { 24, 318, big, big }, { 216, 318, big, big }, { 52, 440, sm, sm }, { 218, 440, sm, sm } };
    for (int i = 0; i < 7; ++i)
    {
        ks[i]->slider.setBounds (rs[i]);
        tags[(size_t) i].centre = { (float) rs[i].getCentreX(), (float) rs[i].getBottom() + (tags[(size_t) i].small ? 10.f : 11.f) };
    }
    footswitch.setBounds (160 - 32, 430, 64, 64);
    meterArea = { 44, 226, 232, 56 };
    tapeArea  = { 112, 324, 96, 66 };
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

    // --- concrete wall: flat colour, a touch of grain ---
    g.setGradientFill (juce::ColourGradient (wall.brighter (0.06f), 0.f, 0.f, wall.darker (0.05f), 0.f, (float) H, false));
    g.fillAll();
    for (int i = 0; i < 7000; ++i)
    {
        g.setColour ((rng.nextBool() ? juce::Colours::black : juce::Colours::white).withAlpha (0.02f + 0.05f * rng.nextFloat()));
        g.fillRect (rng.nextFloat() * (float) W, rng.nextFloat() * (float) H, 1.2f, 1.2f);
    }

    // --- the one spray-painted circle (soft edge + a little overspray) ---
    const juce::Point<float> sun ((float) W - 42.f, 58.f);
    const float R = 48.f;
    g.setGradientFill (juce::ColourGradient (pink, sun.x, sun.y, pink.withAlpha (0.f), sun.x + R * 1.05f, sun.y, true));
    g.fillEllipse (sun.x - R * 1.05f, sun.y - R * 1.05f, R * 2.1f, R * 2.1f);
    g.setColour (pink); g.fillEllipse (sun.x - R * 0.88f, sun.y - R * 0.88f, R * 1.76f, R * 1.76f);
    for (int i = 0; i < 900; ++i)
    {
        const float a = rng.nextFloat() * juce::MathConstants<float>::twoPi, r = R * (0.95f + 0.45f * std::pow (rng.nextFloat(), 2.f));
        const auto pt = sun.getPointOnCircumference (r, a);
        g.setColour (pink.withAlpha (0.12f + 0.4f * rng.nextFloat())); const float d = 0.7f + rng.nextFloat() * 1.4f; g.fillEllipse (pt.x, pt.y, d, d);
    }

    // --- stencil title: bold caps with bridge gaps cut through each letter ---
    juce::GlyphArrangement ga;
    ga.addLineOfText (stencil (54.f, 0.06f), "GS-424", 22.f, 74.f);
    juce::Path letters; ga.createPath (letters);
    {
        juce::Graphics::ScopedSaveState ss (g);
        for (int i = 0; i < ga.getNumGlyphs(); ++i)
        {
            const auto b = ga.getBoundingBox (i, 1, true);
            if (b.getWidth() < 4.f) continue;
            g.excludeClipRegion (juce::Rectangle<float> (b.getX() - 1.f, b.getCentreY() - 1.6f, b.getWidth() + 2.f, 3.2f).toNearestInt().withHeight (3));
            if (i == 0 || i == 1 || i == 5) g.excludeClipRegion (juce::Rectangle<int> ((int) b.getCentreX() - 1, (int) b.getY() - 1, 3, (int) b.getHeight() + 2));
        }
        g.setColour (ink); g.fillPath (letters);
    }
    // a single long drip
    {
        const auto b = ga.getBoundingBox (3, 1, true);                 // the first "4": drip from its upright stem
        const float dx = b.getX() + b.getWidth() * 0.74f;
        g.setColour (pink); g.fillRoundedRectangle (dx - 2.3f, 70.f, 4.6f, 30.f, 2.3f); g.fillEllipse (dx - 3.7f, 96.f, 7.4f, 7.4f);
    }
    g.setColour (ink); g.setFont (stencil (9.f, 0.45f));
    g.drawText ("GAIN STAGE", 24, 84, 150, 12, juce::Justification::centredLeft);

    // --- meter baseline + scale ---
    {
        const auto r = meterArea.toFloat();
        g.setColour (ink); g.fillRect (r.getX(), r.getBottom() - 14.f, r.getWidth(), 1.5f);
        g.setFont (stencil (7.5f, 0.1f));
        const char* t[] = { "-40", "-30", "-20", "-10", "0", "+3" };
        const float f[] = { 0.f, 0.23f, 0.465f, 0.7f, 0.93f, 1.f };
        for (int i = 0; i < 6; ++i)
        {
            const float tx = r.getX() + 4.f + f[i] * (r.getWidth() - 8.f);
            g.setColour (ink); g.fillRect (tx - 0.7f, r.getBottom() - 14.f, 1.4f, 4.f);
            g.setColour (ink.withAlpha (0.7f));
            g.drawText (t[i], juce::Rectangle<float> (tx - 14.f, r.getBottom() - 11.f, 28.f, 10.f), juce::Justification::centred);
        }
    }

    // --- knob labels (plain stencil caps) ---
    g.setColour (ink);
    for (auto& t : tags)
    {
        g.setFont (stencil (t.small ? 9.5f : 12.f, 0.2f));
        g.drawText (t.text, juce::Rectangle<float> (t.centre.x - 40.f, t.centre.y - 8.f, 80.f, 16.f), juce::Justification::centred);
    }

    // --- footer ---
    g.setColour (ink); g.setFont (stencil (7.5f, 0.35f));
    g.drawText ("STEP ON IT", 0, 494, W, 10, juce::Justification::centred);
    g.setColour (ink); g.setFont (stencil (17.f, 0.02f, true));
    g.drawText ("SH0TY", 0, H - 40, W, 22, juce::Justification::centred);
    juce::Path swoosh; swoosh.startNewSubPath (116.f, (float) H - 16.f); swoosh.quadraticTo (160.f, (float) H - 8.f, 206.f, (float) H - 18.f);
    g.setColour (pink); g.strokePath (swoosh, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void Sh0tyGS424Editor::paint (juce::Graphics& g)
{
    using namespace street;
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (background, getLocalBounds().toFloat());

    // status light, sitting in the spray circle: filled when on, an empty ring when off
    const bool lit = footswitch.getToggleState();
    const juce::Point<float> led ((float) getWidth() - 42.f, 58.f);
    g.setColour (lit ? wall.brighter (0.4f) : juce::Colours::transparentBlack);
    g.fillEllipse (led.x - 9.f, led.y - 9.f, 18.f, 18.f);
    g.setColour (ink); g.drawEllipse (led.x - 9.f, led.y - 9.f, 18.f, 18.f, 3.f);

    // thin LED-bar meter: ink bars, the top few in pink
    {
        const auto r = meterArea.toFloat().withTrimmedBottom (18.f).reduced (4.f, 0.f);
        const int n = 30;
        const float bw = r.getWidth() / (float) n;
        for (int i = 0; i < n; ++i)
        {
            const bool on = (float) (i + 1) / (float) n <= needle + 0.001f;
            const auto bar = juce::Rectangle<float> (r.getX() + (float) i * bw + 0.8f, r.getY() + 4.f, bw - 2.2f, r.getHeight() - 4.f);
            g.setColour (on ? (i >= 26 ? pink : ink) : ink.withAlpha (0.12f));
            g.fillRect (bar);
        }
    }

    // two stencilled reels that turn with the signal
    {
        const auto r = tapeArea.toFloat();
        const juce::Point<float> cs[] = { { r.getCentreX() - 25.f, r.getCentreY() - 4.f }, { r.getCentreX() + 25.f, r.getCentreY() - 4.f } };
        g.setColour (ink);
        for (int s = 0; s < 2; ++s)
        {
            g.drawEllipse (cs[s].x - 17.f, cs[s].y - 17.f, 34.f, 34.f, 2.6f);
            for (int k = 0; k < 3; ++k)
            {
                const float a = reelAngle * (s ? 1.f : 1.15f) + (float) k * juce::MathConstants<float>::twoPi / 3.f;
                g.drawLine ({ cs[s], cs[s].getPointOnCircumference (12.f, a) }, 3.f);
            }
            g.fillEllipse (cs[s].x - 3.5f, cs[s].y - 3.5f, 7.f, 7.f);
        }
        g.drawLine (cs[0].x, cs[0].y + 17.f, cs[1].x, cs[1].y + 17.f, 1.8f);          // the tape between them
        g.setColour (pink); g.fillEllipse (r.getCentreX() - 3.f, cs[0].y + 14.f, 6.f, 6.f);
    }
}
