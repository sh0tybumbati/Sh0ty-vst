#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "TubeStage.h"

class Sh0tyKTG1Processor : public juce::AudioProcessor
{
public:
    Sh0tyKTG1Processor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Sh0ty KTG-1"; }
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
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    static constexpr int kOversampleStages = 2; // 4x
    juce::dsp::Oversampling<float> oversampling { 2, kOversampleStages,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };
    ktg1::KTG1 channels[2];

    // continuous params are smoothed per block; switches are read directly
    juce::SmoothedValue<float> bass, mid, treble, od1, master1, od2, master2, output;
    std::atomic<float>* pBass, *pMid, *pTreble, *pOd1, *pMaster1, *pOd2, *pMaster2, *pOutput, *pTrim;
    std::atomic<float>* pBoost1, *pBoost2, *pCrunch2, *pCh2, *pOn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sh0tyKTG1Processor)
};
