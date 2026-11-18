# TROLLfx Vortex Lab

An independent PC research prototype for stereo audio processing and evidence-led study of the Lexicon Vortex. Native DSP here is Vortex-inspired and is not original Lexicon firmware emulation.

## Current status

The first milestone scaffold includes a dependency-free stereo WAV renderer and a small shared-state delay morph. The current engine is a functional DSP experiment, not a complete interface or real-time device player. The ROM archive is in local research only and is excluded from Git.

## Build

Requires CMake 3.20+ and a C++20 compiler. On Windows use a Visual Studio x64 developer shell:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Offline render: `vortex-render input.wav output.wav [morph 0..1] [wet 0..1]`. Input must be mono or stereo PCM 16/24/32-bit or IEEE float32; output is PCM16. The first CLI currently requires stereo.

## Research boundaries

See `docs/` for source references, confirmed hardware facts, unresolved questions, and the initial native-engine design. Never commit images from proprietary ROM dumps. `research_local/` remains ignored.
