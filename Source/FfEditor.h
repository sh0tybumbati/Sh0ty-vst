#pragma once
#include "FfProcessor.h"

// FF-1 editor: the supplied psychedelic Sasquatch poster (Assets/fuzzface.jpg, same proportions as the pedal) with the
// controls placed on the lettering already painted into it: VOLUME and FUZZ are eyeball dials over the two swirl orbs, TRIM
// is on his chest, the Ge / Si switch sits on the tree under its labels, the stomp button is between his feet and the
// status light is a star in the rainbow. Same pedal spec as the others (320 x 548, 80 px knobs, 50 px small knob, 64 px
// stomp, 22 px light).
class MarkerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override {}
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

class Sh0tyFF1Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Sh0tyFF1Editor (Sh0tyFF1Processor&);
    ~Sh0tyFF1Editor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void stepAnimation (int frames) { for (int i = 0; i < frames; ++i) timerCallback(); }   // used by the UI snapshot tool

private:
    struct Knob { juce::Slider slider; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach; };
    void addKnob (Knob&, const juce::String& id, juce::Colour accent, bool small);
    void timerCallback() override;

    Sh0tyFF1Processor& proc;
    MarkerLookAndFeel laf;
    Knob volume, fuzz, trim;
    juce::ToggleButton silicon;
    juce::TextButton footswitch;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> footAttach, siAttach;

    juce::Image art;                      // the finished artwork, lettering and frame included
    float level = 0.f, phase = 0.f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sh0tyFF1Editor)
};
