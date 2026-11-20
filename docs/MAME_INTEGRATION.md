# MAME integration

## Core and machine driver

The emulator uses MAME 0.289 at source commit `d19b5702bdbd1e6c14ecd522e9236154468fc4ef`. The local `tiny` profile is prepared by `tools/mame/prepare_mame.py`; no MAME CPU code is copied or reimplemented. The driver in `tools/mame/vortex_exec.cpp` instantiates MAME's `adsp2105_device`, loads the first boot page with the core's `load_boot_data`, and provides bounded PM/DM spaces.

The driver records on-chip MMR writes, board latch accesses, status reads, and unsupported memory accesses. The first unsupported external PM read asserts the core HALT input. This prevents execution of fabricated `0xFFFFFF` data as if it were valid firmware. Board external wait states and ROM bank translation remain to be implemented.

The machine list has two systems:

- `vortexexec`: local user-supplied verified firmware. ROM bytes are never checked into Git.
- `vortexsynthetic`: a no-dump ROM slot for public tests that generate their own 64 KiB synthetic firmware.

The Windows Release executable was built natively with VS Community 18/MSVC v145 and Genie-generated VS2022 project files. The `tiny` profile contains only the Vortex driver and ADSP21XX CPU. WASAPI/XAudio2 modules are omitted. Run with `-video none -sound none`; no physical audio device or graphical window is used.

## Windows CLI runner

`tools/mame/run_headless.ps1` validates the local image (64 KiB and expected SHA-256), stages it under a temporary MAME ROM directory, performs deterministic soft reset, invokes the MAME debugger, and writes:

- requested CSV trace with PC, opcode at the current instruction hook, MAME disassembly, cumulative/prior-instruction cycles, AX0/AX1/AY0/AY1, AR, MSTAT/SSTAT/IMASK/ICNTL, and IRQ0–IRQ3;
- `.mame.trace`, the original MAME debugger trace;
- `.hardware.log`, driver-reported hardware accesses;
- `.run1.log` (and `.run2.log` with repeat mode), MAME process output.

Examples in Windows PowerShell:

```powershell
tools\mame\run_headless.ps1 `
  -Rom 'C:\Users\Gamer\vortex-win\research_local\Lexicon Vortex 1.1 (27C512).BIN' `
  -Instructions 128 -Trace C:\Users\Gamer\research_local\boot.csv -Repeat

tools\mame\run_headless.ps1 `
  -Rom C:\Users\Gamer\vortex-win\research_local\Lexicon.bin `
  -Cycles 100000 -Trace C:\Users\Gamer\research_local\cycles.csv

tools\mame\run_headless.ps1 `
  -Rom C:\Users\Gamer\vortex-win\research_local\Lexicon.bin `
  -Instructions 4096 -Trace C:\Users\Gamer\research_local\compact.csv `
  -TraceDetail compact
```

`-TraceDetail full` (the default) includes the sampled register and IRQ columns. `compact` retains PC, opcode, disassembly, and cycle columns for longer runs. Both formats use the same MAME-generated instruction records; `-Repeat` compares the selected CSV plus the hardware log.

MAME debugger expressions print the opcode from its live program space (`d@pc & 0xffffff`), and the operation is MAME's decoder output. `softreset` is required at the start of the debugger script because MAME's initial debugger script runs after its first reset-vector instruction. With the explicit reset, the trace starts with `PC=0000`, `OP=1804EF`, `JUMP $004E`.

MAME debugger numeric expressions default to hexadecimal. The runner emits an explicit `0x` step count. Instruction mode stops after the requested number of debugger steps unless the driver halts first. Cycle mode uses the requested count as a step cap; the tested boot prefix consumes at most one MAME core cycle per instruction. The CSV records `totalcycles` at each pre-instruction hook and `lastinstructioncycles` for the preceding instruction. Since the final instruction can finish after the last trace hook, this is an instruction-boundary cycle cap, not an asynchronous exact-cycle stop. Multi-cycle instructions or external wait states may exceed the requested cycle count; the current driver halts before the first unsupported external access. External-memory wait states are not modeled, so counts are not board-accurate timing.

`-Repeat` launches twice from reset and byte-compares both the CSV instruction/register/opcode traces and hardware logs. It fails on any mismatch.

The CSV captures CPU state per instruction, with memory operands in MAME disassembly. The separate hardware log captures accesses handled by the board maps, including unsupported reads/writes and latches. It does not yet hook every indirect internal PM/DM access as a separate event. IRQ0–IRQ3 values are sampled line levels per trace record; no interrupt edge was observed in the initial firmware prefix.

## Tests

With the native MAME executable available, run:

```powershell
tests\test_mame_synthetic.ps1
```

The test generates public synthetic 64 KiB code and checks PC 0 boot, the `JUMP $004E` branch, register state, MMR write, codec-reset latch, five-instruction limit, and repeated trace identity. It does not require Lexicon firmware. Existing Python tests remain independent and run with:

```sh
python3 -m unittest tests.test_adsp_boot tests.test_rom_image -v
```

## Local native build outline

The build source lives in a separate MAME worktree at the pinned commit. Use bundled Genie with `--target=mame --subtarget=tiny --osd=windows --targetos=windows --PLATFORM=x64 vs2022`, then build `build/projects/windows/mametiny/vs2022/mametiny.sln` in Release/x64 through MSBuild with `PlatformToolset=v145`. MAME's generated version/driver/resource files must be created before MSBuild; see `docs/BUILD_WINDOWS.md` for project prerequisites. The resulting console application is `C:\Users\Gamer\mame-vortex\mametiny.exe`.

MAME source references: [ADSP core at pinned commit](https://github.com/mamedev/mame/blob/d19b5702bdbd1e6c14ecd522e9236154468fc4ef/src/devices/cpu/adsp2100/adsp2100.cpp), [ADSP disassembler](https://github.com/mamedev/mame/blob/d19b5702bdbd1e6c14ecd522e9236154468fc4ef/src/devices/cpu/adsp2100/2100dasm.cpp). MAME source files carry BSD-3-Clause notices; distribution of an MAME binary must follow MAME's aggregate licensing requirements.
