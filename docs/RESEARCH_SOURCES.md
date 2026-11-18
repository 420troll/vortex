# Research sources

## Primary documents and code

- Lexicon Vortex Service Manual, mirror: https://www.polynominal.com/sample-packs/lexicon-vortex/lexicon-vortex-service-manual.pdf
- Lexicon service-manual listing: https://lexiconpro.com/en-US/product_documents/Vortex/service_manual
- Lexicon Vortex Owner's Manual, official product document: https://lexiconpro.com/en-US/product_documents/Vortex/owners_manual
- MAME ADSP-2100 family CPU: https://github.com/mamedev/mame/tree/master/src/devices/cpu/adsp2100
- MAME ADSP disassembler: https://github.com/mamedev/mame/blob/master/src/devices/cpu/adsp2100/2100dasm.cpp
- MAME DCS driver context: https://github.com/mamedev/mame/blob/master/src/mame/shared/dcs.cpp
- Firmware archive listing: https://dbwbp.com/index.php/9-misc/37-synth-eprom-dumps
- Archive directory entry: https://dbwbp.com/synthbin/Lexicon%20Vortex%201.1%20(27C512).rar
- Third-party behavioral comparison only: https://www.temeculadsp.com/vext

## Provenance and handling

The archive has been fetched for local analysis only. Its content is proprietary firmware; do not commit or publish it. Archive SHA-256 is recorded in `ROM_FORENSICS.md`. MAME is GPL-2.0-or-later; any reuse must retain applicable notices and remain isolated from independently written DSP code. Verify current source-tree licensing before importing files.

## Documentation reviewed

The official Lexicon product page confirms Vortex is discontinued and links a quickstart guide. The supplied operation manual and Andy Butler page remain to be fully transcribed. Service manual is a scanned 56-page PDF, so exact tables require visual inspection rather than plain text extraction.
