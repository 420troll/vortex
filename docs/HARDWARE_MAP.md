# Hardware map

Evidence labels: **CONFIRMED** means directly stated by the service manual or observed in the MAME runtime trace; **INFERRED** means a supported but unverified interpretation; **UNKNOWN** means unresolved.

## Processor and storage

- **CONFIRMED** ADSP-2105 at 10 MHz, 1,024 × 24-bit internal PM, 512 × 16-bit internal DM.
- **CONFIRMED** 27C512 EPROM, CS4216 codec, four 64K × 4 DRAMs in the standard delay configuration, 93C66 EEPROM.
- **CONFIRMED** U2 is a 74HC259 addressable latch; its outputs power up low (service manual, Chapter 4, printed page 4-4).
- **CONFIRMED** The board uses three latched EPROM address signals (EPROM_A12, EPROM_A14, EEPCS_A15) to expand the 8K program-memory aperture to the 64K EPROM.

## Data memory map

The following aliases are from the service manual, Chapter 4, printed page 4-3. Runtime-observed entries are noted.

| Address | Access / role | Status |
|---|---|---|
| `0x1000–0x1003` | DRAM address write pages 0–3 | CONFIRMED; RAS/CAS sequence on page 4-4 |
| `0x1800` | Display register write | CONFIRMED; runtime write at PC `0x0055` |
| `0x1801` | Status-low read | CONFIRMED; bit layout unknown |
| `0x1C01` | Status-high read | CONFIRMED; bit layout unknown |
| `0x1C00 / 0x1C40` | A/B relay off/on | CONFIRMED |
| `0x1C80 / 0x1CC0` | EPROM A12 low/high | CONFIRMED |
| `0x1D00 / 0x1D40` | EPROM A14 low/high | CONFIRMED |
| `0x1D80 / 0x1DC0` | EEPROM DI low/high | CONFIRMED |
| `0x1E00 / 0x1E40` | EEPROM clock low/high | CONFIRMED |
| `0x1E80 / 0x1EC0` | ADC chip-select asserted/released | CONFIRMED; runtime release at PC `0x0069` |
| `0x1F00 / 0x1F40` | EEPROM CS / EPROM A15 low/high | CONFIRMED; shared control line |
| `0x1F80 / 0x1FC0` | `CODEC_RST/` low/asserted and high/released | CONFIRMED; runtime assertion at PC `0x0053` |
| `0x3400` | DRAM read/write data | CONFIRMED |
| `0x3C00–0x3FFF` | Internal memory and ADSP registers | CONFIRMED; decode implemented in the ADSP core/driver |

The address alias selects the output value for the addressable latch; the data word written does not select signal polarity. The first observed codec write has data `0x1000`, but `0x1F80` selects the low output and asserts active-low reset. The harness logs/latches signal state and marks unimplemented serial or display behavior.

## Program memory map and runtime boundary

The manual maps PM `0x0000–0x07FF` to internal memory, and `0x0800–0x0FFF` plus `0x2000–0x2FFF` to EPROM with two wait states. The runtime firmware reaches `PC=0x019F`, executes `AX0 = PM(I4,M5)` with `I4=0x0800`, and reads the external program space at `PM(0x0800)`. The handler records CPU state and halts because the byte-to-24-bit word packing and bank-address translation are unresolved.

The ROM is 65,536 bytes and locally hash-verified. Its bytes at offset `0x0800` are nonblank, but that alone does not prove the external PM word mapping. No bank translation is guessed. Manual memory/timing reference: [Lexicon Vortex Service Manual](https://civade.com/AudioPro/Lexicon/lexicon_vortex_sm.pdf), Chapter 4, printed pages 4-3–4-5.
