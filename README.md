# TROLLfx Vortex Lab

An independent PC research prototype for stereo audio processing and evidence-led study of the Lexicon Vortex. Native DSP here is Vortex-inspired and is not original Lexicon firmware emulation.

## Current status

The first milestone scaffold includes a dependency-free stereo WAV renderer and a small shared-state delay morph. The current engine is a functional DSP experiment, not a complete interface or real-time device player. The ROM archive is in local research only and is excluded from Git.

## Build

Requires CMake 3.21+ and a C++20 compiler. On Windows, list installed generators with `cmake -G`; the checked-in preset targets Visual Studio 2026 x64:

```powershell
cmake --preset windows-vs2026-x64
cmake --build --preset windows-release
ctest --preset windows-release
```

`vortex-render input.wav output.wav [morph 0..1] [wet 0..1]` performs an offline render. Input accepts mono or stereo PCM 16/24/32-bit or IEEE float32; the renderer currently requires stereo and writes PCM16.

On Windows, `vortex-play input.wav` loops stereo 44.1 or 48 kHz audio through the default output. Use `[`/`]` or `-`/`+` to adjust morph, `0`–`9` to set its position, and `Q` to stop. This is a keyboard-controlled proof of concept, not the final GUI.

## Research boundaries

See `docs/` for source references, confirmed hardware facts, unresolved questions, and the initial native-engine design. Never commit images from proprietary ROM dumps. `research_local/` remains ignored.
