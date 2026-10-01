#include "BoardComponent.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    BoardModel model;
    juce::AudioDeviceManager dm;
    model.buildDefaultBoard();
    {
        BoardComponent board (model, dm);
        board.setSize (1140, 780);

        // UI-level checks: patch with a simulated cable drag, find a cable by position, refuse a loop
        int fz = 0, rack = 0;
        for (auto& m : model.modules) { if (m.type == ModuleType::Fz3) fz = m.id; if (m.type == ModuleType::Ktg1) rack = m.id; }
        model.disconnect (fz, rack);
        const bool dragged = board.simulateCableDrag (fz, rack);
        const bool loop    = board.simulateCableDrag (rack, fz);
        const auto mid = (board.jackPosition (fz, true) + board.jackPosition (rack, false)) / 2;
        std::printf ("drag-connect FZ-3 -> KTG-1: %s\n", dragged ? "OK" : "FAILED");
        std::printf ("drag a loop back (KTG-1 -> FZ-3) is refused: %s\n", ! loop ? "OK" : "FAILED");
        std::printf ("cable hit-test near a jack: %s\n", board.cableAt (board.jackPosition (fz, true) + juce::Point<int> (3, 4)) >= 0 ? "OK" : "FAILED");
        juce::ignoreUnused (mid);
        auto img = board.createComponentSnapshot (board.getLocalBounds(), true, 1.f);
        juce::File out (argc > 1 ? argv[1] : "board.png");
        out.deleteFile();
        juce::FileOutputStream fos (out);
        juce::PNGImageFormat().writeImageToStream (img, fos);
    }
    return 0;
}
