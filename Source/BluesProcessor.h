#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "BluesStage.h"

class Sh0tyBD2Processor : public juce::AudioProcessor
{
public:
    Sh0tyBD2Processor();
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Sh0ty BD-2"; }
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
private:
    static constexpr int kStages = 2; // 4x
    juce::dsp::Oversampling<float> oversampling { 2, kStages,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };
    bd2::BD2 channels[2];
    juce::SmoothedValue<float> onGain, gain, tone, level, mix;
    std::atomic<float>* pOn, *pGain, *pTone, *pLevel, *pMix, *pTrim;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sh0tyBD2Processor)
};
