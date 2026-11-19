import unittest

from tools.rom_inspector.decode_adsp_boot import decode_page


class AdspBootPageTests(unittest.TestCase):
    def test_packs_three_bytes_and_ignores_fourth_lane(self):
        page = bytearray(0x1000)
        page[0:4] = bytes((0x12, 0x34, 0x56, 0))
        page[4:8] = bytes((0xAB, 0xCD, 0xEF, 0x99))
        words = decode_page(bytes(page))
        self.assertEqual(len(words), 8)
        self.assertEqual(words[:2], [0x123456, 0xABCDEF])

    def test_uses_page_length_field_in_eight_word_units(self):
        page = bytearray(0x1000)
        page[3] = 2
        self.assertEqual(len(decode_page(bytes(page))), 24)

    def test_rejects_wrong_page_size_and_impossible_length(self):
        with self.assertRaises(ValueError):
            decode_page(bytes(0x800))
        page = bytearray(0x1000)
        page[3] = 128
        with self.assertRaises(ValueError):
            decode_page(bytes(page))


if __name__ == "__main__":
    unittest.main()
