#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// The two fixed ends of the pedalboard signal chain: Guitar In and Output.
class TerminalBase : public juce::AudioProcessor
{
public:
    TerminalBase() : AudioProcessor (BusesProperties()
                                         .withInput  ("In",  juce::AudioChannelSet::stereo(), true)
                                         .withOutput ("Out", juce::AudioChannelSet::stereo(), true)) {}
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}
};

// Guitar input: a guitar is usually on input 1 only, so "mono" copies input 1 to both channels.
class InputTerminal : public TerminalBase
{
public:
    const juce::String getName() const override { return "Guitar In"; }
    void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        const int n = b.getNumSamples();
        if (b.getNumChannels() >= 2 && mono.load()) b.copyFrom (1, 0, b, 0, 0, n);
        b.applyGain (gain.load());
    }
    std::atomic<bool>  mono { true };
    std::atomic<float> gain { 1.f };
};

// Output: master volume and a peak meter for the UI.
class OutputTerminal : public TerminalBase
{
public:
    const juce::String getName() const override { return "Output"; }
    void processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&) override
    {
        b.applyGain (volume.load());
        float pk = 0.f;
        for (int c = 0; c < b.getNumChannels(); ++c) pk = juce::jmax (pk, b.getMagnitude (c, 0, b.getNumSamples()));
        peak.store (pk);
    }
    std::atomic<float> volume { 1.f };
    std::atomic<float> peak   { 0.f };
};
