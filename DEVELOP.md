# NINE50 Development Guide

## Overview

NINE50 is a French-touch sidechain processor with SP950-style lo-fi emulation, inspired by Daft Punk's *Homework* era. It combines an Alesis 3630-style sidechain compressor with Emu SP-1200/Akai S950 emulation in a single VST3/AU plugin.

## Architecture

### Plugin Structure

```
Source/
├── PluginProcessor.h/.cpp     # Audio processing + parameters
├── PluginEditor.h/.cpp        # GUI with sliders and displays
├── SidechainCompressor.h/.cpp # Compressor DSP class
├── SP950Emulation.h/.cpp      # Lo-fi emulation DSP class
├── GainReductionMeter.h/.cpp  # Visual meter component
└── Tests/
    ├── Main.cpp               # Unit test runner entry point
    ├── SidechainCompressorTests.cpp
    ├── SP950EmulationTests.cpp
    └── PluginProcessorTests.cpp
```

### Processing Flow

1. **Sidechain Compressor** (optional, when sidechain bus is enabled)
   - Sidechain signal is HPF-filtered (optional)
   - RMS detection with attack/release smoothing
   - Gain reduction computed via `(env_dB - threshold) * (1 - 1/ratio)`
   - Applied to main buffer with optional makeup gain

2. **SP950 Emulation**
   - Drive (input gain) applied to signal
   - Layout routing (Mono Sum, Stereo, Mid/Side, etc.)
   - Detune (pitch shift via Lagrange interpolation)
   - Sample rate reduction (downsample to 26.04 kHz)
   - Bit reduction (12-bit with TPDF dither)
   - 6th-order Butterworth LPF (bypassable at val=99)
   - Output gain
   - Dry/wet mix
   - Layout routing applied to final output

## Building

### Prerequisites

- CMake 3.16+
- C++17 compiler
- JUCE (fetched automatically via CMake FetchContent)

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

- **VST3** (default, all platforms)
- **AudioUnit** (macOS only, enabled by `FORMATS VST3 AU`)

## Testing

### Running Tests

```bash
# Run all tests
./build/Source/NINE50Tests_artefacts/NINE50Tests

# Run specific category
./build/Source/NINE50Tests_artefacts/NINE50Tests --category=dsp

# Run specific test by name
./build/Source/NINE50Tests_artefacts/NINE50Tests --name=SP950Emulation
```

### Writing Tests

Tests use JUCE's `UnitTest` framework. Add new test files to `Source/Tests/` and register them in `CMakeLists.txt`:

```cpp
#include <JuceHeader.h>
#include "YourClass.h"

class YourClassTest : public juce::UnitTest {
public:
    YourClassTest()
        : juce::UnitTest("YourClass", juce::UnitTestCategories::dsp) {}

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

- `dsp` - DSP algorithm tests (SP950, SidechainCompressor)
- `audioProcessors` - Plugin processor integration tests

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

### SP950 Emulation

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

## Bus Configuration

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
