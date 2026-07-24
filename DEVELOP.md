# NINE50 Development Guide

## Overview

NINE50 is a French-touch sidechain processor inspired by Daft Punk's *Homework* era. It combines an Alesis 3630-style sidechain compressor with Emu SP-1200 / Akai S950 bitcrush and downsample emulation in a single VST3/AU plugin.

## Architecture

### Plugin Structure

```
Source/
├── PluginProcessor.h/.cpp     # Audio processing + parameters
├── PluginEditor.h/.cpp        # GUI with sliders and displays
├── SidechainCompressor.h/.cpp # Compressor DSP class
├── BitCrushEmulation.h/.cpp       # SP-1200 / S950 bitcrush DSP class
├── GainReductionMeter.h/.cpp  # Visual meter component
├── NINE50LookAndFeel.h/.cpp   # Custom amber/analog LookAndFeel
├── PresetManager.h/.cpp       # Factory + user preset save/load
└── Tests/
    ├── Main.cpp               # Unit test runner entry point
    ├── SidechainCompressorTests.cpp
    ├── BitCrushEmulationTests.cpp
    └── PluginProcessorTests.cpp
```

### Processing Flow

1. **Sidechain Compressor** (optional, when sidechain bus is enabled)
   - Sidechain signal is HPF-filtered (optional)
   - RMS detection with attack/release smoothing
   - Gain reduction computed via `(env_dB - threshold) * (1 - 1/ratio)`
   - Applied to main buffer with optional makeup gain

2. **Bitcrush / Downsample (SP-1200 / S950)**
   - Layout routing first (Mono Sum/L/R affect dry; Stereo L/R/Mid/Side process one component)
   - Drive (input gain)
   - Open input AA (~0.666 × host Nyquist) so aliases fold in during downsample
   - Detune → SP hardware source ratio → `effRate = 26040 / ratio`; NN sample + ZOH (pitch unchanged)
   - Hard midtread 12-bit quantize (no dither)
   - Dark SP output EQ (~7.5 kHz, pitcher lp1 character)
   - User HPF (0–99) then S950 6th-order LPF (0–99, bypass 99)
   - Output gain → dry/wet mix (mix=0 ≡ bypass)

## Building

### Prerequisites

- CMake 3.16+
- C++17 compiler
- JUCE 9.0.0 (fetched automatically via CMake FetchContent)

### Build Commands

```bash
# Configure (with tests)
cmake -B build -DBUILD_TESTING=ON

# Build plugin
cmake --build build -j8

# Build and run tests
cmake --build build -j8 --target NINE50Tests
./build/Source/NINE50Tests_artefacts/NINE50Tests
```

### Plugin Formats

- **VST3** on macOS, Windows, and Linux
- **AudioUnit** on macOS only (`FORMATS` is set per-platform in `Source/CMakeLists.txt`)

## Testing

### Running Tests

```bash
# Run all tests
./build/Source/NINE50Tests_artefacts/NINE50Tests

# Run specific category
./build/Source/NINE50Tests_artefacts/NINE50Tests --category=nine50

# Run specific test by name
./build/Source/NINE50Tests_artefacts/NINE50Tests --name=BitCrushEmulation
```

Note: the test runner currently executes the `nine50` category only (see `Tests/Main.cpp`).

### Writing Tests

Tests use JUCE's `UnitTest` framework. Add new test files to `Source/Tests/` and register them in `CMakeLists.txt`:

```cpp
#include <JuceHeader.h>
#include "YourClass.h"

class YourClassTest : public juce::UnitTest {
public:
    YourClassTest()
        : juce::UnitTest("YourClass", "nine50") {}

    void runTest() override {
        testCase("Description", [&] {
            // Setup
            YourClass obj;

            // Action
            obj.doSomething();

            // Verify
            expect(condition, "Failure message");
        });
    }
};

static YourClassTest yourClassTest;
```

### Test Categories

- `nine50` - All NINE50 unit tests (compressor, bitcrush, processor)

## Parameters

### Sidechain Compressor

| Parameter | Range | Default | Notes |
|-----------|-------|---------|-------|
| Threshold | -60 to 0 dB | -15 dB | Sidechain trigger sensitivity |
| Ratio | 1:1 to 10:1 | 8:1 | Aggressive ducking |
| Attack | 0.1 to 50 ms | 10 ms | Fast for punch |
| Release | 10 to 500 ms | 100 ms | Fast pump-back |
| Makeup | 0 to 30 dB | 0 dB | Output gain compensation |
| HPF | Off/100/200/300 Hz | Off | Sidechain high-pass filter |
| Link | Off/On | Off | Link L/R sidechain detection |
| Comp On | Off/On | On | Stage enable (ON/BYPASS) |

### Bitcrush / Downsample

| Parameter | Range | Default | Notes |
|-----------|-------|---------|-------|
| Drive | -12 to +12 dB | 0 dB | Input gain |
| Detune | -15 to +15 st | 0 st | Pitch shift |
| Ext | Off/On | Off | Extend detune to ±30 st |
| Fine | Off/On | Off | 0.1 st vs 1 st steps |
| Filter | 0-99 | 99 (off) | 6th-order Butterworth LPF |
| Layout | 8 options | Stereo | Channel routing |
| Mix | 0-100% | 100% | Dry/wet |
| Out | -12 to +12 dB | 0 dB | Output gain |
| Link | Off/On | Off | Link Drive/Out gain |
| Crush On | Off/On | On | Stage enable (ON/BYPASS) |

## Coding Guidelines

### C++ Style

- C++17 standard
- Use `static_cast` for type conversions (avoid C-style casts)
- Use `juce::jlimit` for clamping
- Use `juce::jmax`/`juce::jmin` for comparisons
- Prefer `std::unique_ptr` for owned pointers
- Use `juce::AudioBuffer<float>` for audio data

### DSP Guidelines

- Always check for zero samples before processing
- Use `juce::jmin` when iterating over multiple buffers to avoid out-of-bounds access
- Cast `int` to `size_t` when indexing into `std::vector`
- Store dry signal before processing for dry/wet mix
- Apply layout routing after dry/wet mix for consistent behavior

### Test Guidelines

- Test edge cases (zero samples, silent input, extreme parameter values)
- Use RMS or average values when testing effects that cause oscillation
- Isolate specific behaviors by using mix=0 (dry) or mix=1 (wet) as appropriate
- Compare against original signal when testing bypass behavior

## Presets

Factory presets are compiled into `PresetManager` and exposed via the AudioProcessor program API (DAW preset menus) and the in-plugin preset combo.

User presets are XML files with a `.nine50` extension stored at:

```
~/Library/Audio/Presets/NINE50/
```

UI controls (header bar):
- Preset combo — load factory or user presets
- **SAVE** — overwrite current user preset, or Save As for factory presets
- **...** menu — Save As, Delete, Reveal folder, Init


```
Input:  Stereo (required)
Output: Stereo (required)
Sidechain: Stereo (optional, auto-detected)
```

The sidechain bus is automatically detected via `isSidechainEnabled()` which checks if the sidechain input bus is present and enabled.

## Key JUCE APIs

- `AudioProcessorValueTreeState` - Parameter management
- `juce::IIRFilter` - Butterworth LPF/HPF
- `juce::LagrangeInterpolator` - Pitch shifting
- `juce::AudioProcessorEditor` - GUI
- `getBusBuffer()` - Sidechain buffer access
- `SliderAttachment`, `ComboBoxAttachment`, `ButtonAttachment` - GUI binding
