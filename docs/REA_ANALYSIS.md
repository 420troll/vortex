# REA analysis

## Tool availability

No REA (Reverse Engineering Assistant) MCP tools were present in the tool catalog for this session. The available tools were inspected by searching names and descriptions for REA, binary analysis, disassembly, Ghidra, and radare terms. No REA session could be opened, so no file was uploaded and no REA annotations, references, or disassembly were produced.

The installed/configured state reported by the project owner is preserved; this is a session tool-availability blocker, not a claim that REA is absent from the user's machine. Automated analysis should not be trusted until its ADSP-2105 support is tested against known instruction encodings.

## Work possible without REA

- Verified the local RAR archive size and SHA-256.
- Confirmed the ADSP-2105 boot byte packing against Analog Devices' data sheet and MAME's loader.
- Added a small parser for one 4 KiB boot page. It is a packing decoder only, not an instruction decoder or firmware analysis.

## Required next pass

After extraction, first feed known ADSP-2105 words to the selected REA backend and compare its output with MAME's `2100dasm.cpp` / `adsp2100_disassembler`. Record the REA backend/version, input byte order, word width, and matching examples. Then annotate image regions with both ROM offsets and DSP program addresses. Keep automatic classifications marked as hypotheses until execution traces support them.
