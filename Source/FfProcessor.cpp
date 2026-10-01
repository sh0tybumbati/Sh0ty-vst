#include "FfProcessor.h"
#include "FfEditor.h"

juce::AudioProcessorEditor* Sh0tyFF1Processor::createEditor() { return new Sh0tyFF1Editor (*this); }

Sh0tyFF1Processor::Sh0tyFF1Processor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", [] {
          std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
          auto add = [&] (const char* id, const char* name, float def) {
              p.push_back (std::make_unique<juce::AudioParameterFloat> (
                  juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (0.f, 1.f, 0.001f), def)); };
          add ("volume", "Volume", 0.7f); add ("fuzz", "Fuzz", 0.8f);
          p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "silicon", 1 }, "Silicon (off = Germanium)", false));
          p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "trim", 1 }, "Input Trim (dB)",
              juce::NormalisableRange<float> (-24.f, 24.f, 0.1f), 0.f));
          p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "on", 1 }, "On", true));
          return juce::AudioProcessorValueTreeState::ParameterLayout { p.begin(), p.end() };
      }())
{
    auto g = [&] (const char* id) { return apvts.getRawParameterValue (id); };
    pFuzz = g ("fuzz"); pVolume = g ("volume"); pTrim = g ("trim"); pSilicon = g ("silicon"); pOn = g ("on");
}

bool Sh0tyFF1Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void Sh0tyFF1Processor::prepareToPlay (double sr, int block)
{
    oversampling.initProcessing ((size_t) block);
    oversampling.reset();
    setLatencySamples ((int) std::round (oversampling.getLatencyInSamples()));
    for (auto& c : channels) { c.prepare ((float) (sr * (1 << kStages))); c.reset(); }
    auto init = [&] (juce::SmoothedValue<float>& s, float v) { s.reset (sr, 0.03); s.setCurrentAndTargetValue (v); };
    init (fuzz, *pFuzz); init (volume, *pVolume); init (onGain, *pOn > 0.5f ? 1.f : 0.f);
}

void Sh0tyFF1Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(), nCh = juce::jmin (buffer.getNumChannels(), 2);
    auto step = [&] (juce::SmoothedValue<float>& s, float target) { s.setTargetValue (target); return s.skip (n); };
    ff1::Params prm;
    prm.fuzz = step (fuzz, *pFuzz); prm.volume = step (volume, *pVolume);
    const float wet = step (onGain, *pOn > 0.5f ? 1.f : 0.f);
    prm.silicon = pSilicon->load() > 0.5f;
    prm.trim = juce::Decibels::decibelsToGain (pTrim->load());
    for (int ch = 0; ch < nCh; ++ch) channels[ch].setParams (prm);

    juce::dsp::AudioBlock<float> block (buffer);
    auto sub = block.getSubsetChannelBlock (0, (size_t) nCh);
    auto up = oversampling.processSamplesUp (sub);
    for (int ch = 0; ch < nCh; ++ch)
    {
        auto* d = up.getChannelPointer ((size_t) ch);
        for (size_t i = 0; i < up.getNumSamples(); ++i)
        {
            const float dry = d[i];
            d[i] = dry * (1.f - wet) + channels[ch].process (dry) * wet;      // footswitch: a smooth fade between dry and fuzz
        }
    }
    oversampling.processSamplesDown (sub);

    float pk = 0.f;
    for (int ch = 0; ch < nCh; ++ch) pk = juce::jmax (pk, buffer.getMagnitude (ch, 0, n));
    meterLevel.store (pk);
}

void Sh0tyFF1Processor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}
void Sh0tyFF1Processor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

#ifndef SH0TY_STANDALONE_BOARD
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Sh0tyFF1Processor(); }
#endif
