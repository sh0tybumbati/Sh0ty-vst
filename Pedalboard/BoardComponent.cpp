#include "BoardComponent.h"

namespace
{
const juce::Colour feltDark   { 0xff141518 };
const juce::Colour feltLight  { 0xff1d1f24 };
const juce::Colour accent     { 0xffffb347 };
const juce::Colour plate      { 0xff111215 };
const juce::Colour plateLine  { 0xff3b3e46 };
const juce::Colour textDim    { 0xffb9bcc6 };

constexpr float kPedalScale = 0.62f, kRackScale = 0.93f;

juce::Colour cableColour (int srcId)
{
    static const juce::Colour cols[] = { juce::Colour (0xffff3d7f), juce::Colour (0xff19c9e6), juce::Colour (0xffffd23f),
                                         juce::Colour (0xff6bff8f), juce::Colour (0xffff8a3d), juce::Colour (0xffb18cff) };
    return cols[(size_t) std::abs (srcId) % 6];
}

void drawJack (juce::Graphics& g, juce::Point<int> c, const juce::String& label, bool lit, juce::Colour ring)
{
    const auto cf = c.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillEllipse (cf.x - 11.f, cf.y - 9.f, 22.f, 22.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd9dbe0), cf.x - 10.f, cf.y - 10.f, juce::Colour (0xff6c6f77), cf.x + 10.f, cf.y + 10.f, false));
    g.fillEllipse (cf.x - 10.f, cf.y - 10.f, 20.f, 20.f);
    g.setColour (lit ? ring : juce::Colour (0xff22242a)); g.drawEllipse (cf.x - 10.f, cf.y - 10.f, 20.f, 20.f, lit ? 3.f : 1.5f);
    g.setColour (juce::Colour (0xff0a0a0c)); g.fillEllipse (cf.x - 5.f, cf.y - 5.f, 10.f, 10.f);
    g.setColour (textDim); g.setFont (juce::FontOptions (9.f, juce::Font::bold));
    g.drawText (label, juce::Rectangle<float> (cf.x - 20.f, cf.y + 12.f, 40.f, 11.f), juce::Justification::centred);
}
} // namespace

//==============================================================================
// ModuleComponent
//==============================================================================
ModuleComponent::ModuleComponent (BoardComponent& b, BoardModel& m, int moduleId) : board (b), model (m), id (moduleId)
{
    const auto* info = model.find (moduleId);
    type = info->type;
    setRepaintsOnMouseActivity (false);

    if (isTerminalModule (type))
    {
        margin = 24;
        body = { margin, headerH, 124, 194 };
        knob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
        addAndMakeVisible (knob);
        if (auto* in = dynamic_cast<InputTerminal*> (model.processorFor (id)))
        {
            knob.setRange (-24.0, 24.0, 0.1); knob.setTextValueSuffix (" dB");
            knob.setValue (juce::Decibels::gainToDecibels (in->gain.load()), juce::dontSendNotification);
            knob.onValueChange = [this, in] { in->gain = juce::Decibels::decibelsToGain ((float) knob.getValue()); };
            monoToggle.setToggleState (in->mono.load(), juce::dontSendNotification);
            monoToggle.onClick = [this, in] { in->mono = monoToggle.getToggleState(); };
            addAndMakeVisible (monoToggle);

            // which input device feeds the board, and the mute switch (muted at every start)
            sourceBox.setTextWhenNothingSelected ("None");
            sourceBox.onChange = [this]
            {
                const int sel = sourceBox.getSelectedId();
                board.setInputSource (sel <= 1 ? juce::String() : sourceBox.getText());
            };
            addAndMakeVisible (sourceBox);
            muteButton.setClickingTogglesState (true);
            muteButton.setToggleState (in->muted.load(), juce::dontSendNotification);
            auto styleMute = [this] (bool muted)
            {
                muteButton.setButtonText (muted ? "MUTED" : "LIVE");
                muteButton.setColour (juce::TextButton::buttonColourId, muted ? juce::Colour (0xffb3262d) : juce::Colour (0xff1f9d55));
                muteButton.setColour (juce::TextButton::buttonOnColourId, muted ? juce::Colour (0xffb3262d) : juce::Colour (0xff1f9d55));
                muteButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
                muteButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
            };
            styleMute (in->muted.load());
            muteButton.onClick = [this, in, styleMute] { in->muted = muteButton.getToggleState(); styleMute (in->muted.load()); };
            addAndMakeVisible (muteButton);
            refreshSource();
        }
        else if (auto* out = dynamic_cast<OutputTerminal*> (model.processorFor (id)))
        {
            knob.setRange (0.0, 1.5, 0.01);
            knob.setValue (out->volume.load(), juce::dontSendNotification);
            knob.onValueChange = [out, this] { out->volume = (float) knob.getValue(); };
        }
    }
    else
    {
        if (auto* p = model.processorFor (id)) editor.reset (p->createEditorIfNeeded());
        const float s = isRackModule (type) ? kRackScale : kPedalScale;
        margin = isRackModule (type) ? 32 : 24;
        if (editor != nullptr)
        {
            body = { margin, headerH, juce::roundToInt ((float) editor->getWidth() * s), juce::roundToInt ((float) editor->getHeight() * s) };
            addAndMakeVisible (*editor);
            editor->setTransform (juce::AffineTransform::scale (s));
        }
        addAndMakeVisible (closeButton);
        closeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a2c33));
        closeButton.onClick = [this] { board.requestRemove (id); };
    }
    setSize (body.getRight() + margin, body.getBottom() + 6);
    resized();
}

