# ADSP-2105 boot analysis

## Status

- **CONFIRMED** the target CPU family is ADSP-2105 (service manual).
- **CONFIRMED** ADSP-2105 boot interface: 32 KiB byte-wide space in eight 4 KiB pages; reset auto-loads page 0 when `MMAP=0`; boot bytes are packed as 24-bit PM words. MAME uses byte 3 for `(length + 1) * 8` words and bytes 0–2 of each four-byte unit for each instruction word.
- **UNKNOWN** whether this board uses that reset mode, and Vortex-specific ROM contents, length, initialization, and bank transitions.
- **INFERRED** correct execution requires the CPU core plus board-specific memory-mapped data, ROM banking, external delay RAM, status signals, codec I/O, and timing/interrupt behavior.

## MAME assessment

MAME's `adsp2100` family core and disassembler provide the authoritative execution/decoding base. Its ADSP source carries BSD-3-Clause notices. A minimal MAME driver is preferred over extracting CPU semantics into a standalone emulator. See `ADSP2105_BOOT.md` and `MAME_INTEGRATION.md`.

Initial harness should instrument fetches and memory accesses and validate a short known instruction stream before adding codec or real-time peripherals. CPU instruction execution alone is not evidence of successful firmware operation.
