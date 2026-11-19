# MAME integration assessment

## Source review

The current MAME tree contains the ADSP-2100 family core and disassembler under `src/devices/cpu/adsp2100/`, and DCS board code under `src/mame/shared/dcs.cpp`. The core implements the ADSP-2105 memory map, instruction semantics, hardware loop/stack state, interrupt state, SPORT registers, wait-state configuration, and cycle execution through MAME's device/scheduler model. `load_boot_data` implements page-length and byte-to-24-bit packing. The DCS code demonstrates MAME's boot-data loading and serial/interrupt integration patterns, but DCS is not the Lexicon board.

The current repo environment did not contain current MAME source; an unrelated older PinMAME tree includes a legacy ADSP core and disassembler. It is not being treated as authoritative for current MAME implementation details. Sources: [MAME ADSP core](https://github.com/mamedev/mame/blob/master/src/devices/cpu/adsp2100/adsp2100.cpp), [header](https://github.com/mamedev/mame/blob/master/src/devices/cpu/adsp2100/adsp2100.h), [disassembler](https://github.com/mamedev/mame/blob/master/src/devices/cpu/adsp2100/2100dasm.cpp), [DCS support](https://github.com/mamedev/mame/blob/master/src/mame/shared/dcs.cpp).

## Integration choice

**Recommended: minimal MAME machine driver.** It keeps the CPU core, address spaces, timers, and scheduler in their native framework and gives correct cycle/interrupt/SPORT integration with less semantic risk than extracting a CPU core. A standalone harness is attractive for a small build, but pulling apart MAME internals risks losing memory timing and changing core behavior. Do not copy core files into the existing TROLLfx library.

Implement a Vortex machine driver with the processor at the documented 10 MHz clock, explicit boot ROM/page callback, program/data maps, bank latch, DRAM interface, and logged unmodeled reads/writes. Add codec and controls after a ROM trace shows which devices initialization actually touches. Pin the MAME revision and preserve its BSD-3-Clause source notices; verify exact current license/dependency scope before distribution. MAME's DCS code is a pattern, not a ready-made Lexicon device.

## Status

No MAME source was vendored or built, and no CPU instruction has executed in this session. The absence of an extracted ROM is the immediate blocker to validating the machine path. A minimal MAME build is a follow-up after the archive is extractable and its boot page parses.
