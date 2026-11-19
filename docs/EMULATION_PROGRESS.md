# Original firmware emulation progress

## Session status

- Branch: `codex/original-adsp-emulation`.
- Firmware archive: 12,965 bytes; SHA-256 matches the recorded hash.
- Extraction: blocked; no local RAR utility and Windows PowerShell invocation fails through WSL.
- Boot contract: ADSP-2105 page format confirmed from Analog Devices data sheet and MAME loader. Vortex image-specific boot contents remain unknown.
- Added boot-page parser and synthetic tests; no firmware disassembly generated.
- REA: its MCP tools are unavailable in this session.
- MAME integration: source reviewed online; no core built or machine driver implemented.
- Firmware execution: **not achieved**. No original instruction execution, CPU trace, ROM-bank event, hardware access trace, or audio output is claimed.
- Virtual hardware: none implemented.

## Verification performed

Archive verification command:

```sh
stat -c '%n %s bytes' /home/gamer/vortex/research_local/vortex-1.1-27c512.rar
sha256sum /home/gamer/vortex/research_local/vortex-1.1-27c512.rar
```

Synthetic parser test:

```sh
python3 -m unittest tests.test_adsp_boot -v
```

The parser test validates packing mechanics only. No ROM integrity-after-extraction, CPU execution, memory, banking, DRAM, interrupt, codec, deterministic-run, or performance test can yet pass meaningfully because no Vortex image or MAME harness is available.

## Blockers and next action

1. Provide a RAR4 extraction executable to this environment or extract the archive locally into the ignored `research_local` directory. Keep firmware bytes out of Git.
2. Parse page 0 and compare its word count/byte packing with MAME and a known ADSP instruction decoder.
3. Build a minimal MAME driver, trace reset/boot and unknown accesses, then implement only hardware required by observed initialization.
4. After stable execution and SPORT/codec behavior, feed a known signal and capture output; only then investigate A/B morph.

The highest-value immediate technical step is extracting the verified archive. The critical original-code execution milestone remains open.
