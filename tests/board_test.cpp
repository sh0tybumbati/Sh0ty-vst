#include "BoardModel.h"
#include <cstdio>

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { std::printf ("FAIL: %s\n", msg); ++fails; } else std::printf ("ok:   %s\n", msg); } while (0)

// push a few blocks of a sine through the graph; return the peak of the last block
static float runBlocks (BoardModel& b, float amp, int blocks = 40)
{
    juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer midi; float peak = 0.f; double ph = 0.0;
    for (int blk = 0; blk < blocks; ++blk)
    {
        for (int i = 0; i < 512; ++i) { const float s = amp * (float) std::sin (ph); buf.setSample (0, i, s); buf.setSample (1, i, 0.f); ph += 2.0 * juce::MathConstants<double>::pi * 220.0 / 48000.0; }
        b.graph.processBlock (buf, midi);
        peak = juce::jmax (buf.getMagnitude (0, 0, 512), buf.getMagnitude (1, 0, 512));
    }
    return peak;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    BoardModel board;
    board.graph.setPlayConfigDetails (2, 2, 48000.0, 512);
    board.graph.prepareToPlay (48000.0, 512);

    // an empty board is silent (nothing connects input to output)
    CHECK (runBlocks (board, 0.2f) == 0.f, "nothing patched: silence");

    // Guitar In -> Output straight through. Guitar In must start muted (feedback protection).
    CHECK (board.connect (BoardModel::kInputId, BoardModel::kOutputId), "patch Guitar In -> Output");
    auto* inTerm = dynamic_cast<InputTerminal*> (board.processorFor (BoardModel::kInputId));
    CHECK (inTerm->muted.load(), "Guitar In starts muted");
    CHECK (runBlocks (board, 0.2f) == 0.f, "muted input is silent even when patched through");
    inTerm->muted = false;
    const float direct = runBlocks (board, 0.2f);
    CHECK (std::fabs (direct - 0.2f) < 0.01f, "straight cable passes the signal unchanged");
    CHECK (! board.connect (BoardModel::kOutputId, BoardModel::kInputId), "cannot patch backwards (Output -> Guitar In)");
    CHECK (! board.connect (BoardModel::kInputId, BoardModel::kOutputId), "no duplicate cables");

    // mono input copies channel 1 to both channels
    {
        auto* in = dynamic_cast<InputTerminal*> (board.processorFor (BoardModel::kInputId));
        in->mono = true;  juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer m; for (int i = 0; i < 512; ++i) { buf.setSample (0, i, 0.3f); buf.setSample (1, i, 0.f); }
        board.graph.processBlock (buf, m);
        CHECK (buf.getSample (1, 100) > 0.29f, "mono input feeds both channels");
    }

    // a pedal in the chain changes the sound
    board.disconnect (BoardModel::kInputId, BoardModel::kOutputId);
    const int fz = board.addModule (ModuleType::Fz3, { 100, 0 });
    CHECK (board.connect (BoardModel::kInputId, fz) && board.connect (fz, BoardModel::kOutputId), "Guitar In -> FZ-3 -> Output");
    const float fuzzed = runBlocks (board, 0.2f, 60);
    CHECK (fuzzed > 0.001f && std::fabs (fuzzed - 0.2f) > 0.005f, "FZ-3 in the chain processes the signal");

    // chain two more, then loops are rejected
    const int bd = board.addModule (ModuleType::Bd2, { 300, 0 });
    const int gs = board.addModule (ModuleType::Gs424, { 500, 0 });
    const int rk = board.addModule (ModuleType::Ktg1);
    board.disconnect (fz, BoardModel::kOutputId);
    CHECK (board.connect (fz, bd) && board.connect (bd, gs) && board.connect (gs, rk) && board.connect (rk, BoardModel::kOutputId), "FZ-3 -> BD-2 -> GS-424 -> KTG-1 -> Output");
    CHECK (! board.connect (rk, fz), "a loop (KTG-1 back into FZ-3) is refused");
    CHECK (runBlocks (board, 0.2f, 80) > 0.001f, "four units in series still pass signal");

    // rewiring: take the BD-2 out of the chain
    board.disconnect (fz, bd); board.disconnect (bd, gs);
    CHECK (board.connect (fz, gs), "re-patch FZ-3 -> GS-424, skipping the BD-2");
    CHECK (runBlocks (board, 0.2f, 80) > 0.001f, "signal flows after rewiring");

    // removing a module removes its cables
    const size_t before = board.cables.size();
    board.removeModule (gs);
    CHECK (board.cables.size() < before && ! board.isConnected (fz, gs), "removing a unit removes its cables");
    { const float broken = runBlocks (board, 0.2f, 20); std::printf ("      broken-chain peak: %g\n", broken); CHECK (broken < 1.0e-6f, "chain broken: silence (only a decaying filter tail remains)"); }

    // the FF-1 works on the board too, and its germanium / silicon switch survives a save and load
    {
        BoardModel b2; b2.graph.setPlayConfigDetails (2, 2, 48000.0, 512); b2.graph.prepareToPlay (48000.0, 512);
        dynamic_cast<InputTerminal*> (b2.processorFor (BoardModel::kInputId))->muted = false;
        const int ff = b2.addModule (ModuleType::Ff1, { 50, 0 });
        CHECK (b2.connect (BoardModel::kInputId, ff) && b2.connect (ff, BoardModel::kOutputId), "Guitar In -> FF-1 -> Output");
        const float fuzzed = runBlocks (b2, 0.1f, 80);
        CHECK (fuzzed > 0.01f && std::fabs (fuzzed - 0.1f) > 0.005f, "FF-1 on the board processes the signal");
        b2.processorFor (ff)->getParameters();   // (touch the parameter tree)
        for (auto* prm : b2.processorFor (ff)->getParameters())
            if (prm->getName (32).startsWith ("Silicon")) prm->setValueNotifyingHost (1.f);
        auto xml2 = b2.toXml();
        BoardModel b3; CHECK (b3.loadXml (*xml2), "load a board that contains an FF-1");
        bool si = false;
        for (auto& m : b3.modules) if (m.type == ModuleType::Ff1)
            for (auto* prm : b3.processorFor (m.id)->getParameters()) if (prm->getName (32).startsWith ("Silicon")) si = prm->getValue() > 0.5f;
        CHECK (si, "the FF-1's silicon switch survives a save/load");
    }

    // save / load round trip keeps modules, cables and settings
    board.buildDefaultBoard();
    auto* out = dynamic_cast<OutputTerminal*> (board.processorFor (BoardModel::kOutputId)); out->volume = 0.5f;
    auto xml = board.toXml();
    BoardModel other;
    CHECK (dynamic_cast<InputTerminal*> (other.processorFor (BoardModel::kInputId))->muted.load(), "a fresh board starts muted");
    CHECK (other.loadXml (*xml), "load board XML");
    CHECK (dynamic_cast<InputTerminal*> (other.processorFor (BoardModel::kInputId))->muted.load(), "loading a saved board does not unmute the input");
    dynamic_cast<InputTerminal*> (other.processorFor (BoardModel::kInputId))->muted = false;
    CHECK (other.modules.size() == board.modules.size() && other.cables.size() == board.cables.size(), "module and cable counts survive a save/load");
    other.graph.setPlayConfigDetails (2, 2, 48000.0, 512); other.graph.prepareToPlay (48000.0, 512);
    CHECK (dynamic_cast<OutputTerminal*> (other.processorFor (BoardModel::kOutputId))->volume.load() == 0.5f, "settings survive a save/load");
    CHECK (runBlocks (other, 0.2f, 80) > 0.001f, "loaded default board passes signal");

    std::printf (fails ? "\n%d FAILED\n" : "\nall passed\n", fails);
    return fails ? 1 : 0;
}
