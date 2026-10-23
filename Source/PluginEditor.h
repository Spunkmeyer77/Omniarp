#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace tross
{
namespace col
{
    const juce::Colour bgTop    { 0xff3a2f2a }, bgBot { 0xff241c19 };
    const juce::Colour gold     { 0xffd9b77a };
    const juce::Colour ledRed   { 0xffff2a1c }, ledBg { 0xff3c0503 };
    const juce::Colour menuBg   { 0xf0500805 };
}

class ArpLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ArpLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>&, bool isSeparator, bool isActive,
                            bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                            const juce::String& shortcut, const juce::Drawable* icon, const juce::Colour* textColour) override;
    void getIdealPopupMenuItemSize (const juce::String&, bool isSeparator, int standardHeight,
                                    int& w, int& h) override;
    juce::Font getPopupMenuFont() override;
    int getPopupMenuBorderSize() override { return 10; }
};

// Blue rocker switch for HOLD (latch).
class ArpSwitch : public juce::Button
{
public:
    ArpSwitch() : juce::Button ("HOLD") { setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

// Red LED algorithm display that opens a popup list.
class AlgoDisplay : public juce::Component
{
public:
    explicit AlgoDisplay (juce::RangedAudioParameter& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    juce::ParameterAttachment attachment;
    int index = 0;
};

// Red LED key + scale display ("C MINOR" / "OFF").
class ScaleDisplay : public juce::Component
{
public:
    ScaleDisplay (juce::RangedAudioParameter& key, juce::RangedAudioParameter& scale);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    juce::ParameterAttachment keyAtt, scaleAtt;
    int key = 0, scale = 0;
};

// Red LED transpose screen: -12..+12. Arrows step by one, click the middle for a jump grid,
// scroll to step, double-click to reset.
class TransposeDisplay : public juce::Component
{
public:
    explicit TransposeDisplay (juce::RangedAudioParameter& transpose);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    void set (int v);
    void showGrid();
    juce::ParameterAttachment att;
    int value = 0;
    bool gridPending = false;
};

// Small LED switch: "IN KEY".
class LedToggle : public juce::Button
{
public:
    explicit LedToggle (const juce::String& text) : juce::Button (text) { setClickingTogglesState (true); setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

// Invisible click zone over the title that opens the About panel.
class TitleHotspot : public juce::Component
{
public:
    TitleHotspot() { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
    std::function<void()> onClick;
};

class AboutPanel : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { setVisible (false); }
};

// The whole panel at its design size; the editor scales it to the window.
class MainPanel : public juce::Component
{
public:
    static constexpr int W = 1525, H = 210;
    explicit MainPanel (OmniarpProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::String name;
        juce::StringArray marks;           // stepped: one label per position
        juce::String lowLabel, highLabel;  // continuous: end labels
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    void addKnob (Knob&, const juce::String& id, const juce::String& name, juce::StringArray marks,
                  const juce::String& low = "0", const juce::String& high = "1");
    void drawKnobLabels (juce::Graphics&, const Knob&);
    std::vector<Knob*> knobs();

    OmniarpProcessor& proc;
    ArpSwitch holdSwitch;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> holdAtt;
    AlgoDisplay algo;
    ScaleDisplay scaleDisp;
    TransposeDisplay transposeDisp;
    LedToggle inKey { "IN KEY" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> inKeyAtt;
    Knob gate, swing, note, noteRpt, steps, oct, octRpt;
    TitleHotspot title;
    AboutPanel about;
    juce::Image texture;
};
} // namespace tross

class OmniarpEditor : public juce::AudioProcessorEditor
{
public:
    explicit OmniarpEditor (OmniarpProcessor&);
    ~OmniarpEditor() override;
    void resized() override;

private:
    OmniarpProcessor& proc;
    tross::ArpLookAndFeel lnf;
    tross::MainPanel panel;
    bool ready = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OmniarpEditor)
};
