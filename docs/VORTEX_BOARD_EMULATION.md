# Vortex board emulation

## Evidence boundary

The project has a prior manual-derived memory map in [HARDWARE_MAP.md](HARDWARE_MAP.md). This session did not obtain the scanned service-manual PDF, inspect its schematic pages, or verify the table against the original printed figure. Treat those register addresses as reported handoff evidence pending a page/figure audit.

| Subsystem | Current evidence | Emulation status |
|---|---|---|
| ADSP-2105, 10 MHz | Reported by service-manual notes; ADSP family map checked against MAME/data sheet | Not instantiated |
| 27C512 EPROM | Archive is locally present and hash verified; image not extracted | No ROM mapping/banking |
| Four 64K × 4 DRAMs | Reported manual transcription; latch semantics not checked | Not implemented |
| CS4216 stereo codec | Reported component; board registers and SPORT protocol unknown | Not implemented |
| Encoder/status, A/B, expression ADC, display/LED | Existing map transcription has partial status/display/control addresses | Not implemented |
| 93C66 EEPROM | Reported component; pin/register protocol unverified | Not implemented |

## Proposed device boundary

The MAME driver should expose processor-visible behavior rather than gate-level timing: byte-wide boot pages; explicit EPROM bank output latches; data-memory address windows; external DRAM nibble assembly and read/write sequencing; status/control registers; SPORT receive/transmit timing; codec reset/configuration; and serial EEPROM pins. Log every unmapped access with PC, address, value, direction, and emulated cycle. Introduce wait states only where the manual or traces establish them.

No virtual hardware is implemented in this checkout. In particular, no CS4216 samples or authentic firmware audio exist; the current GUI prototype remains separate and must not be described as Lexicon execution.
