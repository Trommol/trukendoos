# Spectral Rot

Spectral Rot is a creative spectral distortion VST3 for heavy melodic bass, breakbeat, dubstep, and resampling workflows.

It is intentionally the chaotic sibling in a two-plugin family:

- **Spectral Rot**: unpredictable spectral corruption, folding, phase/magnitude abuse, and source-preserving chaos.
- **Spectral Distortion**: a later, more controlled plugin built from the same DSP ideas.

## First Prototype

The initial build includes:

- FFT-based spectral processing.
- `Melt` mode for smoother spectral saturation.
- `Rust` mode for unstable bin folding.
- `Glass` mode for spectral delay/freeze textures.
- `Drive`, `Rot`, `Lock`, `Tilt`, `Sub Guard`, `Mix`, and `Output`.

## Build

This project uses JUCE via `external/JUCE`.

```powershell
cmd /c tools\build_vs.cmd
```

The build script uses the Visual Studio Build Tools installation at `C:\BuildTools` and writes artifacts to `build-vs/`.

## Audition Renderer

Until the VST3 build toolchain is installed, generate quick WAV previews with the bundled Python runtime:

```powershell
& 'C:\Users\tremm\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' tools\render_spectral_rot_preview.py --mode all
```

Rendered files are written to `renders/`.
