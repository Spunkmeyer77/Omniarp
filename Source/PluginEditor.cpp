#include "PluginEditor.h"

using namespace juce;

namespace
{
constexpr float kStart = MathConstants<float>::pi * 1.22f;
constexpr float kEnd   = MathConstants<float>::pi * 2.78f;
constexpr int   W = 1280, H = 190;

Point<float> polar (Point<float> c, float r, float a) { return { c.x + r * std::sin (a), c.y - r * std::cos (a) }; }

Font labelFont (float h) { return Font (FontOptions (h, Font::bold)).withExtraKerningFactor (0.04f); }
}

namespace tross
{
//==============================================================================
ArpLookAndFeel::ArpLookAndFeel()
{
    setColour (PopupMenu::backgroundColourId, col::menuBg);
    setColour (PopupMenu::textColourId, col::ledRed);
}

void ArpLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                       float start, float end, Slider&)
{
    const auto b = Rectangle<int> (x, y, w, h).toFloat();
    const auto c = b.getCentre();
    const float R = jmin (b.getWidth(), b.getHeight()) * 0.5f;

    // drop shadow
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillEllipse (Rectangle<float> (R * 2.1f, R * 2.1f).withCentre (c.translated (0, R * 0.12f)));

    // scalloped black skirt
    Path skirt;
    const int lobes = 9;
    for (int i = 0; i <= 180; ++i)
    {
        const float a = MathConstants<float>::twoPi * (float) i / 180.0f;
        const float r = R * (0.93f + 0.07f * std::cos (a * (float) lobes));
        const auto p = polar (c, r, a);
        i == 0 ? skirt.startNewSubPath (p) : skirt.lineTo (p);
    }
    skirt.closeSubPath();
    g.setGradientFill (ColourGradient (Colour (0xff4a4a4e), c.x, c.y - R, Colour (0xff09090a), c.x, c.y + R, false));
    g.fillPath (skirt);
    g.setColour (Colours::white.withAlpha (0.12f));
    g.strokePath (skirt, PathStrokeType (1.0f));

    // chrome cap
    const float cr = R * 0.66f;
    auto cap = Rectangle<float> (cr * 2, cr * 2).withCentre (c);
    ColourGradient chrome (Colour (0xfff4f4f6), c.x - cr * 0.3f, c.y - cr * 0.5f,
                           Colour (0xff6c6e74), c.x + cr * 0.4f, c.y + cr, true);
    chrome.addColour (0.45, Colour (0xffc7c9cf));
    g.setGradientFill (chrome);
    g.fillEllipse (cap);
    // brushed rings
    for (float r = cr * 0.2f; r < cr; r += 2.2f)
    {
        g.setColour (Colours::white.withAlpha (0.06f));
        g.drawEllipse (Rectangle<float> (r * 2, r * 2).withCentre (c), 0.7f);
    }
    g.setColour (Colours::black.withAlpha (0.5f));
    g.drawEllipse (cap, 1.2f);

    // indicator dot on the skirt
    const float a = start + pos * (end - start);
    const auto d = polar (c, R * 0.81f, a);
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillEllipse (Rectangle<float> (R * 0.2f, R * 0.2f).withCentre (d));
    g.setColour (Colours::white);
    g.fillEllipse (Rectangle<float> (R * 0.15f, R * 0.15f).withCentre (d));
}

void ArpLookAndFeel::drawPopupMenuBackground (Graphics& g, int w, int h)
{
    g.setColour (col::menuBg);
    g.fillRoundedRectangle (0, 0, (float) w, (float) h, 10.0f);
}

void ArpLookAndFeel::drawPopupMenuItem (Graphics& g, const Rectangle<int>& area, bool, bool,
                                        bool highlighted, bool ticked, bool, const String& text,
                                        const String&, const Drawable*, const Colour*)
{
    auto r = area.reduced (16, 0);
    if (highlighted)
    {
        g.setColour (col::ledRed.withAlpha (0.18f));
        g.fillRect (area.reduced (6, 2));
    }
    g.setColour (ticked ? Colour (0xffff8070) : col::ledRed);
    g.setFont (getPopupMenuFont());
    g.drawText (text, r, Justification::centredLeft);
    g.setColour (Colours::white.withAlpha (0.18f));
    g.fillRect (r.getX(), r.getBottom() - 1, r.getWidth(), 1);
}

