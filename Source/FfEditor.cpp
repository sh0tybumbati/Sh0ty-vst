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

} // namespace sas

//==============================================================================
// Hardware is brass: knurled brass dials, a domed brass stomp button and a nickel toggle lever for Ge / Si.
namespace brass
{
const juce::Colour hi { 0xfffbe6a0 }, light { 0xffe8bf55 }, mid { 0xffc8962a }, dark { 0xff7d5410 }, deep { 0xff3b2606 };

juce::ColourGradient body (juce::Point<float> c, float r)     // lit from the upper left
{
    juce::ColourGradient gr (light, c.x - r * 0.55f, c.y - r * 0.7f, dark, c.x + r * 0.8f, c.y + r * 0.9f, false);
    gr.addColour (0.35, mid);
    return gr;
}
} // namespace brass

void MarkerLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float startAngle, float endAngle, juce::Slider& s)
{
    using namespace brass;
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = startAngle + pos * (endAngle - startAngle);
    const bool small = s.getProperties().getWithDefault ("small", false);

    // scale ticks on a dark backing so they read over the artwork
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    juce::Path ring; ring.addCentredArc (c.x, c.y, r * 0.93f, r * 0.93f, 0.f, startAngle - 0.12f, endAngle + 0.12f, true);
    g.strokePath (ring, juce::PathStrokeType (r * 0.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.f * (endAngle - startAngle);
        const bool major = i % 5 == 0;
        g.setColour (hi.withAlpha (major ? 1.f : 0.75f));
        g.drawLine ({ c.getPointOnCircumference (r * (major ? 0.82f : 0.86f), a), c.getPointOnCircumference (r * 0.99f, a) }, major ? 2.4f : 1.5f);
    }

    const float kr = r * 0.72f;
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillEllipse (c.x - kr + 1.5f, c.y - kr + 3.5f, kr * 2, kr * 2);
    // knurled skirt
    g.setGradientFill (body (c, kr)); g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    const int ridges = small ? 30 : 40;
    for (int i = 0; i < ridges; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / (float) ridges;
        g.setColour (deep.withAlpha (0.55f));
        g.drawLine ({ c.getPointOnCircumference (kr * 0.86f, a), c.getPointOnCircumference (kr * 0.995f, a) }, small ? 1.2f : 1.6f);
        g.setColour (hi.withAlpha (0.35f));
        g.drawLine ({ c.getPointOnCircumference (kr * 0.86f, a + 0.045f), c.getPointOnCircumference (kr * 0.995f, a + 0.045f) }, 0.8f);
    }
    g.setColour (deep); g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.4f);
    // polished cap with fine concentric turning marks
    const float cr = kr * 0.74f;
    g.setGradientFill (juce::ColourGradient (hi, c.x - cr * 0.6f, c.y - cr * 0.7f, mid.darker (0.25f), c.x + cr, c.y + cr, false));
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
    g.setColour (deep.withAlpha (0.16f));
    for (float t = 0.2f; t < 1.f; t += 0.16f) g.drawEllipse (c.x - cr * t, c.y - cr * t, cr * t * 2, cr * t * 2, 0.7f);
    g.setColour (deep.withAlpha (0.7f)); g.drawEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2, 1.2f);
    // engraved pointer line, filled with black enamel, and a lit dot on the skirt
    juce::Path ptr; ptr.addRoundedRectangle (-1.6f, -cr * 0.92f, 3.2f, cr * 0.82f, 1.4f);
    g.setColour (juce::Colour (0xff1a1006)); g.fillPath (ptr, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
    const auto tip = c.getPointOnCircumference (kr * 0.93f, ang);
    g.setColour (juce::Colour (0xff1a1006)); g.fillEllipse (tip.x - 2.2f, tip.y - 2.2f, 4.4f, 4.4f);
    g.setColour (hi); g.fillEllipse (tip.x - 1.2f, tip.y - 1.6f, 2.2f, 2.2f);
}

// footswitch: a domed brass button with a knurled collar; a warm glow when the pedal is on
void MarkerLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool)
{
    using namespace brass;
    const auto r = b.getLocalBounds().toFloat().reduced (2.f);
    const float d = juce::jmin (r.getWidth(), r.getHeight());
    const auto rc = r.withSizeKeepingCentre (d, d);
    const auto c = rc.getCentre(); const float R = d * 0.5f;
    const bool on = b.getToggleState();
    const bool down = b.isDown();
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillEllipse (rc.translated (1.5f, 3.5f));
    g.setGradientFill (body (c, R)); g.fillEllipse (rc);                                  // collar
    for (int i = 0; i < 44; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 44.f;
        g.setColour (deep.withAlpha (0.55f)); g.drawLine ({ c.getPointOnCircumference (R * 0.88f, a), c.getPointOnCircumference (R * 0.995f, a) }, 1.5f);
    }
    g.setColour (deep); g.drawEllipse (rc, 1.4f);
    const float dr = R * 0.74f * (down ? 0.97f : 1.f);                                    // dome
    g.setGradientFill (juce::ColourGradient (hi, c.x - dr * 0.45f, c.y - dr * 0.55f, mid.darker (0.35f), c.x + dr, c.y + dr, true));
    g.fillEllipse (c.x - dr, c.y - dr, dr * 2, dr * 2);
    g.setColour (deep.withAlpha (0.8f)); g.drawEllipse (c.x - dr, c.y - dr, dr * 2, dr * 2, 1.3f);
    g.setColour (juce::Colours::white.withAlpha (over ? 0.55f : 0.4f)); g.fillEllipse (c.x - dr * 0.5f, c.y - dr * 0.72f, dr * 0.55f, dr * 0.3f);   // highlight
    if (on) { g.setColour (juce::Colour (0xffffc04a).withAlpha (0.2f)); g.fillEllipse (c.x - R * 1.25f, c.y - R * 1.25f, R * 2.5f, R * 2.5f); }
}

