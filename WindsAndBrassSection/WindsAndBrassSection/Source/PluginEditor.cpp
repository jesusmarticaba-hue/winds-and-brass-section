#include "PluginEditor.h"

using namespace juce;

namespace
{
// Siluetas estilizadas de cada instrumento
void drawGlyph (Graphics& g, int kind, Rectangle<float> r, Colour c)
{
    g.setColour (c);
    const float x = r.getX(), y = r.getY(), w = r.getWidth(), h = r.getHeight();
    const float cy = r.getCentreY();
    const PathStrokeType stroke (4.5f, PathStrokeType::curved, PathStrokeType::rounded);

    if (kind == 0) // trompeta
    {
        g.drawLine (x + w * 0.10f, cy + h * 0.10f, x + w * 0.62f, cy + h * 0.10f, 4.f);
        g.drawEllipse (x + w * 0.22f, cy - h * 0.28f, w * 0.34f, h * 0.38f, 3.5f);
        Path bell;
        bell.startNewSubPath (x + w * 0.62f, cy + h * 0.04f);
        bell.lineTo (x + w * 0.94f, cy - h * 0.26f);
        bell.lineTo (x + w * 0.94f, cy + h * 0.46f);
        bell.lineTo (x + w * 0.62f, cy + h * 0.16f);
        bell.closeSubPath();
        g.fillPath (bell);
    }
    else if (kind == 3) // trombón
    {
        g.drawLine (x + w * 0.14f, cy - h * 0.06f, x + w * 0.66f, cy - h * 0.06f, 4.f);
        g.drawLine (x + w * 0.14f, cy + h * 0.20f, x + w * 0.66f, cy + h * 0.20f, 4.f);
        g.drawLine (x + w * 0.14f, cy - h * 0.06f, x + w * 0.14f, cy + h * 0.20f, 4.f);
        g.drawLine (x + w * 0.40f, cy - h * 0.06f, x + w * 0.40f, cy + h * 0.20f, 3.f);
        Path bell;
        bell.startNewSubPath (x + w * 0.66f, cy - h * 0.10f);
        bell.lineTo (x + w * 0.94f, cy - h * 0.32f);
        bell.lineTo (x + w * 0.94f, cy + h * 0.46f);
        bell.lineTo (x + w * 0.66f, cy + h * 0.24f);
        bell.closeSubPath();
        g.fillPath (bell);
    }
    else // saxofones (1 alto, 2 tenor, 4 barítono)
    {
        const float s = (kind == 1) ? 0.82f : (kind == 2 ? 0.92f : 1.0f);
        const auto rr = r.withSizeKeepingCentre (w * s, h * s);
        const float X = rr.getX(), Y = rr.getY(), W = rr.getWidth(), H = rr.getHeight();
        Path sax;
        sax.startNewSubPath (X + W * 0.36f, Y);
        sax.cubicTo (X + W * 0.46f, Y + H * 0.30f, X + W * 0.44f, Y + H * 0.62f, X + W * 0.50f, Y + H * 0.80f);
        sax.cubicTo (X + W * 0.56f, Y + H * 0.98f, X + W * 0.86f, Y + H * 0.98f, X + W * 0.86f, Y + H * 0.60f);
        g.strokePath (sax, stroke);
        Path flare;
        flare.addTriangle (X + W * 0.76f, Y + H * 0.60f, X + W * 0.96f, Y + H * 0.60f, X + W * 1.0f, Y + H * 0.40f);
        g.fillPath (flare);
        if (kind == 4)
            g.drawEllipse (X + W * 0.30f, Y - H * 0.02f, W * 0.22f, H * 0.18f, 3.f);
    }
}

void styleKnob (Slider& s)
{
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
}
} // namespace

