# ROM forensics

## Archive verification

- **CONFIRMED** local archive: `/home/gamer/vortex/research_local/vortex-1.1-27c512.rar` (outside this Windows worktree and ignored by Git).
- **CONFIRMED** archive size: 12,965 bytes; format: RAR v4, Windows-origin archive (`file` output).
- **CONFIRMED** SHA-256: `8afe37707ce163f55285a755ba275ec5b3029cb630bb48028c67c2d4b9894462` (matches the previously recorded value).
- **CONFIRMED** extracted image: `Lexicon Vortex 1.1 (27C512).BIN`, 65,536 bytes, SHA-256 `1e79c704fbef8219b7fc53349064bc1afafd4287c3ca7442bb5587641a450892`.
- The image was located by the supplied hash in the ignored Windows worktree and remains untracked. The execution branch uses that local path as a user-provided input; the binary is not copied into the branch.

## Structure findings

- Page-aligned 4 KiB regions with boot length bytes within the ADSP-2105 limit are candidates at offsets `0000`, `1000`, `3000`, `6000`, `7000`, `A000`, `B000`, `E000`, and `F000`. Offsets `2000`, `4000`, `5000`, `8000`, `9000`, `C000`, and `D000` begin with `0xFF` and request 2048 words, beyond the 1024-word internal PM limit. These are candidate boundaries only; they do not establish ROM-bank wiring.
- ADSP-2105 boot packing is established at the architecture level: boot data carries a length byte and 24-bit program words packed from three of every four byte lanes. See [ADSP2105_BOOT.md](ADSP2105_BOOT.md).
- Page 0 at file offset `0x0000` declares 1024 words. MAME's ADSP disassembler decodes PC `0000–0003` as `JUMP $004E`, `AY0 = $0020`, `JUMP $0009`, and `AY0 = $0200`. An annotated full-page listing is generated locally at ignored `research_local/vortex-page0-mame.asm`; it is intentionally not committed with the proprietary image.
- The local boot-page disassembler uses MAME commit `d19b5702bdbd1e6c14ecd522e9236154468fc4ef`. It invokes MAME's `adsp21xx_disassembler`; that decoder does not independently mark every reserved encoding invalid. Only CPU execution validates reachable instructions.
- Page `0x3000` begins with writes to `DM(0x1D40)`, `DM(0x1CC0)`, and `DM(0x1F40)`, matching address families in the current handoff memory table. This is code/data evidence, not proof of the corresponding latch semantics.
- No ROM-bank transition, DRAM sequence, SPORT setup, or algorithm is confirmed solely from this static scan.

The archive and extracted firmware are proprietary. Do not commit or publish them. `tools/rom_inspector/rom_image.py` verifies the supplied image size/hash and decodes a selected 4096-byte page. `tools/mame/build_disassembler.sh` compiles MAME's unmodified ADSP disassembler and writes the local listing.
