# ADSP-2105 boot analysis

## Status

- **CONFIRMED** the target CPU family is ADSP-2105 (service manual).
- **UNKNOWN** Vortex EPROM boot image encoding and word packing. Do not feed raw EPROM bytes directly to an instruction decoder.
- **UNKNOWN** start address, boot transfer length, initialization sequence, and external EPROM bank transitions.
- **INFERRED** correct execution requires the CPU core plus board-specific memory-mapped data, ROM banking, external delay RAM, status signals, codec I/O, and timing/interrupt behavior.

## MAME assessment

MAME's `adsp2100` device family and disassembler provide a credible base for instruction semantics and decoding. The MAME core is designed within MAME's device framework; a standalone harness is possible but requires extracting/adapting framework dependencies or embedding the device in a minimal MAME driver. This should be decided after ROM boot format is known. Do not copy implementation files before reviewing the MAME GPL-2.0-or-later license and dependency closure.

Initial harness should instrument fetches and memory accesses and validate a short known instruction stream before adding codec or real-time peripherals. CPU instruction execution alone is not evidence of successful firmware operation.
