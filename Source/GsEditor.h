#pragma once
#include "GsProcessor.h"

// Minimal street-art editor for the GS-424. Layout follows the pedal reference: VOLUME / GAIN 2 / GAIN 1 on top,
// BASS / TREBLE below. A concrete wall, one spray-painted circle, stencil lettering, a thin LED-bar meter and two
// stencilled reels that turn with the signal. All artwork is original and drawn in code.
class StreetLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override {}
};

class Sh0tyGS424Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Sh0tyGS424Editor (Sh0tyGS424Processor&);
    ~Sh0tyGS424Editor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void stepAnimation (int frames) { for (int i = 0; i < frames; ++i) timerCallback(); }   // used by the UI snapshot tool

private:
    struct Knob
    {
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    struct Tag { juce::String text; juce::Point<float> centre; float rotation; bool small; };

    void addKnob (Knob&, const juce::String& id, const juce::String& text, juce::Colour accent, bool small);
    void renderBackground();
    void timerCallback() override;

    Sh0tyGS424Processor& proc;
    StreetLookAndFeel laf;
    Knob volume, gain2, gain1, bass, treble, mix, trim;
    juce::TextButton footswitch;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> footAttach;

    std::vector<Tag> tags;
    juce::Image background;
    float needle = 0.f, reelAngle = 0.f;
    juce::Rectangle<int> meterArea, tapeArea;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sh0tyGS424Editor)
};
