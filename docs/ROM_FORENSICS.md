# ROM forensics

## Archive availability

- **CONFIRMED** archive listing includes `Lexicon Vortex 1.1 (27C512).rar` (DBWBP archive directory).
- **CONFIRMED** retrieved archive size: 12,965 bytes.
- **CONFIRMED** archive SHA-256: `8afe37707ce163f55285a755ba275ec5b3029cb630bb48028c67c2d4b9894462`.
- Local path: `research_local/vortex-1.1-27c512.rar` (ignored by Git).
- **UNKNOWN** extracted file name, exact 65,536-byte size, and firmware SHA-256. RAR4 extraction tool was unavailable during this pass.

## Structure findings

- **UNKNOWN** whether extracted image has valid ADSP-2105 boot data.
- **UNKNOWN** executable start, bank mapping, code/data/table regions, parameter tables, dispatch structure, internal program-RAM copy range, and algorithm/morph routines.
- No firmware offsets or instruction addresses are asserted until the image is extracted and boot encoding is verified.

Do not commit extracted ROM bytes or reproduce proprietary contents in documentation.
