# Hardware map

Evidence labels: **CONFIRMED** means directly stated in the service manual or supplied verified handoff facts; **INFERRED** is a reasoned interpretation; **UNKNOWN** is not yet checked against the schematic/table.

## Processor and storage

- **CONFIRMED** ADSP-2105 at 10 MHz; 1,024 × 24-bit internal program RAM and 512 × 16-bit internal data RAM (service manual, Theory of Operation; exact page/figure transcription pending).
- **CONFIRMED** external 27C512 EPROM, CS4216 stereo codec, four 64K × 4 DRAMs in the standard delay configuration, and 93C66 serial EEPROM.
- **CONFIRMED** service manual states approximately two seconds of delay memory.
- **INFERRED** EPROM address selection is mediated by memory-mapped latch/control outputs; a raw byte-linear disassembly is not valid until boot format and banking are established.

## Data memory map (initial verified handoff transcription)

| Address | Access / role | Status |
|---|---|---|
| `0x1000–0x1003` | External DRAM address-page writes | CONFIRMED from supplied manual-derived address table; latch bit details pending |
| `0x1800` | Display register write | CONFIRMED |
| `0x1801` | Low status read | CONFIRMED |
| `0x1F80` | `CODEC_RST/` active-low codec reset assertion | CONFIRMED by service-manual printed page 4-3; observed executed write at PC `0x0053` |
| `0x1FC0` | `CODEC_RST/` release alias | CONFIRMED by service-manual printed page 4-3 |
| `0x1C01` | High status read | CONFIRMED |
| `0x1C00 / 0x1C40` | Relay control | CONFIRMED |
| `0x1C80 / 0x1CC0` | EPROM A12 control | CONFIRMED |
| `0x1D00 / 0x1D40` | EPROM A14 control | CONFIRMED |
| `0x1F00 / 0x1F40` | EEPROM chip select / EPROM A15 control | CONFIRMED; exact signal split pending |
| `0x3400` | DRAM data access | CONFIRMED |
| `0x3C00–0x3FFF` | Internal memory/register region | CONFIRMED; detailed decode pending |

This is a starting transcription from the supplied research handoff, not a substitute for the complete manual table. Do not implement these as fully understood devices until the schematic and access timing are reviewed. ADC control, EEPROM clock/data, wait states, and program-memory mapping remain **UNKNOWN**. The 128-instruction MAME run reaches the codec reset write and continues; the codec itself is not modeled.

## References to inspect next

Service Manual, Chapter 4 Theory of Operation, printed page 4-3 memory map, and circuit schematics. The currently downloaded copy is a scanned PDF. Record printed page and figure number for each correction.
