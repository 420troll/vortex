# ROM forensics

## Archive verification

- **CONFIRMED** local archive: `/home/gamer/vortex/research_local/vortex-1.1-27c512.rar` (outside this Windows worktree and ignored by Git).
- **CONFIRMED** archive size: 12,965 bytes; format: RAR v4, Windows-origin archive (`file` output).
- **CONFIRMED** SHA-256: `8afe37707ce163f55285a755ba275ec5b3029cb630bb48028c67c2d4b9894462` (matches the previously recorded value).
- **UNKNOWN** extracted image name, size, SHA-256, version marker, boot page contents, instruction count, and ROM offsets.
- Extraction was attempted with the available Linux/Windows command paths. No `unrar`, `7z`, `unar`, or `bsdtar` executable/library is available. `powershell.exe` fails with `UtilBindVsockAnyPort` before it can run. Archive bytes were not modified.

## Structure findings

- The archive listing is documented in the prior research record as `Lexicon Vortex 1.1 (27C512).rar`; this session verifies the actual local archive hash, not its contents.
- ADSP-2105 boot packing is established at the architecture level: boot data carries a length byte and 24-bit program words packed from three of every four byte lanes. See [ADSP2105_BOOT.md](ADSP2105_BOOT.md).
- No image-specific boot values, executable regions, constants, ROM-bank transitions, DSP initialization, or algorithms are claimed.

The archive and any extracted firmware are proprietary. Do not commit or publish them. `tools/rom_inspector/decode_adsp_boot.py` accepts a locally extracted 4096-byte page; it prints words, not disassembly.
