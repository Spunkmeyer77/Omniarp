#include "PluginEditor.h"

using namespace juce;

namespace
{
constexpr float kStart = MathConstants<float>::pi * 1.22f;
constexpr float kEnd   = MathConstants<float>::pi * 2.78f;

Point<float> polar (Point<float> c, float r, float a) { return { c.x + r * std::sin (a), c.y - r * std::cos (a) }; }

Font labelFont (float h) { return Font (FontOptions (h, Font::bold)).withExtraKerningFactor (0.04f); }

// Shared red LED screen with glowing text and a dropdown marker.
void drawLed (Graphics& g, Rectangle<float> b, const String& text, bool marker = true)
{
    g.setColour (Colours::black);
    g.fillRoundedRectangle (b, 5.0f);
    auto in = b.reduced (3.0f);
    g.setGradientFill (ColourGradient (tross::col::ledBg.brighter (0.15f), in.getCentreX(), in.getY(),
                                       tross::col::ledBg.darker (0.5f), in.getCentreX(), in.getBottom(), false));
    g.fillRoundedRectangle (in, 4.0f);
    g.setFont (Font (FontOptions (24.8f)).withExtraKerningFactor (0.05f));
    const auto textArea = marker ? in.withTrimmedLeft (4).withTrimmedRight (14) : in;
    g.setColour (tross::col::ledRed.withAlpha (0.35f));
    g.drawText (text, textArea.translated (0, 1).expanded (1), Justification::centred, false);
    g.setColour (tross::col::ledRed);
    g.drawText (text, textArea, Justification::centred, false);
    if (! marker) return;
    Path tri;
    const float tx = in.getRight() - 12, ty = in.getBottom() - 10;
    tri.addTriangle (tx - 5, ty, tx + 5, ty, tx, ty + 6);
    g.strokePath (tri, PathStrokeType (1.2f));
}
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

void ArpLookAndFeel::drawPopupMenuItem (Graphics& g, const Rectangle<int>& area, bool isSeparator, bool,
                                        bool highlighted, bool ticked, bool hasSubMenu, const String& text,
                                        const String&, const Drawable*, const Colour*)
{
    if (isSeparator)
    {
        g.setColour (col::ledRed.withAlpha (0.45f));
        g.fillRect (area.reduced (16, 0).withSizeKeepingCentre (area.getWidth() - 32, 2));
        return;
    }
    auto r = area.reduced (16, 0);
    if (hasSubMenu)
    {
        Path arrow;
        const float x = (float) r.getRight() - 8, y = (float) r.getCentreY();
        arrow.addTriangle (x - 5, y - 6, x - 5, y + 6, x + 3, y);
        g.setColour (col::ledRed);
        g.fillPath (arrow);
    }
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

void ArpLookAndFeel::getIdealPopupMenuItemSize (const String& text, bool isSeparator, int, int& w, int& h)
{
    w = 300; h = isSeparator ? 12 : 40;
    if (text.length() <= 3) { w = 74; h = 36; }   // transpose grid
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
    : attachment (p, [this] (float v) { index = roundToInt (v); repaint(); })
{
    attachment.sendInitialUpdate();
    setMouseCursor (MouseCursor::PointingHandCursor);
}

void AlgoDisplay::paint (Graphics& g)
{
    const auto names = algoNames();
    drawLed (g, getLocalBounds().toFloat(), names[jlimit (0, names.size() - 1, index)]);
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
ScaleDisplay::ScaleDisplay (RangedAudioParameter& k, RangedAudioParameter& s)
    : keyAtt   (k, [this] (float v) { key   = roundToInt (v); repaint(); }),
      scaleAtt (s, [this] (float v) { scale = roundToInt (v); repaint(); })
{
    keyAtt.sendInitialUpdate();
    scaleAtt.sendInitialUpdate();
    setMouseCursor (MouseCursor::PointingHandCursor);
}

void ScaleDisplay::paint (Graphics& g)
{
    const auto scales = scaleNames();
    static const StringArray shortNames { "OFF", "MAJOR", "MINOR", "DORIAN", "PHRYGIAN", "LYDIAN", "MIXO",
                                          "HARM MIN", "PENT MAJ", "PENT MIN", "BLUES" };
    const auto text = scale == 0 ? String ("OFF") : keyNames()[key] + " " + shortNames[jlimit (0, shortNames.size() - 1, scale)];
    drawLed (g, getLocalBounds().toFloat(), text);
}

void ScaleDisplay::mouseDown (const MouseEvent&)
{
    PopupMenu keys;
    const auto kn = keyNames();
    for (int i = 0; i < kn.size(); ++i)
        keys.addItem (100 + i, kn[i], true, i == key);

    PopupMenu m;
    m.setLookAndFeel (&getLookAndFeel());
    m.addSubMenu ("KEY:  " + kn[key], keys);
    m.addSeparator();
    const auto sn = scaleNames();
    for (int i = 0; i < sn.size(); ++i)
        m.addItem (1 + i, sn[i], true, i == scale);

    m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withPreferredPopupDirection (PopupMenu::Options::PopupDirection::downwards),
                     [this] (int r)
                     {
                         if (r >= 100)    keyAtt.setValueAsCompleteGesture ((float) (r - 100));
                         else if (r > 0)  scaleAtt.setValueAsCompleteGesture ((float) (r - 1));
                     });
}

void ScaleDisplay::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w)
{
    const int n = scaleNames().size();
    const int next = jlimit (0, n - 1, scale + (w.deltaY < 0 ? 1 : -1));
    if (next != scale) scaleAtt.setValueAsCompleteGesture ((float) next);
}

//==============================================================================
TransposeDisplay::TransposeDisplay (RangedAudioParameter& p)
    : att (p, [this] (float v) { value = roundToInt (v); repaint(); })
{
    att.sendInitialUpdate();
    setMouseCursor (MouseCursor::PointingHandCursor);
}

void TransposeDisplay::set (int v)
{
    v = jlimit (-12, 12, v);
    if (v != value) att.setValueAsCompleteGesture ((float) v);
}

void TransposeDisplay::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    drawLed (g, b, value > 0 ? "+" + String (value) : String (value), false);

    // hardware-style arrows at both ends
    auto arrow = [&] (float cx, bool left)
    {
        Path p;
        const float cy = b.getCentreY();
        if (left) p.addTriangle (cx + 4, cy - 6, cx + 4, cy + 6, cx - 4, cy);
        else      p.addTriangle (cx - 4, cy - 6, cx - 4, cy + 6, cx + 4, cy);
        g.setColour (col::ledRed.withAlpha (value == (left ? -12 : 12) ? 0.25f : 0.85f));
        g.fillPath (p);
    };
    arrow (b.getX() + 14, true);
    arrow (b.getRight() - 14, false);
}

void TransposeDisplay::mouseDown (const MouseEvent& e)
{
    if (e.x < 26)               { set (value - 1); return; }
    if (e.x > getWidth() - 26)  { set (value + 1); return; }

    if (e.getNumberOfClicks() > 1) { gridPending = false; set (0); return; }   // double-click = 0

    // wait a moment so a double-click can reset instead of opening the grid
    gridPending = true;
    Component::SafePointer<TransposeDisplay> safe (this);
    Timer::callAfterDelay (MouseEvent::getDoubleClickTimeout() + 20, [safe]
    {
        if (safe != nullptr && safe->gridPending) { safe->gridPending = false; safe->showGrid(); }
    });
}

void TransposeDisplay::showGrid()
{
    // 5 columns: -12..-8 | -7..-3 | -2..+2 | +3..+7 | +8..+12
    PopupMenu m;
    m.setLookAndFeel (&getLookAndFeel());
    for (int v = -12; v <= 12; ++v)
    {
        if (v > -12 && (v + 12) % 5 == 0) m.addColumnBreak();
        m.addItem (100 + v, v > 0 ? "+" + String (v) : String (v), true, v == value);
    }
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (this),
                     [this] (int r) { if (r > 0) set (r - 100); });
}

