#include "BluesProcessor.h"
#include "BluesEditor.h"

juce::AudioProcessorEditor* Sh0tyBD2Processor::createEditor() { return new Sh0tyBD2Editor (*this); }

Sh0tyBD2Processor::Sh0tyBD2Processor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", [] {
          std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
          auto add = [&] (const char* id, const char* name, float def) {
              p.push_back (std::make_unique<juce::AudioParameterFloat> (
                  juce::ParameterID { id, 1 }, name,
                  juce::NormalisableRange<float> (0.f, 1.f, 0.001f), def)); };
          add ("gain", "Gain", 0.5f); add ("tone", "Tone", 0.5f);
          add ("level", "Level", 0.5f); add ("mix", "Mix", 1.f);
          p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "trim", 1 }, "Input Trim (dB)",
              juce::NormalisableRange<float> (-24.f, 24.f, 0.1f), 0.f));
          return juce::AudioProcessorValueTreeState::ParameterLayout { p.begin(), p.end() };
      }())
{
    pGain = apvts.getRawParameterValue ("gain"); pTone = apvts.getRawParameterValue ("tone");
    pLevel = apvts.getRawParameterValue ("level"); pMix = apvts.getRawParameterValue ("mix");
    pTrim = apvts.getRawParameterValue ("trim");
}

bool Sh0tyBD2Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void Sh0tyBD2Processor::prepareToPlay (double sr, int block)
{
    oversampling.initProcessing ((size_t) block);
    oversampling.reset();
    setLatencySamples ((int) std::round (oversampling.getLatencyInSamples()));
    for (auto& c : channels) c.prepare ((float) (sr * (1 << kStages)));
    for (auto* s : { &gain, &tone, &level, &mix }) s->reset (sr, 0.03);
    gain.setCurrentAndTargetValue (*pGain); tone.setCurrentAndTargetValue (*pTone);
    level.setCurrentAndTargetValue (*pLevel); mix.setCurrentAndTargetValue (*pMix);
}

void Sh0tyBD2Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(), nCh = juce::jmin (buffer.getNumChannels(), 2);
    gain.setTargetValue (*pGain); tone.setTargetValue (*pTone);
    level.setTargetValue (*pLevel); mix.setTargetValue (*pMix);
    bd2::Params prm;
    prm.gain = gain.skip (n); prm.tone = tone.skip (n); prm.level = level.skip (n); prm.mix = mix.skip (n);
    prm.trim = juce::Decibels::decibelsToGain (pTrim->load());

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

void Sh0tyBD2Processor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}
void Sh0tyBD2Processor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Sh0tyBD2Processor(); }
