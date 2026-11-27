#include "PluginEditor.h"
#include <cmath>

//==============================================================================
// Look and feel
//==============================================================================
PautaLookAndFeel::PautaLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, Pauta::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
}

void PautaLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float pos, float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (2.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);

    // Arco de fondo y arco de valor
    const float arcR = radius - 3.0f;

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (Pauta::stroke);
    g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
    g.setColour (accent);
    g.strokePath (value, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Cuerpo de la perilla
    const float bodyR = radius - 11.0f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2e3945), centre.x, centre.y - bodyR,
                                             juce::Colour (0xff131920), centre.x, centre.y + bodyR, false));
    g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

    g.setColour (Pauta::stroke.brighter (0.35f));
    g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    // Indicador
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.6f, -bodyR + 3.0f, 3.2f, bodyR * 0.5f, 1.5f);
    g.setColour (Pauta::text);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

//==============================================================================
// Contenido
//==============================================================================
GateContent::GateContent (GateProcessor& p)
    : proc (p),
      typeAttachment (*p.apvts.getParameter (IDs::type), [this] (float v) { updateType (v > 0.5f); })
{
    setOpaque (true);

    pType  = proc.apvts.getRawParameterValue (IDs::type);
    pFloor = proc.apvts.getRawParameterValue (IDs::floorDb);
    pOpen  = proc.apvts.getRawParameterValue (IDs::thresholdOpen);
    pClose = proc.apvts.getRawParameterValue (IDs::thresholdClose);
    pRatio = proc.apvts.getRawParameterValue (IDs::ratio);

    struct KnobDef { const char* id; const char* label; juce::Colour colour; };

    const KnobDef defs[8] =
    {
        { IDs::floorDb,        "FLOOR",      Pauta::cyan   },
        { IDs::thresholdOpen,  "GATE OPEN",  Pauta::green  },
        { IDs::thresholdClose, "GATE CLOSE", Pauta::green  },
        { IDs::attack,         "ATTACK",     Pauta::amber  },
        { IDs::release,        "RELEASE",    Pauta::amber  },
        { IDs::hold,           "HOLD",       Pauta::amber  },
        { IDs::ratio,          "RATIO",      Pauta::violet },
        { IDs::output,         "OUTPUT",     Pauta::text   }
    };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.label = defs[i].label;

        auto& s = k.slider;
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 86, 18);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                               juce::MathConstants<float>::pi * 2.75f, true);
        s.setColour (juce::Slider::rotarySliderFillColourId, defs[i].colour);
        s.setColour (juce::Slider::textBoxTextColourId, Pauta::text);
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (s);

        k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, defs[i].id, s);
    }

    addAndMakeVisible (gateButton);
    addAndMakeVisible (expanderButton);
    addAndMakeVisible (bypassButton);

    gateButton.onClick     = [this] { typeAttachment.setValueAsCompleteGesture (0.0f); };
    expanderButton.onClick = [this] { typeAttachment.setValueAsCompleteGesture (1.0f); };

    bypassButton.setClickingTogglesState (true);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.apvts, IDs::bypass, bypassButton);

    typeAttachment.sendInitialUpdate();

    startTimerHz (30);
}

GateContent::~GateContent()
{
    stopTimer();
}

void GateContent::updateType (bool expander)
{
    gateButton.setToggleState (! expander, juce::dontSendNotification);
    expanderButton.setToggleState (expander, juce::dontSendNotification);

    // Ratio solo se usa en modo Expander
    auto& ratioSlider = knobs[6].slider;
    ratioSlider.setEnabled (expander);
    ratioSlider.setAlpha (expander ? 1.0f : 0.4f);

    repaint();
}

