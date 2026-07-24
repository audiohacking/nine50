# NINE50

A French-touch sidechain processor inspired by Daft Punk's *Homework* era. Combines an Alesis 3630-style sidechain compressor with Emu SP-1200 / Akai S950 bitcrush and downsample emulation in a single VST3/AU plugin.

![NINE50 Screenshot](docs/nine50-ui.png)

## Features

- Modern analog-inspired UI with labeled rotary controls and amber GR metering
- Factory presets plus user save/load (`.nine50` files in `~/Library/Audio/Presets/NINE50`)

### Sidechain Compressor
- VCA-style feed-forward compressor with aggressive ratios (up to 10:1)
- Optional sidechain input bus for external key signals
- High-pass filter on sidechain (100/200/300 Hz)
- Makeup gain compensation
- Channel linking for stereo sidechain detection

### Bitcrush / Downsample (SP-1200 / S950)
- 12-bit bit reduction with TPDF dither
- Sample rate reduction to 26.04 kHz (SP-1200 characteristic)
- Detune/pitch shift with semitone or fine (0.1 st) steps
- 6th-order Butterworth low-pass filter (bypassable)
- Multiple channel layouts: Mono Sum, Mono L/R, Stereo, Stereo L/R, Mid/Side
- Drive and output gain with link option
- Dry/wet mix control

## Requirements

- **macOS**: 10.15 or later

## Installation

### Download (Recommended)

Download the latest release from the [Releases page](https://github.com/audiohacking/nine50/releases).

**macOS**:
1. Download `NINE50-macOS.zip`
2. Extract the archive
3. Double-click `NINE50.vst3` or `NINE50.component` to install
4. Restart your DAW

> **Windows**: Support planned — coming in a future release.

### Build from Source

#### Prerequisites

- C++17 compatible compiler (Xcode 12+)
- Xcode Command Line Tools

#### Build Instructions

```bash
# Clone the repository
git clone https://github.com/audiohacking/nine50.git
cd NINE50

# Configure (VST3 + AU)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j8

# The plugin will be in build/Source/NINE50_artefacts/Release/
```

#### Build with Tests

```bash
cmake -B build -DBUILD_TESTING=ON
cmake --build build -j8
ctest --test-dir build
```

See [DEVELOP.md](DEVELOP.md) for detailed development guidelines.

## Usage

1. Add NINE50 to a bus/aux track in your DAW
2. Route a sidechain signal (e.g., kick drum) to the sidechain input
3. Adjust the Threshold and Ratio to control ducking depth
4. Use the BITCRUSH section to add SP-1200 / S950 character to the wet signal
5. Bypass either stage with the ON/BYPASS switches
6. Fine-tune with Attack, Release, and Makeup controls

## License

[GNU Affero General Public License v3.0](LICENSE)

## Credits

- Inspired by the French-touch house production techniques of Daft Punk, Cassius, and others
- SP-1200 and S950 emulation based on classic sampler characteristics
- Compressor design inspired by the Alesis 3630
