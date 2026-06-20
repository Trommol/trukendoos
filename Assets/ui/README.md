# trukendoos UI Asset Contract

This folder is the handoff point for generated UI art before it gets wired into JUCE.

## Canvas

- Plugin editor size: `1480 x 940`
- All full-screen/backplate assets should be authored at `1480 x 940`.
- Mode and harmonizer pedals should be authored with transparent background unless noted.
- Keep control target areas clean enough that JUCE knobs, labels, switches, and the output scope can be drawn over them.

## Always Visible Layers

| File | Size | Notes |
| --- | --- | --- |
| `backplate.png` | `1480 x 940` | Full plugin backplate. No interactive controls baked in. |
| `preset_plate.png` | approx `460 x 82` | Preset name/menu plate at the top center. |
| `global_bus_plate.png` | approx `1380 x 120` | Bottom global-control plate. Must leave room for harmonizers, HP/LP, width, mix. |
| `input_badge.png` | approx `130 x 130` | Input gain badge/plate. |
| `output_badge.png` | approx `130 x 130` | Output gain badge/plate. |

## Mode Header Plates

These are the draggable chain blocks on the left/top chain area. The generated name plates are locked in style.

| File | Target Text | Notes |
| --- | --- | --- |
| `header_melt.png` | `MELT` | Teal/green plate. |
| `header_rust.png` | `RUST` | Rust/orange plate. |
| `header_glass.png` | `GLASS` | Purple/glass plate. |

## Mode Pedal Baseplates

Each mode pedal needs an output scope window near the bottom. The scope itself stays live JUCE-rendered.

| File | Controls | Notes |
| --- | --- | --- |
| `pedal_rust.png` | 5 knobs + 1 switch | Knobs: Corrode, Edge, Fold, Ring, Redux. Switch: Edge Pre/Post. |
| `pedal_melt.png` | 4 knobs | Knobs: Morph, Band Spread, Motion, Depth. |
| `pedal_glass.png` | 7 knobs | Knobs: Time, Feedback, Texture, Drift, Shatter, Sway, Dry/Wet. |

## Harmonizer Pedal

| File | Controls | Notes |
| --- | --- | --- |
| `pedal_harmonizer.png` | 2 enable buttons + 4 slider/knob targets | Harmonizer 1 and 2: Enable, Pitch, Mix. |

## Knobs

Chosen directions:

| File | Used For | Notes |
| --- | --- | --- |
| `knob_melt.png` | Melt mode controls | Chosen: first knob from third row. Teal/illustrated style. |
| `knob_rust.png` | Rust mode controls | Chosen: first knob from second row. Corroded heavy style. |
| `knob_glass.png` | Glass mode controls | Chosen: second knob from third row. Spectral/glassy style. |
| `knob_global.png` | Global controls | Can be a neutral variant, or reused from the current coral knob style. |
| `knob_harmonizer.png` | Harmonizer controls | Should match the harmonizer pedal, not the mode pedals. |

## Export Rules

- Export production layers as PNG.
- Use transparent PNG for pedals, headers, knobs, badges, and plates where possible.
- Do not bake knob labels into the image unless the final layout is locked.
- Do not bake the output waveform/scope line into the pedal art.
- Keep fake/random generated text off final production layers.
- If using chroma key removal, use flat `#00ff00` only for temporary sources, then export transparent PNGs here.

