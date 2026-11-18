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
- PCM tests verify 44.1/48 kHz stereo PCM16 format values, per-frame byte sizing, exact one-second WAV frame counts and DSP bypass data preservation.
- GUI executable: `C:\Users\Gamer\vortex-win\build\windows-vs2026-x64\Release\TROLLfx-Vortex-Lab.exe`.
- Renderer and console player remain in the same `Release` directory.

## WinMM timing investigation — 2026-11-18

The original player used `CALLBACK_NULL` and repeatedly polled `WHDR_DONE` with `Sleep(1)`. The source format math was already correct: two channels, 16-bit PCM, `nBlockAlign=4`, and `nAvgBytesPerSec=sampleRate*4`; buffers are sized in stereo frames, not individual channel samples. This did not explain the reported slow playback.

Added `vortex-winmm-timing-probe.exe`, a manual Windows device probe that sends one second of a 440 Hz stereo PCM16 signal directly to WinMM, then exercises the application's bypassed multi-buffer path. On the default `Microsoft Sound Mapper` device:

| Check | 44.1 kHz | 48 kHz |
|---|---:|---:|
| Direct one-buffer completion | 1.001 s | 1.001 s |
| Three-buffer queue, 256 frames per buffer | 2.290 s / 44,100 completed frames | 2.468 s / 48,000 completed frames |
| Three-buffer queue, 1,024 frames per buffer | 1.021 s / 44,100 completed frames | 1.000 s / 48,000 completed frames |

The buffered runs used DSP bypass; measured render/quantization maxima were below 60 microseconds. The 256-frame queue showed that individual short WinMM submissions did not sustain the requested rate on this mapper even though the PCM format and direct one-second transfer were correct. The host now uses three 1,024-frame buffers (4,096 bytes each at PCM16 stereo), and the same queued timing probe completed one second of frames in approximately one second with no queue-empty events. This is the evidence for the scheduling adjustment; do not generalize it to every WinMM driver without testing there.

The worker waits on a WinMM completion event and a shutdown event, inspects `WHDR_DONE`, and returns only completed headers to the free pool. It renders into fixed arrays and submits recycled blocks; it performs no file or UI work and takes no blocking application lock. Queue-empty counts indicate when the app's tracked queue reached zero; legacy WinMM does not expose every downstream hardware underrun. The GUI reports requested WinMM format, block frames/bytes, queued headers, queue-empty events, render time, sent/completed frames, and device error code.

The probe exercised the Windows default waveOut endpoint and measured timing. No person listened to the output, no physical speaker/headphone acoustic check or loopback capture was made, and stop/reopen after actual device removal was not verified. Thus audible correction on the user's physical output device remains to be confirmed. Automated one-second tests validate WAV frame duration and bypass preservation but do not substitute for device playback.

The canonical checkout remains at `/home/gamer/vortex`. Visual Studio projects were built in a Windows filesystem worktree because MSBuild against the WSL UNC path produced path-case and incremental-build warnings. No Linux object files were used.
