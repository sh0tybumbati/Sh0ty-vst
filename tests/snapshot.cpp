#include "../Source/FuzzProcessor.h"
#include "../Source/FuzzEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    Sh0tyFZ3Processor proc;
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.f);
    juce::File out (argc > 1 ? argv[1] : "fz3.png");
    out.deleteFile();
    juce::FileOutputStream fos (out);
    juce::PNGImageFormat().writeImageToStream (img, fos);
    return 0;
}