ModuleComponent::~ModuleComponent() { editor.reset(); }

void ModuleComponent::refreshSource()
{
    if (type != ModuleType::GuitarIn) return;
    sourceBox.clear (juce::dontSendNotification);
    sourceBox.addItem ("None (no input)", 1);
    const auto names = board.inputSources();
    for (int i = 0; i < names.size(); ++i) sourceBox.addItem (names[i], i + 2);
    const auto cur = board.currentInputSource();
    const int idx = cur.isEmpty() ? -1 : names.indexOf (cur);
    sourceBox.setSelectedId (idx >= 0 ? idx + 2 : 1, juce::dontSendNotification);
}

juce::Point<int> ModuleComponent::localJack (bool output) const
{
    const int y = headerH + body.getHeight() / 2;
    return { output ? getWidth() - margin / 2 : margin / 2, y };
}
juce::Point<int> ModuleComponent::getJackPosition (bool output) const { return getPosition() + localJack (output); }

bool ModuleComponent::overJack (juce::Point<int> p, bool& output) const
{
    if (hasOutputJack() && p.getDistanceFrom (localJack (true)) <= 15)  { output = true;  return true; }
    if (hasInputJack()  && p.getDistanceFrom (localJack (false)) <= 15) { output = false; return true; }
    return false;
}

void ModuleComponent::resized()
{
    if (editor != nullptr)     // a transform scales the position too, so aim for body.x / scale
    {
        const float sc = isRackModule (type) ? kRackScale : kPedalScale;
        editor->setTopLeftPosition (juce::roundToInt ((float) body.getX() / sc), juce::roundToInt ((float) body.getY() / sc));
    }
    closeButton.setBounds (getWidth() - margin - 20, 2, 18, 16);
    if (type == ModuleType::GuitarIn)
    {
        sourceBox.setBounds (body.getX() + 4, body.getY() + 14, 116, 24);
        knob.setBounds (body.getX() + 20, body.getY() + 54, 84, 84);
        monoToggle.setBounds (body.getX() + 28, body.getY() + 140, 70, 20);
        muteButton.setBounds (body.getX() + 8, body.getBottom() - 30, 108, 26);
    }
    else if (type == ModuleType::Output)
        knob.setBounds (body.getX() + 20, body.getY() + 40, 84, 84);
}

