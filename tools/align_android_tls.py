#!/usr/bin/env python3
"""Raise an AArch64 Android executable PT_TLS alignment to Bionic's 64-byte minimum."""

import struct
import sys


def fail(message):
    print(f"align_android_tls: {message}", file=sys.stderr)
    raise SystemExit(1)


def main(path):
    with open(path, "r+b") as elf:
        data = elf.read()
        if len(data) < 64 or data[:4] != b"\x7fELF":
            fail("not an ELF file")
        if data[4:6] != b"\x02\x01":
            fail("expected a little-endian ELF64 file")

        machine = struct.unpack_from("<H", data, 18)[0]
        if machine != 183:
            fail("expected an AArch64 executable")

        phoff = struct.unpack_from("<Q", data, 32)[0]
        phentsize, phnum = struct.unpack_from("<HH", data, 54)
        if phentsize < 56 or phoff + phentsize * phnum > len(data):
            fail("invalid program-header table")

        tls_headers = []
        for index in range(phnum):
            offset = phoff + index * phentsize
            p_type = struct.unpack_from("<I", data, offset)[0]
            if p_type == 7:
                p_offset = struct.unpack_from("<Q", data, offset + 8)[0]
                p_vaddr = struct.unpack_from("<Q", data, offset + 16)[0]
                p_align = struct.unpack_from("<Q", data, offset + 48)[0]
                tls_headers.append((offset, p_offset, p_vaddr, p_align))

        if len(tls_headers) != 1:
            fail(f"expected one PT_TLS header, found {len(tls_headers)}")

        header, p_offset, p_vaddr, p_align = tls_headers[0]
        new_align = max(64, p_align)
        if p_offset % new_align != p_vaddr % new_align:
            fail("PT_TLS file offset and virtual address are not congruent at 64-byte alignment")
        if p_align < new_align:
            elf.seek(header + 48)
            elf.write(struct.pack("<Q", new_align))
        print(f"PT_TLS vaddr=0x{p_vaddr:x} offset=0x{p_offset:x} align={new_align}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        fail("usage: align_android_tls.py <ELF>")
    main(sys.argv[1])
