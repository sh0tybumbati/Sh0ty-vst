#include "KtgEditor.h"

namespace bauhaus
{
const juce::Colour cream  { 0xffefe6d0 };
const juce::Colour black  { 0xff141414 };
const juce::Colour yellow { 0xfff2c21b };
const juce::Colour blue   { 0xff1f4fa3 };
const juce::Colour red    { 0xffd6321f };

juce::Font sans (float size, float kern = 0.12f)
{
    juce::Font f (juce::FontOptions (size, juce::Font::bold));
    f.setExtraKerningFactor (kern);
    return f;
}

void text (juce::Graphics& g, const juce::String& t, juce::Rectangle<int> r, float size, juce::Colour c,
           juce::Justification j = juce::Justification::centred, float kern = 0.12f)
{
    g.setColour (c);
    g.setFont (sans (size, kern));
    g.drawText (t, r, j);
}
void text (juce::Graphics& g, const char* t, juce::Rectangle<int> r, float size, juce::Colour c,
           juce::Justification j = juce::Justification::centred, float kern = 0.12f)
{
    text (g, juce::String::fromUTF8 (t), r, size, c, j, kern);
}
} // namespace bauhaus

void BauhausLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                           float startAngle, float endAngle, juce::Slider& s)
{
    using namespace bauhaus;
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto c = b.getCentre();
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const float ang = startAngle + pos * (endAngle - startAngle);
    const juce::Colour accent ((juce::uint32) (int) s.getProperties().getWithDefault ("accent", (int) yellow.getARGB()));

    // square tick marks
    g.setColour (black);
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.f * (endAngle - startAngle);
        const auto p = c.getPointOnCircumference (r * 0.97f, a);
        const float sz = (i % 5 == 0) ? 4.f : 2.4f;
        g.fillRect (p.x - sz * 0.5f, p.y - sz * 0.5f, sz, sz);
    }

    // value arc (accent), thick, flat ends
    const float ar = r * 0.8f;
    juce::Path track, value;
    track.addCentredArc (c.x, c.y, ar, ar, 0.f, startAngle, endAngle, true);
    value.addCentredArc (c.x, c.y, ar, ar, 0.f, startAngle, ang, true);
    g.setColour (black.withAlpha (0.15f));
    g.strokePath (track, juce::PathStrokeType (5.f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
    g.setColour (accent);
    g.strokePath (value, juce::PathStrokeType (5.f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));

    // black disc, white pointer bar, accent hub
    const float br = r * 0.62f;
    g.setColour (black);
    g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);
    g.setColour (cream);
    juce::Path bar;
    bar.addRectangle (-1.8f, -br * 0.88f, 3.6f, br * 0.66f);
    g.fillPath (bar, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
    g.setColour (accent);
    g.fillEllipse (c.x - 3.f, c.y - 3.f, 6.f, 6.f);
}

void BauhausLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool)
{
    using namespace bauhaus;
    const auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const juce::String kind = b.getProperties().getWithDefault ("kind", "pull").toString();
    const juce::Colour accent ((juce::uint32) (int) b.getProperties().getWithDefault ("accent", (int) yellow.getARGB()));
    const bool on = b.getToggleState();

    if (kind == "power")
    {
        const float d = juce::jmin (r.getWidth(), r.getHeight());
        const auto rc = r.withSizeKeepingCentre (d, d);
        g.setColour (on ? yellow : black);
        g.fillEllipse (rc);
        g.setColour (black);
        g.drawEllipse (rc, 3.f);
        g.setColour (on ? black : yellow);
        juce::Path sym;
        sym.addCentredArc (rc.getCentreX(), rc.getCentreY(), d * 0.22f, d * 0.22f, 0.f, 0.6f, 2.f * juce::MathConstants<float>::pi - 0.6f, true);
        g.strokePath (sym, juce::PathStrokeType (3.f));
        g.fillRect (rc.getCentreX() - 1.5f, rc.getCentreY() - d * 0.32f, 3.f, d * 0.26f);
        return;
    }

    g.setColour (on ? accent : cream);
    g.fillRect (r);
    g.setColour (black);
    g.drawRect (r, over ? 3.f : 2.f);

    if (kind == "sel")
        text (g, b.getButtonText(), b.getLocalBounds(), 22.f, on ? cream : black, juce::Justification::centred, 0.f);
    else
    {
        text (g, b.getButtonText(), b.getLocalBounds(), 9.f, on ? (accent == yellow ? black : cream) : black, juce::Justification::centred, 0.04f);
    }
}

