#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <functional>
#include "Terminals.h"

enum class ModuleType { GuitarIn, Output, Ktg1, Fz3, Bd2, Gs424 };

inline bool isRackModule (ModuleType t)     { return t == ModuleType::Ktg1; }
inline bool isTerminalModule (ModuleType t) { return t == ModuleType::GuitarIn || t == ModuleType::Output; }
juce::String moduleTypeName (ModuleType);
juce::String moduleTypeKey (ModuleType);
bool         moduleTypeFromKey (const juce::String&, ModuleType&);

struct ModuleInfo
{
    int id = 0;
    ModuleType type = ModuleType::Fz3;
    juce::AudioProcessorGraph::NodeID node {};
    juce::Point<int> pos;          // pedals / terminals: top-left on the board. Rack modules ignore this.
};
struct Cable { int src = 0, dst = 0; };

// The audio side of the pedalboard: a processor graph plus the list of modules and cables. No GUI in here.
class BoardModel
{
public:
    static constexpr int kInputId = 1, kOutputId = 2;

    BoardModel();
    ~BoardModel();

    int  addModule (ModuleType, juce::Point<int> pos = {}, int forcedId = 0);
    void removeModule (int id);
    bool connect (int srcId, int dstId);             // every cable carries a stereo pair
    void disconnect (int srcId, int dstId);
    bool isConnected (int srcId, int dstId) const;
    bool canConnect (int srcId, int dstId) const;

    void clearBoard();                               // removes pedals, rack units and cables; keeps the terminals
    void buildDefaultBoard();
    void ensureDeviceConnections();                  // call after the audio device changes

    const ModuleInfo* find (int id) const;
    ModuleInfo* find (int id);
    juce::AudioProcessor* processorFor (int id) const;

    std::unique_ptr<juce::XmlElement> toXml() const;
    bool loadXml (const juce::XmlElement&);

    juce::AudioProcessorGraph graph;
    std::vector<ModuleInfo> modules;
    std::vector<Cable> cables;
    std::function<void()> onStructureChanged;        // called after modules / cables change

private:
    std::unique_ptr<juce::AudioProcessor> makeProcessor (ModuleType) const;
    void changed() { if (onStructureChanged) onStructureChanged(); }

    juce::AudioProcessorGraph::NodeID deviceIn {}, deviceOut {};
    int nextId = 10;
};