// the Ge / Si switch: a nickel toggle lever, thrown left for germanium and right for silicon
void MarkerLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    const auto r = b.getLocalBounds().toFloat();
    const bool si = b.getToggleState();
    const juce::Point<float> pivot (r.getCentreX(), r.getBottom() - 16.f);

    // hex nut and bushing
    juce::Path nut;
    for (int i = 0; i < 6; ++i)
    {
        const auto p = pivot.getPointOnCircumference (15.f, juce::MathConstants<float>::pi / 6.f + (float) i * juce::MathConstants<float>::pi / 3.f);
        if (i == 0) nut.startNewSubPath (p); else nut.lineTo (p);
    }
    nut.closeSubPath();
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillPath (nut, juce::AffineTransform::translation (1.5f, 3.f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff2f2ee), pivot.x - 14.f, pivot.y - 14.f, juce::Colour (0xff6b6e68), pivot.x + 14.f, pivot.y + 14.f, false));
    g.fillPath (nut);
    g.setColour (juce::Colour (0xff2a2c28)); g.strokePath (nut, juce::PathStrokeType (1.2f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe8e8e2), pivot.x - 8.f, pivot.y - 8.f, juce::Colour (0xff53564f), pivot.x + 8.f, pivot.y + 8.f, false));
    g.fillEllipse (pivot.x - 9.f, pivot.y - 9.f, 18.f, 18.f);
    g.setColour (juce::Colour (0xff2a2c28)); g.drawEllipse (pivot.x - 9.f, pivot.y - 9.f, 18.f, 18.f, 1.f);

    // the bat: a tapered lever with a rounded tip, thrown to one side
    const float ang = juce::degreesToRadians (si ? 38.f : -38.f), len = r.getHeight() - 22.f;
    juce::Path bat;
    bat.startNewSubPath (-4.6f, 0.f); bat.lineTo (-3.0f, -len + 4.f);
    bat.quadraticTo (0.f, -len - 3.f, 3.0f, -len + 4.f); bat.lineTo (4.6f, 0.f); bat.closeSubPath();
    const auto xf = juce::AffineTransform::rotation (ang).translated (pivot.x, pivot.y);
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillPath (bat, xf.translated (2.f, 3.f));
    const auto tip = pivot + juce::Point<float> (std::sin (ang), -std::cos (ang)) * len;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffafaf6), pivot.x - 6.f, pivot.y - len, juce::Colour (0xff60645c), pivot.x + 8.f, pivot.y, false));
    g.fillPath (bat, xf);
    g.setColour (juce::Colour (0xff2a2c28)); g.strokePath (bat, juce::PathStrokeType (1.1f), xf);
    g.setColour (juce::Colours::white.withAlpha (over ? 0.8f : 0.55f));
    g.drawLine (juce::Line<float> (pivot + juce::Point<float> (std::sin (ang), -std::cos (ang)) * 8.f - juce::Point<float> (std::cos (ang), std::sin (ang)) * 1.8f,
                                   tip - juce::Point<float> (std::cos (ang), std::sin (ang)) * 1.4f), 1.4f);
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
    silicon.setBounds       ({ 52 - 26, 268 - 36, 52, 64 });   // between the painted Ge and Si labels
    trim.slider.setBounds   ({ 161 - 25, 270 - 25, 50, 50 });
    footswitch.setBounds    ({ 172 - 32, 436 - 32, 64, 64 });
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

    // status light: a brass bezel with a jewel, lit when the pedal is on
    const bool lit = footswitch.getToggleState();
    const juce::Point<float> eye (kLight.toFloat());
    if (lit)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffd24a).withAlpha (0.75f), eye.x, eye.y, juce::Colour (0xffffd24a).withAlpha (0.f), eye.x + 30.f, eye.y, true));
        g.fillEllipse (eye.x - 30.f, eye.y - 30.f, 60.f, 60.f);
    }
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillEllipse (eye.x - 11.f + 1.f, eye.y - 11.f + 2.f, 22.f, 22.f);
    g.setGradientFill (brass::body (eye, 11.f)); g.fillEllipse (eye.x - 11.f, eye.y - 11.f, 22.f, 22.f);
    g.setColour (brass::deep); g.drawEllipse (eye.x - 11.f, eye.y - 11.f, 22.f, 22.f, 1.2f);
    g.setGradientFill (juce::ColourGradient (lit ? juce::Colour (0xfffff2a8) : juce::Colour (0xff6a3a14), eye.x - 3.f, eye.y - 4.f,
                                             lit ? juce::Colour (0xffff9a1a) : juce::Colour (0xff2a1408), eye.x + 7.f, eye.y + 7.f, true));
    g.fillEllipse (eye.x - 7.f, eye.y - 7.f, 14.f, 14.f);
    g.setColour (brass::deep); g.drawEllipse (eye.x - 7.f, eye.y - 7.f, 14.f, 14.f, 1.f);
    g.setColour (juce::Colours::white.withAlpha (lit ? 0.8f : 0.3f)); g.fillEllipse (eye.x - 4.5f, eye.y - 5.f, 4.f, 3.f);
}
