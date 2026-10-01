#pragma once
#include "FfProcessor.h"

// FF-1 editor: a psychedelic Sasquatch marker drawing (Assets/sasquatch.jpg, cropped to the pedal) with the controls
// built into his face. VOLUME and FUZZ are his eyes, the Ge/Si switch sits in his mouth, the status light is his third
// eye. Same pedal spec as the others (320 x 548, 80 px knobs, 50 px small knob, 64 px stomp, 22 px light).
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
    void renderOverlay();
    void timerCallback() override;

    Sh0tyFF1Processor& proc;
    MarkerLookAndFeel laf;
    Knob volume, fuzz, trim;
    juce::ToggleButton silicon;
    juce::TextButton footswitch;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> footAttach, siAttach;

    juce::Image art, overlay;             // the drawing, and the static lettering / border on top of it
    float level = 0.f, phase = 0.f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sh0tyFF1Editor)
};
