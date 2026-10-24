#include "PluginEditor.h"

#include <BinaryData.h>

namespace
{
constexpr int kWidth = 900;
constexpr int kHeight = 640;
constexpr int kFloorTop = 488;

const juce::Colour kPink (0xffff7eb6);
const juce::Colour kHotPink (0xffff4f9f);
const juce::Colour kLavender (0xffb69cff);
const juce::Colour kButter (0xffffe07a);

juce::Path makeHeart (float cx, float cy, float size)
{
    juce::Path p;
    p.startNewSubPath (0.0f, 0.6f);
    p.cubicTo (-1.2f, -0.2f, -0.8f, -1.2f, 0.0f, -0.6f);
    p.cubicTo (0.8f, -1.2f, 1.2f, -0.2f, 0.0f, 0.6f);
    p.closeSubPath();
    p.applyTransform (juce::AffineTransform::scale (size).translated (cx, cy));
    return p;
}

juce::Path makeSparkle (float cx, float cy, float size)
{
    juce::Path p;
    p.startNewSubPath (0.0f, -1.0f);
    p.quadraticTo (0.14f, -0.14f, 1.0f, 0.0f);
    p.quadraticTo (0.14f, 0.14f, 0.0f, 1.0f);
    p.quadraticTo (-0.14f, 0.14f, -1.0f, 0.0f);
    p.quadraticTo (-0.14f, -0.14f, 0.0f, -1.0f);
    p.closeSubPath();
    p.applyTransform (juce::AffineTransform::scale (size).translated (cx, cy));
    return p;
}

std::unique_ptr<juce::Drawable> loadSvg (const char* data, int size)
{
    if (auto xml = juce::XmlDocument::parse (juce::String::fromUTF8 (data, size)))
        return juce::Drawable::createFromSVG (*xml);
    return nullptr;
}

juce::Font boldFont (float height)
{
    return juce::Font (juce::FontOptions (height, juce::Font::bold));
}
} // namespace

//==============================================================================
KawaiiLookAndFeel::KawaiiLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, juce::Colours::white);
}

void KawaiiLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float pos, float startAngle, float endAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
    const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto centre = bounds.getCentre();
    const float radius = diameter * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // glowing track
    const float trackR = radius - 3.0f;
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, trackR, trackR, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff5b4a6e));
    g.strokePath (track, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, trackR, trackR, 0.0f, startAngle, angle, true);
    g.setColour (kPink.withAlpha (0.35f));
    g.strokePath (value, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (kPink);
    g.strokePath (value, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // candy knob body
    const float bodyR = radius * 0.66f;
    const auto body = juce::Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (centre);
    g.setColour (juce::Colour (0x40000000));
    g.fillEllipse (body.translated (0.0f, 3.0f));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffe3f1), centre.x, body.getY(),
                                             juce::Colour (0xffff9fcd), centre.x, body.getBottom(), false));
    g.fillEllipse (body);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawEllipse (body.reduced (1.0f), 2.5f);

    // gloss
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.75f), centre.x, body.getY(),
                                             juce::Colours::white.withAlpha (0.0f), centre.x, centre.y, false));
    g.fillEllipse (body.reduced (bodyR * 0.22f, 0.0f).withHeight (bodyR * 0.95f).translated (0.0f, bodyR * 0.08f));

    // pointer with a little heart on the end
    const float s = std::sin (angle), c = std::cos (angle);
    const juce::Point<float> tip (centre.x + s * bodyR * 0.62f, centre.y - c * bodyR * 0.62f);
    g.setColour (kHotPink);
    g.drawLine (centre.x + s * bodyR * 0.18f, centre.y - c * bodyR * 0.18f, tip.x, tip.y, 3.5f);
    g.setColour (juce::Colours::white);
    g.fillPath (makeHeart (tip.x, tip.y, bodyR * 0.30f));
    g.setColour (kHotPink);
    g.fillPath (makeHeart (tip.x, tip.y, bodyR * 0.22f));
}