void TransposeDisplay::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w)
{
    set (value + (w.deltaY > 0 ? 1 : -1));
}

//==============================================================================
void LedToggle::paintButton (Graphics& g, bool over, bool)
{
    auto b = getLocalBounds().toFloat();
    const bool on = getToggleState();
    auto led = Rectangle<float> (10, 10).withCentre ({ b.getX() + 7, b.getCentreY() });
    g.setColour (Colours::black);
    g.fillEllipse (led.expanded (2));
    if (on) { g.setColour (col::ledRed.withAlpha (0.35f)); g.fillEllipse (led.expanded (4)); }
    g.setColour (on ? col::ledRed : col::ledBg.brighter (0.3f));
    g.fillEllipse (led);
    g.setColour (col::gold.withAlpha (over ? 1.0f : 0.85f));
    g.setFont (labelFont (13));
    g.drawText (getButtonText(), b.withTrimmedLeft (18), Justification::centredLeft);
}

//==============================================================================
void AboutPanel::paint (Graphics& g)
{
    g.fillAll (Colours::black.withAlpha (0.72f));
    auto card = getLocalBounds().toFloat().withSizeKeepingCentre (460, 180);
    g.setGradientFill (ColourGradient (col::bgTop.brighter (0.15f), card.getX(), card.getY(), col::bgBot, card.getX(), card.getBottom(), false));
    g.fillRoundedRectangle (card, 10);
    g.setColour (col::gold);
    g.drawRoundedRectangle (card.reduced (4), 8, 1.5f);

    auto r = card.reduced (20, 16);
    g.setFont (labelFont (36));
    g.drawText ("OMNIARP", r.removeFromTop (44), Justification::centred);
    g.setColour (col::ledRed);
    g.setFont (labelFont (13));
    g.drawText ("ARPEGGIATOR  -  VERSION " JucePlugin_VersionString, r.removeFromTop (24), Justification::centred);
    g.setColour (col::gold.withAlpha (0.85f));
    g.setFont (labelFont (15));
    g.drawText ("TICCO ROSS", r.removeFromTop (26), Justification::centred);
    g.setFont (Font (FontOptions (13.0f)));
    g.drawText ("ticcoross@gmail.com", r.removeFromTop (20), Justification::centred);
    g.setColour (col::gold.withAlpha (0.45f));
    g.setFont (Font (FontOptions (11.0f)));
    g.drawText ("click anywhere to close", r, Justification::centredBottom);
}

