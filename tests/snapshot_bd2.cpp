#include "../Source/BluesProcessor.h"
#include "../Source/BluesEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    Sh0tyBD2Processor proc;
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.f);
    juce::File out (argc > 1 ? argv[1] : "bd2.png");
    out.deleteFile();
    juce::FileOutputStream fos (out);
    juce::PNGImageFormat().writeImageToStream (img, fos);
    return 0;
}
