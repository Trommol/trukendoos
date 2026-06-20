# Spectral Rot

Spectral Rot is the first plugin in a two-plugin spectral distortion family.

The goal is not transparent mixing, repair, or resonance suppression. The goal is a sound-design weapon for heavy melodic bass music: unpredictable, animated, destructive when pushed, but still capable of preserving pitch, sub weight, and the broad identity of the source.

## Product Split

### Spectral Rot

The wild plugin.

- Designed for resampling, bass sound design, breaks, granular chains, and post-synth mangling.
- Strong personality, unstable controls, destructive modes, and performable macro behavior.
- Freakshow-style spirit: strange, dramatic, funny, and slightly hostile, but useful.
- Good results should appear quickly from simple sources like sine waves.

### Spectral Distortion

The controlled sibling, built later.

- More predictable and organized.
- Same broad DSP family, but with clearer transfer curves, repeatable modes, and less random motion.
- Designed for "I know what I want" distortion rather than "surprise me, but keep it musical."

## Core DSP Idea

Spectral Rot processes short overlapping FFT frames instead of clipping the waveform directly.

Basic signal path:

```text
input audio
-> windowed FFT
-> spectral analysis
-> spectral distortion modes
-> stability / tone protection
-> inverse FFT
-> overlap-add output
```

The plugin can modify:

- Magnitude per frequency bin.
- Phase per frequency bin.
- Energy distribution between neighboring bins.
- Broad spectral envelope.
- Stable tonal components versus noisy components.

This makes it possible to create distortion that behaves unlike time-domain waveshaping.

## MVP Modes

### Melt

Spectral time-melt.

- Splits the FFT spectrum into low, mid, and high time regions.
- Low bins can stay immediate while mids and highs arrive later.
- Adds subtle per-bin delay modulation so the source appears to bend, sag, and liquefy.
- Movement must be input-dependent: silence stays silent.

Possible implementation:

- Store previous FFT frames in spectral history buffers.
- Read bins from different frame offsets based on frequency.
- Use short modulation offsets on delay time, not autonomous tones.

### Rust

Bin folding and unstable midrange corrosion.

- Strong partials spill into nearby bins.
- Generates growl, scraping, and metallic movement.
- Should be dangerous but still bass-music useful.

Possible implementation:

- Detect bins above a local average.
- Reflect part of their energy into nearby bins.
- Modulate fold distance over time.
- Keep sub and fundamental protected.

### Glass

Spectral delay/freeze.

- Per-bin delay, feedback, and pitch drift.
- Should create Portal-like/Freakshow-like soundscapes, tails, and spectral artifacts.
- Delay tails are intentional and may continue after input stops.

Possible implementation:

- Store FFT bins in circular history buffers.
- Read delayed bins using frequency-dependent delay times.
- Feed delayed bins back through spectral distortion and phase drift.

## First Control Set

### Drive

Main amount of spectral nonlinear processing.

### Rot

The main chaos macro. Increases instability, fold depth, phase abuse, freeze probability, and modulation range depending on mode.

### Lock

Preserves the source identity.

At high values:

- Protect the fundamental area.
- Preserve broad spectral envelope.
- Reduce unstable bin scattering.
- Keep the sound more playable.

At low values:

- Let bins corrupt more freely.
- Allow more phase damage and spectral holes.

### Tilt

Biases the distortion darker or brighter.

### Motion

Animation rate/depth for spectral modulation.

### Sub Guard

Protects low frequencies from spectral chaos.

### Mix

Dry/wet blend.

### Output

Post gain.

## Useful Advanced Features

These are not required for the first build.

- Delta monitor for hearing only the added/removed spectral material.
- MIDI note/fundamental tracking for better Lock behavior.
- Sidechain spectral feeding, where one sound corrupts another.
- Per-mode random seed control.
- Oversampled pre/post clipping stage.
- Freeze button.
- Panic button that randomizes internal rot state.

## Technical Direction

Recommended implementation stack:

- JUCE for plugin format, UI, parameters, VST3 export, and cross-platform project structure.
- A separate C++ DSP core that does not depend heavily on UI code.
- Real-time safe allocation strategy: allocate FFT buffers up front.
- Latency reported to the host based on FFT size and overlap.
- Start mono/stereo only; no surround.

Initial FFT direction:

- FFT size: 1024 or 2048 samples.
- Overlap: 4x.
- Window: Hann or sqrt-Hann depending on overlap-add choice.
- Sample rates: 44.1 kHz to 192 kHz.
- Internal smoothing to prevent frame-to-frame splatter.

Important risks:

- Spectral processing creates latency.
- Phase manipulation can sound smeary or hollow.
- FFT artifacts are part of the character, but must feel intentional.
- CPU can rise quickly if the modes become too complex.

## First Prototype Goal

Build a minimal version with:

- One FFT processor.
- Stereo support.
- Melt mode.
- Rust mode.
- Glass mode.
- Drive, Rot, Lock, Tilt, Sub Guard, Mix, Output.
- A simple functional UI.

The first sonic test should be:

1. Load a sine wave or Serum sine bass.
2. Insert Spectral Rot.
3. Push Drive and Rot.
4. Verify that it creates usable harmonic material.
5. Raise Lock/Sub Guard and verify the pitch and sub survive.

If this test feels good, the plugin is worth expanding.