//==============================================================================
MainPanel::MainPanel (OmniarpProcessor& p)
    : proc (p),
      algo (*p.apvts.getParameter ("algo")),
      scaleDisp (*p.apvts.getParameter ("key"), *p.apvts.getParameter ("scale")),
      transposeDisp (*p.apvts.getParameter ("transpose"))
{
    addAndMakeVisible (holdSwitch);
    holdAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "hold", holdSwitch);
    addAndMakeVisible (algo);
    addAndMakeVisible (scaleDisp);
    addAndMakeVisible (transposeDisp);
    addAndMakeVisible (inKey);
    inKeyAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "inkey", inKey);

    addKnob (gate,      "gate",      "GATE",      {});
    addKnob (swing,     "swing",     "SWING",     {});
    addKnob (note,      "rate",      "NOTE",      rateNames());
    addKnob (noteRpt,   "noteRpt",   "NOTE RPT",  { "1", "2", "3", "4", "5" });
    addKnob (steps,     "steps",     "STEPS",     {}, "OFF", "16");
    addKnob (oct,       "oct",       "OCT",       octNames());
    addKnob (octRpt,    "octRpt",    "OCT RPT",   { "1", "2", "3", "4", "5" });

    addAndMakeVisible (title);
    title.onClick = [this] { about.setVisible (true); about.toFront (false); };
    addChildComponent (about);

    texture = Image (Image::ARGB, W, H, true);
    juce::Random rng (606);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            texture.setPixelAt (x, y, Colour::fromRGBA (0, 0, 0, (uint8) rng.nextInt (22)));

    setSize (W, H);
}

std::vector<MainPanel::Knob*> MainPanel::knobs()
{
    return { &gate, &swing, &note, &noteRpt, &steps, &oct, &octRpt };
}

void MainPanel::addKnob (Knob& k, const String& id, const String& name, StringArray marks,
                         const String& low, const String& high)
{
    k.name = name;
    k.marks = std::move (marks);
    k.lowLabel = low;
    k.highLabel = high;
    k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    k.slider.setRotaryParameters (kStart, kEnd, true);
    k.slider.setMouseDragSensitivity (180);
    k.slider.setPopupDisplayEnabled (true, false, this);
    addAndMakeVisible (k.slider);
    k.att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);
}

void MainPanel::drawKnobLabels (Graphics& g, const Knob& k)
{
    const auto b = k.slider.getBounds().toFloat();
    const auto c = b.getCentre();
    const float R = jmin (b.getWidth(), b.getHeight()) * 0.5f;

    g.setColour (col::gold);
    g.setFont (labelFont (k.name.length() > 8 ? 16.0f : 19.0f));
    g.drawText (k.name, Rectangle<float> (150, 24).withCentre ({ c.x, c.y + R + 26 }), Justification::centred);

    if (k.marks.isEmpty())
    {
        const int n = 21;
        for (int i = 0; i < n; ++i)
        {
            const float a = kStart + (kEnd - kStart) * (float) i / (float) (n - 1);
            const bool major = i == 0 || i == n - 1 || i == n / 2;
            g.drawLine (Line<float> (polar (c, R + 6, a), polar (c, R + (major ? 16.0f : 11.0f), a)), major ? 2.0f : 1.2f);
        }
        const bool wide = k.lowLabel.length() > 1;
        const float push = wide ? 20.0f : 10.0f, drop = wide ? 0.0f : 10.0f;
        g.setFont (labelFont (wide ? 13.0f : 17.0f));
        g.drawText (k.lowLabel,  Rectangle<float> (40, 20).withCentre (polar (c, R + 24, kStart).translated (-push, drop)), Justification::centred);
        g.drawText (k.highLabel, Rectangle<float> (40, 20).withCentre (polar (c, R + 24, kEnd).translated (push, drop)), Justification::centred);
        return;
    }

    const int n = k.marks.size();
    g.setFont (labelFont (n > 6 ? 12.5f : 17.0f));
    for (int i = 0; i < n; ++i)
    {
        const float a = kStart + (kEnd - kStart) * (float) i / (float) (n - 1);
        g.fillEllipse (Rectangle<float> (4.5f, 4.5f).withCentre (polar (c, R + 7, a)));
        g.drawText (k.marks[i], Rectangle<float> (40, 16).withCentre (polar (c, R + (n > 6 ? 30.0f : 22.0f), a)), Justification::centred);
    }
}

