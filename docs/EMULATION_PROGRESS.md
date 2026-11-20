# Original firmware emulation progress

## Current result

- Native Windows x64 MAME executable: `C:\Users\Gamer\mame-vortex\mametiny.exe`, MAME 0.289, source commit `d19b5702bdbd1e6c14ecd522e9236154468fc4ef`.
- Original ROM: 65,536 bytes, SHA-256 `1e79c704fbef8219b7fc53349064bc1afafd4287c3ca7442bb5587641a450892`. It remains ignored/local and is not committed.
- `tools/mame/run_headless.ps1` runs the native MAME driver, checks ROM size/hash, captures a CSV plus raw MAME trace and board-access log, supports instruction or core-cycle budgets, and can run twice to compare determinism.
- Reset tracing now starts at PC `0000`. `softreset` must be issued by the debugger script because MAME processes its initial debugger script only after executing the first reset-vector instruction. The trace reports live PM opcode reads from MAME's address space.
- Repeated original-ROM run (`-Instructions 128 -Repeat`) produces identical CSV and hardware logs. The trace has **48 records**: 47 instructions completed, then the firmware reached `PC=0x019F` and blocked during its external PM operand read with `totalcycles=47`. It does not reach normal audio or a stable processing loop.

## Executed trace excerpt

Trace rows record CPU state immediately before the listed instruction. MAME's opcode field is read from the actual PM space and MAME supplies the disassembly.

```text
PC    OP      Instruction                 AX0    Previous cycles
0000  1804EF  JUMP $004E                  0000   0
004E  3C0003  IMASK = $0000                0000   1
004F  4081A0  AX0 = $081A                  0000   1
0050  93FFF0  DM($3FFF) = AX0              081A   1
0051  410000  AX0 = $1000                  081A   1
0052  93FFE0  DM($3FFE) = AX0              1000   1
0053  91F800  DM($1F80) = AX0              1000   1
0055  918000  DM($1800) = AX0              F000   1
0069  91EC00  DM($1EC0) = AX0              F000   1
...
019F  500001  AX0 = PM(I4,M5)              3564   1
```

The final trace record attempts `PM(0x0800)`. The handler records PC, address, AX/AY/AR, I4/M5 and cycle count, then asserts the ADSP HALT input. The CPU is no longer allowed to continue through fabricated `0xFFFFFF` program data. The access-time pre-state is included in the hardware log.

## Hardware activity observed

The run logs these external writes before the first blocker:

| PC | DM address | Observed action | Evidence |
|---:|---:|---|---|
| `0050` | `3FFF` | write `081A` to ADSP system control MMR | MAME CPU and driver log |
| `0052` | `3FFE` | write `1000` to ADSP wait-state MMR latch | MAME CPU and driver log; full effects not modeled |
| `0053` | `1F80` | assert active-low CS4216 reset | Runtime trace plus service manual Chapter 4, printed page 4-3. The address alias selects the low latch output; data `1000` is irrelevant to polarity. |
| `0055` | `1800` | write `F000` to display latch | Runtime trace plus service manual page 4-3. Scan multiplex timing is not modeled. |
| `0069` | `1EC0` | release active-low ADC chip select | Runtime trace plus service manual page 4-3. TLC549 serial conversion is not modeled. |

No EPROM A12/A14/A15 bank-select writes, DRAM data accesses, EEPROM serial transfers, or SPORT interrupts appear before the first external PM read. Service-manual PM mapping identifies `0800–0FFF` and `2000–2FFF` as EPROM space. The exact translation to the 27C512's byte addresses and 24-bit MAME PM words is unresolved, so no bank model is guessed.

## Determinism and tests

- Original firmware: two independent reset/run processes; CSV opcode/disassembly/register/cycle rows and hardware logs matched byte-for-byte. MAME's cycle counter is 47 before the blocked operand read.
- MAME-backed public fixture: `tests/test_mame_synthetic.ps1` generates a clean synthetic 64 KiB ROM and checks PC0, branch to `004E`, register update, MMR write, codec-reset latch, exact 5-instruction budget, and identical repeat traces. Passed on native MAME/Windows.
- Python synthetic suite: `python3 -m unittest tests.test_adsp_boot tests.test_rom_image -v` (8 tests passed).
- `git diff --check`: clean after final edits.

## Interpretation and next blocker

The processor executes the initial firmware setup and reaches a firmware `DO` loop that reads program memory through `I4=0x0800`. This is not a DSP processing loop: the access is external EPROM data/code space and the harness does not yet map it. The exact CPU register state at the first unsupported access is logged before the operand read: `AX0=0x3564`, `AX1=0`, `AY0=0x00FF`, `AY1=0`, `AR=0x00000800`, `I4=0x0800`, `M5=1`, `PC=0x019F`, `totalcycles=47`.

Next implement and verify the 27C512 program-memory bus and bank translation from the manual's three latch signals and ADSP-2105 external PM interface. Then model the documented two wait states and resume only after the fetched 24-bit word matches bytes from the verified ROM. Do not add a fake ROM mapping, DRAM fill, SPORT timing, or audio until its address and timing behavior are evidenced.

## Branch and isolation

The validated source/docs are committed to `codex/adsp2105-original-execution` at the current follow-up commit. All ROM copies and traces remain under ignored local research directories. The existing Vortex Lab GUI/audio worktree was not modified.