void ModuleComponent::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    if (isRackModule (type))
    {
        // rack ears with screw holes
        for (bool left : { true, false })
        {
            const auto ear = juce::Rectangle<float> (left ? 0.f : r.getRight() - (float) margin, 0.f, (float) margin, r.getHeight() - 4.f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8a8d94), ear.getX(), 0.f, juce::Colour (0xff4a4d54), ear.getRight(), 0.f, false));
            g.fillRoundedRectangle (ear, 3.f);
            for (int i = 0; i < 3; ++i)
            {
                const float cy = ear.getY() + 40.f + (float) i * (ear.getHeight() - 80.f) * 0.5f;
                g.setColour (juce::Colour (0xff15161a)); g.fillEllipse (ear.getCentreX() - 4.f, cy - 4.f, 8.f, 8.f);
            }
        }
        g.setColour (plate); g.fillRect ((float) margin, 0.f, r.getWidth() - 2.f * (float) margin, r.getHeight() - 4.f);
    }
    else
    {
        g.setColour (plate); g.fillRoundedRectangle (r.reduced (2.f).withTrimmedBottom (2.f), 8.f);
        g.setColour (plateLine); g.drawRoundedRectangle (r.reduced (2.f).withTrimmedBottom (2.f), 8.f, 1.2f);
    }
    // header / drag handle
    const auto hdr = juce::Rectangle<float> ((float) margin, 0.f, r.getWidth() - 2.f * (float) margin, (float) headerH);
    g.setColour (juce::Colour (0xff23252b)); g.fillRect (hdr);
    g.setColour (textDim); g.setFont (juce::FontOptions (11.f, juce::Font::bold));
    g.drawText (moduleTypeName (type), hdr.withTrimmedLeft (8.f), juce::Justification::centredLeft);
    for (int i = 0; i < 3; ++i) { g.setColour (juce::Colour (0xff4d505a)); g.fillRect (hdr.getCentreX() - 14.f + (float) i * 10.f, 8.f, 6.f, 4.f); }   // grip

    if (isTerminalModule (type))
    {
        g.setColour (textDim); g.setFont (juce::FontOptions (9.f, juce::Font::bold));
        if (type == ModuleType::GuitarIn)
        {
            g.drawText ("SOURCE", juce::Rectangle<float> ((float) body.getX(), (float) body.getY() + 1.f, 124.f, 11.f), juce::Justification::centred);
            g.drawText ("GAIN", juce::Rectangle<float> ((float) body.getX(), (float) body.getY() + 42.f, 124.f, 11.f), juce::Justification::centred);
        }
        else
            g.drawText ("VOLUME", juce::Rectangle<float> ((float) body.getX(), (float) body.getY() + 26.f, 124.f, 11.f), juce::Justification::centred);
    }
    if (isTerminalModule (type) && type == ModuleType::Output)
    {
        // level meter next to the knob
        auto* out = dynamic_cast<OutputTerminal*> (model.processorFor (id));
        const float lvl = out != nullptr ? juce::jlimit (0.f, 1.f, (juce::Decibels::gainToDecibels (out->peak.load(), -60.f) + 60.f) / 63.f) : 0.f;
        const auto m = juce::Rectangle<float> ((float) body.getRight() - 14.f, (float) body.getY() + 40.f, 8.f, 120.f);
        g.setColour (juce::Colour (0xff0b0b0e)); g.fillRect (m);
        g.setColour (lvl > 0.93f ? juce::Colour (0xffff3d7f) : juce::Colour (0xff4dff88)); g.fillRect (m.withTop (m.getBottom() - m.getHeight() * lvl));
    }

    const bool outLit = model.cables.end() != std::find_if (model.cables.begin(), model.cables.end(), [this] (const Cable& c) { return c.src == id; });
    const bool inLit  = model.cables.end() != std::find_if (model.cables.begin(), model.cables.end(), [this] (const Cable& c) { return c.dst == id; });
    if (hasInputJack())  drawJack (g, localJack (false), "IN",  inLit,  accent);
    if (hasOutputJack()) drawJack (g, localJack (true),  "OUT", outLit, accent);
}

void ModuleComponent::mouseDown (const juce::MouseEvent& e)
{
    bool out = false;
    if (overJack (e.getPosition(), out)) { drag = Drag::Cable; cableFromOutput = out; board.beginCable (*this, out); return; }
    if (e.y < headerH && e.x >= margin && e.x < getWidth() - margin) { drag = Drag::Move; dragOffset = e.getPosition(); board.bringToFront (*this); }
}

void ModuleComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (drag == Drag::Cable) { board.dragCable (e.getEventRelativeTo (&board).getPosition()); return; }
    if (drag != Drag::Move) return;
    auto p = e.getEventRelativeTo (getParentComponent()).getPosition() - dragOffset;
    if (isRackModule (type)) p.x = getX();                                      // rack units only slide up and down
    p.x = juce::jlimit (0, juce::jmax (0, getParentWidth() - getWidth()), p.x);
    p.y = juce::jlimit (isRackModule (type) ? board.toolbarHeight() + 4 : board.boardTop(), juce::jmax (board.boardTop(), getParentHeight() - getHeight()), p.y);
    setTopLeftPosition (p);
    board.moduleMoved (*this);
}

void ModuleComponent::mouseUp (const juce::MouseEvent& e)
{
    if (drag == Drag::Cable) board.endCable (e.getEventRelativeTo (&board).getPosition());
    drag = Drag::None;
}

