# Vortex board emulation

## Evidence and current boundary

The board map and latch behavior below are transcribed from the Vortex Service Manual, Chapter 4, printed pages 4-3 and 4-4. The manual states the ADSP-2105 runs at 10 MHz, has 1K × 24-bit PM and 512 × 16-bit DM on chip, and requires external-memory wait states. The emulator uses MAME's ADSP-2105 core; it does not yet model board-level wait states or a functioning DSP/codec path.

| Subsystem | Confirmed evidence | Emulation status |
|---|---|---|
| ADSP-2105 | 10 MHz crystal; internal PM/DM sizes | MAME core instantiated. Boot page transfer is performed by MAME `load_boot_data`; reset trace starts at PC 0. External access wait states are not yet applied. |
| 27C512 EPROM | Manual maps external PM at `0800–0FFF` and `2000–2FFF`; `EPROM_A12`, `EPROM_A14`, and `EEPCS_A15` expand the 8K address window to 64K. | Image is hash-validated and supplies the boot page. External PM reads remain unmapped; exact byte/word packing and bank-window translation are not verified, so reads stop the CPU instead of fabricating instructions. |
| U2 addressable latch | U2 is a 74HC259. Manual says all outputs are low after reset. | Address aliases are modeled as named signal latches and logged. EPROM latches are recorded but not connected to PM mapping yet. |
| CS4216 reset | `1F80` asserts active-low `CODEC_RST/`; `1FC0` releases it. | Reset latch implemented. The first firmware write at PC `0053` uses address `1F80`, so reset is asserted; the data value `1000` does not determine polarity. CS4216 is not emulated. |
| Display/status | `1800` display latch; `1801` and `1C01` read status muxes. U23/U24 drive the time-multiplexed display. | Display writes are latched/logged without scan timing. Status reads return pulled-high inactive state `FFFF`; physical controls are not modeled. |
| EEPROM / ADC | 93C66 EEPROM and TLC549 ADC share serial control signals; `1D80/1DC0` DI, `1E00/1E40` clock, `1E80/1EC0` ADC CS, `1F00/1F40` EEPROM CS / EPROM A15. | Signal latches and accesses are logged. Serial protocols and ADC values are not modeled. |
| Delay DRAM | Four 64K × 4 DRAMs; U12/U13 hold address pages. Manual describes RAS/CAS sequencing and internal wait state. | DRAM access halts as unmodeled. Address-latch writes are not yet observed on the captured reset prefix and are not implemented. |
| SPORT / audio | CS4216 uses 8 MHz resonator; DSP/codec serial timing is described in manual Chapter 4. | No SPORT events have been observed in the current trace prefix; no serial timing, interrupts, or audio are generated. |

## Observed execution boundary

The deterministic original-ROM trace executes 48 instructions and reports 47 ADSP cycles before the first unsupported access: instruction PC `019F` reads `PM(0x0800)` through `AX0 = PM(I4,M5)`. The service manual classifies `0800–0FFF` as EPROM program memory. The handler logs the access and asserts the ADSP HALT input; the returned placeholder value is not allowed to drive a continuing fake loop. This is the current first blocker, not a stable processing loop.

The ROM has non-blank packed bytes at offset `0x0800`, but translating the CPU PM address and the three EPROM bank signals into the core's 24-bit PM word is not inferred from that coincidence. Resolve the physical address/data packing from the ADSP interface plus the board schematic, then add the external ROM window and verify the returned instruction/data words against the image before resuming execution.

## Next hardware work

1. Reconstruct EPROM PM bus translation, including power-on latch levels and all A12/A14/A15 bank cases. Validate at least one observed fetch/read against raw ROM offsets.
2. Model external PM's two wait states and ensure MAME cycle counts account for them.
3. Add U12/U13 DRAM address latch and DRAM data sequencing only when the firmware reaches those accesses; use the manual's RAS/CAS sequence rather than a flat zero-filled array.
4. Implement a real input/status matrix, EEPROM protocol, CS4216 SPORT framing, and interrupt cadence only after disassembly or runtime evidence establishes their initialization requirements.

Manual source: [Lexicon Vortex Service Manual](https://civade.com/AudioPro/Lexicon/lexicon_vortex_sm.pdf), Chapter 4, printed pages 4-3–4-5 and main-board schematics.
