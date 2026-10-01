#include "PluginProcessor.h"
#include "KtgEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout Sh0tyKTG1Processor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto knob = [&] (const char* id, const char* name, float def)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (0.f, 10.f, 0.01f), def * 10.f));
    };
    auto sw = [&] (const char* id, const char* name, bool def)
    {
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def));
    };
    knob ("bass",    "Bass",             0.5f);
    knob ("mid",     "Mid",              0.5f);
    knob ("treble",  "Treble",           0.5f);
    knob ("od1",     "Ch1 Overdrive",    0.4f);
    knob ("master1", "Ch1 Master",       0.5f);
    knob ("od2",     "Ch2 Overdrive",    0.5f);
    knob ("master2", "Ch2 Master",       0.5f);
    knob ("output",  "Output",           0.5f);
    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "trim", 1 }, "Input Trim (dB)",
        juce::NormalisableRange<float> (-24.f, 24.f, 0.1f), 0.f));
    sw ("boost1",  "Ch1 Pull Boost",  false);
    sw ("boost2",  "Ch2 Pull Boost",  false);
    sw ("crunch2", "Ch2 Pull Crunch", false);
    sw ("ch2",     "Channel 2 Selected", false);
    sw ("on",      "On",              true);
    return { p.begin(), p.end() };
}

juce::AudioProcessorEditor* Sh0tyKTG1Processor::createEditor() { return new Sh0tyKTG1Editor (*this); }

Sh0tyKTG1Processor::Sh0tyKTG1Processor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    auto g = [&] (const char* id) { return apvts.getRawParameterValue (id); };
    pBass = g ("bass"); pMid = g ("mid"); pTreble = g ("treble");
    pOd1 = g ("od1"); pMaster1 = g ("master1"); pOd2 = g ("od2"); pMaster2 = g ("master2");
    pOutput = g ("output"); pTrim = g ("trim");
    pBoost1 = g ("boost1"); pBoost2 = g ("boost2"); pCrunch2 = g ("crunch2"); pCh2 = g ("ch2"); pOn = g ("on");
}

bool Sh0tyKTG1Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void Sh0tyKTG1Processor::prepareToPlay (double sr, int block)
{
    oversampling.initProcessing ((size_t) block);
    oversampling.reset();
    setLatencySamples ((int) std::round (oversampling.getLatencyInSamples()));

    for (auto& c : channels)
        c.prepare ((float) (sr * (1 << kOversampleStages)));

    auto init = [&] (juce::SmoothedValue<float>& s, std::atomic<float>* v)
    {
        s.reset (sr, 0.03);
        s.setCurrentAndTargetValue (v->load() * 0.1f);
    };
    init (bass, pBass); init (mid, pMid); init (treble, pTreble);
    init (od1, pOd1); init (master1, pMaster1); init (od2, pOd2); init (master2, pMaster2);
    init (output, pOutput);
}

void Sh0tyKTG1Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nCh = juce::jmin (buffer.getNumChannels(), 2);

    auto step = [&] (juce::SmoothedValue<float>& s, std::atomic<float>* v)
    {
        s.setTargetValue (v->load() * 0.1f);
        return s.skip (n);
    };
    ktg1::Params prm;
    prm.bass = step (bass, pBass); prm.mid = step (mid, pMid); prm.treble = step (treble, pTreble);
    prm.od1 = step (od1, pOd1); prm.master1 = step (master1, pMaster1);
    prm.od2 = step (od2, pOd2); prm.master2 = step (master2, pMaster2);
    prm.output = step (output, pOutput);
    prm.trim = juce::Decibels::decibelsToGain (pTrim->load());
    prm.boost1 = pBoost1->load() > 0.5f; prm.boost2 = pBoost2->load() > 0.5f;
    prm.crunch2 = pCrunch2->load() > 0.5f; prm.ch2 = pCh2->load() > 0.5f;
    prm.on = pOn->load() > 0.5f;

    for (int ch = 0; ch < nCh; ++ch) channels[ch].setParams (prm);

    juce::dsp::AudioBlock<float> block (buffer);
    auto sub = block.getSubsetChannelBlock (0, (size_t) nCh);
    auto up = oversampling.processSamplesUp (sub);

    for (int ch = 0; ch < nCh; ++ch)
    {
        auto* d = up.getChannelPointer ((size_t) ch);
        for (size_t i = 0; i < up.getNumSamples(); ++i)
            d[i] = channels[ch].process (d[i]);
    }
    oversampling.processSamplesDown (sub);
}

void Sh0tyKTG1Processor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void Sh0tyKTG1Processor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// The pedalboard app links these processors directly and has no plugin entry point.
#ifndef SH0TY_STANDALONE_BOARD
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Sh0tyKTG1Processor(); }
#endif
