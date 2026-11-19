# ADSP-2105 boot format

## Confirmed architecture behavior

The Analog Devices ADSP-2105 data sheet describes a 32 KiB byte-wide boot space split into eight 4 KiB pages. With `MMAP=0`, reset automatically boots page 0. The boot interface packs bytes into 24-bit program words and loads on-chip program RAM. The chip has 1,024 × 24-bit program words and 512 × 16-bit data words. The data sheet describes the boot page length byte as a count in groups of eight program words.

MAME's current `adsp2100.cpp` `load_boot_data` implementation computes `(srcdata[3] + 1) * 8` words, then forms each word from `srcdata[i*4+0..2]` as big-endian 24-bit instruction data. Its ADSP-2105 program map places internal program RAM at `0x0000–0x03ff` under the boot mapping. The first execution address is therefore treated as `PC=0` by the core reset/boot path; the Vortex-specific reset wiring and mode pin have not yet been checked against the schematic.

References: [Analog Devices ADSP-2105 data sheet](https://www.analog.com/media/en/technical-documentation/data-sheets/adsp-2105.pdf); [MAME ADSP-2100 core](https://github.com/mamedev/mame/blob/master/src/devices/cpu/adsp2100/adsp2100.cpp).

## Local parser

`tools/rom_inspector/decode_adsp_boot.py` implements this page packing and prints program address, page-relative ROM byte offset, and raw 24-bit word. It deliberately does not label words as valid instructions: MAME's ADSP disassembler has not been run against the Vortex image. A synthetic unit test checks packing and length handling. No firmware page was available to parse during this session.

## Vortex-specific unknowns

- Whether the 27C512 is physically wired as a direct 64 KiB window, how its 32 KiB boot-visible space is banked, and what A12/A14/A15 latches do during execution.
- Whether reset uses the default boot path and whether the image begins at archive offset zero without a header or transform.
- Actual length byte, image entry sequence, loaded-word count, reachable external program regions, and any subsequent software boot-page load.

Thus, boot format is confirmed for the processor family, but no Vortex boot sequence or entry point has been verified from firmware.
