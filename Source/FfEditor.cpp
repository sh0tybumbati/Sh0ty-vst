#include "FfEditor.h"
#include <BinaryData.h>

namespace sas
{
const juce::Colour ink     { 0xff130a1c };
const juce::Colour magenta { 0xffff2fa8 };
const juce::Colour orange  { 0xffff8a1f };
const juce::Colour yellow  { 0xffffe23a };
const juce::Colour lime    { 0xff8dff3a };
const juce::Colour cyan    { 0xff2fe6ff };
const juce::Colour blue    { 0xff3a5bff };
const juce::Colour violet  { 0xff9b3aff };

juce::Colour rainbow (int i)
{
    static const juce::Colour c[] = { magenta, orange, yellow, lime, cyan, blue, violet };
    return c[(size_t) (((i % 7) + 7) % 7)];
}

juce::Font marker (float size, bool italic = true, float kern = 0.03f)
{
    juce::Font f (juce::FontOptions (size, italic ? (juce::Font::bold | juce::Font::italic) : juce::Font::bold));
    f.setExtraKerningFactor (kern);
    return f;
}

void stroke (juce::Graphics& g, const juce::Path& p, juce::Colour c, float w)
{
    g.setColour (ink); g.strokePath (p, juce::PathStrokeType (w + 4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c);   g.strokePath (p, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Path spiral (juce::Point<float> c, float r, float turns)
{
    juce::Path p;
    const int n = 70;
    for (int i = 0; i <= n; ++i)
    {
        const float t = (float) i / (float) n, a = t * turns * juce::MathConstants<float>::twoPi, rr = r * (0.12f + 0.88f * t);
        const auto pt = c.getPointOnCircumference (rr, a);
        if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    return p;
}

// smooth open curve through points (Catmull-Rom -> cubic Beziers)
juce::Path smoothOpen (const std::vector<juce::Point<float>>& p)
{
    juce::Path path;
    const int n = (int) p.size();
    if (n < 3) return path;
    auto at = [&] (int i) { return p[(size_t) juce::jlimit (0, n - 1, i)]; };
    path.startNewSubPath (p[0]);
    for (int i = 0; i < n - 1; ++i)
    {
        const auto p0 = at (i - 1), p1 = at (i), p2 = at (i + 1), p3 = at (i + 2);
        path.cubicTo (p1 + (p2 - p0) / 6.f, p2 - (p3 - p1) / 6.f, p2);
    }
    return path;
}

// marker lettering: coloured fill, thick ink outline, optional offset shadow
void lettering (juce::Graphics& g, const juce::String& text, juce::Point<float> at, float size, juce::Colour fill,
                bool centred, juce::Colour shadow = juce::Colours::transparentBlack, bool italic = true)
{
    juce::GlyphArrangement ga;
    ga.addLineOfText (marker (size, italic, 0.05f), text, 0.f, 0.f);
    const auto bb = ga.getBoundingBox (0, -1, true);
    ga.removeRangeOfGlyphs (0, -1);
    ga.addLineOfText (marker (size, italic, 0.05f), text, centred ? at.x - bb.getWidth() * 0.5f : at.x, at.y);
    juce::Path p; ga.createPath (p);
    if (! shadow.isTransparent()) { g.setColour (shadow); g.fillPath (p, juce::AffineTransform::translation (3.5f, 3.5f)); }
    g.setColour (ink); g.strokePath (p, juce::PathStrokeType (size * 0.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (fill); g.fillPath (p);
}
} // namespace sas

//==============================================================================
// Knobs are eyeballs: white of the eye, a radial-striped iris (like the drawing's own eyes), a pupil, and a pointer.
void MarkerLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float startAngle, float endAngle, juce::Slider& s)
{
    using namespace sas;
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = startAngle + pos * (endAngle - startAngle);
    const bool small = s.getProperties().getWithDefault ("small", false);
    const juce::Colour accent ((juce::uint32) (int) s.getProperties().getWithDefault ("accent", (int) orange.getARGB()));

    // value arc: fat marker line around the eye
    const float ar = r * 0.93f;
    juce::Path track, value;
    track.addCentredArc (c.x, c.y, ar, ar, 0.f, startAngle, endAngle, true);
    value.addCentredArc (c.x, c.y, ar, ar, 0.f, startAngle, ang, true);
    g.setColour (ink.withAlpha (0.5f)); g.strokePath (track, juce::PathStrokeType (small ? 5.f : 7.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (pos > 0.002f) stroke (g, value, accent, small ? 3.f : 4.5f);

    // eyeball
    const float kr = r * 0.74f;
    g.setColour (juce::Colours::black.withAlpha (0.4f)); g.fillEllipse (c.x - kr + 1.5f, c.y - kr + 3.f, kr * 2, kr * 2);
    g.setColour (juce::Colour (0xfffff6e4)); g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    const float ir = kr * 0.8f;
    g.setColour (accent.darker (0.3f)); g.fillEllipse (c.x - ir, c.y - ir, ir * 2, ir * 2);
    const int spokes = small ? 18 : 26;
    for (int i = 0; i < spokes; ++i)         // radial iris strokes, alternating colours
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / (float) spokes;
        g.setColour (i % 2 ? accent.brighter (0.35f) : yellow);
        g.drawLine ({ c.getPointOnCircumference (ir * 0.34f, a), c.getPointOnCircumference (ir * 0.96f, a) }, small ? 2.2f : 3.2f);
    }
    g.setColour (ink); g.drawEllipse (c.x - ir, c.y - ir, ir * 2, ir * 2, small ? 2.f : 2.8f);
    const float pr = ir * 0.36f;
    g.setColour (ink); g.fillEllipse (c.x - pr, c.y - pr, pr * 2, pr * 2);
    g.setColour (juce::Colours::white); g.fillEllipse (c.x - pr * 0.9f, c.y - pr * 1.1f, pr * 0.6f, pr * 0.6f);
    g.setColour (ink); g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, small ? 3.5f : 5.f);
    // pointer: a thick marker line from the pupil out across the iris
    juce::Path ptr; ptr.startNewSubPath (c.getPointOnCircumference (pr * 1.15f, ang)); ptr.lineTo (c.getPointOnCircumference (kr * 0.96f, ang));
    stroke (g, ptr, juce::Colours::white, small ? 2.4f : 3.2f);
}

// footswitch: a hypnotic spiral, bright when on
void MarkerLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool)
{
    using namespace sas;
    const auto r = b.getLocalBounds().toFloat().reduced (3.f);
    const float d = juce::jmin (r.getWidth(), r.getHeight());
    const auto rc = r.withSizeKeepingCentre (d, d);
    const bool on = b.getToggleState();
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillEllipse (rc.translated (2.f, 4.f));
    g.setColour (ink); g.fillEllipse (rc);
    const auto face = rc.reduced (4.f);
    g.setColour (on ? magenta : juce::Colour (0xff5a2f86)); g.fillEllipse (face.translated (1.2f, 1.f));
    g.setColour (on ? magenta.brighter (0.15f) : juce::Colour (0xff6a3a98)); g.fillEllipse (face);
    g.setColour (over ? yellow : ink); g.drawEllipse (face, over ? 3.f : 2.5f);
    const auto sp = spiral (face.getCentre(), face.getWidth() * 0.4f, 2.2f);
    g.setColour (ink); g.strokePath (sp, juce::PathStrokeType (on ? 6.f : 5.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (on ? juce::Colours::white : juce::Colour (0xff9a6ac8)); g.strokePath (sp, juce::PathStrokeType (on ? 3.2f : 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// the Ge / Si switch, sitting in his mouth: a chunky pill with an eyeball thumb
void MarkerLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    using namespace sas;
    const auto r = b.getLocalBounds().toFloat().reduced (3.f);
    const bool si = b.getToggleState();
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (r.translated (2.f, 3.f), r.getHeight() * 0.5f);
    g.setColour (si ? cyan.darker (0.35f) : orange.darker (0.25f)); g.fillRoundedRectangle (r.translated (1.2f, 1.f), r.getHeight() * 0.5f);
    g.setColour (si ? cyan : orange); g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (over ? yellow : ink); g.drawRoundedRectangle (r, r.getHeight() * 0.5f, over ? 4.5f : 4.f);
    g.setColour (ink); g.setFont (marker (15.f, false, 0.05f));
    if (si) g.drawText ("SI", r.withTrimmedRight (r.getWidth() * 0.45f).translated (3.f, 0.f), juce::Justification::centred);   // the mode's name sits on the side the thumb left free
    else    g.drawText ("GE", r.withTrimmedLeft (r.getWidth() * 0.45f).translated (-3.f, 0.f), juce::Justification::centred);
    const float td = r.getHeight() - 8.f;
    const auto thumb = juce::Rectangle<float> (si ? r.getRight() - td - 4.f : r.getX() + 4.f, r.getY() + 4.f, td, td);
    g.setColour (juce::Colours::white); g.fillEllipse (thumb);
    g.setColour (ink); g.drawEllipse (thumb, 3.f);
    g.setColour (si ? blue : magenta); g.fillEllipse (thumb.getCentreX() - td * 0.2f, thumb.getCentreY() - td * 0.2f, td * 0.4f, td * 0.4f);
    g.setColour (ink); g.fillEllipse (thumb.getCentreX() - td * 0.09f, thumb.getCentreY() - td * 0.09f, td * 0.18f, td * 0.18f);
}

namespace
{
const juce::Point<int> kVolume { 56, 134 }, kFuzz { 265, 134 }, kLight { 184, 124 };
}

//==============================================================================
Sh0tyFF1Editor::Sh0tyFF1Editor (Sh0tyFF1Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    using namespace sas;
    setLookAndFeel (&laf);
    art = juce::ImageCache::getFromMemory (BinaryData::fuzzface_jpg, BinaryData::fuzzface_jpgSize);
    addKnob (volume, "volume", orange,  false);     // the left eye
    addKnob (fuzz,   "fuzz",   magenta, false);     // the right eye
    addKnob (trim,   "trim",   lime,    true);

    silicon.setClickingTogglesState (true);
    silicon.setTooltip ("Germanium (left) or silicon (right) transistors");
    addAndMakeVisible (silicon);
    siAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "silicon", silicon);

    footswitch.setClickingTogglesState (true);
    footswitch.setTooltip ("Bypass footswitch");
    footswitch.onStateChange = [this] { repaint(); };
    addAndMakeVisible (footswitch);
    footAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "on", footswitch);

    setSize (320, 548);
    startTimerHz (30);
}

Sh0tyFF1Editor::~Sh0tyFF1Editor() { stopTimer(); setLookAndFeel (nullptr); }

void Sh0tyFF1Editor::addKnob (Knob& k, const juce::String& id, juce::Colour accent, bool small)
{
    k.slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setRotaryParameters (juce::degreesToRadians (-140.f), juce::degreesToRadians (140.f), true);
    k.slider.getProperties().set ("small", small);
    k.slider.getProperties().set ("accent", (int) accent.getARGB());
    addAndMakeVisible (k.slider);
    k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);
}

void Sh0tyFF1Editor::timerCallback()
{
    const float db = juce::Decibels::gainToDecibels (proc.meterLevel.load(), -60.f);
    const float target = juce::jlimit (0.f, 1.f, (db + 40.f) / 40.f);
    level += (target - level) * (target > level ? 0.5f : 0.1f);
    phase += 0.03f + level * 0.2f;
    repaint (0, 60, 320, 140);        // the glow around the orbs
}

void Sh0tyFF1Editor::resized()
{
    // positions follow the lettering painted into the artwork: VOLUME / FUZZ sit under the two swirl orbs, TRIM on his
    // chest, Ge / Si above the switch on the tree, the stomp button between his feet
    volume.slider.setBounds ({ kVolume.x - 40, kVolume.y - 40, 80, 80 });
    fuzz.slider.setBounds   ({ kFuzz.x - 40, kFuzz.y - 40, 80, 80 });
    silicon.setBounds       ({ 12, 280, 84, 40 });
    trim.slider.setBounds   ({ 161 - 25, 270 - 25, 50, 50 });
    footswitch.setBounds    ({ 172 - 32, 424 - 32, 64, 64 });
}

//------------------------------------------------------------------------------ paint
void Sh0tyFF1Editor::paint (juce::Graphics& g)
{
    using namespace sas;
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    const auto bounds = getLocalBounds().toFloat();
    g.drawImage (art, bounds);

    // the two orbs glow with the signal
    for (auto eye : { std::make_pair (kVolume.toFloat(), orange), std::make_pair (kFuzz.toFloat(), magenta) })
    {
        const float a = 0.10f + 0.55f * level, r = 50.f + 6.f * std::sin (phase);
        g.setGradientFill (juce::ColourGradient (eye.second.withAlpha (a), eye.first.x, eye.first.y, eye.second.withAlpha (0.f), eye.first.x + r, eye.first.y, true));
        g.fillEllipse (eye.first.x - r, eye.first.y - r, r * 2, r * 2);
    }

    // status light: a painted star in the rainbow, lit when the pedal is on
    const bool lit = footswitch.getToggleState();
    const juce::Point<float> eye (kLight.toFloat());
    if (lit)
    {
        g.setGradientFill (juce::ColourGradient (lime.withAlpha (0.8f), eye.x, eye.y, lime.withAlpha (0.f), eye.x + 30.f, eye.y, true));
        g.fillEllipse (eye.x - 30.f, eye.y - 30.f, 60.f, 60.f);
        for (int i = 0; i < 10; ++i)       // little rays
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / 10.f + phase * 0.2f;
            juce::Path ray; ray.startNewSubPath (eye.getPointOnCircumference (15.f, a)); ray.lineTo (eye.getPointOnCircumference (22.f + (i % 2) * 5.f, a));
            stroke (g, ray, yellow, 2.4f);
        }
    }
    g.setColour (ink); g.fillEllipse (eye.x - 11.f, eye.y - 11.f, 22.f, 22.f);
    g.setColour (lit ? yellow : juce::Colour (0xff4a2a6a)); g.fillEllipse (eye.x - 8.f, eye.y - 8.f, 16.f, 16.f);
    g.setColour (lit ? juce::Colour (0xff1a0c05) : juce::Colour (0xff22123a)); g.fillEllipse (eye.x - 3.5f, eye.y - 3.5f, 7.f, 7.f);
    if (lit) { g.setColour (juce::Colours::white); g.fillEllipse (eye.x - 5.f, eye.y - 6.f, 3.5f, 3.f); }
}
