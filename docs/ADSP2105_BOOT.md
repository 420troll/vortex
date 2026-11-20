# ADSP-2105 boot format

## Confirmed architecture behavior

The Analog Devices ADSP-2105 data sheet describes a 32 KiB byte-wide boot space split into eight 4 KiB pages. With `MMAP=0`, reset automatically boots page 0 into internal PM addresses `0x0000–0x03ff`; the reset vector is `PC=0x0000`. The processor has 1,024 × 24-bit program words and 512 × 16-bit data words. The page-length byte selects a count in groups of eight words.

MAME's ADSP core resets `PC` to zero and exposes `load_boot_data`, which computes `(srcdata[3] + 1) * 8` words and packs bytes 0–2 of each four-byte unit into one 24-bit word. The ADSP core does **not** automatically call this helper on reset. The harness copies page 0 into the driver's `0x0000–0x03ff` program RAM using MAME's helper to represent the ADSP hardware boot transfer. The Vortex `MMAP` pin and 64 KiB EPROM bank wiring remain to be checked against schematics.

## Page 0 disassembly

MAME's ADSP disassembler produces these first words, with ROM file offsets and processor word addresses:

| File offset | PC | Word | Instruction |
|---:|---:|---:|---|
| `0x0000` | `0x0000` | `0x1804EF` | `JUMP $004E` |
| `0x0004` | `0x0001` | `0x400204` | `AY0 = $0020` |
| `0x0008` | `0x0002` | `0x18009F` | `JUMP $0009` |
| `0x000C` | `0x0003` | `0x402004` | `AY0 = $0200` |

All four encodings are recognized by MAME's decoder as ADSP instructions. Their control-flow reachability differs: the reset branch at PC 0 targets `0x004E`; MAME applies this direct jump immediately, so PC 1 is decoded but is not the next executed instruction. The full annotated listing for all 1024 words is available locally at ignored `research_local/vortex-page0-mame.asm`.

References: [Analog Devices ADSP-2105 data sheet](https://www.analog.com/media/en/technical-documentation/data-sheets/adsp-2105.pdf); [MAME ADSP-2100 core](https://github.com/mamedev/mame/blob/master/src/devices/cpu/adsp2100/adsp2100.cpp).

## Local parser

`tools/rom_inspector/rom_image.py` validates the user-provided 64 KiB image size and SHA-256 and decodes one aligned page. Eight Python unit tests use independent synthetic fixtures; `tests/test_mame_synthetic.ps1` additionally drives MAME's CPU with generated public synthetic firmware. `tools/mame/build_disassembler.sh` compiles MAME's own `2100dasm.cpp` from pinned source to create the annotated local listing.

## Vortex-specific unknowns

- Whether the 27C512 is physically wired as a direct 64 KiB window, how its 32 KiB boot-visible space is banked, and what A12/A14/A15 latches do during execution.
- Whether the physical board straps `MMAP=0`, or how it maps the 64 KiB EPROM onto the 32 KiB ADSP boot window.
- Whether the image's nine plausible 4 KiB page headers represent boot pages, bank contents, or another organization.
- Which PC ranges are reachable from normal reset and any later software boot-page load.

The processor's reset behavior and image's reset-vector jump have now also been observed in MAME execution. The first debugger trace missed PC `0x0000` because MAME processes the `-debugscript` after its first reset-vector instruction. The reproducible script begins with `softreset`, then enables tracing; the captured row is `PC=0000 OP=1804EF JUMP $004E`, followed by the instruction at `PC=004E`. The word is fetched live from the MAME CPU's program address space for the trace, not copied from a predicted disassembly.

The original-ROM trace proceeds through `PC=0053 DM($1F80)=AX0`, which asserts the codec's active-low reset, and later to the first external PM read at `PC=019F`, `PM(0x0800)`. It has not yet verified external PM data or firmware beyond that read.