void MainPanel::paint (Graphics& g)
{
    g.setGradientFill (ColourGradient (col::bgTop, 0, 0, col::bgBot, 0, (float) H, false));
    g.fillAll();
    g.drawImageAt (texture, 0, 0);
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawHorizontalLine (1, 0, (float) W);
    g.setColour (Colours::black.withAlpha (0.6f));
    g.drawRect (getLocalBounds(), 1);

    g.setColour (col::gold);
    g.setFont (labelFont (20));
    g.drawText ("HOLD", Rectangle<int> (6, holdSwitch.getBottom() + 8, 64, 24), Justification::centred);
    g.setFont (labelFont (17));
    g.drawText ("ALGORITHM", algo.getBounds().translated (0, algo.getHeight() + 4).withHeight (20), Justification::centredLeft);
    g.drawText ("SCALE", scaleDisp.getBounds().translated (0, scaleDisp.getHeight() + 4).withHeight (20), Justification::centredLeft);
    g.drawText ("TRANSPOSE", transposeDisp.getBounds().translated (0, transposeDisp.getHeight() + 6).withHeight (20).expanded (20, 0), Justification::centred);


    // title (plug-in name only) + dimmed version
    g.setColour (col::gold);
    g.setFont (labelFont (28));
    g.drawText ("OMNIARP", title.getBounds(), Justification::centredLeft);
    g.setColour (col::gold.withAlpha (0.4f));
    g.setFont (labelFont (12));
    g.drawText ("v" JucePlugin_VersionString, Rectangle<int> (title.getRight() + 8, title.getY() + 11, 80, 16), Justification::centredLeft);

    for (auto* k : knobs())
        drawKnobLabels (g, *k);
}

void MainPanel::resized()
{
    holdSwitch.setBounds (14, 46, 48, 112);

    // click zone = exactly the title lettering
    const auto titleW = roundToInt (GlyphArrangement::getStringWidth (labelFont (28), "OMNIARP"));
    title.setBounds (98, 12, titleW + 2, 30);

    algo.setBounds (98, 52, 195, 40);
    scaleDisp.setBounds (98, 122, 195, 40);

    const int knob = 80, firstX = 390, gap = 150, cy = 100;
    int i = 0;
    for (auto* k : knobs())
        k->slider.setBounds (Rectangle<int> (knob, knob).withCentre ({ firstX + gap * i++, cy }));

    transposeDisp.setBounds (Rectangle<int> (120, 40).withCentre ({ firstX + gap * i - 5, cy - 4 }));
    inKey.setBounds (transposeDisp.getX() + 22, transposeDisp.getY() - 26, 90, 20);

    about.setBounds (getLocalBounds());
}
} // namespace tross

//==============================================================================
OmniarpEditor::OmniarpEditor (OmniarpProcessor& p)
    : AudioProcessorEditor (p), proc (p), panel (p)
{
    setLookAndFeel (&lnf);
    panel.setLookAndFeel (&lnf);
    addAndMakeVisible (panel);

    constexpr int W = tross::MainPanel::W, H = tross::MainPanel::H;
    const double scale = jlimit (0.6, 2.0, (double) proc.apvts.state.getProperty ("uiScale", 1.0));
    setResizable (true, true);
    setResizeLimits (W * 6 / 10, H * 6 / 10, W * 2, H * 2);
    getConstrainer()->setFixedAspectRatio ((double) W / (double) H);

    setSize (roundToInt (W * scale), roundToInt (H * scale));
    ready = true;
}

OmniarpEditor::~OmniarpEditor()
{
    panel.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void OmniarpEditor::resized()
{
    const float scale = (float) getWidth() / (float) tross::MainPanel::W;
    panel.setTransform (AffineTransform::scale (scale));
    panel.setBounds (0, 0, tross::MainPanel::W, tross::MainPanel::H);
    if (ready && getWidth() > 0)
        proc.apvts.state.setProperty ("uiScale", scale, nullptr);   // remembered with the project
}
