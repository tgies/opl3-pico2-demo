#!/usr/bin/env python3
"""Pack a VGM (or gzip-compressed VGZ) into a tune blob and optionally flash
it at the fixed offset the TUNE_FROM_FLASH player builds read. Firmware
uploads only erase the sectors the UF2 covers, so the song persists across
code reflashes and only needs loading once (or when changing songs).

Blob layout (little-endian): "OPL3" magic, u32 data_len, 64-byte track name,
64-byte artist (UTF-8, NUL-padded), then the uncompressed VGM data.

Usage:
    vgm_to_flash.py song.vgm                      # writes song.tune.bin
    vgm_to_flash.py song.vgz --load               # builds + flashes (BOOTSEL)
    vgm_to_flash.py song.vgm --name "Foo" --artist "Bar" --offset 0x200000
"""
import argparse
import glob
import gzip
import os
import shutil
import struct
import subprocess
import sys

MAGIC = b"OPL3"
XIP_BASE = 0x10000000
DEFAULT_OFFSET = 0x200000   # keep in sync with TUNE_FLASH_OFFSET in main.cpp


def read_vgm(path):
    data = open(path, "rb").read()
    if data[:2] == b"\x1f\x8b":  # gzip magic -> .vgz
        data = gzip.decompress(data)
    if data[:4] != b"Vgm ":
        sys.exit(f"{path}: not a VGM file (bad magic {data[:4]!r})")
    return data


def parse_gd3(data):
    """Return (track_en, author_en) from the GD3 tag, or (None, None)."""
    gd3_rel = struct.unpack("<I", data[0x14:0x18])[0]
    if not gd3_rel:
        return None, None
    g = 0x14 + gd3_rel
    if data[g:g + 4] != b"Gd3 ":
        return None, None
    payload_len = struct.unpack("<I", data[g + 8:g + 12])[0]
    raw = data[g + 12:g + 12 + payload_len]
    fields = raw.decode("utf-16-le", errors="replace").split("\x00")
    track = fields[0].strip() if len(fields) > 0 and fields[0].strip() else None
    author = fields[6].strip() if len(fields) > 6 and fields[6].strip() else None
    return track, author


def find_picotool():
    p = os.environ.get("PICOTOOL") or shutil.which("picotool")
    if p:
        return p
    pattern = os.path.expanduser("~/.platformio/packages/tool-picotool*/picotool*")
    for cand in sorted(glob.glob(pattern)):
        if os.path.isfile(cand) and os.access(cand, os.X_OK):
            return cand
    return None


def fixed_field(s):
    b = s.encode("utf-8")[:63]
    return b + b"\x00" * (64 - len(b))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("input")
    ap.add_argument("-o", "--out", help="output blob path (default INPUT.tune.bin)")
    ap.add_argument("--offset", type=lambda v: int(v, 0), default=DEFAULT_OFFSET,
                    help="flash offset (default 0x%x; must match TUNE_FLASH_OFFSET)"
                         % DEFAULT_OFFSET)
    ap.add_argument("--name", help="override track name (else from GD3)")
    ap.add_argument("--artist", help="override artist (else from GD3)")
    ap.add_argument("--load", action="store_true",
                    help="flash the blob via picotool (device in BOOTSEL)")
    args = ap.parse_args()

    data = read_vgm(args.input)
    gd3_name, gd3_artist = parse_gd3(data)
    name = args.name or gd3_name or "Unknown"
    artist = args.artist or gd3_artist or "Unknown"

    blob = MAGIC + struct.pack("<I", len(data)) \
         + fixed_field(name) + fixed_field(artist) + data
    out = args.out or os.path.splitext(args.input)[0] + ".tune.bin"
    open(out, "wb").write(blob)
    addr = XIP_BASE + args.offset
    print(f"wrote {out}: {name} / {artist} "
          f"({len(data)} bytes VGM, {len(blob)} bytes blob, flash @0x{addr:08x})")

    if args.load:
        pt = find_picotool()
        if not pt:
            sys.exit("picotool not found (set PICOTOOL or install it)")
        cmd = [pt, "load", "-t", "bin", out, "-o", f"0x{addr:08x}"]
        print("+ " + " ".join(cmd))
        subprocess.run(cmd, check=True)
        print("done; flash code as usual (pio run -e <env> -t upload)")
    else:
        print(f"flash with: picotool load -t bin {out} -o 0x{addr:08x}")


if __name__ == "__main__":
    main()