void ArpLookAndFeel::getIdealPopupMenuItemSize (const String&, bool, int, int& w, int& h)
{
    w = 300; h = 40;
}

Font ArpLookAndFeel::getPopupMenuFont() { return Font (FontOptions (22.0f)).withExtraKerningFactor (0.05f); }

//==============================================================================
void ArpSwitch::paintButton (Graphics& g, bool over, bool)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0b0b0c));
    g.fillRoundedRectangle (b, 4.0f);
    auto inner = b.reduced (5.0f);
    const bool on = getToggleState();

    const Colour hi = on ? Colour (0xff3d6cf0) : Colour (0xff1b2a55);
    const Colour lo = on ? Colour (0xff163fae) : Colour (0xff0c1530);
    // rocker: lit half on top when on
    auto top = inner.removeFromTop (inner.getHeight() * (on ? 0.25f : 0.75f));
    g.setGradientFill (ColourGradient (hi.brighter (0.25f), top.getX(), top.getY(), hi, top.getRight(), top.getBottom(), false));
    g.fillRect (top);
    g.setGradientFill (ColourGradient (lo, inner.getX(), inner.getY(), lo.darker (0.4f), inner.getX(), inner.getBottom(), false));
    g.fillRect (inner);
    if (over) { g.setColour (Colours::white.withAlpha (0.06f)); g.fillRect (b.reduced (5.0f)); }

    // padlock glyph: shackle closed when HOLD is on
    const auto c = b.reduced (5.0f).getCentre().translated (0, on ? -6.0f : 6.0f);
    g.setColour (Colours::white.withAlpha (on ? 0.9f : 0.35f));
    g.fillRoundedRectangle (Rectangle<float> (16, 12).withCentre (c.translated (0, 4)), 2.0f);
    Path shackle;
    const float lift = on ? 0.0f : 4.0f;
    shackle.startNewSubPath (c.x - 5, c.y - 2);
    shackle.lineTo (c.x - 5, c.y - 6 - lift);
    shackle.addCentredArc (c.x, c.y - 6 - lift, 5, 5, 0, -MathConstants<float>::halfPi, MathConstants<float>::halfPi);
    shackle.lineTo (c.x + 5, c.y - 2 - (on ? 0.0f : lift));
    g.strokePath (shackle, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
}

//==============================================================================
AlgoDisplay::AlgoDisplay (RangedAudioParameter& p)
    : param (p), attachment (p, [this] (float v) { index = roundToInt (v); repaint(); })
{
    attachment.sendInitialUpdate();
}

void AlgoDisplay::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (Colours::black);
    g.fillRoundedRectangle (b, 5.0f);
    auto in = b.reduced (3.0f);
    g.setGradientFill (ColourGradient (col::ledBg.brighter (0.15f), in.getCentreX(), in.getY(), col::ledBg.darker (0.5f), in.getCentreX(), in.getBottom(), false));
    g.fillRoundedRectangle (in, 4.0f);

    const auto names = algoNames();
    const auto text = names[jlimit (0, names.size() - 1, index)];
    g.setFont (Font (FontOptions (in.getHeight() * 0.62f)).withExtraKerningFactor (0.05f));
    g.setColour (col::ledRed.withAlpha (0.35f));                       // glow
    g.drawText (text, in.translated (0, 1).expanded (1), Justification::centred);
    g.setColour (col::ledRed);
    g.drawText (text, in, Justification::centred);

    Path tri;                                                           // dropdown arrow
    const float tx = in.getRight() - 12, ty = in.getBottom() - 11;
    tri.addTriangle (tx - 5, ty, tx + 5, ty, tx, ty + 6);
    g.strokePath (tri, PathStrokeType (1.2f));
}

