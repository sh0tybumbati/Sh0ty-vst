#include "PluginProcessor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout Sh0tyKTG1Processor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto add = [&] (const char* id, const char* name, float def)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (0.f, 1.f, 0.001f), def));
    };
    add ("drive",  "Drive",  0.5f);
    add ("tone",   "Tone",   0.5f);
    add ("level",  "Level",  0.6f);
    add ("bright", "Bright", 0.0f);
    add ("mix",    "Mix",    1.0f);
    return { p.begin(), p.end() };
}

Sh0tyKTG1Processor::Sh0tyKTG1Processor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pDrive  = apvts.getRawParameterValue ("drive");
    pTone   = apvts.getRawParameterValue ("tone");
    pLevel  = apvts.getRawParameterValue ("level");
    pBright = apvts.getRawParameterValue ("bright");
    pMix    = apvts.getRawParameterValue ("mix");
}

bool Sh0tyKTG1Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

int Sh0tyKTG1Processor::getLatencySamplesInt() const
{
    return (int) std::round (oversampling.getLatencyInSamples());
}

void Sh0tyKTG1Processor::prepareToPlay (double sr, int block)
{
    oversampling.initProcessing ((size_t) block);
    oversampling.reset();
    setLatencySamples (getLatencySamplesInt());

    for (auto& c : channels)
        c.prepare ((float) (sr * (1 << kOversampleStages)));

    for (auto* s : { &drive, &tone, &level, &bright, &mix })
        s->reset (sr, 0.03);
    drive.setCurrentAndTargetValue (*pDrive);   tone.setCurrentAndTargetValue (*pTone);
    level.setCurrentAndTargetValue (*pLevel);   bright.setCurrentAndTargetValue (*pBright);
    mix.setCurrentAndTargetValue (*pMix);
}

void Sh0tyKTG1Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nCh = juce::jmin (buffer.getNumChannels(), 2);

    drive.setTargetValue (*pDrive);   tone.setTargetValue (*pTone);
    level.setTargetValue (*pLevel);   bright.setTargetValue (*pBright);
    mix.setTargetValue (*pMix);

    // Per-block (smoothed) params; sampled once per block start-to-end average.
    ktg1::Params prm;
    prm.drive  = drive.skip (n);   prm.tone = tone.skip (n);
    prm.level  = level.skip (n);   prm.bright = bright.skip (n);
    prm.mix    = mix.skip (n);

    juce::dsp::AudioBlock<float> block (buffer);
    auto sub = block.getSubsetChannelBlock (0, (size_t) nCh);
    auto up = oversampling.processSamplesUp (sub);

    for (int ch = 0; ch < nCh; ++ch)
    {
        auto* d = up.getChannelPointer ((size_t) ch);
        for (size_t i = 0; i < up.getNumSamples(); ++i)
            d[i] = channels[ch].process (d[i], prm);
    }
    oversampling.processSamplesDown (sub);
}

juce::AudioProcessorEditor* Sh0tyKTG1Processor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
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

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Sh0tyKTG1Processor(); }
