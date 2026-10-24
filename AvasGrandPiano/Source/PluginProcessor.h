#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <atomic>

#include "PianoEngine.h"

//==============================================================================
// Avas Grand Piano - the plugin "brain": receives MIDI, runs the piano engine,
// and owns the knobs (parameters).
class AvasProcessor : public juce::AudioProcessor
{
public:
    AvasProcessor();
    ~AvasProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Concert Grand"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState; // the on-screen keyboard talks through this

    // Bit i of word (note / 64) is set when a note-on arrives; the editor reads and clears
    // these to spawn the little floating hearts and stars.
    std::atomic<uint64_t> noteOnBits[2] { 0, 0 };

private:
    void handleMidi (const juce::MidiMessage& m);

    ava::PianoEngine engine;
    std::atomic<float>* volumeParam = nullptr;
    std::atomic<float>* sparkleParam = nullptr;
    std::atomic<float>* sustainParam = nullptr;
    std::atomic<float>* roomParam = nullptr;
    std::atomic<float>* widthParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AvasProcessor)
};
