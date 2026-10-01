#include "BoardModel.h"
#include "PluginProcessor.h"   // KTG-1
#include "FuzzProcessor.h"
#include "BluesProcessor.h"
#include "GsProcessor.h"
#include "FfProcessor.h"
#include "ReProcessor.h"

using IOProc = juce::AudioProcessorGraph::AudioGraphIOProcessor;

juce::String moduleTypeName (ModuleType t)
{
    switch (t) { case ModuleType::GuitarIn: return "GUITAR IN"; case ModuleType::Output: return "OUTPUT";
                 case ModuleType::Ktg1: return "KTG-1"; case ModuleType::Fz3: return "FZ-3";
                 case ModuleType::Bd2: return "BD-2"; case ModuleType::Gs424: return "GS-424"; case ModuleType::Ff1: return "FF-1"; case ModuleType::Re201: return "RE-201"; }
    return {};
}
juce::String moduleTypeKey (ModuleType t)
{
    switch (t) { case ModuleType::GuitarIn: return "guitarin"; case ModuleType::Output: return "output";
                 case ModuleType::Ktg1: return "ktg1"; case ModuleType::Fz3: return "fz3";
                 case ModuleType::Bd2: return "bd2"; case ModuleType::Gs424: return "gs424"; case ModuleType::Ff1: return "ff1"; case ModuleType::Re201: return "re201"; }
    return {};
}
bool moduleTypeFromKey (const juce::String& k, ModuleType& out)
{
    for (auto t : { ModuleType::GuitarIn, ModuleType::Output, ModuleType::Ktg1, ModuleType::Fz3, ModuleType::Bd2, ModuleType::Gs424, ModuleType::Ff1, ModuleType::Re201 })
        if (moduleTypeKey (t) == k) { out = t; return true; }
    return false;
}

BoardModel::BoardModel()
{
    graph.setPlayConfigDetails (2, 2, 48000.0, 512);
    deviceIn  = graph.addNode (std::make_unique<IOProc> (IOProc::audioInputNode))->nodeID;
    deviceOut = graph.addNode (std::make_unique<IOProc> (IOProc::audioOutputNode))->nodeID;

    for (auto [id, type] : { std::pair<int, ModuleType> { kInputId, ModuleType::GuitarIn }, { kOutputId, ModuleType::Output } })
    {
        ModuleInfo m; m.id = id; m.type = type;
        m.node = graph.addNode (makeProcessor (type))->nodeID;
        modules.push_back (m);
    }
    ensureDeviceConnections();
}

BoardModel::~BoardModel()
{
    graph.clear();
}

std::unique_ptr<juce::AudioProcessor> BoardModel::makeProcessor (ModuleType t) const
{
    switch (t)
    {
        case ModuleType::GuitarIn: return std::make_unique<InputTerminal>();
        case ModuleType::Output:   return std::make_unique<OutputTerminal>();
        case ModuleType::Ktg1:     return std::make_unique<Sh0tyKTG1Processor>();
        case ModuleType::Fz3:      return std::make_unique<Sh0tyFZ3Processor>();
        case ModuleType::Bd2:      return std::make_unique<Sh0tyBD2Processor>();
        case ModuleType::Gs424:    return std::make_unique<Sh0tyGS424Processor>();
        case ModuleType::Ff1:      return std::make_unique<Sh0tyFF1Processor>();
        case ModuleType::Re201:    return std::make_unique<Sh0tyRE201Processor>();
    }
    return nullptr;
}

void BoardModel::ensureDeviceConnections()
{
    const auto in = find (kInputId), out = find (kOutputId);
    if (in == nullptr || out == nullptr) return;
    for (int ch = 0; ch < 2; ++ch)
    {
        graph.addConnection ({ { deviceIn, ch }, { in->node, ch } });      // may fail when the device has fewer channels
        graph.addConnection ({ { out->node, ch }, { deviceOut, ch } });
    }
}

const ModuleInfo* BoardModel::find (int id) const { for (auto& m : modules) if (m.id == id) return &m; return nullptr; }
ModuleInfo* BoardModel::find (int id)             { for (auto& m : modules) if (m.id == id) return &m; return nullptr; }
juce::AudioProcessor* BoardModel::processorFor (int id) const
{
    if (auto* m = find (id))
        if (auto* n = graph.getNodeForId (m->node)) return n->getProcessor();
    return nullptr;
}

int BoardModel::addModule (ModuleType t, juce::Point<int> pos, int forcedId)
{
    if (isTerminalModule (t)) return 0;
    ModuleInfo m; m.id = forcedId > 0 ? forcedId : nextId++; m.type = t; m.pos = pos;
    nextId = juce::jmax (nextId, m.id + 1);
    m.node = graph.addNode (makeProcessor (t))->nodeID;
    modules.push_back (m);
    changed();
    return m.id;
}

void BoardModel::removeModule (int id)
{
    const auto* m = find (id);
    if (m == nullptr || isTerminalModule (m->type)) return;
    for (int i = (int) cables.size(); --i >= 0;)
        if (cables[(size_t) i].src == id || cables[(size_t) i].dst == id) disconnect (cables[(size_t) i].src, cables[(size_t) i].dst);
    graph.removeNode (m->node);
    modules.erase (std::remove_if (modules.begin(), modules.end(), [id] (const ModuleInfo& x) { return x.id == id; }), modules.end());
    changed();
}

bool BoardModel::isConnected (int s, int d) const
{
    for (auto& c : cables) if (c.src == s && c.dst == d) return true;
    return false;
}

