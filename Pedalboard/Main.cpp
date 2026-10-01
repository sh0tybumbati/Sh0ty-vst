#include "BoardComponent.h"

// Sh0ty Pedalboard: a standalone app that hosts the KTG-1, FZ-3, BD-2 and GS-424 in a patchable graph.
class PedalboardApplication : public juce::JUCEApplication, private juce::ChangeListener
{
public:
    const juce::String getApplicationName() override    { return "Sh0ty Pedalboard"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise (const juce::String&) override
    {
        juce::LookAndFeel::getDefaultLookAndFeel();
        if (auto* lf = dynamic_cast<juce::LookAndFeel_V4*> (&juce::LookAndFeel::getDefaultLookAndFeel()))
            lf->setColourScheme (juce::LookAndFeel_V4::getMidnightColourScheme());

        std::unique_ptr<juce::XmlElement> audioState;
        if (audioFile().existsAsFile()) audioState = juce::XmlDocument::parse (audioFile());
        deviceManager.initialise (2, 2, audioState.get(), true);
        {   // always start with no input device open: an open mic next to the speakers is an instant feedback loop
            auto setup = deviceManager.getAudioDeviceSetup();
            setup.inputDeviceName = {}; setup.inputChannels.clear(); setup.useDefaultInputChannels = false;
            deviceManager.setAudioDeviceSetup (setup, true);
        }
        deviceManager.addChangeListener (this);

        // build the board (default, or the one from last time) while the graph is not yet running
        bool loaded = false;
        if (lastBoardFile().existsAsFile())
            if (auto xml = juce::XmlDocument::parse (lastBoardFile())) loaded = board.loadXml (*xml);
        if (! loaded) board.buildDefaultBoard();

        window = std::make_unique<MainWindow> (getApplicationName(), board, deviceManager);
        player.setProcessor (&board.graph);
        deviceManager.addAudioCallback (&player);
        board.ensureDeviceConnections();
    }

    void shutdown() override
    {
        saveState();
        deviceManager.removeAudioCallback (&player);
        deviceManager.removeChangeListener (this);
        player.setProcessor (nullptr);
        window.reset();             // the editors must go before the processors
    }

    void systemRequestedQuit() override { quit(); }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (const juce::String& name, BoardModel& m, juce::AudioDeviceManager& dm)
            : DocumentWindow (name, juce::Colour (0xff0e0f12), DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new BoardComponent (m, dm), false);
            setResizable (true, true);
            setResizeLimits (900, 560, 3000, 2000);
            centreWithSize (1140, 780);
            setVisible (true);
        }
        void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
        BoardComponent* board() { return dynamic_cast<BoardComponent*> (getContentComponent()); }
    };

    static juce::File dataDir()
    {
        auto d = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Sh0ty");
        d.createDirectory(); return d;
    }
    static juce::File lastBoardFile() { return dataDir().getChildFile ("pedalboard_last.sh0tyboard"); }
    static juce::File audioFile()     { return dataDir().getChildFile ("pedalboard_audio.xml"); }

    void saveState()
    {
        if (auto xml = board.toXml()) xml->writeTo (lastBoardFile());
        if (auto xml = deviceManager.createStateXml()) xml->writeTo (audioFile());
    }

    void changeListenerCallback (juce::ChangeBroadcaster*) override { board.ensureDeviceConnections(); }

    BoardModel board;
    juce::AudioDeviceManager deviceManager;
    juce::AudioProcessorPlayer player;
    std::unique_ptr<MainWindow> window;
};

START_JUCE_APPLICATION (PedalboardApplication)
