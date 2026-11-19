# Original firmware emulation progress

## Session status

- Emulator work is isolated in `C:\\Users\\Gamer\\vortex-emulation`, based on `2a31e49`; the Windows GUI/audio worktree was not modified. Native MAME sources are a separate pinned worktree at `C:\\Users\\Gamer\\mame-vortex`.
- Local ROM: 65,536 bytes, SHA-256 `1e79c704fbef8219b7fc53349064bc1afafd4287c3ca7442bb5587641a450892`. The file remains under ignored `research_local/` and is not included in Git.
- Page 0 is 1,024 24-bit instructions, packed in the format used by MAME's ADSP boot loader. MAME's own disassembler has produced a complete local annotated listing at ignored `research_local/vortex-page0-mame.asm`.
- Reset starts at processor `PC=0`. The first word `0x1804EF` is decoded as `JUMP $004E`; therefore the first candidate initialization routine begins at `PC=0x004E`. PC 1 is not the next executed instruction following this direct jump.
- The remaining first words decode as `AY0 = $0020` (PC 1), `JUMP $0009` (PC 2), and `AY0 = $0200` (PC 3). MAME recognizes all four encodings; only PC 0 is on the reset path without another control-flow edge.
- **Original firmware execution in MAME is achieved for the initial boot routine.** A native Windows x64 `mametiny.exe` linked from pinned MAME commit `d19b5702bdbd1e6c14ecd522e9236154468fc4ef`, with the ADSP21XX CPU and Vortex driver. MAME's debugger `step 128` trace executed from PC `0x004E` through subsequent original instructions. It is an executed instruction trace, not just static disassembly. The first trace row reports PC `0x004E`; the debugger attached after reset's boot transfer and branch, so this capture does not itself show PC 0's branch instruction. The trace shows each instruction at one reported cycle in this capture; do not interpret this as board-accurate external wait-state timing.
- REA tools were not available. No ADSP instruction was executed in REA.

## Verification performed

- ROM size and SHA-256 were checked against the user-provided values.
- `tools/mame/build_disassembler.sh` compiled MAME's actual ADSP disassembler at commit `d19b5702bdbd1e6c14ecd522e9236154468fc4ef`; it decoded all 1,024 page-0 words with ROM offsets and processor word addresses.
- Native Windows Release build linked `C:\\Users\\Gamer\\mame-vortex\\mametiny.exe`; `-version` reports MAME 0.289 at the pinned commit and `-listfull vortexexec` registers the system. WASAPI/XAudio2 sources and registrations were excluded; the executable was run with `-video none -sound none`, with no physical audio device or window required. MSBuild's link succeeded, though its optional post-build step reports `pwsh.exe` missing; the produced executable runs.
- Captured 128-instruction debugger trace: `C:\\Users\\Gamer\\mame-vortex\\research_vortexexec\\boot.trace` (ignored local research artifact). It records AX0/AX1/AY0/AY1, AR, MSTAT, SSTAT, IMASK, instruction cycles, total cycles, PC and MAME-decoded operation. At `PC=0x004E`, `IMASK=0`; at `0x004F`, `AX0=0x081A`; at `0x0050`, the core executes `DM($3FFF) = AX0`; at `0x0052`, it executes `DM($3FFE) = AX0`; at `0x0053`, it executes `DM($1F80) = AX0`.
- `python3 -m unittest tests.test_adsp_boot tests.test_rom_image -v`: 8 synthetic tests passed. These check parser and page-packing behavior, not the proprietary ROM or MAME execution.
- `git diff --check`: clean.

## First unresolved board dependency

The first observed external-device operation is the executed write `PC=0x0053: DM(0x1F80)=AX0` (`AX0=0x1000`). The Vortex service-manual memory-map table identifies `0x1F80` as active-low `CODEC_RST/`; `0x1FC0` is its deassert/release alias. The harness now records the reset latch and logs the codec as unmodeled. This is the first hardware dependency reached, but not yet a blocker: execution continued after the write in the 128-instruction trace. The next display operation is `PC=0x0055: DM(0x1800)=AX0`. The CS4216 codec itself, external EPROM mapping, DRAM, and board timing are not modeled.

## Next minimum work

1. Add a stable Windows command-line wrapper with the requested `--rom/--cycles/--trace` interface; the current native interface is MAME's `vortexexec` system and debugger script (`step N`), not a standalone runner.
2. Attach the debugger before the reset vector to capture PC 0 and the executed `JUMP $004E`, and add opcode words directly to the CSV trace. The current trace's disassembly is from MAME's debugger; its first record starts at `0x004E`.
3. Add repeat-run trace comparison and public synthetic integration tests. Existing 8 synthetic parser tests do not exercise the MAME CPU.
4. Model the next device only when the trace shows its behavior matters; codec reset is observed and logged, but codec/SPORT operation is not modeled.

Do not infer normal Vortex startup or authentic audio from instruction decoding alone.
