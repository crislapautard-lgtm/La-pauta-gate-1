#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <functional>
#include <memory>
#include "PluginProcessor.h"

//==============================================================================
// Colores y medidas de la interfaz (diseño propio de La Pauta)
namespace Pauta
{
    inline const juce::Colour bg     { 0xff0f1318 };
    inline const juce::Colour panel  { 0xff171d24 };
    inline const juce::Colour stroke { 0xff2a343f };
    inline const juce::Colour text   { 0xffe8eef4 };
    inline const juce::Colour dim    { 0xff8493a2 };
    inline const juce::Colour amber  { 0xffffb02e };
    inline const juce::Colour cyan   { 0xff3fd0ff };
    inline const juce::Colour green  { 0xff46e08a };
    inline const juce::Colour red    { 0xffff5a4d };
    inline const juce::Colour violet { 0xffb58cff };

    // Tamaño base del diseño. La ventana se escala a partir de este tamaño.
    constexpr int baseWidth  = 760;
    constexpr int baseHeight = 440;
    constexpr double aspect  = (double) baseWidth / (double) baseHeight;

    // Límites de tamaño de la ventana (75 % a 200 %)
    constexpr int minWidth  = 570;
    constexpr int maxWidth  = 1520;

    inline juce::Font font (float height, bool bold = false)
    {
        return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
    }
}

//==============================================================================
class PautaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PautaLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    juce::Font getLabelFont (juce::Label&) override { return Pauta::font (12.0f); }
};

//==============================================================================
// Botón en forma de píldora (Gate / Expander / Bypass)
class PillButton : public juce::Button
{
public:
    PillButton (const juce::String& label, juce::Colour accentColour)
        : juce::Button (label), accent (accentColour) {}

    void paintButton (juce::Graphics& g, bool isMouseOver, bool isButtonDown) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool on = getToggleState();
        const float radius = r.getHeight() * 0.5f;

        juce::Colour fill = Pauta::panel;
        if (on)              fill = accent.withAlpha (0.92f);
        else if (isButtonDown) fill = Pauta::stroke.brighter (0.2f);
        else if (isMouseOver)  fill = Pauta::panel.brighter (0.2f);

        g.setColour (fill);
        g.fillRoundedRectangle (r, radius);

        g.setColour (on ? accent : Pauta::stroke.brighter (0.3f));
        g.drawRoundedRectangle (r, radius, 1.2f);

        g.setColour (on ? Pauta::bg : Pauta::dim.brighter (0.3f));
        g.setFont (Pauta::font (12.0f, true));
        g.drawText (getButtonText(), r, juce::Justification::centred, false);
    }

private:
    juce::Colour accent;
};

//==============================================================================
// Agarradera de esquina: arrastrarla cambia el tamaño del plugin
class CornerGrip : public juce::Component
{
public:
    enum Corner { topLeft = 0, topRight, bottomLeft, bottomRight };

    explicit CornerGrip (Corner c) : corner (c)
    {
        switch (c)
        {
            case topLeft:     setMouseCursor (juce::MouseCursor::TopLeftCornerResizeCursor);     break;
            case topRight:    setMouseCursor (juce::MouseCursor::TopRightCornerResizeCursor);    break;
            case bottomLeft:  setMouseCursor (juce::MouseCursor::BottomLeftCornerResizeCursor);  break;
            case bottomRight: setMouseCursor (juce::MouseCursor::BottomRightCornerResizeCursor); break;
        }
    }

    Corner getCorner() const noexcept { return corner; }

    std::function<void()> onPress;
    std::function<void (int dx, int dy)> onDrag;   // desplazamiento del ratón desde que se pulsó

    void paint (juce::Graphics& g) override
    {
        const bool left = (corner == topLeft || corner == bottomLeft);
        const bool top  = (corner == topLeft || corner == topRight);

        const float w = (float) getWidth();
        const float h = (float) getHeight();
        const float x = left ? 5.0f : w - 5.0f;
        const float y = top  ? 5.0f : h - 5.0f;
        const float dx = left ? 9.0f : -9.0f;
        const float dy = top  ? 9.0f : -9.0f;

        juce::Path p;
        p.startNewSubPath (x, y + dy);
        p.lineTo (x, y);
        p.lineTo (x + dx, y);

        g.setColour (hover ? Pauta::amber : Pauta::dim.withAlpha (0.7f));
        g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void mouseEnter (const juce::MouseEvent&) override { hover = true;  repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { hover = false; repaint(); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        pressPos = e.getScreenPosition();
        if (onPress) onPress();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        const auto d = e.getScreenPosition() - pressPos;
        if (onDrag) onDrag (d.x, d.y);
    }

private:
    Corner corner;
    bool hover = false;
    juce::Point<int> pressPos;
};

//==============================================================================
// Contenido del plugin con tamaño fijo de diseño (760 x 440). El editor lo escala.
class GateContent : public juce::Component, private juce::Timer
{
public:
    explicit GateContent (GateProcessor&);
    ~GateContent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateType (bool expander);

    void paintHeader (juce::Graphics&);
    void paintGraph  (juce::Graphics&, juce::Rectangle<float> area);
    void paintMeters (juce::Graphics&, juce::Rectangle<float> area);
    void drawBar (juce::Graphics&, juce::Rectangle<float> area, const juce::String& name,
                  float value, float lo, float hi, juce::Colour colour, bool fromRight,
                  const juce::String& valueText);

    struct Knob
    {
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        juce::String label;
        juce::Rectangle<int> slot;
    };

    GateProcessor& proc;

    std::array<Knob, 8> knobs;

    PillButton gateButton     { "GATE",     Pauta::cyan };
    PillButton expanderButton { "EXPANDER", Pauta::cyan };
    PillButton bypassButton   { "BYPASS",   Pauta::red  };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    juce::ParameterAttachment typeAttachment;

    juce::Rectangle<float> graphArea  { 20.0f, 68.0f, 400.0f, 240.0f };
    juce::Rectangle<float> metersArea { 436.0f, 68.0f, 304.0f, 240.0f };
    juce::Rectangle<float> knobsArea  { 20.0f, 316.0f, 720.0f, 108.0f };

    // Valores que se muestran en los medidores
    float inDb = -100.0f, outDb = -100.0f, grDb = 0.0f;
    bool gateOpenState = false;

    std::atomic<float>* pType  = nullptr;
    std::atomic<float>* pFloor = nullptr;
    std::atomic<float>* pOpen  = nullptr;
    std::atomic<float>* pClose = nullptr;
    std::atomic<float>* pRatio = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GateContent)
};

//==============================================================================
class GateEditor : public juce::AudioProcessorEditor
{
public:
    explicit GateEditor (GateProcessor&);
    ~GateEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void resizeFromGrip (CornerGrip::Corner corner, int dx, int dy);

    GateProcessor& proc;

    PautaLookAndFeel laf;           // debe vivir más que los componentes
    GateContent content;
    std::array<std::unique_ptr<CornerGrip>, 4> grips;
    juce::ComponentBoundsConstrainer constrainer;

    int dragStartWidth = Pauta::baseWidth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GateEditor)
};
