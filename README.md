# TROLLfx Vortex Lab

An independent PC research prototype for stereo audio processing and evidence-led study of the Lexicon Vortex. It includes a portable DSP library, WAV tools, a WinMM playback host, and a native Win32 GUI. DSP here is Vortex-inspired and is not original Lexicon firmware emulation.

## Current status

The first playable GUI provides stereo WAV transport, continuous morphing, levels, output meters, and export. The effects remain a TROLLfx DSP experiment. The ROM archive is in local research only and excluded from Git.

## Build

Requires CMake 3.21+ and a C++20 compiler. On Windows, list installed generators with `cmake -G`; the checked-in preset targets Visual Studio 2026 x64:

```powershell
cmake --preset windows-vs2026-x64
cmake --build --preset windows-release
ctest --preset windows-release
```

`vortex-render input.wav output.wav [morph 0..1] [wet 0..1]` performs an offline render. Input accepts mono or stereo PCM 16/24/32-bit or IEEE float32; the renderer currently requires stereo and writes PCM16.

On Windows, double-click `TROLLfx-Vortex-Lab.exe` to launch the GUI without a terminal window. It opens stereo 44.1/48 kHz WAV files, provides Play/Pause/Stop/Loop, continuous A/B morph, wet/dry and input/output levels, stereo output meters, and WAV export. `vortex-play input.wav` remains available as a console/debug player.

## Research boundaries

See `docs/` for source references, confirmed hardware facts, unresolved questions, and the initial native-engine design. Never commit images from proprietary ROM dumps. `research_local/` remains ignored.
