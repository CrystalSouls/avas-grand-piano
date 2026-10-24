#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <array>
#include <memory>
#include <vector>

#include "PluginProcessor.h"

//==============================================================================
// Pink glossy knobs with a little heart on the pointer.
class KawaiiLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KawaiiLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
};

//==============================================================================
class AvasEditor : public juce::AudioProcessorEditor,
                   private juce::Timer
{
public:
    explicit AvasEditor (AvasProcessor&);
    ~AvasEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Particle
    {
        float x, y, vx, vy, life, size;
        juce::Colour colour;
        bool isHeart;
    };

    void timerCallback() override;
    void renderArt();
    void spawnParticles (int note);
    void drawTitle (juce::Graphics&);
    void drawPanel (juce::Graphics&);
    void drawFloor (juce::Graphics&);

    AvasProcessor& processor;
    KawaiiLookAndFeel lookAndFeel;

    std::array<Knob, 5> knobs;
    juce::MidiKeyboardComponent keyboard;

    std::unique_ptr<juce::Drawable> backgroundDrawable, girlDrawable;
    juce::Image backgroundImage, girlImage;

    std::vector<Particle> particles;
    juce::Random random;
    float phase = 0.0f;

    juce::Rectangle<int> panelBounds;
    juce::Rectangle<float> girlBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AvasEditor)
};
