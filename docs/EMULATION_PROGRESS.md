# Original firmware emulation progress

## Session status

- Work is isolated on branch `codex/adsp2105-original-execution`; the Windows worktree and its uncommitted GUI/audio changes were not modified.
- Local ROM: 65,536 bytes, SHA-256 `1e79c704fbef8219b7fc53349064bc1afafd4287c3ca7442bb5587641a450892`. The file remains under ignored `research_local/` and is not included in Git.
- Page 0 is 1,024 24-bit instructions, packed in the format used by MAME's ADSP boot loader. MAME's own disassembler has produced a complete local annotated listing at ignored `research_local/vortex-page0-mame.asm`.
- Reset starts at processor `PC=0`. The first word `0x1804EF` is decoded as `JUMP $004E`; therefore the first candidate initialization routine begins at `PC=0x004E`. PC 1 is not the next executed instruction following this direct jump.
- The remaining first words decode as `AY0 = $0020` (PC 1), `JUMP $0009` (PC 2), and `AY0 = $0200` (PC 3). MAME recognizes all four encodings; only PC 0 is on the reset path without another control-flow edge.
- **Original ROM execution in MAME is not yet achieved.** The Vortex machine driver compiles against the pinned MAME ADSP core, but the full MAME executable has not linked in this container. Therefore there is no genuine ROM instruction trace, observed CPU state, cycle trace, or reached hardware-access blocker.
- REA tools were not available. No ADSP instruction was executed in REA.

## Verification performed

- ROM size and SHA-256 were checked against the user-provided values.
- `tools/mame/build_disassembler.sh` compiled MAME's actual ADSP disassembler at commit `d19b5702bdbd1e6c14ecd522e9236154468fc4ef`; it decoded all 1,024 page-0 words with ROM offsets and processor word addresses.
- The focused Vortex driver and ADSP-2100 CPU/disassembler sources compiled. The MAME executable link failed because this environment lacks a complete SDL/X11 runtime development dependency chain. This is a build-environment blocker; no trace was generated.
- `python3 -m unittest tests.test_adsp_boot tests.test_rom_image -v`: 8 synthetic tests passed. These check parser and page-packing behavior, not the proprietary ROM or MAME execution.
- `git diff --check`: clean.

## First unresolved board dependency

Static analysis of the reset path at `PC=0x004E` shows writes to system control MMR `DM(0x3FFF)`, wait-state MMR `DM(0x3FFE)`, then `DM(0x1F80)` at `PC=0x0053`, followed by a display write at `DM(0x1800)` at `PC=0x0055`. The service-manual map documents the display and nearby EPROM/bank-select ranges but does not identify `0x1F80`. Thus `DM(0x1F80)` is the first *statically identified candidate* unresolved board dependency, not an observed execution blocker.

## Next minimum work

1. Complete the pinned MAME build with SDL/X11 development dependencies, or use a headless MAME build configuration that still includes the debugger.
2. Run the local ROM script twice and capture an actual trace, MAME CPU state, cycles, and first unknown access. Validate PM contents against the 1,024-word listing.
3. Confirm `DM(0x1F80)` semantics from schematics or trace and implement only the minimum necessary mapping before proceeding to later initialization.

Do not infer normal Vortex startup or authentic audio from instruction decoding alone.
