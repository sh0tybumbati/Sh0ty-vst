#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "FuzzFaceStage.h"

class Sh0tyFF1Processor : public juce::AudioProcessor
{
public:
    Sh0tyFF1Processor();
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Sh0ty FF-1"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> meterLevel { 0.f };   // output peak of the last block (drives the editor's swirling aura)

private:
    static constexpr int kStages = 1;        // 2x: the circuit solve is the expensive part
    juce::dsp::Oversampling<float> oversampling { 2, kStages, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };
    ff1::FF1 channels[2];
    juce::SmoothedValue<float> fuzz, volume, onGain;
    std::atomic<float>* pFuzz, *pVolume, *pTrim, *pSilicon, *pOn;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sh0tyFF1Processor)
};
