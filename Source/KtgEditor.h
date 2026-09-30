#pragma once
#include "PluginProcessor.h"

// Bauhaus editor laid out like the rack unit: INPUT + shared tone | CHANNEL 1 |
// CHANNEL 2 | OUTPUT, channel selector, power. Primary colours, flat shapes.
// Original artwork; not the Duncan logo or trade dress.
class BauhausLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override {}
};

class Sh0tyKTG1Editor : public juce::AudioProcessorEditor
{
public:
    explicit Sh0tyKTG1Editor (Sh0tyKTG1Processor&);
    ~Sh0tyKTG1Editor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    struct Toggle
    {
        juce::TextButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attach;
    };
    void addKnob (Knob&, const juce::String& id, const juce::String& text, juce::Colour accent);
    void addToggle (Toggle&, const juce::String& id, const juce::String& text, const juce::String& kind, juce::Colour accent);

    Sh0tyKTG1Processor& proc;
    BauhausLookAndFeel laf;
    Knob input, bass, mid, treble, od1, master1, od2, master2, output;
    Toggle boost1, boost2, crunch2, power;
    juce::TextButton sel1, sel2;
    juce::ParameterAttachment chAttach;
    bool ch2State = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sh0tyKTG1Editor)
};
