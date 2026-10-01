#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "SpaceEchoStage.h"

// Rack unit: the RE-201 tape echo + spring reverb. Dry stereo passes straight through; the echo/reverb return is mono
// (as on the real unit) and is added on top. MIC 1 / MIC 2 set how much of the left / right input is sent to the
// echo, INSTRUMENT VOLUME is the overall send level into the mic amp.
class Sh0tyRE201Processor : public juce::AudioProcessor
{
public:
    Sh0tyRE201Processor();
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Sh0ty RE-201"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> vuLevel { 0.f }, peakLamp { 0.f };   // read by the editor's meter and PEAK lamp

private:
    re201::SpaceEcho echo;
    juce::SmoothedValue<float> mic1, mic2, inst, outGain;
    std::atomic<float>* pMic1, *pMic2, *pInst, *pMode, *pBass, *pTreble, *pReverb, *pRate, *pIntensity, *pEcho, *pOut, *pCancel, *pOn;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sh0tyRE201Processor)
};
