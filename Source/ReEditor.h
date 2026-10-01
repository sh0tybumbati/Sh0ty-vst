#pragma once
#include "ReProcessor.h"

// RE-201 editor: a retro-futuristic wasteland-vault look. Riveted, weathered olive steel, Bakelite and chrome dials,
// a phosphor-green CRT read-out for the mode / head timing, an analogue VU with a radiation-trefoil PEAK lamp, and a
// little animated tape transport. 1000 x 360 rack unit. All artwork is original and drawn in code.
class VaultLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos,
                           float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

class Sh0tyRE201Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Sh0tyRE201Editor (Sh0tyRE201Processor&);
    ~Sh0tyRE201Editor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void stepAnimation (int frames) { for (int i = 0; i < frames; ++i) timerCallback(); }   // used by the UI snapshot tool

private:
    struct Knob { juce::Slider slider; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach; };
    void addKnob (Knob&, const juce::String& id, juce::Colour accent);
    void renderBackground();
    void paintCrt (juce::Graphics&, juce::Rectangle<float>);
    void paintVu (juce::Graphics&, juce::Rectangle<float>);
    void paintTransport (juce::Graphics&);
    void timerCallback() override;
    float value (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }

    Sh0tyRE201Processor& proc;
    VaultLookAndFeel laf;
    Knob mic1, mic2, inst, mode, bass, treble, reverb, rate, intensity, echo;
    juce::Slider outLevel;
    juce::ToggleButton cancel, power;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> cancelAttach, powerAttach;

    juce::Image background;
    float needle = 0.f, reelAngle = 0.f, flicker = 0.f, lamp = 0.f;
    int frame = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sh0tyRE201Editor)
};
