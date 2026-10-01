#include "../Source/GsProcessor.h"
#include "../Source/GsEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    Sh0tyGS424Processor proc;
    if (argc > 2 && juce::String (argv[2]) == "off")
        proc.apvts.getParameter ("on")->setValueNotifyingHost (0.f);
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    if (argc > 2 && juce::String (argv[2]) == "play")   // advance the VU needle / reels
    {
        proc.meterLevel.store (0.35f);
        static_cast<Sh0tyGS424Editor*> (ed.get())->stepAnimation (25);
    }
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.f);
    juce::File out (argc > 1 ? argv[1] : "gs.png");
    out.deleteFile();
    juce::FileOutputStream fos (out);
    juce::PNGImageFormat().writeImageToStream (img, fos);
    return 0;
}