void GateContent::resized()
{
    gateButton.setBounds     (300, 14, 92, 28);
    expanderButton.setBounds (396, 14, 92, 28);
    bypassButton.setBounds   (640, 14, 100, 28);

    auto row = knobsArea.toNearestInt().reduced (0, 4);
    const int slotW = row.getWidth() / (int) knobs.size();

    for (auto& k : knobs)
    {
        k.slot = row.removeFromLeft (slotW);
        auto s = k.slot;
        s.removeFromTop (20);              // espacio para el nombre
        k.slider.setBounds (s);
    }
}

void GateContent::timerCallback()
{
    const float inLin  = proc.inPeak.exchange (0.0f);
    const float outLin = proc.outPeak.exchange (0.0f);
    const float gMin   = proc.minGain.exchange (1.0f);

    const float newIn  = juce::Decibels::gainToDecibels (inLin,  -100.0f);
    const float newOut = juce::Decibels::gainToDecibels (outLin, -100.0f);

    inDb  = juce::jmax (newIn,  inDb  - 2.5f);
    outDb = juce::jmax (newOut, outDb - 2.5f);

    const float grTarget = -juce::Decibels::gainToDecibels (gMin, -80.0f);   // 0 .. 80
    grDb = grTarget > grDb ? grTarget : juce::jmax (grTarget, grDb - 4.0f);

    gateOpenState = proc.gateIsOpen.load();

    repaint (graphArea.getSmallestIntegerContainer());
    repaint (metersArea.getSmallestIntegerContainer());
}

//------------------------------------------------------------------------------
void GateContent::paint (juce::Graphics& g)
{
    g.fillAll (Pauta::bg);

    paintHeader (g);
    paintGraph (g, graphArea);
    paintMeters (g, metersArea);

    // Panel de las perillas
    g.setColour (Pauta::panel);
    g.fillRoundedRectangle (knobsArea, 8.0f);
    g.setColour (Pauta::stroke);
    g.drawRoundedRectangle (knobsArea, 8.0f, 1.0f);

    g.setFont (Pauta::font (11.0f, true));
    for (auto& k : knobs)
    {
        auto labelArea = k.slot.toFloat().removeFromTop (22.0f);
        g.setColour (k.slider.isEnabled() ? Pauta::dim.brighter (0.4f) : Pauta::dim.withAlpha (0.5f));
        g.drawText (k.label, labelArea, juce::Justification::centred, false);
    }
}

void GateContent::paintHeader (juce::Graphics& g)
{
    g.setFont (Pauta::font (20.0f, true));
    g.setColour (Pauta::amber);
    g.drawText ("LA PAUTA", 20, 8, 110, 26, juce::Justification::centredLeft, false);

    g.setColour (Pauta::text);
    g.drawText ("GATE C1", 126, 8, 110, 26, juce::Justification::centredLeft, false);

    g.setFont (Pauta::font (10.0f, true));
    g.setColour (Pauta::dim);
    g.drawText ("NOISE GATE / EXPANDER", 20, 34, 200, 14, juce::Justification::centredLeft, false);

    g.setColour (Pauta::stroke);
    g.drawLine (0.0f, 56.0f, (float) Pauta::baseWidth, 56.0f, 1.0f);
}

