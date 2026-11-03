# Pulse Forge v0.2 — versatile hybrid drum instrument

Pulse Forge is a JUCE/C++ VST3 drum instrument prototype designed for FL Studio.
This version adds a practical hybrid workflow: each of the four voices can use a synthesized
sound or load a WAV/AIFF sample, with sample playback available independently for each voice.

## Features in v0.2
- Four voices: Kick, Snare, Hat, Perc.
- Per-voice source selector: Synth or Sample.
- Load WAV/AIFF sample files per voice from the plugin UI.
- 16-step host-synchronised sequencer.
- Per-step enable/disable and MIDI audition (36 Kick, 38 Snare, 42 Hat, 45 Perc).
- Global Mutation: adjusts synthesis character and saturation.
- Output gain control.
- Save/restore pattern, source selection, sample path references, Mutation and output gain.
- Versatile by design: use pure synthesized drums, samples, or mix sources across voices.

## Build on Windows
Requirements for a local build: Visual Studio 2022 with Desktop development with C++, CMake, and JUCE 8. The included GitHub Actions workflow pins JUCE 8.0.6 and builds the VST3 on a Windows runner.

1. Download/extract JUCE from https://juce.com/get-juce/
2. In Developer PowerShell, from this folder:
   `cmake -S . -B build -DJUCE_DIR="C:/path/to/JUCE"`
3. Compile:
   `cmake --build build --config Release`
4. Find `Pulse Forge.vst3` in the build output and install/copy the bundle to:
   `%COMMONPROGRAMFILES%/VST3`
5. Rescan plugins in FL Studio.

## Build automatically on GitHub (Windows x64)

1. Create a new GitHub repository and upload the contents of this project folder (including `.github/workflows/build-windows.yml`).
2. Push to the `main` or `master` branch, open a pull request, or open **Actions → Build Pulse Forge VST3 (Windows x64) → Run workflow**.
3. Wait for the workflow to finish. A successful build publishes an artifact named `PulseForge-Windows-x64-VST3` for 14 days.
4. Download that artifact from the completed workflow run and extract `PulseForge-Windows-x64-VST3.zip`. Copy the included `Pulse Forge.vst3` folder to `%COMMONPROGRAMFILES%\VST3`, then rescan plugins in FL Studio.

The workflow compiles the project; it does **not** automatically test audio output, FL Studio host sync, or runtime stability. A green build only confirms that the compiler and linker succeeded.

## Notes
- Sample paths are saved in the plugin state; keep source files at stable paths.
- This prototype supports WAV/AIFF via JUCE's audio format readers.
- The sequencer uses host playhead timing. Host transport / loop / seek behavior should be
  tested in FL Studio; sample-accurate event scheduling and advanced swing are future work.
- This is source code, not a precompiled VST3 binary. Build and test before production use.
