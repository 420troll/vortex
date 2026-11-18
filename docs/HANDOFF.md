# Handoff

## Current state — 2026-11-18

- New local project at `/home/gamer/vortex`, separate from the active `/home/gamer/TROLLfx_RTA` checkout.
- GitHub repository `https://github.com/420troll/vortex` exists; local `main` is initialized and tracks no remote branch yet.
- Research archive downloaded to ignored `research_local/`; archive SHA-256: `8afe37707ce163f55285a755ba275ec5b3029cb630bb48028c67c2d4b9894462`.
- Archive extraction has not run. The 65,536-byte firmware image hash and ROM analysis are unknown.
- Service manual downloaded to `/tmp/vortex-service.pdf` (not part of repository). The PDF is scanned; detailed memory-map transcription still needs page-by-page visual review.
- Prototype code and tests are present. Build verification is currently blocked because CMake is not installed in this environment. g++ is available; a direct compiler check is the next local action.

## Next actions

1. Confirm the GitHub repository is ready for push, commit this scaffold, and push `main`.
2. Install/use CMake in the development environment and run the included test target.
3. Extract the archive locally using an available RAR4 tool; record extracted bytes and SHA-256 only in docs.
4. Visually transcribe the service manual's Theory of Operation pages and schematics into `HARDWARE_MAP.md`.
5. Add exact MAME source version/license review before considering code integration.

No Force deployment is part of this project phase.