void GateContent::paintGraph (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (Pauta::panel);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (Pauta::stroke);
    g.drawRoundedRectangle (r, 8.0f, 1.0f);

    constexpr float minDb = -80.0f, maxDb = 0.0f;

    const auto plot = r.reduced (34.0f, 24.0f);

    auto xOf = [&] (float db)
    {
        return plot.getX() + (juce::jlimit (minDb, maxDb, db) - minDb) / (maxDb - minDb) * plot.getWidth();
    };
    auto yOf = [&] (float db)
    {
        return plot.getBottom() - (juce::jlimit (minDb, maxDb, db) - minDb) / (maxDb - minDb) * plot.getHeight();
    };

    // Rejilla y escalas
    g.setFont (Pauta::font (10.0f));
    for (int db = -80; db <= 0; db += 20)
    {
        const float px = xOf ((float) db);
        const float py = yOf ((float) db);

        g.setColour (Pauta::stroke.withAlpha (0.8f));
        g.drawLine (px, plot.getY(), px, plot.getBottom(), 1.0f);
        g.drawLine (plot.getX(), py, plot.getRight(), py, 1.0f);

        g.setColour (Pauta::dim);
        g.drawText (juce::String (db), juce::Rectangle<float> (px - 16.0f, plot.getBottom() + 4.0f, 32.0f, 12.0f),
                    juce::Justification::centred, false);
        g.drawText (juce::String (db), juce::Rectangle<float> (r.getX() + 4.0f, py - 6.0f, 26.0f, 12.0f),
                    juce::Justification::centredRight, false);
    }

    g.setFont (Pauta::font (10.0f, true));
    g.setColour (Pauta::dim);
    g.drawText ("TRANSFER CURVE  (IN dB  >  OUT dB)", juce::Rectangle<float> (r.getX() + 12.0f, r.getY() + 4.0f, 260.0f, 14.0f),
                juce::Justification::centredLeft, false);

    // Parámetros actuales
    const bool  expander = pType->load() > 0.5f;
    const float floorDb  = pFloor->load();
    const bool  floorInf = floorDb <= -79.9f;
    const float openDb   = pOpen->load();
    const float closeDb  = juce::jmin (pClose->load(), openDb);
    const float ratio    = juce::jmax (1.0f, pRatio->load());

    // Línea de ganancia unitaria
    {
        juce::Path unity;
        unity.startNewSubPath (xOf (minDb), yOf (minDb));
        unity.lineTo (xOf (maxDb), yOf (maxDb));

        juce::Path dashed;
        const float dashes[2] = { 4.0f, 4.0f };
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, unity, dashes, 2);
        g.setColour (Pauta::dim.withAlpha (0.45f));
        g.fillPath (dashed);
    }

    // Marcas de umbral: Gate Open / Gate Close
    {
        const float xo = xOf (openDb);
        const float xc = xOf (closeDb);

        g.setColour (Pauta::green.withAlpha (0.85f));
        g.drawLine (xo, plot.getY(), xo, plot.getBottom(), 1.2f);

        juce::Path closeLine;
        closeLine.startNewSubPath (xc, plot.getY());
        closeLine.lineTo (xc, plot.getBottom());
        juce::Path dashed;
        const float dashes[2] = { 3.0f, 3.0f };
        juce::PathStrokeType (1.2f).createDashedStroke (dashed, closeLine, dashes, 2);
        g.setColour (Pauta::green.withAlpha (0.55f));
        g.fillPath (dashed);

        g.setFont (Pauta::font (9.0f, true));
        g.setColour (Pauta::green);
        g.drawText ("OPEN",  juce::Rectangle<float> (xo + 3.0f,  plot.getY() + 1.0f, 40.0f, 11.0f), juce::Justification::centredLeft,  false);
        g.setColour (Pauta::green.withAlpha (0.7f));
        g.drawText ("CLOSE", juce::Rectangle<float> (xc - 43.0f, plot.getY() + 1.0f, 40.0f, 11.0f), juce::Justification::centredRight, false);
    }

    // Curva de transferencia
    auto closedOut = [&] (float x)
    {
        if (! expander)
            return floorInf ? -200.0f : x + floorDb;

        float red = juce::jmax (0.0f, closeDb - x) * (ratio - 1.0f);
        red = juce::jmin (red, floorInf ? 200.0f : -floorDb);
        return x - red;
    };

    juce::Path curve;
    bool started = false;
    auto addPoint = [&] (float inDbValue, float outDbValue)
    {
        const float px = xOf (inDbValue);
        const float py = yOf (outDbValue);
        if (! started) { curve.startNewSubPath (px, py); started = true; }
        else           { curve.lineTo (px, py); }
    };

    bool jumped = false;
    for (float x = minDb; x <= maxDb + 0.01f; x += 0.5f)
    {
        if (! jumped && x >= openDb)
        {
            addPoint (openDb, closedOut (openDb));   // fin del tramo cerrado
            addPoint (openDb, openDb);               // salto de apertura
            jumped = true;
        }

        addPoint (x, x < openDb ? closedOut (x) : x);
    }

    g.setColour (Pauta::cyan.withAlpha (0.22f));
    g.strokePath (curve, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (Pauta::cyan);
    g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Punto en vivo (nivel de entrada contra nivel de salida)
    if (inDb > -79.0f)
    {
        const float lx = xOf (inDb);
        const float ly = yOf (juce::jmax (outDb, minDb));

        g.setColour (Pauta::amber.withAlpha (0.3f));
        g.fillEllipse (lx - 8.0f, ly - 8.0f, 16.0f, 16.0f);
        g.setColour (Pauta::amber);
        g.fillEllipse (lx - 4.0f, ly - 4.0f, 8.0f, 8.0f);
    }
}

