#include "GsProcessor.h"
#include "GsEditor.h"

juce::AudioProcessorEditor* Sh0tyGS424Processor::createEditor() { return new Sh0tyGS424Editor (*this); }

Sh0tyGS424Processor::Sh0tyGS424Processor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", [] {
          std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
          auto add = [&] (const char* id, const char* name, float def) {
              p.push_back (std::make_unique<juce::AudioParameterFloat> (
                  juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (0.f, 1.f, 0.001f), def)); };
          add ("volume", "Volume", 0.5f); add ("gain2", "Gain 2", 0.3f); add ("gain1", "Gain 1", 0.4f);
          add ("bass", "Bass", 0.5f); add ("treble", "Treble", 0.5f); add ("mix", "Mix", 1.f);
          p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "trim", 1 }, "Input Trim (dB)",
              juce::NormalisableRange<float> (-24.f, 24.f, 0.1f), 0.f));
          p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "on", 1 }, "On", true));
          return juce::AudioProcessorValueTreeState::ParameterLayout { p.begin(), p.end() };
      }())
{
    auto g = [&] (const char* id) { return apvts.getRawParameterValue (id); };
    pGain1 = g ("gain1"); pGain2 = g ("gain2"); pBass = g ("bass"); pTreble = g ("treble");
    pVolume = g ("volume"); pMix = g ("mix"); pTrim = g ("trim"); pOn = g ("on");
}

bool Sh0tyGS424Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void Sh0tyGS424Processor::prepareToPlay (double sr, int block)
{
    oversampling.initProcessing ((size_t) block);
    oversampling.reset();
    setLatencySamples ((int) std::round (oversampling.getLatencyInSamples()));
    for (auto& c : channels) c.prepare ((float) (sr * (1 << kStages)));
    auto init = [&] (juce::SmoothedValue<float>& s, float v) { s.reset (sr, 0.03); s.setCurrentAndTargetValue (v); };
    init (gain1, *pGain1); init (gain2, *pGain2); init (bass, *pBass); init (treble, *pTreble);
    init (volume, *pVolume); init (mix, *pMix); init (onGain, *pOn > 0.5f ? 1.f : 0.f);
}

void Sh0tyGS424Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(), nCh = juce::jmin (buffer.getNumChannels(), 2);
    auto step = [&] (juce::SmoothedValue<float>& s, float target) { s.setTargetValue (target); return s.skip (n); };
    gs424::Params prm;
    prm.gain1 = step (gain1, *pGain1); prm.gain2 = step (gain2, *pGain2);
    prm.bass = step (bass, *pBass); prm.treble = step (treble, *pTreble); prm.volume = step (volume, *pVolume);
    prm.mix = step (mix, *pMix) * step (onGain, *pOn > 0.5f ? 1.f : 0.f);
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
            const float wet = channels[ch].process (d[i]);
            d[i] = d[i] * (1.f - prm.mix) + wet * prm.mix;
        }
    }
    oversampling.processSamplesDown (sub);

    float pk = 0.f;
    for (int ch = 0; ch < nCh; ++ch) pk = juce::jmax (pk, buffer.getMagnitude (ch, 0, n));
    meterLevel.store (pk);
}

void Sh0tyGS424Processor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}
void Sh0tyGS424Processor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Sh0tyGS424Processor(); }