void ModuleComponent::mouseMove (const juce::MouseEvent& e)
{
    bool out = false;
    setMouseCursor (overJack (e.getPosition(), out) ? juce::MouseCursor::CrosshairCursor
                    : (e.y < headerH && e.x >= margin && e.x < getWidth() - margin) ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
}

//==============================================================================
// Cable layer: draws every cable on top of the modules and only catches the mouse near a cable.
//==============================================================================
class CableLayer : public juce::Component
{
public:
    CableLayer (BoardComponent& b, bool plugsOnly) : board (b), plugs (plugsOnly) { setInterceptsMouseClicks (! plugsOnly, false); }
    void paint (juce::Graphics& g) override { if (plugs) board.paintPlugs (g); else board.paintCables (g); }
    bool hitTest (int x, int y) override { return ! plugs && board.cableAt ({ x, y }) >= 0; }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! e.mods.isPopupMenu()) return;
        const int i = board.cableAt (e.getPosition());
        if (i < 0) return;
        juce::PopupMenu m; m.addItem (1, "Delete cable");
        m.showMenuAsync ({}, [this, i] (int r) { if (r == 1) board.deleteCable (i); });
    }
    void mouseDoubleClick (const juce::MouseEvent& e) override { const int i = board.cableAt (e.getPosition()); if (i >= 0) board.deleteCable (i); }
private:
    BoardComponent& board;
    bool plugs;
};

//==============================================================================
// BoardComponent
//==============================================================================
BoardComponent::BoardComponent (BoardModel& m, juce::AudioDeviceManager& dm) : model (m), deviceManager (dm)
{
    cableLayer = std::make_unique<CableLayer> (*this, false);
    plugLayer  = std::make_unique<CableLayer> (*this, true);
    addAndMakeVisible (*cableLayer);
    addAndMakeVisible (*plugLayer);
    for (auto* b : { &addButton, &audioButton, &saveButton, &loadButton, &resetButton })
    {
        b->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a2c33));
        b->setColour (juce::TextButton::textColourOffId, juce::Colours::white);
        addAndMakeVisible (*b);
    }
    addButton.setColour (juce::TextButton::buttonColourId, accent); addButton.setColour (juce::TextButton::textColourOffId, juce::Colours::black);
    addButton.onClick = [this] { showAddMenu(); };
    audioButton.onClick = [this] { showAudioSettings(); };
    saveButton.onClick = [this] { saveBoard(); };
    loadButton.onClick = [this] { loadBoard(); };
    resetButton.onClick = [this] { resetBoard(); };
    addButton.setTooltip ("Add a pedal or rack unit"); audioButton.setTooltip ("Choose the audio input and output");
    resetButton.setTooltip ("Back to the default board");

    model.onStructureChanged = [this] { syncModules(); };
    deviceManager.addChangeListener (this);
    syncModules();
    startTimerHz (30);
}

BoardComponent::~BoardComponent()
{
    deviceManager.removeChangeListener (this);
    model.onStructureChanged = nullptr;
    stopTimer();
    destroyModuleComponents();
}

void BoardComponent::destroyModuleComponents() { modules.clear(); }

ModuleComponent* BoardComponent::componentFor (int id) const
{
    for (auto* m : modules) if (m->getModuleId() == id) return m;
    return nullptr;
}

void BoardComponent::syncModules()
{
    for (auto& info : model.modules)
        if (componentFor (info.id) == nullptr)
        {
            auto* c = modules.add (new ModuleComponent (*this, model, info.id));
            addAndMakeVisible (c);
        }
    layoutAll();
    updateZOrder();
    repaint();
}

void BoardComponent::updateZOrder()
{
    // rack units at the bottom, then the cable bodies, then the pedals / terminals, then the plug heads on top
    std::vector<juce::Component*> racks, others;
    for (int i = 0; i < getNumChildComponents(); ++i)
        if (auto* m = dynamic_cast<ModuleComponent*> (getChildComponent (i))) (isRackModule (m->getType()) ? racks : others).push_back (m);
    for (auto* c : racks)  c->toFront (false);
    cableLayer->toFront (false);
    for (auto* c : others) c->toFront (false);       // keeps their current relative order, so a dragged pedal stays on top
    plugLayer->toFront (false);
}

void BoardComponent::bringToFront (ModuleComponent& m)
{
    m.toFront (false);
    updateZOrder();
}

void BoardComponent::repaintCables() { cableLayer->repaint(); plugLayer->repaint(); }

void BoardComponent::layoutAll()
{
    const int W = getWidth();
    int y = toolbarHeight() + 8;
    bool anyRack = false;
    for (auto& info : model.modules)
        if (isRackModule (info.type))
            if (auto* c = componentFor (info.id)) { c->setTopLeftPosition ((W - c->getWidth()) / 2, y); y += c->getHeight() + 4; anyRack = true; }
    boardTopY = y + (anyRack ? 10 : 0);
    for (auto& info : model.modules)
        if (! isRackModule (info.type))
            if (auto* c = componentFor (info.id))
            {
                const int x = juce::jlimit (0, juce::jmax (0, W - c->getWidth()), info.pos.x);
                const int yy = boardTopY + juce::jmax (0, info.pos.y);
                c->setTopLeftPosition (x, yy);
            }
    cableLayer->setBounds (getLocalBounds());
    plugLayer->setBounds (getLocalBounds());
    renderBackground();
}

