# Handoff

## Current state — 2026-11-18

- Repository: `https://github.com/420troll/vortex`, branch `main`.
- Canonical WSL checkout: `/home/gamer/vortex`.
- Native Win32 GUI target: `TROLLfx-Vortex-Lab.exe`; console tools remain available.
- DSP/audio session is separate from WinMM and the UI. UI commands travel through a bounded SPSC queue.
- Release build passed with Visual Studio 18 2026 / MSVC 14.51. CTest passed 3/3, including GUI launch/close and transport/DSP integration tests.
- Windows build worktree and executable: `C:\Users\Gamer\vortex-win\build\windows-vs2026-x64\Release\TROLLfx-Vortex-Lab.exe`.
- Physical audio output/device recovery remains untested.

## Next actions

1. Verify playback, pause, stop, loop, end-of-file, and live morph dragging on a Windows audio device.
2. Add output-device selection and improve reconnect behavior after audio hardware changes.
3. Extract local firmware archive and record extracted size/hash without committing it.
4. Transcribe service-manual memory tables and schematics into `HARDWARE_MAP.md`.
5. Review MAME source version and licensing before considering CPU integration.

No Force deployment is part of this phase.
