# Validation

## Included checks

`vortex-audio-tests` checks a PCM16 stereo WAV roundtrip, output metadata, finite DSP samples, and wet/dry impulse behavior.

## Windows x64 result — 2026-11-18

- Generator: Visual Studio 18 2026, x64.
- Compiler: MSVC 19.51.36260 (toolset directory 14.51.36231), Windows SDK 10.0.26100.
- CMake configure: passed with `windows-vs2026-x64`.
- Release build: passed for `vortex-render` and `vortex-play`, with no compiler warnings.
- CTest: 1/1 `vortex-audio-tests` passed.
- Windows offline-render check: passed with a generated 48 kHz stereo fixture; output had 2,400 frames, finite PCM16 samples, and peak 6,000 at 50% wet.
- Renderer: `C:\Users\Gamer\vortex-win\build\windows-vs2026-x64\Release\vortex-render.exe`.
- Player: `C:\Users\Gamer\vortex-win\build\windows-vs2026-x64\Release\vortex-play.exe`.

The canonical checkout remains at `/home/gamer/vortex`. Visual Studio projects were built in a Windows filesystem worktree because MSBuild against the WSL UNC path produced path-case and incremental-build warnings. No Linux object files were used. Actual audio playback requires a working Windows output device and was not exercised in this verification.