void GateContent::drawBar (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& name,
                           float value, float lo, float hi, juce::Colour colour, bool fromRight,
                           const juce::String& valueText)
{
    const auto labelArea = area.removeFromLeft (30.0f);
    const auto valueArea = area.removeFromRight (58.0f);

    g.setColour (Pauta::dim);
    g.setFont (Pauta::font (11.0f, true));
    g.drawText (name, labelArea, juce::Justification::centredLeft, false);

    g.setColour (Pauta::text);
    g.setFont (Pauta::font (11.0f));
    g.drawText (valueText, valueArea, juce::Justification::centredRight, false);

    const auto bar = area.reduced (0.0f, 3.0f);
    g.setColour (Pauta::bg);
    g.fillRoundedRectangle (bar, 3.0f);

    const float prop = juce::jlimit (0.0f, 1.0f, (value - lo) / (hi - lo));
    auto fill = bar.reduced (1.5f);
    const auto filled = fromRight ? fill.removeFromRight (fill.getWidth() * prop)
                                  : fill.removeFromLeft  (fill.getWidth() * prop);

    g.setColour (colour);
    g.fillRoundedRectangle (filled, 2.0f);

    g.setColour (Pauta::stroke);
    g.drawRoundedRectangle (bar, 3.0f, 1.0f);
}

void GateContent::paintMeters (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (Pauta::panel);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (Pauta::stroke);
    g.drawRoundedRectangle (r, 8.0f, 1.0f);

    auto inner = r.reduced (16.0f, 12.0f);

    g.setColour (Pauta::dim);
    g.setFont (Pauta::font (10.0f, true));
    g.drawText ("LEVELS", inner.removeFromTop (14.0f), juce::Justification::centredLeft, false);
    inner.removeFromTop (8.0f);

    auto dbString = [] (float db) { return db < -79.0f ? juce::String ("-inf") : juce::String (db, 1) + " dB"; };

    drawBar (g, inner.removeFromTop (22.0f), "IN",  inDb,  -80.0f, 0.0f, Pauta::cyan, false, dbString (inDb));
    inner.removeFromTop (12.0f);
    drawBar (g, inner.removeFromTop (22.0f), "OUT", outDb, -80.0f, 0.0f, Pauta::cyan, false, dbString (outDb));
    inner.removeFromTop (12.0f);
    drawBar (g, inner.removeFromTop (22.0f), "GR",  grDb,   0.0f, 60.0f, Pauta::red,  true,
             "-" + juce::String (grDb, 1) + " dB");

    // Estado de la puerta
    auto ledArea = inner.removeFromBottom (54.0f);
    g.setColour (Pauta::bg);
    g.fillRoundedRectangle (ledArea, 6.0f);
    g.setColour (Pauta::stroke);
    g.drawRoundedRectangle (ledArea, 6.0f, 1.0f);

    const auto ledColour = gateOpenState ? Pauta::green : Pauta::red.withAlpha (0.55f);
    const auto c = ledArea.getCentre();
    const float ledX = ledArea.getX() + 34.0f;

    if (gateOpenState)
    {
        g.setColour (ledColour.withAlpha (0.25f));
        g.fillEllipse (ledX - 14.0f, c.y - 14.0f, 28.0f, 28.0f);
    }
    g.setColour (ledColour);
    g.fillEllipse (ledX - 8.0f, c.y - 8.0f, 16.0f, 16.0f);

    g.setColour (gateOpenState ? Pauta::green : Pauta::dim.brighter (0.2f));
    g.setFont (Pauta::font (16.0f, true));
    g.drawText (gateOpenState ? "GATE OPEN" : "GATE CLOSED",
                juce::Rectangle<float> (ledX + 22.0f, ledArea.getY(), ledArea.getWidth() - 70.0f, ledArea.getHeight()),
                juce::Justification::centredLeft, false);
}

