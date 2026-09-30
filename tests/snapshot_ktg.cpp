#include "../Source/PluginProcessor.h"
#include "../Source/KtgEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    Sh0tyKTG1Processor proc;
    if (argc > 2 && juce::String (argv[2]) == "off")
        proc.apvts.getParameter ("on")->setValueNotifyingHost (0.f);
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
    juce::File out (argc > 1 ? argv[1] : "ktg.png");
    out.deleteFile();
    juce::FileOutputStream fos (out);
    juce::PNGImageFormat().writeImageToStream (img, fos);
    return 0;
}
