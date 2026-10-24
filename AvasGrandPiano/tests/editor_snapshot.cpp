// Developer tool: draws the plugin window into a PNG (no screen needed), pretending
// a few keys are being played so the lit keys show up.
//
//   AvaSnapshot out.png
#include "../Source/PluginEditor.h"
#include "../Source/PluginProcessor.h"

int main (int argc, char** argv)
{
    const juce::String outPath = argc > 1 ? juce::String (argv[1]) : juce::String ("snapshot.png");

    juce::ScopedJuceInitialiser_GUI gui;

    AvasProcessor processor;
    processor.prepareToPlay (44100.0, 512);

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());

    // a little chord on the on-screen keyboard
    for (int n : { 48, 60, 64, 67, 72, 76 })
    {
        processor.keyboardState.noteOn (1, n, 0.8f);
        processor.noteOnBits[n >> 6].fetch_or (uint64_t (1) << (n & 63));
    }

    juce::Image image (juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true);
    {
        juce::Graphics g (image);
        editor->paintEntireComponent (g, true);
    }

    juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (outPath));
    out.deleteFile();
    juce::FileOutputStream stream (out);
    juce::PNGImageFormat png;
    if (stream.openedOk() && png.writeImageToStream (image, stream))
        std::printf ("wrote %s (%d x %d)\n", out.getFullPathName().toRawUTF8(), image.getWidth(), image.getHeight());
    else
        std::printf ("could not write %s\n", out.getFullPathName().toRawUTF8());

    editor = nullptr;
    return 0;
}
