#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AvasProcessor::createParameterLayout()
{
    using PF = juce::AudioParameterFloat;
    using Attr = juce::AudioParameterFloatAttributes;

    auto percentText = [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; };
    auto percentParse = [] (const juce::String& t) { return t.getFloatValue() / 100.0f; };

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<PF> (
        juce::ParameterID { "volume", 1 }, "Volume",
        juce::NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f,
        Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; })
              .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); })));

    params.push_back (std::make_unique<PF> (
        juce::ParameterID { "sparkle", 1 }, "Sparkle",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f,
        Attr().withStringFromValueFunction (percentText).withValueFromStringFunction (percentParse)));

    params.push_back (std::make_unique<PF> (
        juce::ParameterID { "sustain", 1 }, "Sustain",
        juce::NormalisableRange<float> (0.4f, 1.8f, 0.01f), 1.0f,
        Attr().withStringFromValueFunction (percentText).withValueFromStringFunction (percentParse)));

    params.push_back (std::make_unique<PF> (
        juce::ParameterID { "room", 1 }, "Room",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f,
        Attr().withStringFromValueFunction (percentText).withValueFromStringFunction (percentParse)));

    params.push_back (std::make_unique<PF> (
        juce::ParameterID { "width", 1 }, "Width",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f,
        Attr().withStringFromValueFunction (percentText).withValueFromStringFunction (percentParse)));

    return { params.begin(), params.end() };
}

//==============================================================================
AvasProcessor::AvasProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AvasState", createParameterLayout())
{
    volumeParam = apvts.getRawParameterValue ("volume");
    sparkleParam = apvts.getRawParameterValue ("sparkle");
    sustainParam = apvts.getRawParameterValue ("sustain");
    roomParam = apvts.getRawParameterValue ("room");
    widthParam = apvts.getRawParameterValue ("width");
}

void AvasProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
}

bool AvasProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void AvasProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
    {
        const int note = m.getNoteNumber();
        engine.noteOn (note, m.getVelocity());
        noteOnBits[note >> 6].fetch_or (uint64_t (1) << (note & 63));
    }
    else if (m.isNoteOff())
    {
        engine.noteOff (m.getNoteNumber());
    }
    else if (m.isController() && m.getControllerNumber() == 64)
    {
        engine.setSustainPedal (m.getControllerValue() >= 64);
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        engine.allNotesOff();
    }
}

void AvasProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);
    if (buffer.getNumChannels() < 2)
        return;

    // notes played on the on-screen keyboard join the real MIDI stream
    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    ava::Settings s;
    s.volumeDb = volumeParam->load();
    s.brightness = sparkleParam->load();
    s.sustain = sustainParam->load();
    s.room = roomParam->load();
    s.width = widthParam->load();
    engine.setSettings (s);

    float* left = buffer.getWritePointer (0);
    float* right = buffer.getWritePointer (1);

    int pos = 0;
    for (const auto metadata : midi)
    {
        const int at = juce::jlimit (0, numSamples, metadata.samplePosition);
        if (at > pos)
        {
            engine.render (left + pos, right + pos, at - pos);
            pos = at;
        }
        handleMidi (metadata.getMessage());
    }
    if (pos < numSamples)
        engine.render (left + pos, right + pos, numSamples - pos);

    midi.clear(); // an instrument doesn't pass MIDI on
}

juce::AudioProcessorEditor* AvasProcessor::createEditor()
{
    return new AvasEditor (*this);
}

void AvasProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AvasProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AvasProcessor();
}
