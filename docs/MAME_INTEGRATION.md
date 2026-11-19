# MAME integration

## Chosen approach

The prototype is a small MAME machine driver so instruction execution, address spaces, device scheduling, and debugger tracing stay inside MAME. It is added to MAME's `tiny` subtarget by `tools/mame/prepare_mame.py`; MAME itself is not vendored. The execution build pins upstream commit `d19b5702bdbd1e6c14ecd522e9236154468fc4ef`. A Windows x64 Release console executable has been built with the installed VS18/MSVC v145 toolset and MAME's bundled Genie VS project generator. The local build disables WASAPI and XAudio2 and uses `-video none -sound none`.

The driver source is `tools/mame/vortex_exec.cpp`. It:

- Registers an ADSP-2105 at the service-manual 10 MHz clock.
- Maps the 1K PM boot RAM at `0x0000–0x03ff`, 512 words of DM RAM at `0x3800–0x39ff`, and bounds the CPU address spaces to 14 bits.
- Uses MAME's `load_boot_data` helper at machine reset to transfer page 0 into PM RAM, because the ADSP core resets `PC` but does not itself perform the hardware boot transfer.
- Implements reset defaults for the ADSP DM wait-state register and write latches for the MMRs encountered during startup. These latches record values but do not model timer, SPORT, or wait-state effects.
- Logs every access outside the modeled on-chip RAM/MMRs. Unmapped reads return explicit `0xffff` or `0xffffff` defaults and log the address and current instruction PC; unmapped writes are logged and ignored.
- Handles the service-manual codec-reset aliases `DM(0x1F80)` (assert active-low reset) and `DM(0x1FC0)` (release reset) as logged latches. It does not emulate the CS4216.

The external program/data mappings and the ROM bank latch remain unknown. No guessed bank behavior is installed. The CPU's on-chip memory map and boot mechanics are based on the ADSP-2105 datasheet; the Vortex data memory addresses are cross-checked against `HARDWARE_MAP.md`.

## Windows build/run evidence

The successful local executable is `C:\Users\Gamer\mame-vortex\mametiny.exe`; it reports MAME 0.289 and includes only the `vortexexec` tiny profile. The source tree and build output are separate from the application repository. The reproducible setup currently requires CMake-installed bundled Genie, native MSBuild, and native Python to generate MAME's version/driver/resource files; the final build instructions need a checked-in PowerShell wrapper. The currently verified debugger invocation is:

```powershell
& C:\Users\Gamer\mame-vortex\mametiny.exe vortexexec `
  -rompath C:\Users\Gamer\mame-vortex\roms -video none -sound none `
  -skip_gameinfo -debug -debugscript C:\Users\Gamer\mame-vortex\research_vortexexec\boot.script -log
```

The debugger script runs `step 128`, captures PC, MAME disassembly, selected registers and cycle fields, then exits. The first recorded instruction is at `0x004E`, because MAME has performed the page transfer and the reset branch before this debugger trace starts. It is not yet the requested `--rom/--cycles/--trace` CLI, and its trace has not yet been proven repeat-identical.

## Run locally

The proprietary ROM is not a build input in Git. The user supplies a local path and the runner validates the 65,536-byte size and SHA-256 before copying it to MAME's ignored local ROM directory.

```sh
tools/mame/run_local_rom.sh /path/to/mame-source /path/to/Lexicon\ Vortex\ 1.1\ \(27C512\).BIN 128
```

The runner builds MAME's `tiny` target, starts the harness twice for a fixed instruction budget, captures CPU trace/state/cycle fields, compares the traces for determinism, and requires the first unknown external write (`PC=0x0053`, `DM=0x1F80`) to appear in MAME's log. Generated ROM copies and traces stay under ignored `research_local/` in the MAME source tree.

The disassembly helper also compiles MAME's unmodified ADSP disassembler implementation from the same pinned source:

```sh
tools/mame/build_disassembler.sh /path/to/mame-source
research_local/mame-adsp-disassemble /path/to/ROM.bin 0 > research_local/vortex-page0-mame.asm
```

## Validation boundary

The MAME debugger trace records executed PC/opcode/operation and selected CPU registers/cycles. The memory handlers identify the first unsupported board access. Successful execution through several original instructions demonstrates CPU-core execution only. It does not demonstrate correct external ROM banking, DRAM, reset pin wiring, codec/SPORT operation, full firmware startup, or authentic audio.

MAME sources: [ADSP core](https://github.com/mamedev/mame/blob/d19b5702bdbd1e6c14ecd522e9236154468fc4ef/src/devices/cpu/adsp2100/adsp2100.cpp), [ADSP disassembler](https://github.com/mamedev/mame/blob/d19b5702bdbd1e6c14ecd522e9236154468fc4ef/src/devices/cpu/adsp2100/2100dasm.cpp). MAME's ADSP files carry BSD-3-Clause notices; the MAME application/framework has broader aggregate licensing, so distribution of a binary must follow MAME's license requirements.
