#pragma once
#include "BluesProcessor.h"

// Art-nouveau editor. Layout follows the pedal: Level (left), Gain (right),
// Tone (centre, lower), CHECK jewel at the apex of the arch. Original artwork.
class NouveauLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sh0tyBD2Editor)
};
