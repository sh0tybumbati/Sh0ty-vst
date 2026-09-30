# Sh0ty VST plugins

Two plugins: **Sh0ty FZ-3 Fuzz** (Fuzz, Tone, Volume, Mix; inspired by the Boss FZ-3) and **Sh0ty KTG-1** — a tube-preamp plugin (VST3 / AU / Standalone) inspired by the Seymour Duncan KTG-1.
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

### CachyOS / Arch Linux
    sudo pacman -S --needed base-devel cmake git alsa-lib freetype2 fontconfig \
      libx11 libxinerama libxrandr libxcursor libxext mesa curl
    git clone https://github.com/sh0tybumbati/sh0ty-vst.git && cd sh0ty-vst
    git checkout ccr-0f36f608-4k30zx
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j
    ctest --test-dir build --output-on-failure
    mkdir -p ~/.vst3 && cp -r build/*_artefacts/Release/VST3/*.vst3 ~/.vst3/

Standalone apps (no DAW needed) are in `build/*_artefacts/Release/Standalone/`.

![FZ-3 editor](docs/fz3-ui.png)

## FZ-3 model
The FZ-3 DSP follows the signal path of the "Boss FZ3" schematic (JFET buffers, Q1 treble shelf,
Q3 fuzz stage, Q4, tone network, volume). The Fuzz pot's gain law is an estimate because the
schematic draws it as a placeholder; the sheet's clean "2 CH MIXER" branch is not modelled (use Mix).
