# Validation

## Included checks

`vortex-audio-tests` checks a PCM16 stereo WAV roundtrip, output metadata, finite DSP samples, and wet/dry impulse behavior.

## Windows x64 result — 2026-11-18

- Generator: Visual Studio 18 2026, x64.
- Compiler: MSVC 19.51.36260 (toolset directory 14.51.36231), Windows SDK 10.0.26100.
- CMake configure: passed with `windows-vs2026-x64`.
- Release build: passed for `TROLLfx-Vortex-Lab`, `vortex-render`, `vortex-play`, and tests, with no compiler warnings.
- CTest: 3/3 passed (`vortex-audio-tests`, `vortex-session-tests`, `vortex-gui-startup`).
- GUI smoke test launched the Windows-subsystem executable, found its visible window, confirmed it remained open for 500 ms, sent close, and observed clean exit.
- Session tests cover bounded command delivery, live morph updates and smoothing, output bounds/meters, pause/resume, stop/restart, looping, end-of-file, WAV export, and missing/malformed WAV rejection.
- GUI executable: `C:\Users\Gamer\vortex-win\build\windows-vs2026-x64\Release\TROLLfx-Vortex-Lab.exe`.
- Renderer and console player remain in the same `Release` directory.

The canonical checkout remains at `/home/gamer/vortex`. Visual Studio projects were built in a Windows filesystem worktree because MSBuild against the WSL UNC path produced path-case and incremental-build warnings. No Linux object files were used. WinMM initialization and buffer submission compile, but physical audio playback and device removal/recovery were not exercised.
