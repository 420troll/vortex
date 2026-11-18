# Handoff

## Current state — 2026-11-18

- Project repository: `https://github.com/420troll/vortex`, branch `main`.
- Canonical WSL checkout: `/home/gamer/vortex`.
- ROM archive remains local and ignored; its archive hash and extraction status are documented in `ROM_FORENSICS.md`.
- Windows x64 CMake preset, offline renderer, automated test, and WinMM real-time playback proof of concept are implemented.
- Visual Studio 18 2026 / MSVC 14.51 Release build passed; CTest passed 1/1; Windows WAV render verified.
- Windows build worktree and ignored output are under `C:\Users\Gamer\vortex-win\build\windows-vs2026-x64\` to avoid Visual Studio issues with WSL UNC paths.

## Next actions

1. Add a graphical transport and continuously adjustable morph control over the existing playback host.
2. Add output-device selection, input/output levels, and meters; test on a Windows audio device.
3. Extract the firmware archive locally and record extracted size and SHA-256 without committing firmware.
4. Transcribe the service manual memory tables and schematics into `HARDWARE_MAP.md`.
5. Review exact MAME source version and licensing before considering CPU core integration.

No Force deployment is part of this project phase.