void AlgoDisplay::mouseDown (const MouseEvent&)
{
    PopupMenu m;
    m.setLookAndFeel (&getLookAndFeel());
    const auto names = algoNames();
    for (int i = 0; i < names.size(); ++i)
        m.addItem (i + 1, String (i + 1) + ". " + names[i], true, i == index);

    m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withPreferredPopupDirection (PopupMenu::Options::PopupDirection::downwards),
                     [this] (int r) { if (r > 0) attachment.setValueAsCompleteGesture ((float) (r - 1)); });
}

void AlgoDisplay::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w)
{
    const int n = algoNames().size();
    const int next = jlimit (0, n - 1, index + (w.deltaY < 0 ? 1 : -1));
    if (next != index) attachment.setValueAsCompleteGesture ((float) next);
}

//==============================================================================
void AboutPanel::paint (Graphics& g)
{
    g.fillAll (Colours::black.withAlpha (0.72f));
    auto card = getLocalBounds().toFloat().withSizeKeepingCentre (440, 160);
    g.setGradientFill (ColourGradient (col::bgTop.brighter (0.15f), card.getX(), card.getY(), col::bgBot, card.getX(), card.getBottom(), false));
    g.fillRoundedRectangle (card, 10);
    g.setColour (col::gold);
    g.drawRoundedRectangle (card.reduced (4), 8, 1.5f);

    auto r = card.reduced (20, 14);
    g.setFont (labelFont (34));
    g.drawText ("OMNIARP", r.removeFromTop (42), Justification::centred);
    g.setColour (col::ledRed);
    g.setFont (labelFont (13));
    g.drawText ("ARPEGGIATOR  -  VERSION " JucePlugin_VersionString, r.removeFromTop (22), Justification::centred);
    g.setColour (col::gold.withAlpha (0.85f));
    g.setFont (labelFont (14));
    g.drawText ("TICCO ROSS", r.removeFromTop (24), Justification::centred);
    g.setFont (Font (FontOptions (13.0f)));
    g.drawText ("ticcoross@gmail.com", r.removeFromTop (20), Justification::centred);
    g.setColour (col::gold.withAlpha (0.45f));
    g.setFont (Font (FontOptions (11.0f)));
    g.drawText ("click anywhere to close", r, Justification::centredBottom);
}
} // namespace tross

//==============================================================================
OmniarpEditor::OmniarpEditor (OmniarpProcessor& p)
    : AudioProcessorEditor (p), proc (p), algo (*p.apvts.getParameter ("algo"))
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (arpSwitch);
    arpAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "hold", arpSwitch);
    addAndMakeVisible (algo);

    addKnob (gate,    "gate",    "GATE",     {});
    addKnob (swing,   "swing",   "SWING",    {});
    addKnob (note,    "rate",    "NOTE",     tross::rateNames());
    addKnob (noteRpt, "noteRpt", "NOTE RPT", { "1", "2", "3", "4", "5" });
    addKnob (oct,     "oct",     "OCT",      tross::octNames());
    addKnob (octRpt,  "octRpt",  "OCT RPT",  { "1", "2", "3", "4", "5" });

    addChildComponent (about);

    // Subtle grain texture, generated once.
    texture = Image (Image::ARGB, W, H, true);
    Random rng (606);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            texture.setPixelAt (x, y, Colour::fromRGBA (0, 0, 0, (uint8) rng.nextInt (22)));

    setSize (W, H);
}

OmniarpEditor::~OmniarpEditor() { setLookAndFeel (nullptr); }

void OmniarpEditor::addKnob (Knob& k, const String& id, const String& name, StringArray marks)
{
    k.name = name;
    k.marks = std::move (marks);
    k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    k.slider.setRotaryParameters (kStart, kEnd, true);
    k.slider.setMouseDragSensitivity (180);
    k.slider.setPopupDisplayEnabled (true, false, this);
    addAndMakeVisible (k.slider);
    k.att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);
}

