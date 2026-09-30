#pragma once
#include "FuzzProcessor.h"

// Art-deco editor. Layout follows the pedal: Level (top left), Fuzz (top right),
// Tone (centre, lower), CHECK jewel on top. Original artwork.
class DecoLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override {}
};

class Sh0tyFZ3Editor : public juce::AudioProcessorEditor
{
public:
    explicit Sh0tyFZ3Editor (Sh0tyFZ3Processor&);
    ~Sh0tyFZ3Editor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    void addKnob (Knob&, const juce::String& paramId, const juce::String& text, bool small);

    Sh0tyFZ3Processor& proc;
    DecoLookAndFeel laf;
    Knob level, fuzz, tone, mix, trim;
    juce::TextButton footswitch;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> footAttach;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sh0tyFZ3Editor)
};
