#include "FuzzProcessor.h"

Sh0tyFZ3Processor::Sh0tyFZ3Processor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", [] {
          std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
          auto add = [&] (const char* id, const char* name, float def) {
              p.push_back (std::make_unique<juce::AudioParameterFloat> (
                  juce::ParameterID { id, 1 }, name,
                  juce::NormalisableRange<float> (0.f, 1.f, 0.001f), def)); };
          add ("fuzz", "Fuzz", 0.7f); add ("tone", "Tone", 0.5f);
          add ("volume", "Volume", 0.5f); add ("mix", "Mix", 1.f);
          return juce::AudioProcessorValueTreeState::ParameterLayout { p.begin(), p.end() };
      }())
{
    pFuzz = apvts.getRawParameterValue ("fuzz");   pTone = apvts.getRawParameterValue ("tone");
    pVolume = apvts.getRawParameterValue ("volume"); pMix = apvts.getRawParameterValue ("mix");
}

bool Sh0tyFZ3Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void Sh0tyFZ3Processor::prepareToPlay (double sr, int block)
{
    oversampling.initProcessing ((size_t) block);
    oversampling.reset();
    setLatencySamples ((int) std::round (oversampling.getLatencyInSamples()));
    for (auto& c : channels) c.prepare ((float) (sr * (1 << kStages)));
    for (auto* s : { &fuzz, &tone, &volume, &mix }) s->reset (sr, 0.03);
    fuzz.setCurrentAndTargetValue (*pFuzz); tone.setCurrentAndTargetValue (*pTone);
    volume.setCurrentAndTargetValue (*pVolume); mix.setCurrentAndTargetValue (*pMix);
}

void Sh0tyFZ3Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(), nCh = juce::jmin (buffer.getNumChannels(), 2);
    fuzz.setTargetValue (*pFuzz); tone.setTargetValue (*pTone);
    volume.setTargetValue (*pVolume); mix.setTargetValue (*pMix);
    fz3::Params prm;
    prm.fuzz = fuzz.skip (n); prm.tone = tone.skip (n); prm.volume = volume.skip (n); prm.mix = mix.skip (n);

    juce::dsp::AudioBlock<float> block (buffer);
    auto sub = block.getSubsetChannelBlock (0, (size_t) nCh);
    auto up = oversampling.processSamplesUp (sub);
    for (int ch = 0; ch < nCh; ++ch)
    {
        auto* d = up.getChannelPointer ((size_t) ch);
        for (size_t i = 0; i < up.getNumSamples(); ++i) d[i] = channels[ch].process (d[i], prm);
    }
    oversampling.processSamplesDown (sub);
}

void Sh0tyFZ3Processor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}
void Sh0tyFZ3Processor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Sh0tyFZ3Processor(); }
