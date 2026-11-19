#!/usr/bin/env python3
"""Decode one byte-wide ADSP-2105 boot page into 24-bit program words.

This implements the byte packing documented by the ADSP-2105 data sheet and
MAME's ADSP-21xx load_boot_data routine. It does not disassemble instructions.
"""
import argparse
import pathlib


PAGE_BYTES = 0x1000
MAX_PROGRAM_WORDS = 0x400


def decode_page(data: bytes) -> list[int]:
    if len(data) != PAGE_BYTES:
        raise ValueError(f"expected one {PAGE_BYTES}-byte boot page, got {len(data)}")
    word_count = (data[3] + 1) * 8
    if word_count > MAX_PROGRAM_WORDS:
        raise ValueError(f"page length requests {word_count} words; ADSP-2105 has 1024 PM words")
    return [
        (data[i * 4] << 16) | (data[i * 4 + 1] << 8) | data[i * 4 + 2]
        for i in range(word_count)
    ]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("page", type=pathlib.Path, help="exactly 4096 bytes from an ADSP-2105 boot page")
    parser.add_argument("--base-pc", type=lambda s: int(s, 0), default=0)
    args = parser.parse_args()
    words = decode_page(args.page.read_bytes())
    for index, word in enumerate(words):
        print(f"PC {args.base_pc + index:04X}  ROM+{index * 4:04X}  {word:06X}")
    print(f"decoded_words: {len(words)}")


if __name__ == "__main__":
    main()
