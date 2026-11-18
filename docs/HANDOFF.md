# Handoff

## Current state — 2026-11-18

- Repository: `https://github.com/420troll/vortex`, branch `main`.
- Canonical WSL checkout: `/home/gamer/vortex`.
- Native Win32 GUI target: `TROLLfx-Vortex-Lab.exe`; console tools remain available.
- DSP/audio session is separate from WinMM and the UI. UI commands travel through a bounded SPSC queue.
- Release build passed with Visual Studio 18 2026 / MSVC 14.51. CTest passed 3/3, including GUI launch/close and transport/DSP integration tests.
- Windows build worktree and executable: `C:\Users\Gamer\vortex-win\build\windows-vs2026-x64\Release\TROLLfx-Vortex-Lab.exe`.
- The WinMM worker now uses a completion-event-driven three-buffer queue and reuses only headers marked done. Device timing probe: direct one-second buffers completed in about 1.001 s at both rates; 1,024-frame queued playback completed 44.1/48k frames in 1.021/1.000 s. At 256 frames, queued playback took 2.29/2.47 s, identifying the small-buffer WinMM submission cadence as the scheduling defect on the tested Microsoft Sound Mapper.
- Acoustic playback has not been auditioned or loopback-captured, and device removal/recovery has not been tested. See `VALIDATION.md` for exact evidence and limits.

## Next actions

1. Audition the timing probe and GUI on the intended physical Windows audio device; verify loop boundaries, controls, and recovery after device removal.
2. Add output-device selection and improve reconnect behavior after audio hardware changes.
3. Extract local firmware archive and record extracted size/hash without committing it.
4. Transcribe service-manual memory tables and schematics into `HARDWARE_MAP.md`.
5. Review MAME source version and licensing before considering CPU integration.

No Force deployment is part of this phase.
