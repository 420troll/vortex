#!/usr/bin/env python3
"""Inspect user-supplied ROM bytes without modifying or distributing them."""
import argparse
import hashlib
import pathlib
import re

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("image", type=pathlib.Path)
    args = ap.parse_args()
    data = args.image.read_bytes()
    print(f"path: {args.image}")
    print(f"size: {len(data)} bytes (0x{len(data):X})")
    print(f"sha256: {hashlib.sha256(data).hexdigest()}")
    print(f"all-FF: {data.count(bytes([255])) == len(data)}")
    print("printable strings (offset, text):")
    for match in re.finditer(rb"[\x20-\x7e]{5,}", data):
        print(f"  0x{match.start():04X}: {match.group().decode('ascii')}")

if __name__ == "__main__":
    main()