//==============================================================================
AvasEditor::AvasEditor (AvasProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lookAndFeel);
    setSize (kWidth, kHeight);
    setResizable (false, false);

    static const char* ids[5] = { "volume", "sparkle", "sustain", "room", "width" };
    static const char* names[5] = { "Volume", "Sparkle", "Sustain", "Room", "Width" };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 86, 22);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                      juce::MathConstants<float>::pi * 2.8f, true);
        k.slider.setPopupDisplayEnabled (false, false, this);
        addAndMakeVisible (k.slider);

        k.label.setText (names[i], juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setFont (boldFont (16.0f));
        k.label.setColour (juce::Label::textColourId, juce::Colour (0xffffd6ea));
        addAndMakeVisible (k.label);

        k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.apvts, ids[i], k.slider);
    }

    keyboard.setAvailableRange (21, 108);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setKeyWidth (16.5f);
    keyboard.setLowestVisibleKey (21);
    keyboard.setVelocity (0.75f, true);
    keyboard.setWantsKeyboardFocus (true);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xfffffbfd));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff35283f));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xffe9c6da));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour (0x66ffc4e3));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour (0xaaff5fa8));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colour (0x40a070a0));
    addAndMakeVisible (keyboard);

    backgroundDrawable = loadSvg (BinaryData::background_svg, BinaryData::background_svgSize);
    girlDrawable = loadSvg (BinaryData::ava_girl_svg, BinaryData::ava_girl_svgSize);
    renderArt();

    startTimerHz (30);
}

AvasEditor::~AvasEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void AvasEditor::renderArt()
{
    const float scale = 2.0f; // sharp on high-DPI screens
    girlBounds = { 26.0f, 92.0f, 291.0f, 400.0f };

    backgroundImage = juce::Image (juce::Image::ARGB, (int) (kWidth * scale), (int) (kHeight * scale), true);
    if (backgroundDrawable != nullptr)
    {
        juce::Graphics g (backgroundImage);
        backgroundDrawable->drawWithin (g, backgroundImage.getBounds().toFloat(),
                                        juce::RectanglePlacement::stretchToFit, 1.0f);
    }

    girlImage = juce::Image (juce::Image::ARGB, (int) (girlBounds.getWidth() * scale),
                             (int) (girlBounds.getHeight() * scale), true);
    if (girlDrawable != nullptr)
    {
        juce::Graphics g (girlImage);
        girlDrawable->drawWithin (g, girlImage.getBounds().toFloat(),
                                  juce::RectanglePlacement::stretchToFit, 1.0f);
    }
}

//==============================================================================
void AvasEditor::resized()
{
    panelBounds = { 352, 128, 520, 268 };

    const int colW = (panelBounds.getWidth() - 24) / (int) knobs.size();
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int x = panelBounds.getX() + 12 + (int) i * colW;
        knobs[i].label.setBounds (x, panelBounds.getY() + 58, colW, 24);
        knobs[i].slider.setBounds (x, panelBounds.getY() + 84, colW, 168);
    }

    keyboard.setBounds (21, 506, 858, 122);
}

//==============================================================================
void AvasEditor::drawTitle (juce::Graphics& g)
{
    juce::GlyphArrangement glyphs;
    glyphs.addJustifiedText (boldFont (58.0f), "Ava's Grand Piano", 0.0f, 62.0f,
                             (float) kWidth, juce::Justification::centred);
    juce::Path text;
    glyphs.createPath (text);

    // soft shadow, white sticker outline, candy gradient fill
    g.setColour (juce::Colour (0x558040a0));
    g.strokePath (text, juce::PathStrokeType (11.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                  juce::AffineTransform::translation (0.0f, 5.0f));
    g.setColour (juce::Colours::white);
    g.strokePath (text, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff7eb6), 0.0f, 20.0f,
                                             juce::Colour (0xffa97bff), 0.0f, 68.0f, false));
    g.fillPath (text);

    // little decorations either side of the title
    const auto bounds = text.getBounds();
    g.setColour (kButter);
    g.fillPath (makeSparkle (bounds.getX() - 30.0f, 44.0f + 4.0f * std::sin (phase), 14.0f));
    g.fillPath (makeSparkle (bounds.getRight() + 30.0f, 44.0f + 4.0f * std::sin (phase + 2.0f), 14.0f));
    g.setColour (kPink);
    g.fillPath (makeHeart (bounds.getX() - 58.0f, 60.0f, 11.0f));
    g.fillPath (makeHeart (bounds.getRight() + 58.0f, 60.0f, 11.0f));
}

