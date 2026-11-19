"""Validate a user-supplied Lexicon Vortex ROM and decode ADSP boot pages."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path

ROM_SIZE = 0x10000
ROM_SHA256 = "1e79c704fbef8219b7fc53349064bc1afafd4287c3ca7442bb5587641a450892"
PAGE_BYTES = 0x1000
MAX_PROGRAM_WORDS = 0x400


def validate_rom(path: Path, *, expected_size: int = ROM_SIZE, expected_sha256: str = ROM_SHA256) -> bytes:
    data = path.read_bytes()
    if len(data) != expected_size:
        raise ValueError(f"expected {expected_size} bytes, got {len(data)}")
    digest = hashlib.sha256(data).hexdigest()
    if digest != expected_sha256:
        raise ValueError(f"SHA-256 mismatch: expected {expected_sha256}, got {digest}")
    return data


def decode_page(data: bytes) -> list[int]:
    if len(data) != PAGE_BYTES:
        raise ValueError(f"expected one {PAGE_BYTES}-byte boot page, got {len(data)}")
    count = (data[3] + 1) * 8
    if count > MAX_PROGRAM_WORDS:
        raise ValueError(f"page length requests {count} words; ADSP-2105 has 1024 PM words")
    return [
        (data[i * 4] << 16) | (data[i * 4 + 1] << 8) | data[i * 4 + 2]
        for i in range(count)
    ]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path, help="user-provided 64 KiB firmware image")
    parser.add_argument("--page-offset", type=lambda s: int(s, 0), default=0)
    args = parser.parse_args()
    image = validate_rom(args.rom)
    if args.page_offset < 0 or args.page_offset + PAGE_BYTES > len(image):
        parser.error("page offset must select a complete 4096-byte page within the ROM")
    page = image[args.page_offset : args.page_offset + PAGE_BYTES]
    words = decode_page(page)
    for index, word in enumerate(words):
        print(f"ROM+{args.page_offset + index * 4:04X} PC {index:04X} {word:06X}")
    print(f"decoded_words: {len(words)}")


if __name__ == "__main__":
    main()
