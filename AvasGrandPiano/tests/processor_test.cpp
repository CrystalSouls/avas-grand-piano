// Developer tool: sends MIDI through the real plugin class and checks audio comes out.
#include "../Source/PluginProcessor.h"
#include <cstdio>

static float peak (const juce::AudioBuffer<float>& b)
{
    return juce::jmax (b.getMagnitude (0, 0, b.getNumSamples()), b.getMagnitude (1, 0, b.getNumSamples()));
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    int failures = 0;
    for (int block : { 64, 480, 1024 })
    {
        AvasProcessor p;
        p.setPlayConfigDetails (0, 2, 48000.0, block);
        p.prepareToPlay (48000.0, block);
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;

        buf.clear();
        midi.addEvent (juce::MidiMessage::controllerEvent (1, 64, 127), 0);
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 10);
        midi.addEvent (juce::MidiMessage::noteOff (1, 60, (juce::uint8) 0), block / 2);
        p.processBlock (buf, midi);
        const float first = peak (buf);

        float later = 0; bool bad = false;
        for (int i = 0; i < 200; ++i) // ~ a few seconds of pedal-held ring
        {
            buf.clear(); juce::MidiBuffer empty;
            p.processBlock (buf, empty);
            later = peak (buf);
            for (int c = 0; c < 2; ++c)
                for (int s = 0; s < block; ++s)
                    if (! std::isfinite (buf.getSample (c, s)) || std::abs (buf.getSample (c, s)) > 1.0f) bad = true;
        }
        const bool ok = first > 0.01f && later > 0.0f && ! bad;
        std::printf ("block %4d: first-block peak %.3f, still ringing peak %.5f, bad samples: %s -> %s\n",
                     block, first, later, bad ? "YES" : "no", ok ? "PASS" : "FAIL");
        failures += ok ? 0 : 1;
    }
    return failures;
}