void AvasEditor::drawPanel (juce::Graphics& g)
{
    const auto r = panelBounds.toFloat();

    // a glossy black grand-piano lid
    g.setColour (juce::Colour (0x44502060));
    g.fillRoundedRectangle (r.translated (0.0f, 7.0f), 34.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3d3050), r.getX(), r.getY(),
                                             juce::Colour (0xff100c16), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 34.0f);

    juce::Path gloss;
    gloss.addRoundedRectangle (r.getX() + 10.0f, r.getY() + 8.0f, r.getWidth() - 20.0f, r.getHeight() * 0.40f, 26.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.22f), 0.0f, r.getY(),
                                             juce::Colours::white.withAlpha (0.0f), 0.0f, r.getY() + r.getHeight() * 0.42f, false));
    g.fillPath (gloss);

    g.setColour (kPink);
    g.drawRoundedRectangle (r.reduced (1.5f), 34.0f, 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawRoundedRectangle (r.reduced (8.0f), 27.0f, 1.2f);

    // heading
    g.setColour (juce::Colour (0xffffd6ea));
    g.setFont (boldFont (20.0f));
    g.drawText ("Piano Settings", panelBounds.getX(), panelBounds.getY() + 16, panelBounds.getWidth(), 28,
                juce::Justification::centred);
    g.setColour (kPink);
    g.fillPath (makeHeart (r.getCentreX() - 92.0f, r.getY() + 31.0f, 8.0f));
    g.fillPath (makeHeart (r.getCentreX() + 92.0f, r.getY() + 31.0f, 8.0f));

    // love note under the panel
    g.setFont (boldFont (19.0f));
    const juce::String note = "made with love for Ava";
    const float cx = r.getCentreX(), y = r.getBottom() + 34.0f;
    g.setColour (juce::Colour (0x66ffffff));
    g.drawText (note, (int) cx - 150, (int) y + 1, 300, 26, juce::Justification::centred);
    g.setColour (juce::Colour (0xffc2509a));
    g.drawText (note, (int) cx - 150, (int) y, 300, 26, juce::Justification::centred);
    g.setColour (kHotPink);
    g.fillPath (makeHeart (cx - 118.0f, y + 14.0f, 9.0f));
    g.fillPath (makeHeart (cx + 118.0f, y + 14.0f, 9.0f));
}

void AvasEditor::drawFloor (juce::Graphics& g)
{
    juce::Path floor;
    floor.startNewSubPath (0.0f, (float) kHeight);
    floor.lineTo (0.0f, (float) kFloorTop);
    for (float x = 0.0f; x < (float) kWidth; x += 30.0f)
        floor.quadraticTo (x + 15.0f, (float) kFloorTop - 22.0f, x + 30.0f, (float) kFloorTop);
    floor.lineTo ((float) kWidth, (float) kHeight);
    floor.closeSubPath();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffa8d6), 0.0f, (float) kFloorTop - 20.0f,
                                             juce::Colour (0xffb98cff), 0.0f, (float) kHeight, false));
    g.fillPath (floor);

    juce::Path edge;
    edge.startNewSubPath (0.0f, (float) kFloorTop);
    for (float x = 0.0f; x < (float) kWidth; x += 30.0f)
        edge.quadraticTo (x + 15.0f, (float) kFloorTop - 22.0f, x + 30.0f, (float) kFloorTop);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.strokePath (edge, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (juce::Colours::white.withAlpha (0.28f));
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 31; ++col)
            g.fillEllipse (14.0f + (float) col * 30.0f + (row % 2 ? 15.0f : 0.0f), (float) kFloorTop + 8.0f + (float) row * 44.0f, 6.0f, 6.0f);
}

