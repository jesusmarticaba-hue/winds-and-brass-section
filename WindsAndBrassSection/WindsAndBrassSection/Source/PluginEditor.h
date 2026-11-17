#pragma once

#include "PluginProcessor.h"

// Estética original: "art déco" en azul medianoche, latón bruñido y destellos turquesa.
namespace ui
{
const juce::Colour bgTop    { 0xff0b1720 };
const juce::Colour bgBottom { 0xff16313c };
const juce::Colour panel    { 0xff102530 };
const juce::Colour gold     { 0xffe0ae4a };
const juce::Colour copper   { 0xffb86b2b };
const juce::Colour cream    { 0xfff2e6c4 };
const juce::Colour glow     { 0xff3fd0b8 };
}

struct WbsLook : public juce::LookAndFeel_V4
{
    WbsLook()
    {
        setColour (juce::ComboBox::backgroundColourId, ui::panel);
        setColour (juce::ComboBox::textColourId, ui::cream);
        setColour (juce::ComboBox::outlineColourId, ui::copper);
        setColour (juce::ComboBox::arrowColourId, ui::gold);
        setColour (juce::PopupMenu::backgroundColourId, ui::panel);
        setColour (juce::PopupMenu::textColourId, ui::cream);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::copper);
        setColour (juce::PopupMenu::highlightedTextColourId, ui::cream);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float a0, float a1, juce::Slider&) override
    {
        auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.f);
        const float d = juce::jmin (r.getWidth(), r.getHeight());
        const auto c = r.getCentre();
        const float angle = a0 + pos * (a1 - a0);

        juce::Path track, value;
        track.addCentredArc (c.x, c.y, d / 2, d / 2, 0.f, a0, a1, true);
        value.addCentredArc (c.x, c.y, d / 2, d / 2, 0.f, a0, angle, true);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.strokePath (track, juce::PathStrokeType (4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (ui::gold);
        g.strokePath (value, juce::PathStrokeType (4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float kd = d * 0.68f;
        g.setGradientFill (juce::ColourGradient (ui::gold.brighter (0.2f), c.x - kd * 0.3f, c.y - kd * 0.3f,
                                                 ui::copper.darker (0.6f), c.x + kd * 0.4f, c.y + kd * 0.4f, true));
        g.fillEllipse (c.x - kd / 2, c.y - kd / 2, kd, kd);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawEllipse (c.x - kd / 2, c.y - kd / 2, kd, kd, 1.2f);

        juce::Path p;
        p.addRoundedRectangle (-1.8f, -kd / 2 + 2.f, 3.6f, kd * 0.28f, 1.5f);
        g.setColour (ui::bgTop);
        g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                           const juce::Slider::SliderStyle, juce::Slider&) override
    {
        const float cx = (float) x + (float) w * 0.5f;
        const float top = (float) y + 6.f, bottom = (float) (y + h) - 6.f;

        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (cx - 3.f, top, 6.f, bottom - top, 3.f);

        g.setGradientFill (juce::ColourGradient (ui::gold, cx, pos, ui::copper, cx, bottom, false));
        g.fillRoundedRectangle (cx - 3.f, pos, 6.f, bottom - pos, 3.f);

        auto thumb = juce::Rectangle<float> (cx - 15.f, pos - 7.f, 30.f, 14.f);
        g.setGradientFill (juce::ColourGradient (ui::cream, thumb.getX(), thumb.getY(),
                                                 ui::gold.darker (0.3f), thumb.getX(), thumb.getBottom(), false));
        g.fillRoundedRectangle (thumb, 4.f);
        g.setColour (ui::bgTop.withAlpha (0.7f));
        g.drawLine (thumb.getX() + 5.f, pos, thumb.getRight() - 5.f, pos, 1.2f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool) override
    {
        auto r = b.getLocalBounds().toFloat();
        const bool on = b.getToggleState();
        const auto lamp = juce::Rectangle<float> (r.getX() + 4.f, r.getCentreY() - 8.f, 16.f, 16.f);

        if (on)
        {
            g.setColour (ui::glow.withAlpha (0.25f));
            g.fillEllipse (lamp.expanded (5.f));
        }
        g.setColour (on ? ui::glow : ui::panel.brighter (0.2f));
        g.fillEllipse (lamp);
        g.setColour (ui::copper);
        g.drawEllipse (lamp, 1.5f);

        g.setColour (over ? ui::cream : ui::gold);
        g.setFont (juce::Font (juce::FontOptions (13.f, juce::Font::bold)));
        g.drawText (on ? "ACTIVE" : "MUTED", r.withTrimmedLeft (28.f), juce::Justification::centredLeft);
    }
};

// Botón en forma de píldora para cada articulación
class ArticButton : public juce::Button
{
public:
    ArticButton() : juce::Button ("artic") {}

    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        auto b = getLocalBounds().toFloat().reduced (2.f);
        const bool on = getToggleState();

        if (on)
        {
            g.setColour (ui::glow.withAlpha (0.25f));
            g.fillRoundedRectangle (b.expanded (2.f), 15.f);
        }
        g.setGradientFill (on ? juce::ColourGradient (ui::gold, 0.f, b.getY(), ui::copper, 0.f, b.getBottom(), false)
                              : juce::ColourGradient (ui::panel.brighter (0.15f), 0.f, b.getY(), ui::panel, 0.f, b.getBottom(), false));
        g.fillRoundedRectangle (b, 14.f);
        g.setColour (over ? ui::cream : ui::copper);
        g.drawRoundedRectangle (b, 14.f, 1.5f);

        g.setColour (on ? ui::bgTop : ui::cream);
        g.setFont (juce::Font (juce::FontOptions (14.f, juce::Font::bold)));
        g.drawText (getButtonText(), b, juce::Justification::centred);
    }
};

class WbsEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit WbsEditor (WbsProcessor&);
    ~WbsEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    WbsProcessor& proc;
    WbsLook look;

    std::array<juce::ToggleButton, wbs::kNumPlayers> power;
    std::array<juce::Slider, wbs::kNumPlayers> fader;
    std::array<ArticButton, wbs::NumArticulations> artic;
    juce::Slider vibrato, human, bright, reverb, master;
    juce::ComboBox mode, root, quality;
    std::array<juce::Slider*, 5> knobs;
    std::array<juce::Rectangle<int>, wbs::kNumPlayers> cards;

    // Los attachments se declaran al final para destruirse antes que los componentes
    std::vector<std::unique_ptr<SA>> sliderAtt;
    std::vector<std::unique_ptr<BA>> buttonAtt;
    std::vector<std::unique_ptr<CA>> comboAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WbsEditor)
};
