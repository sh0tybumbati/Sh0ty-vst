#pragma once
#include "BoardModel.h"

class BoardComponent;
class CableLayer;

// One unit on the board: a pedal, a rack unit, or the Guitar In / Output terminal. Hosts the plugin's own editor
// (scaled down), a drag handle, a close button and its input / output jacks.
class ModuleComponent : public juce::Component
{
public:
    ModuleComponent (BoardComponent&, BoardModel&, int moduleId);
    ~ModuleComponent() override;

    int getModuleId() const { return id; }
    ModuleType getType() const { return type; }
    bool hasInputJack() const  { return type != ModuleType::GuitarIn; }
    bool hasOutputJack() const { return type != ModuleType::Output; }
    juce::Point<int> getJackPosition (bool output) const;     // in the board's coordinates

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    juce::Point<int> localJack (bool output) const;
    bool overJack (juce::Point<int> local, bool& output) const;

    BoardComponent& board;
    BoardModel& model;
    int id; ModuleType type;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::TextButton closeButton { "x" };
    juce::Slider knob;                      // terminals only
    juce::ToggleButton monoToggle { "MONO" };

    enum class Drag { None, Move, Cable } drag = Drag::None;
    bool cableFromOutput = false;
    juce::Point<int> dragOffset;
    int headerH = 20, margin = 24;
    juce::Rectangle<int> body;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleComponent)
};

// The pedalboard: rack units on top, pedals below, virtual cables over everything.
class BoardComponent : public juce::Component, private juce::Timer
{
public:
    BoardComponent (BoardModel&, juce::AudioDeviceManager&);
    ~BoardComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // services for the modules
    void beginCable (ModuleComponent&, bool fromOutput);
    void dragCable (juce::Point<int> boardPos);
    void endCable (juce::Point<int> boardPos);
    void moduleMoved (ModuleComponent&);
    void requestRemove (int moduleId);
    juce::Point<int> jackPosition (int moduleId, bool output) const;
    int boardTop() const { return boardTopY; }

    // actions
    void showAddMenu();
    void showAudioSettings();
    void saveBoard();
    void loadBoard();
    void resetBoard();
    void clearBoard();
    bool loadFromXml (const juce::XmlElement&);
    std::unique_ptr<juce::XmlElement> toXml();
    void addModuleOfType (ModuleType);
    bool simulateCableDrag (int srcId, int dstId);   // drag from src's output jack to dst's input jack (used by tests)

    // cables
    void paintCables (juce::Graphics&);
    int  cableAt (juce::Point<int> p) const;
    void deleteCable (int index);

    int toolbarHeight() const { return 44; }

private:
    friend class CableLayer;
    void timerCallback() override;
    void syncModules();
    void destroyModuleComponents();
    void layoutAll();
    ModuleComponent* componentFor (int id) const;
    juce::Path cablePath (juce::Point<int> a, juce::Point<int> b) const;
    void renderBackground();

    BoardModel& model;
    juce::AudioDeviceManager& deviceManager;
    juce::OwnedArray<ModuleComponent> modules;
    std::unique_ptr<CableLayer> cableLayer;
    juce::TextButton addButton { "+ ADD" }, audioButton { "AUDIO" }, saveButton { "SAVE" }, loadButton { "LOAD" }, resetButton { "RESET" };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Image background;
    int boardTopY = 60;
    float meter = 0.f;

    // cable being dragged
    bool dragging = false, dragFromOutput = true; int dragModule = 0; juce::Point<int> dragPos;
    int hoverModule = -1; bool hoverValid = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoardComponent)
};