Sh0tyKTG1Editor::Sh0tyKTG1Editor (Sh0tyKTG1Processor& p)
    : AudioProcessorEditor (&p), proc (p),
      chAttach (*p.apvts.getParameter ("ch2"), [this] (float v)
      {
          ch2State = v > 0.5f;
          sel1.setToggleState (! ch2State, juce::dontSendNotification);
          sel2.setToggleState (ch2State, juce::dontSendNotification);
          repaint();
      }, nullptr)
{
    using namespace bauhaus;
    setLookAndFeel (&laf);

    addKnob (input,   "trim",    "INPUT",    black);
    addKnob (bass,    "bass",    "BASS",     yellow);
    addKnob (mid,     "mid",     "MID",      yellow);
    addKnob (treble,  "treble",  "TREBLE",   yellow);
    addKnob (od1,     "od1",     "OVERDRIVE", blue);
    addKnob (master1, "master1", "MASTER",    blue);
    addKnob (od2,     "od2",     "OVERDRIVE", red);
    addKnob (master2, "master2", "MASTER",    red);
    addKnob (output,  "output",  "OUTPUT",   yellow);
    input.slider.setTextValueSuffix (" dB");

    addToggle (boost1,  "boost1",  "PULL BOOST",  "pull",  blue);
    addToggle (boost2,  "boost2",  "PULL BOOST",  "pull",  red);
    addToggle (crunch2, "crunch2", "PULL CRUNCH", "pull",  red);
    addToggle (power,   "on",      "",            "power", yellow);

    for (auto* b : { &sel1, &sel2 })
    {
        b->getProperties().set ("kind", "sel");
        b->getProperties().set ("accent", (int) black.getARGB());
        addAndMakeVisible (*b);
    }
    sel1.setButtonText ("1"); sel2.setButtonText ("2");
    sel1.onClick = [this] { chAttach.setValueAsCompleteGesture (0.f); };
    sel2.onClick = [this] { chAttach.setValueAsCompleteGesture (1.f); };
    power.button.onStateChange = [this] { repaint(); };
    chAttach.sendInitialUpdate();

    setSize (1000, 288);
}

Sh0tyKTG1Editor::~Sh0tyKTG1Editor() { setLookAndFeel (nullptr); }

void Sh0tyKTG1Editor::addKnob (Knob& k, const juce::String& id, const juce::String& text, juce::Colour accent)
{
    k.slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setRotaryParameters (juce::degreesToRadians (-140.f), juce::degreesToRadians (140.f), true);
    k.slider.getProperties().set ("accent", (int) accent.getARGB());
    addAndMakeVisible (k.slider);
    k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (bauhaus::sans (11.f));
    k.label.setColour (juce::Label::textColourId, bauhaus::black);
    addAndMakeVisible (k.label);
}

void Sh0tyKTG1Editor::addToggle (Toggle& t, const juce::String& id, const juce::String& text,
                                 const juce::String& kind, juce::Colour accent)
{
    t.button.setButtonText (text);
    t.button.setClickingTogglesState (true);
    t.button.getProperties().set ("kind", kind);
    t.button.getProperties().set ("accent", (int) accent.getARGB());
    addAndMakeVisible (t.button);
    t.attach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, t.button);
}

void Sh0tyKTG1Editor::resized()
{
    const int kd = 74;
    auto slot = [&] (Knob& k, int slotX, int slotW)
    {
        k.slider.setBounds (slotX + (slotW - kd) / 2, 100, kd, kd);
        k.label.setBounds (slotX, 176, slotW, 18);
    };
    slot (input, 24, 80);  slot (bass, 104, 80);  slot (mid, 184, 80);  slot (treble, 264, 80);
    slot (od1, 360, 80);   slot (master1, 440, 80);
    slot (od2, 536, 80);   slot (master2, 616, 80);
    slot (output, 716, 80);

    boost1.button.setBounds (364, 204, 72, 26);
    boost2.button.setBounds (540, 204, 72, 26);
    crunch2.button.setBounds (620, 204, 72, 26);

    sel1.setBounds (808, 108, 40, 40);
    sel2.setBounds (856, 108, 40, 40);
    power.button.setBounds (906, 102, 56, 56);
}

