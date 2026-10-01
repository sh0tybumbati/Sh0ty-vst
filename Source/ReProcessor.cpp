#include "ReProcessor.h"
#include "ReEditor.h"

juce::AudioProcessorEditor* Sh0tyRE201Processor::createEditor() { return new Sh0tyRE201Editor (*this); }

Sh0tyRE201Processor::Sh0tyRE201Processor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", [] {
          std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
          auto add = [&] (const char* id, const char* name, float def) {
              p.push_back (std::make_unique<juce::AudioParameterFloat> (
                  juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (0.f, 1.f, 0.001f), def)); };
          add ("mic1", "Mic Volume 1", 0.7f); add ("mic2", "Mic Volume 2", 0.7f); add ("inst", "Instrument Volume", 0.7f);
          p.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "mode", 1 }, "Mode Selector", 1, 12, 7));
          add ("bass", "Bass", 0.5f); add ("treble", "Treble", 0.5f); add ("reverb", "Reverb Volume", 0.3f);
          add ("rate", "Repeat Rate", 0.5f); add ("intensity", "Intensity", 0.35f); add ("echo", "Echo Volume", 0.6f);
          p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "outlevel", 1 }, "Output Level",
              juce::StringArray { "H", "M", "L" }, 0));
          p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "cancel", 1 }, "Echo Cancel", false));
          p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "on", 1 }, "Power", true));
          return juce::AudioProcessorValueTreeState::ParameterLayout { p.begin(), p.end() };
      }())
{
    auto g = [&] (const char* id) { return apvts.getRawParameterValue (id); };
    pMic1 = g ("mic1"); pMic2 = g ("mic2"); pInst = g ("inst"); pMode = g ("mode"); pBass = g ("bass"); pTreble = g ("treble");
    pReverb = g ("reverb"); pRate = g ("rate"); pIntensity = g ("intensity"); pEcho = g ("echo"); pOut = g ("outlevel");
    pCancel = g ("cancel"); pOn = g ("on");
}

bool Sh0tyRE201Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

static float outGainFor (int sw) { return sw == 0 ? 1.f : sw == 1 ? 0.5f : 0.25f; }     // H / M / L pad
static float sendGain (float k) { return 2.f * k * k; }                                // audio-taper volume pots

void Sh0tyRE201Processor::prepareToPlay (double sr, int)
{
    echo.prepare (sr);
    auto init = [&] (juce::SmoothedValue<float>& s, float v) { s.reset (sr, 0.03); s.setCurrentAndTargetValue (v); };
    init (mic1, sendGain (*pMic1)); init (mic2, sendGain (*pMic2)); init (inst, sendGain (*pInst));
    init (outGain, outGainFor ((int) *pOut));
}

void Sh0tyRE201Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(), nCh = juce::jmin (buffer.getNumChannels(), 2);

    re201::Params prm;
    prm.mode = (int) pMode->load(); prm.repeatRate = pRate->load(); prm.intensity = pIntensity->load();
    prm.echoVol = pEcho->load(); prm.reverbVol = pReverb->load(); prm.bass = pBass->load(); prm.treble = pTreble->load();
    prm.echoCancel = pCancel->load() > 0.5f; prm.on = pOn->load() > 0.5f;
    echo.setParams (prm);

    mic1.setTargetValue (sendGain (*pMic1)); mic2.setTargetValue (sendGain (*pMic2));
    inst.setTargetValue (sendGain (*pInst)); outGain.setTargetValue (outGainFor ((int) *pOut));

    auto* l = buffer.getWritePointer (0);
    auto* r = nCh > 1 ? buffer.getWritePointer (1) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        const float g1 = mic1.getNextValue(), g2 = mic2.getNextValue(), gi = inst.getNextValue(), go = outGain.getNextValue();
        const float dl = l[i], dr = r != nullptr ? r[i] : dl;
        const float send = 0.5f * gi * (g1 * dl + g2 * dr);
        const float wet = echo.process (send);
        l[i] = (dl + wet) * go;
        if (r != nullptr) r[i] = (dr + wet) * go;
    }
    vuLevel.store (echo.vuLevel()); peakLamp.store (echo.peakLamp());
}

void Sh0tyRE201Processor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}
void Sh0tyRE201Processor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

#ifndef SH0TY_STANDALONE_BOARD
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Sh0tyRE201Processor(); }
#endif
