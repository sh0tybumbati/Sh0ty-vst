# Sh0ty KTG-1

A tube-preamp plugin (VST3 / AU / Standalone) inspired by the Seymour Duncan KTG-1.
Built with JUCE. It is a generic triode-style model (asymmetric saturation, coupling
caps, 4x oversampling), **not** a component-level model of the real circuit.

Controls: Drive, Tone, Level, Bright, Mix.

## Build
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j
    ctest --test-dir build      # DSP tests

The VST3 lands in `build/Sh0tyKTG1_artefacts/Release/VST3/`. Copy it to your DAW's VST3 folder
(Windows: `C:\Program Files\Common Files\VST3`, macOS: `~/Library/Audio/Plug-Ins/VST3`,
Linux: `~/.vst3`). GitHub Actions builds Windows/macOS/Linux artifacts on every push.

Linux build deps: libasound2-dev libfreetype-dev libfontconfig1-dev libx11-dev libxinerama-dev
libxrandr-dev libxcursor-dev libxext-dev libgl1-mesa-dev
