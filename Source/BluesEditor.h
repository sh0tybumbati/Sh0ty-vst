#pragma once
#include "BluesProcessor.h"

// Art-nouveau editor. Layout follows the pedal: Level (left), Gain (right),
// Tone (centre, lower), CHECK jewel at the apex of the arch. Original artwork.
class NouveauLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override {}
};

class Sh0tyBD2Editor : public juce::AudioProcessorEditor
{
public:
    explicit Sh0tyBD2Editor (Sh0tyBD2Processor&);
    ~Sh0tyBD2Editor() override;
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

    Sh0tyBD2Processor& proc;
    NouveauLookAndFeel laf;
    Knob level, gain, tone, mix, trim;
    juce::TextButton footswitch;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> footAttach;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sh0tyBD2Editor)
};