bool BoardModel::canConnect (int s, int d) const
{
    const auto* a = find (s); const auto* b = find (d);
    if (a == nullptr || b == nullptr || s == d || isConnected (s, d)) return false;
    if (a->type == ModuleType::Output || b->type == ModuleType::GuitarIn) return false;
    // refuse a cable that would close a loop: is `s` already downstream of `d`?
    std::vector<int> stack { d }, seen;
    while (! stack.empty())
    {
        const int cur = stack.back(); stack.pop_back();
        if (cur == s) return false;
        if (std::find (seen.begin(), seen.end(), cur) != seen.end()) continue;
        seen.push_back (cur);
        for (auto& c : cables) if (c.src == cur) stack.push_back (c.dst);
    }
    return graph.canConnect ({ { a->node, 0 }, { b->node, 0 } });
}

bool BoardModel::connect (int s, int d)
{
    if (! canConnect (s, d)) return false;
    const auto* a = find (s); const auto* b = find (d);
    for (int ch = 0; ch < 2; ++ch) graph.addConnection ({ { a->node, ch }, { b->node, ch } });
    cables.push_back ({ s, d });
    changed();
    return true;
}

void BoardModel::disconnect (int s, int d)
{
    const auto* a = find (s); const auto* b = find (d);
    if (a == nullptr || b == nullptr) return;
    for (int ch = 0; ch < 2; ++ch) graph.removeConnection ({ { a->node, ch }, { b->node, ch } });
    cables.erase (std::remove_if (cables.begin(), cables.end(), [s, d] (const Cable& c) { return c.src == s && c.dst == d; }), cables.end());
    changed();
}

void BoardModel::clearBoard()
{
    auto saved = std::move (onStructureChanged); onStructureChanged = nullptr;
    for (int i = (int) modules.size(); --i >= 0;)
        if (! isTerminalModule (modules[(size_t) i].type)) removeModule (modules[(size_t) i].id);
    for (int i = (int) cables.size(); --i >= 0;) disconnect (cables[(size_t) i].src, cables[(size_t) i].dst);
    onStructureChanged = std::move (saved);
    changed();
}

void BoardModel::buildDefaultBoard()
{
    clearBoard();
    auto saved = std::move (onStructureChanged); onStructureChanged = nullptr;
    find (kInputId)->pos  = { 8, 40 };
    find (kOutputId)->pos = { 958, 40 };
    const int rack = addModule (ModuleType::Ktg1);
    const int gs   = addModule (ModuleType::Gs424, { 190, 0 });
    const int bd   = addModule (ModuleType::Bd2,   { 446, 0 });
    const int fz   = addModule (ModuleType::Fz3,   { 702, 0 });
    connect (kInputId, gs); connect (gs, bd); connect (bd, fz); connect (fz, rack); connect (rack, kOutputId);
    onStructureChanged = std::move (saved);
    changed();
}

std::unique_ptr<juce::XmlElement> BoardModel::toXml() const
{
    auto root = std::make_unique<juce::XmlElement> ("Sh0tyBoard");
    root->setAttribute ("version", 1);
    for (auto& m : modules)
    {
        auto* e = root->createNewChildElement ("Module");
        e->setAttribute ("id", m.id); e->setAttribute ("type", moduleTypeKey (m.type));
        e->setAttribute ("x", m.pos.x); e->setAttribute ("y", m.pos.y);
        if (auto* p = processorFor (m.id))
        {
            if (auto* in = dynamic_cast<InputTerminal*> (p))  { e->setAttribute ("mono", in->mono.load() ? 1 : 0); e->setAttribute ("gain", (double) in->gain.load()); }
            else if (auto* out = dynamic_cast<OutputTerminal*> (p)) e->setAttribute ("volume", (double) out->volume.load());
            else { juce::MemoryBlock mb; p->getStateInformation (mb); e->setAttribute ("state", mb.toBase64Encoding()); }
        }
    }
    for (auto& c : cables) { auto* e = root->createNewChildElement ("Cable"); e->setAttribute ("src", c.src); e->setAttribute ("dst", c.dst); }
    return root;
}

bool BoardModel::loadXml (const juce::XmlElement& root)
{
    if (! root.hasTagName ("Sh0tyBoard")) return false;
    auto saved = std::move (onStructureChanged); onStructureChanged = nullptr;
    clearBoard();
    for (auto* e : root.getChildWithTagNameIterator ("Module"))
    {
        ModuleType t;
        if (! moduleTypeFromKey (e->getStringAttribute ("type"), t)) continue;
        const int id = e->getIntAttribute ("id");
        const juce::Point<int> pos (e->getIntAttribute ("x"), e->getIntAttribute ("y"));
        int use = id;
        if (isTerminalModule (t)) { if (auto* m = find (id)) m->pos = pos; }
        else use = addModule (t, pos, id);
        if (auto* p = processorFor (use))
        {
            if (auto* in = dynamic_cast<InputTerminal*> (p))  { in->mono = e->getIntAttribute ("mono", 1) != 0; in->gain = (float) e->getDoubleAttribute ("gain", 1.0); }
            else if (auto* out = dynamic_cast<OutputTerminal*> (p)) out->volume = (float) e->getDoubleAttribute ("volume", 1.0);
            else if (e->hasAttribute ("state")) { juce::MemoryBlock mb; mb.fromBase64Encoding (e->getStringAttribute ("state")); p->setStateInformation (mb.getData(), (int) mb.getSize()); }
        }
    }
    for (auto* e : root.getChildWithTagNameIterator ("Cable")) connect (e->getIntAttribute ("src"), e->getIntAttribute ("dst"));
    onStructureChanged = std::move (saved);
    changed();
    return true;
}