void Sh0tyKTG1Editor::paint (juce::Graphics& g)
{
    using namespace bauhaus;
    const int W = getWidth(), H = getHeight();
    g.fillAll (cream);

    // header bar with the geometric mark
    g.setColour (black);
    g.fillRect (0, 0, W, 52);
    text (g, "KTG-1", { 24, 6, 130, 40 }, 34.f, yellow, juce::Justification::centredLeft, 0.05f);
    text (g, "SH0TY  \xC2\xB7  KING TONE GENERATOR  \xC2\xB7  TUBE GUITAR PRE-AMP",
          { 170, 14, 620, 24 }, 12.f, cream, juce::Justification::centredLeft, 0.2f);
    g.setColour (yellow); g.fillEllipse ((float) W - 118.f, 10.f, 32.f, 32.f);
    g.setColour (blue);   g.fillRect ((float) W - 78.f, 10.f, 32.f, 32.f);
    { juce::Path tri; tri.addTriangle ((float) W - 38.f, 42.f, (float) W - 6.f, 42.f, (float) W - 22.f, 10.f);
      g.setColour (red); g.fillPath (tri); }

    struct Card { int x0, x1; juce::Colour strip; juce::Colour ink; const char* title; };
    const Card cards[] = {
        { 24,  344, yellow, black, "INPUT  \xC2\xB7  TONE" },
        { 360, 520, blue,   cream, "CHANNEL 1" },
        { 536, 696, red,    cream, "CHANNEL 2" },
        { 712, 976, black,  yellow, "OUTPUT" },
    };
    for (auto& c : cards)
    {
        const juce::Rectangle<int> r (c.x0, 64, c.x1 - c.x0, 184);
        g.setColour (black);
        g.drawRect (r, 3);
        g.setColour (c.strip);
        g.fillRect (r.getX(), r.getY(), r.getWidth(), 26);
        text (g, c.title, { r.getX() + 10, r.getY(), r.getWidth() - 40, 26 }, 12.f, c.ink,
              juce::Justification::centredLeft, 0.2f);
    }

    // channel LEDs: lit square-in-circle for the selected channel
    auto led = [&] (int cx, bool lit, juce::Colour lc)
    {
        if (lit)   // glow
        {
            g.setGradientFill (juce::ColourGradient (lc.withAlpha (0.6f), (float) cx, 79.f, lc.withAlpha (0.f), (float) cx + 16.f, 79.f, true));
            g.fillEllipse ((float) cx - 16.f, 63.f, 32.f, 32.f);
        }
        g.setColour (cream); g.fillEllipse ((float) cx - 8.f, 71.f, 16.f, 16.f);
        g.setColour (lit ? lc : black.withAlpha (0.25f)); g.fillEllipse ((float) cx - 5.f, 74.f, 10.f, 10.f);
        g.setColour (black); g.drawEllipse ((float) cx - 8.f, 71.f, 16.f, 16.f, 2.f);
    };
    const bool powered = power.button.getToggleState();
    led (504, powered && ! ch2State, yellow);
    led (680, powered && ch2State, yellow);

    // selector title
    text (g, "CHANNEL SELECTOR", { 800, 154, 104, 14 }, 8.f, black, juce::Justification::centred, 0.1f);
    text (g, "ON / OFF", { 896, 164, 76, 14 }, 9.f, black);

    // footer bar
    g.setColour (black);
    g.fillRect (0, 262, W, H - 262);
    g.setColour (yellow); g.fillRect (24, 268, 14, 14);
    g.setColour (blue);   g.fillEllipse (44.f, 268.f, 14.f, 14.f);
    g.setColour (red);    { juce::Path t; t.addTriangle (64.f, 282.f, 78.f, 282.f, 71.f, 268.f); g.fillPath (t); }
    text (g, "TUBE GUITAR PRE-AMP  \xC2\xB7  2 CHANNEL  \xC2\xB7  SHARED 3-BAND EQ",
          { 100, 264, 600, 20 }, 10.f, cream, juce::Justification::centredLeft, 0.2f);
}