void BoardComponent::resized()
{
    auto r = getLocalBounds().removeFromTop (toolbarHeight()).reduced (8, 7);
    addButton.setBounds (r.removeFromLeft (84)); r.removeFromLeft (8);
    audioButton.setBounds (r.removeFromLeft (74)); r.removeFromLeft (8);
    saveButton.setBounds (r.removeFromLeft (62)); r.removeFromLeft (6);
    loadButton.setBounds (r.removeFromLeft (62)); r.removeFromLeft (6);
    resetButton.setBounds (r.removeFromLeft (66));
    layoutAll();
}

void BoardComponent::timerCallback()
{
    if (auto* out = dynamic_cast<OutputTerminal*> (model.processorFor (BoardModel::kOutputId)))
    {
        const float lvl = juce::jlimit (0.f, 1.f, (juce::Decibels::gainToDecibels (out->peak.load(), -60.f) + 60.f) / 63.f);
        meter += (lvl - meter) * (lvl > meter ? 0.6f : 0.15f);
    }
    if (auto* c = componentFor (BoardModel::kOutputId)) c->repaint();
    repaint (getWidth() - 220, 0, 220, toolbarHeight());
}

void BoardComponent::renderBackground()
{
    const int W = juce::jmax (1, getWidth()), H = juce::jmax (1, getHeight());
    background = juce::Image (juce::Image::RGB, W, H, true);
    juce::Graphics g (background);
    g.fillAll (feltDark);
    juce::Random rng (7);
    for (int i = 0; i < 9000; ++i)
    {
        g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.015f + 0.03f * rng.nextFloat()));
        g.fillRect (rng.nextFloat() * (float) W, rng.nextFloat() * (float) H, 1.4f, 1.4f);
    }
    // rack rails behind the rack units
    if (boardTopY > toolbarHeight() + 20)
    {
        const int top = toolbarHeight() + 4, bottom = boardTopY - 6;
        g.setColour (juce::Colour (0xff0d0e11)); g.fillRect (0, top, W, bottom - top);
        for (int rail : { 8, W - 24 })
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6d7078), (float) rail, 0.f, juce::Colour (0xff3a3d44), (float) rail + 16.f, 0.f, false));
            g.fillRect (rail, top, 16, bottom - top);
            for (int y = top + 14; y < bottom - 8; y += 24) { g.setColour (juce::Colour (0xff0d0e11)); g.fillEllipse ((float) rail + 4.f, (float) y, 8.f, 8.f); }
        }
        g.setColour (accent.withAlpha (0.5f)); g.setFont (juce::FontOptions (10.f, juce::Font::bold));
        g.drawText ("RACK", 28, top + 2, 60, 12, juce::Justification::centredLeft);
    }
    // pedalboard with hook-and-loop strips
    const auto brd = juce::Rectangle<float> (6.f, (float) boardTopY - 2.f, (float) W - 12.f, (float) H - (float) boardTopY - 4.f);
    g.setGradientFill (juce::ColourGradient (feltLight, 0.f, brd.getY(), feltDark.brighter (0.05f), 0.f, brd.getBottom(), false));
    g.fillRoundedRectangle (brd, 10.f);
    g.setColour (juce::Colour (0xff2c2f36)); g.drawRoundedRectangle (brd, 10.f, 1.5f);
    for (float y = brd.getY() + 70.f; y < brd.getBottom() - 10.f; y += 84.f)
    {
        g.setColour (juce::Colour (0xff101114)); g.fillRect (brd.getX() + 12.f, y, brd.getWidth() - 24.f, 6.f);
        g.setColour (juce::Colours::white.withAlpha (0.04f)); g.fillRect (brd.getX() + 12.f, y, brd.getWidth() - 24.f, 1.f);
    }
    g.setColour (accent.withAlpha (0.55f)); g.setFont (juce::FontOptions (10.f, juce::Font::bold));
    g.drawText (juce::String::fromUTF8 ("PEDALBOARD  \xC2\xB7  drag a header to move  \xC2\xB7  drag from a jack to patch  \xC2\xB7  double-click a cable to remove it"),
                juce::Rectangle<int> (20, H - 18, W - 40, 12), juce::Justification::centredLeft);
}