WbsEditor::WbsEditor (WbsProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&look);

    for (int i = 0; i < wbs::kNumPlayers; ++i)
    {
        const String id (wbs::specs()[(size_t) i].id);
        addAndMakeVisible (power[(size_t) i]);
        buttonAtt.push_back (std::make_unique<BA> (proc.apvts, "on_" + id, power[(size_t) i]));

        auto& f = fader[(size_t) i];
        f.setSliderStyle (Slider::LinearVertical);
        f.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (f);
        sliderAtt.push_back (std::make_unique<SA> (proc.apvts, "lvl_" + id, f));
    }

    const auto names = wbs::articulationNames();
    for (int i = 0; i < wbs::NumArticulations; ++i)
    {
        auto& b = artic[(size_t) i];
        b.setButtonText (names[i]);
        b.onClick = [this, i]
        {
            if (auto* prm = proc.apvts.getParameter ("artic"))
                prm->setValueNotifyingHost (prm->convertTo0to1 ((float) i));
        };
        addAndMakeVisible (b);
    }

    knobs = { &vibrato, &human, &bright, &reverb, &master };
    const char* ids[] = { "vibrato", "human", "bright", "reverb", "master" };
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        styleKnob (*knobs[i]);
        addAndMakeVisible (*knobs[i]);
        sliderAtt.push_back (std::make_unique<SA> (proc.apvts, ids[i], *knobs[i]));
    }

    mode.addItemList (StringArray { "Unison", "Poly Section", "Auto Section" }, 1);
    root.addItemList (wbs::rootNames(), 1);
    quality.addItemList (wbs::chordNames(), 1);
    for (auto* c : { &mode, &root, &quality })
        addAndMakeVisible (*c);
    comboAtt.push_back (std::make_unique<CA> (proc.apvts, "mode", mode));
    comboAtt.push_back (std::make_unique<CA> (proc.apvts, "root", root));
    comboAtt.push_back (std::make_unique<CA> (proc.apvts, "quality", quality));

    setSize (860, 580);
    startTimerHz (30);
}

WbsEditor::~WbsEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void WbsEditor::timerCallback()
{
    const int a = proc.shownArtic.load();
    for (int i = 0; i < wbs::NumArticulations; ++i)
        artic[(size_t) i].setToggleState (i == a, dontSendNotification);
    repaint();
}

void WbsEditor::resized()
{
    mode.setBounds (getWidth() - 200, 30, 165, 28);

    const int x0 = 35, y0 = 112, cw = 150, ch = 262, gap = 10;
    for (int i = 0; i < wbs::kNumPlayers; ++i)
    {
        const auto card = Rectangle<int> (x0 + i * (cw + gap), y0, cw, ch);
        cards[(size_t) i] = card;
        power[(size_t) i].setBounds (card.getX() + 14, card.getY() + 108, cw - 28, 26);
        fader[(size_t) i].setBounds (card.getCentreX() - 25, card.getY() + 140, 50, ch - 150);
    }

    const int ay = 412, aw = 120, ag = 10;
    const int ax = (getWidth() - (6 * aw + 5 * ag)) / 2;
    for (int i = 0; i < wbs::NumArticulations; ++i)
        artic[(size_t) i].setBounds (ax + i * (aw + ag), ay, aw, 34);

    for (size_t i = 0; i < knobs.size(); ++i)
        knobs[i]->setBounds (40 + (int) i * 100, 468, 84, 84);

    root.setBounds (585, 482, 70, 28);
    quality.setBounds (662, 482, 150, 28);
}

