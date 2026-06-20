# Build Notes

The repo contains a JUCE VST3/Standalone project. Visual Studio Build Tools 2022 are installed at:

```text
C:\BuildTools
```

Use the local build script so the environment is cleaned before MSBuild runs:

```powershell
cmd /c tools\build_vs.cmd
```

This script clears a duplicate `PATH`/`Path` environment key that otherwise causes MSBuild to fail with:

```text
System.ArgumentException: Item has already been added. Key in dictionary: 'PATH' Key being added: 'Path'
```

Current output:

- VST3 target: `build-vs/SpectralRot_artefacts/Release/VST3/Spectral Rot.vst3`
- Standalone target: `build-vs/SpectralRot_artefacts/Release/Standalone/Spectral Rot.exe`

The VST3 has also been copied to the per-user VST3 path:

```text
C:\Users\tremm\AppData\Local\Programs\Common\VST3\Spectral Rot.vst3
```

The system-wide VST3 path exists but was not writable from this shell:

```text
C:\Program Files\Common Files\VST3
```

Next step: scan the user VST3 folder in a DAW and tune each mode against sine bass, distorted Serum/Rift chains, and resampled break material.
