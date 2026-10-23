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
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    int index = 0;
};

// Custom About panel.
class AboutPanel : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { setVisible (false); }
};
} // namespace tross

class OmniarpEditor : public juce::AudioProcessorEditor
{
public:
    explicit OmniarpEditor (OmniarpProcessor&);
    ~OmniarpEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::String name;
        juce::StringArray marks;   // empty -> drawn as tick scale with 0/1 ends
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    void addKnob (Knob&, const juce::String& id, const juce::String& name, juce::StringArray marks);
    void drawKnobLabels (juce::Graphics&, const Knob&);

    OmniarpProcessor& proc;
    tross::ArpLookAndFeel lnf;
    tross::ArpSwitch arpSwitch;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> arpAtt;
    tross::AlgoDisplay algo;
    Knob gate, swing, note, noteRpt, oct, octRpt;
    tross::AboutPanel about;
    juce::Rectangle<int> logoArea;
    juce::Image texture;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OmniarpEditor)
};
