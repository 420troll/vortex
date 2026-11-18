# Open questions

1. What are the exact Vortex EPROM bytes, size, and SHA-256 after extraction?
2. What is the ADSP-2105 boot word format used by this board, and what portion is copied into internal program RAM?
3. What are the exact program and data maps, latch bit assignments, wait states, and DRAM access timing from manual/schematic?
4. How are EPROM A12/A14/A15 bank outputs combined and when are they changed?
5. Which status bits represent codec/ADC readiness, buttons, and device state?
6. Which MAME integration route is maintainable under its GPL license and framework dependencies?
7. Which of the 16 documented positions share DSP structures, and what precisely changes through each A/B morph?
8. Which timing/modulation controls are stored per endpoint versus shared globally?
9. Which lightweight cross-platform audio backend fits license and Windows packaging needs?
10. What Windows host/device and WAV formats are required for initial playback and export?

No unanswered firmware detail should be promoted from inferred to confirmed without a source citation, ROM offset, instruction address, or reproducible test.
