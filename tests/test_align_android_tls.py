#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import os
import struct
import subprocess
import sys
import tempfile
import unittest


SCRIPT = os.path.join(os.path.dirname(__file__), "..", "tools", "align_android_tls.py")


def make_elf(path, p_offset=0x100, p_vaddr=0x200, p_align=8):
    data = bytearray(120)
    data[:6] = b"\x7fELF\x02\x01"
    struct.pack_into("<H", data, 18, 183)
    struct.pack_into("<Q", data, 32, 64)
    struct.pack_into("<HH", data, 54, 56, 1)
    struct.pack_into("<IIQQQQQQ", data, 64, 7, 4, p_offset, p_vaddr, p_vaddr, 0, 64, p_align)
    with open(path, "wb") as elf:
        elf.write(data)


class AlignAndroidTlsTest(unittest.TestCase):
    def test_raises_tls_alignment_when_layout_is_congruent(self):
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "fixture")
            make_elf(path)
            result = subprocess.run([sys.executable, SCRIPT, path], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            with open(path, "rb") as elf:
                data = elf.read()
            self.assertEqual(struct.unpack_from("<Q", data, 64 + 48)[0], 64)

    def test_rejects_layout_with_noncongruent_tls_addresses(self):
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "fixture")
            make_elf(path, p_vaddr=0x208)
            result = subprocess.run([sys.executable, SCRIPT, path], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            with open(path, "rb") as elf:
                data = elf.read()
            self.assertEqual(struct.unpack_from("<Q", data, 64 + 48)[0], 8)


if __name__ == "__main__":
    unittest.main()