void BoardComponent::paint (juce::Graphics& g)
{
    g.drawImageAt (background, 0, 0);
    // toolbar
    g.setColour (juce::Colour (0xff0e0f12)); g.fillRect (0, 0, getWidth(), toolbarHeight());
    g.setColour (accent); g.fillRect (0, toolbarHeight() - 2, getWidth(), 2);
    g.setColour (juce::Colours::white); g.setFont (juce::FontOptions (15.f, juce::Font::bold));
    g.drawText ("SH0TY PEDALBOARD", getWidth() - 360, 0, 200, toolbarHeight(), juce::Justification::centredRight);
    // output meter
    const auto m = juce::Rectangle<float> ((float) getWidth() - 140.f, 16.f, 120.f, 12.f);
    g.setColour (juce::Colour (0xff1a1b20)); g.fillRoundedRectangle (m, 3.f);
    g.setColour (meter > 0.93f ? juce::Colour (0xffff3d7f) : juce::Colour (0xff4dff88)); g.fillRoundedRectangle (m.withWidth (m.getWidth() * meter), 3.f);
    g.setColour (textDim); g.setFont (juce::FontOptions (9.f, juce::Font::bold)); g.drawText ("OUT", m.getX(), m.getBottom() + 1.f, 120.f, 10.f, juce::Justification::centredLeft);
}

//------------------------------------------------------------------------------ cables
juce::Point<int> BoardComponent::jackPosition (int id, bool output) const
{
    if (auto* c = componentFor (id)) return c->getJackPosition (output);
    return {};
}

juce::Path BoardComponent::cablePath (juce::Point<int> a, juce::Point<int> b) const
{
    const auto p1 = a.toFloat(), p2 = b.toFloat();
    const float dx = juce::jmax (60.f, std::abs (p2.x - p1.x) * 0.45f), sag = 36.f + std::abs (p2.y - p1.y) * 0.25f;
    juce::Path p;
    p.startNewSubPath (p1);
    p.cubicTo (p1.x + dx, p1.y + sag, p2.x - dx, p2.y + sag, p2.x, p2.y);
    return p;
}

