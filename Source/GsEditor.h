#pragma once
#include "GsProcessor.h"

// Retro cassette-futurism editor for the GS-424. Layout follows the pedal reference: VOLUME / GAIN 2 / GAIN 1 on
// top, BASS / TREBLE below, plus a VU meter and a cassette whose reels turn with the signal. Original artwork.
class CassetteLookAndFeel : public juce::LookAndFeel_V4
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
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    void addKnob (Knob&, const juce::String& id, const juce::String& text, bool small);
    void timerCallback() override;

    Sh0tyGS424Processor& proc;
    CassetteLookAndFeel laf;
    Knob volume, gain2, gain1, bass, treble, mix, trim;
    juce::TextButton footswitch;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> footAttach;

    float needle = 0.f, reelAngle = 0.f;
    juce::Rectangle<int> vuArea, tapeArea;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sh0tyGS424Editor)
};