void OmniarpEditor::drawKnobLabels (Graphics& g, const Knob& k)
{
    const auto b = k.slider.getBounds().toFloat();
    const auto c = b.getCentre();
    const float R = jmin (b.getWidth(), b.getHeight()) * 0.5f;

    g.setColour (tross::col::gold);
    g.setFont (labelFont (19));
    g.drawText (k.name, Rectangle<float> (150, 24).withCentre ({ c.x, c.y + R + 26 }), Justification::centred);

    if (k.marks.isEmpty())
    {
        // continuous: graduated tick scale with 0 and 1 at the ends
        const int n = 21;
        for (int i = 0; i < n; ++i)
        {
            const float a = kStart + (kEnd - kStart) * (float) i / (float) (n - 1);
            const bool major = i == 0 || i == n - 1 || i == n / 2;
            g.drawLine (Line<float> (polar (c, R + 6, a), polar (c, R + (major ? 16.0f : 11.0f), a)), major ? 2.0f : 1.2f);
        }
        g.setFont (labelFont (17));
        g.drawText ("0", Rectangle<float> (20, 20).withCentre (polar (c, R + 22, kStart).translated (-6, 10)), Justification::centred);
        g.drawText ("1", Rectangle<float> (20, 20).withCentre (polar (c, R + 22, kEnd).translated (6, 10)), Justification::centred);
        return;
    }

    // stepped: a dot + value label at each position
    const int n = k.marks.size();
    g.setFont (labelFont (n > 6 ? 12.5f : 17.0f));
    for (int i = 0; i < n; ++i)
    {
        const float a = kStart + (kEnd - kStart) * (float) i / (float) (n - 1);
        g.fillEllipse (Rectangle<float> (4.5f, 4.5f).withCentre (polar (c, R + 7, a)));
        g.drawText (k.marks[i], Rectangle<float> (40, 16).withCentre (polar (c, R + (n > 6 ? 30.0f : 22.0f), a)), Justification::centred);
    }
}

void OmniarpEditor::paint (Graphics& g)
{
    g.setGradientFill (ColourGradient (tross::col::bgTop, 0, 0, tross::col::bgBot, 0, (float) H, false));
    g.fillAll();
    g.drawImageAt (texture, 0, 0);
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawHorizontalLine (1, 0, (float) W);
    g.setColour (Colours::black.withAlpha (0.6f));
    g.drawRect (getLocalBounds(), 1);

    g.setColour (tross::col::gold);
    g.setFont (labelFont (20));
    g.drawText ("HOLD", Rectangle<int> (8, arpSwitch.getBottom() + 8, 60, 24), Justification::centred);
    g.drawText ("ALGORITHM", algo.getBounds().translated (0, algo.getHeight() + 6).withHeight (24), Justification::centredLeft);

    // logo (click for About)
    g.setFont (labelFont (26));
    g.drawText ("OMNIARP", logoArea.withHeight (30), Justification::centredLeft);
    g.setColour (tross::col::gold.withAlpha (0.55f));
    g.setFont (labelFont (11));
    g.drawText ("TICCO ROSS", Rectangle<int> (100, 48, 200, 16), Justification::centredLeft);

    for (auto* k : { &gate, &swing, &note, &noteRpt, &oct, &octRpt })
        drawKnobLabels (g, *k);
}

void OmniarpEditor::resized()
{
    arpSwitch.setBounds (14, 40, 48, 104);
    logoArea = { 98, 18, 200, 50 };
    algo.setBounds (98, 76, 250, 56);

    const int knob = 84, firstX = 440, gap = 150, cy = 90;
    int i = 0;
    for (auto* k : { &gate, &swing, &note, &noteRpt, &oct, &octRpt })
    {
        k->slider.setBounds (Rectangle<int> (knob, knob).withCentre ({ firstX + gap * i, cy }));
        ++i;
    }

    about.setBounds (getLocalBounds());
}

void OmniarpEditor::mouseDown (const MouseEvent& e)
{
    if (logoArea.contains (e.getPosition()))
    {
        about.setVisible (true);
        about.toFront (false);
    }
}
