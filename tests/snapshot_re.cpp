#include "../Source/ReProcessor.h"
#include "../Source/ReEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    Sh0tyRE201Processor proc;
    const juce::String arg = argc > 2 ? argv[2] : "";
    if (arg == "off")    proc.apvts.getParameter ("on")->setValueNotifyingHost (0.f);
    if (arg == "cancel") proc.apvts.getParameter ("cancel")->setValueNotifyingHost (1.f);
    if (arg == "reverb") proc.apvts.getParameter ("mode")->setValueNotifyingHost (proc.apvts.getParameter ("mode")->convertTo0to1 (10.f));
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    if (arg == "play")      // needle up, PEAK lamp lit
    {
        proc.vuLevel.store (0.35f); proc.peakLamp.store (1.f);
        static_cast<Sh0tyRE201Editor*> (ed.get())->stepAnimation (20);
    }
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.f);
    juce::File out (argc > 1 ? argv[1] : "re201.png");
    out.deleteFile();
    juce::FileOutputStream fos (out);
    juce::PNGImageFormat().writeImageToStream (img, fos);
    return 0;
}
