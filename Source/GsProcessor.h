#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "PortaStage.h"

class Sh0tyGS424Processor : public juce::AudioProcessor
{
public:
    Sh0tyGS424Processor();
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Sh0ty GS-424"; }
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
    std::atomic<float> meterLevel { 0.f };   // output peak of the last block, for the VU meter

private:
    static constexpr int kStages = 2; // 4x
    juce::dsp::Oversampling<float> oversampling { 2, kStages, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };
    gs424::GS424 channels[2];
    juce::SmoothedValue<float> gain1, gain2, bass, treble, volume, mix, onGain;
    std::atomic<float>* pGain1, *pGain2, *pBass, *pTreble, *pVolume, *pMix, *pTrim, *pOn;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sh0tyGS424Processor)
};
