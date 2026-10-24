# 🎹 Ava's Grand Piano

A simple, realistic grand piano plugin with a kawaii anime-girl theme. Made with love for Ava.

- **Formats:** VST3 (Windows + Mac), AU (Mac, for GarageBand / Logic), and a Standalone app you can just double-click and play
- **Sound:** a physically modeled grand piano (no sample files). Every note has real string stiffness, two slightly detuned strings that "shimmer", hammer thump, felt dampers and a working sustain pedal.
- **Knobs:** Volume, Sparkle (bright ↔ mellow), Sustain, Room, Width
- **Play it:** MIDI keyboard, or click the on-screen keys, or use your computer keyboard (A W S E D F T G Y H U J K...). Hearts and stars float up when you play ✨

---

## Getting the plugin (no programming needed)

The plugin gets built for you automatically by GitHub.

1. Make a free account at <https://github.com> and click **New repository** (name it `avas-grand-piano`).
2. Upload everything in this folder to it (the **Add file → Upload files** button works; the hidden `.github` folder must be included. Dragging the whole folder in works).
3. Click the **Actions** tab. A build called *Build Avas Grand Piano* starts by itself (about 10–15 minutes the first time).
4. When it has a green tick, click it and scroll to **Artifacts** at the bottom. Download **AvasGrandPiano-Windows** or **AvasGrandPiano-Mac**.

## Installing

**Windows**
Copy `Avas Grand Piano.vst3` (it's a folder; copy the whole thing) into
`C:\Program Files\Common Files\VST3`
then rescan plugins in your music program. `Avas Grand Piano.exe` is the standalone app.

**Mac**
Copy `Avas Grand Piano.vst3` into `/Library/Audio/Plug-Ins/VST3` and `Avas Grand Piano.component` into `/Library/Audio/Plug-Ins/Components`.
The first time, macOS may say it can't verify the developer: right-click the file → **Open**, or go to *System Settings → Privacy & Security → Open Anyway*.

In your music program, add **Avas Grand Piano** as an *instrument* track and play it with a MIDI keyboard.

## Building it yourself (optional, for grown-ups)

You need CMake 3.22+ and a C++17 compiler (Visual Studio 2022 on Windows, Xcode on Mac, or GCC + the JUCE Linux packages).

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Add `-DAVA_COPY_AFTER_BUILD=ON` to copy the plugin into your plugin folder automatically.
JUCE is downloaded automatically (needs internet the first time).

## What's in the folder

| Path | What it is |
|---|---|
| `Source/PianoEngine.h` | the piano sound (plain C++, no framework) |
| `Source/PluginProcessor.*` | MIDI in, audio out, the knobs |
| `Source/PluginEditor.*` | the kawaii window |
| `Assets/*.svg` | Ava and the sky background (made by `tools/make_art.py`) |
| `tests/render_test.cpp` | renders notes to a .wav so you can test the sound without a DAW |
| `demo/AvasGrandPiano_demo.mp3` | a short piece played by the engine, so you can hear it right away |

## Licence note

The plugin is built with [JUCE](https://juce.com), which is free for personal, non-commercial and open-source use (and is used here under JUCE's own licence terms). If Ava ever wants to *sell* this plugin, check JUCE's licensing page first.