namespace
{
void strokeCable (juce::Graphics& g, const juce::Path& p, juce::Colour c)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.strokePath (p, juce::PathStrokeType (7.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), juce::AffineTransform::translation (0.f, 3.f));
    g.setColour (juce::Colour (0xff0b0b0d)); g.strokePath (p, juce::PathStrokeType (6.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c); g.strokePath (p, juce::PathStrokeType (3.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (juce::Colours::white.withAlpha (0.35f)); g.strokePath (p, juce::PathStrokeType (1.1f), juce::AffineTransform::translation (0.f, -1.f));
}

// the part of a path between two arc lengths, as a polyline
juce::Path subPath (const juce::Path& p, float from, float to)
{
    juce::Path out;
    float at = 0.f; bool started = false;
    juce::PathFlatteningIterator it (p, juce::AffineTransform(), 0.5f);
    while (it.next())
    {
        const float len = juce::Line<float> (it.x1, it.y1, it.x2, it.y2).getLength();
        if (len <= 0.f) continue;
        const float s0 = at, s1 = at + len; at = s1;
        if (s1 < from || s0 > to) continue;
        const float t0 = juce::jmax (0.f, (from - s0) / len), t1 = juce::jmin (1.f, (to - s0) / len);
        const juce::Point<float> a (it.x1 + (it.x2 - it.x1) * t0, it.y1 + (it.y2 - it.y1) * t0), b (it.x1 + (it.x2 - it.x1) * t1, it.y1 + (it.y2 - it.y1) * t1);
        if (! started) { out.startNewSubPath (a); started = true; }
        out.lineTo (b);
    }
    return out;
}
float pathLength (const juce::Path& p)
{
    float l = 0.f; juce::PathFlatteningIterator it (p, juce::AffineTransform(), 0.5f);
    while (it.next()) l += juce::Line<float> (it.x1, it.y1, it.x2, it.y2).getLength();
    return l;
}
} // namespace

// Cable bodies: this layer sits above the rack units and below the pedals, so cables tuck neatly under the pedals.
void BoardComponent::paintCables (juce::Graphics& g)
{
    for (auto& c : model.cables)
        strokeCable (g, cablePath (jackPosition (c.src, true), jackPosition (c.dst, false)), cableColour (c.src));
}

// The top layer: a short stub where each cable leaves its jack, the plug head on the jack itself, the cable being dragged.
void BoardComponent::paintPlugs (juce::Graphics& g)
{
    for (auto& c : model.cables)
    {
        const auto a = jackPosition (c.src, true), b = jackPosition (c.dst, false);
        const auto path = cablePath (a, b);
        const float len = pathLength (path), stub = juce::jmin (34.f, len * 0.4f);
        const auto col = cableColour (c.src);
        strokeCable (g, subPath (path, 0.f, stub), col);
        strokeCable (g, subPath (path, len - stub, len), col);
        for (auto pt : { a, b })
        {
            const auto f = pt.toFloat();
            g.setColour (juce::Colour (0xff0b0b0d)); g.fillEllipse (f.x - 7.f, f.y - 7.f, 14.f, 14.f);
            g.setColour (col); g.fillEllipse (f.x - 4.f, f.y - 4.f, 8.f, 8.f);
            g.setColour (juce::Colours::white.withAlpha (0.45f)); g.fillEllipse (f.x - 2.5f, f.y - 3.f, 3.f, 2.f);
        }
    }
    if (dragging)
    {
        const auto fixed = jackPosition (dragModule, dragFromOutput);
        strokeCable (g, dragFromOutput ? cablePath (fixed, dragPos) : cablePath (dragPos, fixed), juce::Colours::white.withAlpha (0.9f));
    }
    if (hoverModule >= 0)       // ring around the jack the cable would land on
    {
        const auto pt = jackPosition (hoverModule, ! dragFromOutput).toFloat();
        g.setColour (hoverValid ? juce::Colour (0xff4dff88) : juce::Colour (0xffff3d3d)); g.drawEllipse (pt.x - 15.f, pt.y - 15.f, 30.f, 30.f, 3.f);
    }
}

int BoardComponent::cableAt (juce::Point<int> p) const
{
    if (dragging) return -1;
    for (int i = (int) model.cables.size(); --i >= 0;)
    {
        const auto path = cablePath (jackPosition (model.cables[(size_t) i].src, true), jackPosition (model.cables[(size_t) i].dst, false));
        juce::PathFlatteningIterator fi (path, juce::AffineTransform(), 1.5f);     // walks the curve as short segments
        juce::Point<float> nearest;
        while (fi.next()) if (juce::Line<float> (fi.x1, fi.y1, fi.x2, fi.y2).getDistanceFromPoint (p.toFloat(), nearest) < 8.f) return i;
    }
    return -1;
}

void BoardComponent::deleteCable (int i)
{
    if (i < 0 || i >= (int) model.cables.size()) return;
    const auto c = model.cables[(size_t) i];
    model.disconnect (c.src, c.dst);
}

void BoardComponent::beginCable (ModuleComponent& m, bool fromOutput)
{
    dragging = true; dragFromOutput = fromOutput; dragModule = m.getModuleId();
    dragPos = m.getJackPosition (fromOutput); hoverModule = -1;
    repaintCables();
}

void BoardComponent::dragCable (juce::Point<int> p)
{
    dragPos = p; hoverModule = -1;
    for (auto* m : modules)
    {
        if (m->getModuleId() == dragModule) continue;
        const bool wantsOutput = ! dragFromOutput;            // dragging from an output looks for an input jack
        if ((wantsOutput ? m->hasOutputJack() : m->hasInputJack()) && p.getDistanceFrom (m->getJackPosition (wantsOutput)) <= 22)
        {
            hoverModule = m->getModuleId();
            hoverValid = dragFromOutput ? model.canConnect (dragModule, hoverModule) : model.canConnect (hoverModule, dragModule);
        }
    }
    repaintCables();
}

void BoardComponent::endCable (juce::Point<int> p)
{
    dragCable (p);
    const int target = hoverModule; const bool valid = hoverValid, fromOut = dragFromOutput; const int src = dragModule;
    dragging = false; hoverModule = -1;
    if (target >= 0 && valid) { if (fromOut) model.connect (src, target); else model.connect (target, src); }
    repaintCables();
}

bool BoardComponent::simulateCableDrag (int srcId, int dstId)
{
    auto* c = componentFor (srcId);
    if (c == nullptr || componentFor (dstId) == nullptr) return false;
    beginCable (*c, true);
    const auto target = jackPosition (dstId, false);
    dragCable (target + juce::Point<int> (4, 3));          // a little off-centre, as a hand would be
    endCable (target + juce::Point<int> (4, 3));
    return model.isConnected (srcId, dstId);
}

void BoardComponent::moduleMoved (ModuleComponent& m)
{
    if (auto* info = model.find (m.getModuleId()))
    {
        if (isRackModule (info->type))
        {
            // reorder rack units by their vertical position
            std::vector<std::pair<int, size_t>> racks;
            for (size_t i = 0; i < model.modules.size(); ++i) if (isRackModule (model.modules[i].type)) racks.push_back ({ componentFor (model.modules[i].id)->getY(), i });
            std::vector<ModuleInfo> ordered;
            std::vector<size_t> idx; for (auto& r : racks) idx.push_back (r.second);
            std::sort (racks.begin(), racks.end());
            for (auto& r : racks) ordered.push_back (model.modules[r.second]);
            for (size_t k = 0; k < idx.size(); ++k) model.modules[idx[k]] = ordered[k];
            layoutAll();
            m.toFront (true);
        }
        else info->pos = { m.getX(), m.getY() - boardTopY };
    }
    updateZOrder();
    repaintCables();
}

void BoardComponent::requestRemove (int id)
{
    // the component (and its editor) must go before the processor does
    juce::MessageManager::callAsync ([this, id, safe = juce::Component::SafePointer<BoardComponent> (this)]
    {
        if (safe == nullptr) return;
        if (auto* c = componentFor (id)) modules.removeObject (c, true);
        model.removeModule (id);
        layoutAll(); repaintCables(); repaint();
    });
}

//------------------------------------------------------------------------------ actions
void BoardComponent::addModuleOfType (ModuleType t)
{
    int n = 0; for (auto& m : model.modules) if (! isRackModule (m.type) && ! isTerminalModule (m.type)) ++n;
    const juce::Point<int> pos { 190 + (n % 3) * 250, (n / 3) * 40 };
    model.addModule (t, pos);
}

void BoardComponent::showAddMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader ("Rack");
    m.addItem (1, "KTG-1  \xE2\x80\x94  tube preamp (rack unit)");
    m.addSectionHeader ("Pedals");
    m.addItem (2, "FZ-3  \xE2\x80\x94  fuzz");
    m.addItem (3, "BD-2  \xE2\x80\x94  blues driver");
    m.addItem (4, "GS-424  \xE2\x80\x94  gain stage");
    m.addItem (5, "FF-1  \xE2\x80\x94  germanium / silicon fuzz");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&addButton), [this] (int r)
    {
        switch (r) { case 1: addModuleOfType (ModuleType::Ktg1); break; case 2: addModuleOfType (ModuleType::Fz3); break;
                     case 3: addModuleOfType (ModuleType::Bd2);  break; case 4: addModuleOfType (ModuleType::Gs424); break; case 5: addModuleOfType (ModuleType::Ff1); break; default: break; }
    });
}

void BoardComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (auto* c = componentFor (BoardModel::kInputId)) c->refreshSource();
}

juce::StringArray BoardComponent::inputSources() const
{
    if (auto* type = deviceManager.getCurrentDeviceTypeObject()) return type->getDeviceNames (true);
    return {};
}

juce::String BoardComponent::currentInputSource() const
{
    const auto setup = deviceManager.getAudioDeviceSetup();
    return setup.inputChannels.isZero() ? juce::String() : setup.inputDeviceName;
}

void BoardComponent::setInputSource (const juce::String& name)
{
    auto setup = deviceManager.getAudioDeviceSetup();
    if (name.isEmpty()) { setup.inputDeviceName = {}; setup.inputChannels.clear(); setup.useDefaultInputChannels = false; }
    else                { setup.inputDeviceName = name; setup.useDefaultInputChannels = true; }
    deviceManager.setAudioDeviceSetup (setup, true);
}

void BoardComponent::showAudioSettings()
{
    auto* sel = new juce::AudioDeviceSelectorComponent (deviceManager, 0, 2, 0, 2, false, false, true, false);
    sel->setSize (520, 420);
    juce::DialogWindow::LaunchOptions o;
    o.content.setOwned (sel); o.dialogTitle = "Audio settings"; o.componentToCentreAround = this;
    o.dialogBackgroundColour = juce::Colour (0xff1d1f24); o.escapeKeyTriggersCloseButton = true; o.useNativeTitleBar = true; o.resizable = false;
    o.launchAsync();
}

std::unique_ptr<juce::XmlElement> BoardComponent::toXml() { return model.toXml(); }

bool BoardComponent::loadFromXml (const juce::XmlElement& xml)
{
    destroyModuleComponents();           // editors first, then the processors they belong to
    const bool ok = model.loadXml (xml);
    syncModules();
    return ok;
}

void BoardComponent::resetBoard()  { destroyModuleComponents(); model.buildDefaultBoard(); syncModules(); }
void BoardComponent::clearBoard()  { destroyModuleComponents(); model.clearBoard(); syncModules(); }

void BoardComponent::saveBoard()
{
    chooser = std::make_unique<juce::FileChooser> ("Save pedalboard", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("my-board.sh0tyboard"), "*.sh0tyboard");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult(); if (f == juce::File()) return;
        if (auto xml = toXml()) xml->writeTo (f.withFileExtension ("sh0tyboard"));
    });
}

void BoardComponent::loadBoard()
{
    chooser = std::make_unique<juce::FileChooser> ("Load pedalboard", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.sh0tyboard");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult(); if (! f.existsAsFile()) return;
        if (auto xml = juce::XmlDocument::parse (f)) loadFromXml (*xml);
    });
}