void AvasEditor::paint (juce::Graphics& g)
{
    g.setOpacity (1.0f);
    g.drawImage (backgroundImage, getLocalBounds().toFloat());

    // twinkling sparkles
    static const float tw[8][3] = { { 340, 108, 9 }, { 880, 118, 10 }, { 330, 440, 8 }, { 870, 420, 9 },
                                    { 600, 100, 7 }, { 12, 470, 8 }, { 450, 118, 6 }, { 700, 108, 6 } };
    for (int i = 0; i < 8; ++i)
    {
        const float a = 0.55f + 0.45f * std::sin (phase * 1.3f + (float) i * 1.7f);
        g.setColour (juce::Colours::white.withAlpha (a));
        g.fillPath (makeSparkle (tw[i][0], tw[i][1], tw[i][2] * (0.8f + 0.3f * a)));
    }

    // Ava gently bobbing along to the music
    // (JUCE multiplies images by the current colour's alpha, so reset it first)
    g.setOpacity (1.0f);
    const float bob = 3.0f * std::sin (phase * 0.9f);
    g.drawImage (girlImage, girlBounds.translated (0.0f, bob));

    drawFloor (g);
    drawTitle (g);
    drawPanel (g);
}

void AvasEditor::paintOverChildren (juce::Graphics& g)
{
    for (const auto& p : particles)
    {
        g.setColour (p.colour.withAlpha (juce::jlimit (0.0f, 1.0f, p.life)));
        g.fillPath (p.isHeart ? makeHeart (p.x, p.y, p.size) : makeSparkle (p.x, p.y, p.size));
    }
}

//==============================================================================
void AvasEditor::spawnParticles (int note)
{
    static const juce::Colour palette[] = { juce::Colour (0xffff7eb6), juce::Colour (0xffffe07a), juce::Colour (0xffb69cff),
                                            juce::Colour (0xff7fd9ff), juce::Colours::white, juce::Colour (0xffff9ccf) };
    const float t = juce::jlimit (0.0f, 1.0f, (float) (note - 21) / 87.0f);
    const float x = (float) keyboard.getX() + t * (float) keyboard.getWidth();

    for (int i = 0; i < 2; ++i)
    {
        Particle p;
        p.x = x + random.nextFloat() * 14.0f - 7.0f;
        p.y = (float) keyboard.getY() - 2.0f;
        p.vx = random.nextFloat() * 1.2f - 0.6f;
        p.vy = -(1.6f + random.nextFloat() * 1.6f);
        p.life = 1.0f;
        p.size = 6.0f + random.nextFloat() * 6.0f;
        p.colour = palette[random.nextInt (6)];
        p.isHeart = (i == 0);
        particles.push_back (p);
    }
    if (particles.size() > 160)
        particles.erase (particles.begin(), particles.begin() + 40);
}

void AvasEditor::timerCallback()
{
    phase += 0.07f;

    for (int w = 0; w < 2; ++w)
    {
        const uint64_t bits = processor.noteOnBits[w].exchange (0);
        if (bits == 0)
            continue;
        for (int b = 0; b < 64; ++b)
            if (bits & (uint64_t (1) << b))
                spawnParticles (w * 64 + b);
    }

    for (auto& p : particles)
    {
        p.x += p.vx + 0.5f * std::sin (p.y * 0.05f);
        p.y += p.vy;
        p.vy *= 0.985f;
        p.life -= 0.016f;
    }
    particles.erase (std::remove_if (particles.begin(), particles.end(),
                                     [] (const Particle& p) { return p.life <= 0.0f; }),
                     particles.end());

    repaint();
}
