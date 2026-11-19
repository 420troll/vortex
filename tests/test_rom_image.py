import hashlib
import tempfile
import unittest
from pathlib import Path

from tools.rom_inspector.rom_image import decode_page, validate_rom


class RomImageTests(unittest.TestCase):
    def test_user_path_size_and_sha256_validation(self):
        image = bytes(range(256)) * 2
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "fixture.bin"
            path.write_bytes(image)
            self.assertEqual(validate_rom(path, expected_size=len(image), expected_sha256=hashlib.sha256(image).hexdigest()), image)

    def test_rom_size_mismatch_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "short.bin"
            path.write_bytes(b"fixture")
            with self.assertRaisesRegex(ValueError, "expected 65536 bytes"):
                validate_rom(path)

    def test_hash_mismatch_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "fixture.bin"
            path.write_bytes(b"fixture")
            with self.assertRaisesRegex(ValueError, "SHA-256 mismatch"):
                validate_rom(path, expected_size=7, expected_sha256="0" * 64)

    def test_boot_page_length_and_24_bit_byte_packing(self):
        page = bytearray(0x1000)
        page[0:8] = bytes((0x12, 0x34, 0x56, 0x00, 0xAB, 0xCD, 0xEF, 0xFF))
        words = decode_page(bytes(page))
        self.assertEqual(len(words), 8)
        self.assertEqual(words[:2], [0x123456, 0xABCDEF])

    def test_boot_page_size_and_bounds(self):
        with self.assertRaisesRegex(ValueError, "expected one 4096-byte"):
            decode_page(bytes(0x0fff))
        page = bytearray(0x1000)
        page[3] = 0xff
        with self.assertRaisesRegex(ValueError, "requests 2048 words"):
            decode_page(bytes(page))


if __name__ == "__main__":
    unittest.main()