void WbsEditor::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    g.setGradientFill (ColourGradient (ui::bgTop, 0.f, 0.f, ui::bgBottom, 0.f, b.getBottom(), false));
    g.fillAll();

    // Rayos "sunburst" art déco desde el centro superior
    const Point<float> c (b.getCentreX(), -50.f);
    for (int i = 0; i < 40; ++i)
    {
        const float a = MathConstants<float>::pi * (0.06f + 0.88f * (float) i / 39.f);
        g.setColour (ui::gold.withAlpha (i % 2 ? 0.04f : 0.09f));
        g.drawLine (c.x, c.y, c.x + std::cos (a) * 1100.f, c.y + std::sin (a) * 1100.f, 1.2f);
    }

    // Título
    g.setColour (ui::gold);
    g.setFont (Font (FontOptions ("Georgia", 32.f, Font::bold)));
    g.drawText ("WINDS & BRASS SECTION", Rectangle<int> (0, 14, getWidth(), 44), Justification::centred);
    g.setColour (ui::cream.withAlpha (0.75f));
    g.setFont (Font (FontOptions (13.f)));
    g.drawText ("H O R N   &   R E E D   S E C T I O N   F O R   N O T A T I O N", Rectangle<int> (0, 58, getWidth(), 20),
                Justification::centred);

    // Filete decorativo con rombo
    const float ly = 88.f, mx = b.getCentreX();
    g.setColour (ui::copper);
    g.drawLine (40.f, ly, mx - 16.f, ly, 1.5f);
    g.drawLine (mx + 16.f, ly, b.getRight() - 40.f, ly, 1.5f);
    Path dia;
    dia.addQuadrilateral (mx - 10.f, ly, mx, ly - 7.f, mx + 10.f, ly, mx, ly + 7.f);
    g.setColour (ui::gold);
    g.fillPath (dia);

    // Tarjetas de intérpretes
    for (int i = 0; i < wbs::kNumPlayers; ++i)
    {
        const auto card = cards[(size_t) i].toFloat();
        const float act = jlimit (0.f, 1.f, proc.activity[(size_t) i].load());

        g.setGradientFill (ColourGradient (ui::panel.brighter (0.12f), card.getX(), card.getY(),
                                           ui::panel.darker (0.3f), card.getX(), card.getBottom(), false));
        g.fillRoundedRectangle (card, 12.f);

        if (act > 0.01f)
        {
            g.setColour (ui::glow.withAlpha (0.35f * act));
            g.drawRoundedRectangle (card.expanded (2.f), 14.f, 4.f);
        }
        g.setColour (act > 0.01f ? ui::glow.withAlpha (0.5f + 0.5f * act) : ui::copper.withAlpha (0.8f));
        g.drawRoundedRectangle (card, 12.f, 1.6f);

        drawGlyph (g, i, Rectangle<float> (card.getX() + 30.f, card.getY() + 14.f, card.getWidth() - 60.f, 58.f),
                   ui::gold.interpolatedWith (ui::glow, act * 0.8f));

        g.setColour (ui::cream);
        g.setFont (Font (FontOptions ("Georgia", 17.f, Font::bold)));
        g.drawText (String (wbs::specs()[(size_t) i].name).toUpperCase(),
                    Rectangle<float> (card.getX(), card.getY() + 78.f, card.getWidth(), 22.f), Justification::centred);
    }

    // Etiquetas
    g.setColour (ui::gold);
    g.setFont (Font (FontOptions (13.f, Font::bold)));
    g.drawText ("ARTICULATION", Rectangle<int> (35, 388, 300, 20), Justification::centredLeft);
    g.setColour (ui::cream.withAlpha (0.6f));
    g.setFont (Font (FontOptions (12.f)));
    g.drawText ("Keyswitches: MIDI 24-29 (C1-F1)", Rectangle<int> (getWidth() - 335, 388, 300, 20),
                Justification::centredRight);

    const char* labels[] = { "VIBRATO", "HUMANIZE", "BRIGHTNESS", "REVERB", "MASTER" };
    g.setColour (ui::gold);
    g.setFont (Font (FontOptions (12.f, Font::bold)));
    for (size_t i = 0; i < knobs.size(); ++i)
        g.drawText (labels[i], Rectangle<int> (40 + (int) i * 100 - 8, 554, 100, 18), Justification::centred);

    g.drawText ("AUTO SECTION CHORD", Rectangle<int> (585, 456, 230, 20), Justification::centredLeft);
    g.setColour (ui::cream.withAlpha (0.55f));
    g.setFont (Font (FontOptions (11.f)));
    g.drawText ("2+ notes: lowest = root", Rectangle<int> (585, 514, 230, 18), Justification::centredLeft);
}