//==============================================================================
// Editor
//==============================================================================
GateEditor::GateEditor (GateProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p), content (p)
{
    setLookAndFeel (&laf);

    addAndMakeVisible (content);

    // Las cuatro esquinas cambian el tamaño
    for (size_t i = 0; i < grips.size(); ++i)
    {
        auto grip = std::make_unique<CornerGrip> (static_cast<CornerGrip::Corner> (i));
        auto* raw = grip.get();

        grip->onPress = [this] { dragStartWidth = getWidth(); };
        grip->onDrag  = [this, raw] (int dx, int dy) { resizeFromGrip (raw->getCorner(), dx, dy); };

        addAndMakeVisible (*grip);
        grips[i] = std::move (grip);
    }

    // Si el DAW ofrece su propio borde para redimensionar, mantiene la misma proporción
    constrainer.setFixedAspectRatio (Pauta::aspect);
    constrainer.setSizeLimits (Pauta::minWidth, (int) std::lround (Pauta::minWidth / Pauta::aspect),
                               Pauta::maxWidth, (int) std::lround (Pauta::maxWidth / Pauta::aspect));
    setConstrainer (&constrainer);
    setResizable (true, false);

    const int w = juce::jlimit (Pauta::minWidth, Pauta::maxWidth, proc.uiWidth);
    setSize (w, (int) std::lround (w / Pauta::aspect));
}

GateEditor::~GateEditor()
{
    setLookAndFeel (nullptr);
}

void GateEditor::paint (juce::Graphics& g)
{
    g.fillAll (Pauta::bg);
}

void GateEditor::resized()
{
    const float scale = (float) getWidth() / (float) Pauta::baseWidth;

    content.setTransform (juce::AffineTransform::scale (scale));
    content.setBounds (0, 0, Pauta::baseWidth, Pauta::baseHeight);

    const int s = 22;
    const int w = getWidth();
    const int h = getHeight();

    grips[CornerGrip::topLeft]    ->setBounds (0,     0,     s, s);
    grips[CornerGrip::topRight]   ->setBounds (w - s, 0,     s, s);
    grips[CornerGrip::bottomLeft] ->setBounds (0,     h - s, s, s);
    grips[CornerGrip::bottomRight]->setBounds (w - s, h - s, s, s);

    proc.uiWidth = w;
}

void GateEditor::resizeFromGrip (CornerGrip::Corner corner, int dx, int dy)
{
    const bool left = (corner == CornerGrip::topLeft  || corner == CornerGrip::bottomLeft);
    const bool top  = (corner == CornerGrip::topLeft  || corner == CornerGrip::topRight);

    // Arrastrar hacia afuera de la esquina agranda; hacia adentro achica.
    const double deltaW = left ? -dx : dx;
    const double deltaH = (top ? -dy : dy) * Pauta::aspect;   // alto convertido a ancho

    const double delta = std::abs (deltaW) >= std::abs (deltaH) ? deltaW : deltaH;

    const int newW = juce::jlimit (Pauta::minWidth, Pauta::maxWidth, (int) std::lround (dragStartWidth + delta));
    const int newH = (int) std::lround (newW / Pauta::aspect);

    setSize (newW, newH);
}
