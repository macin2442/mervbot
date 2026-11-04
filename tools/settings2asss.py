#!/usr/bin/env python3
"""Turn a zone's arena settings, as MERVBot saved them, into an ASSS arena.conf fragment.

MERVBot writes the settings packet every zone sends on entering an arena (S2C 0x0F, 1428 bytes) to
get/settings-<arena>.bin. ASSS builds that packet from its config (core/clientset.c), taking the key names in packet
order from core/clientset.def; this script reverses that, with the names read from the clientset.def of the ASSS
version the result is for.

    settings2asss.py get/settings-0.bin --clientset-def ~/asss/src/core/clientset.def > settings.conf

The packet holds only what the client needs: ship and weapon settings, prize weights, spawn positions, soccer, radar
and the like. Server-side rules (scoring and rewards, death prize weights, team limits, flag games, modules, LVZ
files, the map name) are not in it.
"""

import argparse
import re
import struct
import sys
from datetime import date

SETTINGS_SIZE = 1428
SETTINGS_TYPE = 0x0F
# cfg->SHIP_NAMES in ASSS
SHIP_NAMES = ["Warbird", "Javelin", "Spider", "Leviathan", "Terrier", "Weasel", "Lancaster", "Shark"]
# ASSS multiplies these long_set entries (by index) by 1000 (clientset.c, "the funky ones")
TIMES_1000 = {0, 1, 10, 11, 16}

# Bit fields, least significant bit first (GCC on little-endian), from ASSS's packets/clientset.h and clientset.c:
# (config key or None for padding, width)
BIT_SET = [  # first 4 bytes; the config key for each field from load_settings()
    (None, 8),  # type, 0x0F
    ("Bullet:ExactDamage", 1),
    ("Spectator:HideFlags", 1),
    ("Spectator:NoXRadar", 1),
    ("Misc:SlowFrameCheck", 3),
    ("Misc:DisableScreenshot", 1),
    (None, 1),
    ("Misc:MaxTimerDrift", 3),
    ("Soccer:DisableWallPass", 1),
    ("Soccer:DisableBallKilling", 1),
    (None, 11),
]
WEAPON_BITS = [
    ("ShrapnelMax", 5), ("ShrapnelRate", 5), ("CloakStatus", 2), ("StealthStatus", 2), ("XRadarStatus", 2),
    ("AntiWarpStatus", 2), ("InitialGuns", 2), ("MaxGuns", 2), ("InitialBombs", 2), ("MaxBombs", 2),
    ("DoubleBarrel", 1), ("EmpBomb", 1), ("SeeMines", 1), ("Unused1", 3),
]
MISC_BITFIELD = [("SeeBombLevel", 2), ("DisableFastShooting", 1), ("Radius", 8), (None, 5)]
MISC_BITFIELD_NAME = "___MiscBitfield___"  # its place among the ship's shorts (clientset.def)
SPAWN_BITS = [("X", 10), ("Y", 10), ("Radius", 9), (None, 3)]


def bits(value, layout):
    """(name, value) for each named field of a bit field, least significant bits first."""
    out, shift = [], 0
    for name, width in layout:
        if name:
            out.append((name, (value >> shift) & ((1 << width) - 1)))
        shift += width
    return out


def read_names(path):
    """The string arrays of clientset.def: {array name: [entries]}."""
    src = re.sub(r"/\*.*?\*/", "", open(path, encoding="latin-1").read(), flags=re.S)
    arrays = {}
    for m in re.finditer(r"static\s+char\s*\*\s*(\w+)\s*\[\]\s*=\s*\{(.*?)\};", src, re.S):
        arrays[m.group(1)] = re.findall(r'"([^"]*)"', m.group(2))
    return arrays


def convert(data, names):
    """{section: [(key, value, comment)]} in packet order."""
    expect = {"long_names": 20, "short_names": 58, "byte_names": 32, "prizeweight_names": 28,
              "ship_long_names": 2, "ship_short_names": 49, "ship_byte_names": 18}
    for array, count in expect.items():
        if len(names.get(array, [])) != count:
            sys.exit(f"clientset.def: {array} has {len(names.get(array, []))} entries, expected {count} – "
                     "a different packet layout than this script knows")

    conf = {}

    def put(section, key, value, comment=None):
        conf.setdefault(section, []).append((key, value, comment))

    def put_full(fullkey, value, comment=None):
        section, key = fullkey.split(":", 1)
        put(section, key, value, comment)

    (bit_set,) = struct.unpack_from("<I", data, 0)
    for key, value in bits(bit_set, BIT_SET):
        put_full(key, value)

    offset = 4
    for ship in SHIP_NAMES:
        longs = struct.unpack_from("<2i", data, offset)
        shorts = struct.unpack_from("<49h", data, offset + 8)
        bytes_ = struct.unpack_from("<18B", data, offset + 106)
        (weapons,) = struct.unpack_from("<I", data, offset + 124)
        for key, value in zip(names["ship_long_names"], longs):
            put(ship, key, value)
        for key, value in zip(names["ship_short_names"], shorts):
            if key == MISC_BITFIELD_NAME:
                for bkey, bvalue in bits(value & 0xFFFF, MISC_BITFIELD):
                    put(ship, bkey, bvalue)
            else:
                put(ship, key, value)
        for key, value in zip(names["ship_byte_names"], bytes_):
            put(ship, key, value)
        for key, value in bits(weapons, WEAPON_BITS):
            put(ship, key, value)
        offset += 144

    longs = struct.unpack_from("<20i", data, offset)
    for i, (key, value) in enumerate(zip(names["long_names"], longs)):
        if i in TIMES_1000:
            comment = None if value % 1000 == 0 else f"sent as {value}: not a multiple of 1000, ASSS can't match it"
            put_full(key, value // 1000, comment)
        else:
            put_full(key, value)
    offset += 80

    for team in range(4):
        (spawn,) = struct.unpack_from("<I", data, offset + team * 4)
        for suffix, value in bits(spawn, SPAWN_BITS):
            put("Spawn", f"Team{team}-{suffix}", value)
    offset += 16

    for key, value in zip(names["short_names"], struct.unpack_from("<58h", data, offset)):
        put_full(key, value)
    offset += 116
    for key, value in zip(names["byte_names"], struct.unpack_from("<32B", data, offset)):
        put_full(key, value)
    offset += 32
    for key, value in zip(names["prizeweight_names"], struct.unpack_from("<28B", data, offset)):
        put_full(key, value)
    offset += 28
    assert offset == SETTINGS_SIZE

    return conf


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("settings", help="settings packet saved by MERVBot (get/settings-<arena>.bin)")
    parser.add_argument("--clientset-def", required=True,
                        help="ASSS's src/core/clientset.def, of the ASSS version the result is for")
    parser.add_argument("--map", help="also write [General] Map = MAP (the map name isn't in the settings)")
    parser.add_argument("-o", "--output", help="write here instead of standard output")
    args = parser.parse_args()

    data = open(args.settings, "rb").read()
    if len(data) != SETTINGS_SIZE or data[0] != SETTINGS_TYPE:
        sys.exit(f"{args.settings}: not a settings packet ({len(data)} bytes, type {data[:1].hex() or '-'}; "
                 f"expected {SETTINGS_SIZE} bytes, type 0f)")

    conf = convert(data, read_names(args.clientset_def))
    if args.map:
        conf = {"General": [("Map", args.map, None)], **conf}

    lines = [
        f"; Arena settings converted from {args.settings} on {date.today().isoformat()} by settings2asss.py",
        "; (MERVBot tools/). Only what the zone sends its clients; server-side rules (scoring, rewards, death prize",
        "; weights, team limits, flag games, modules, LVZ files) are not included.",
    ]
    for section, entries in conf.items():
        lines += ["", f"[{section}]"]
        for key, value, comment in entries:
            lines.append(f"{key} = {value}" + (f"  ; {comment}" if comment else ""))
    text = "\n".join(lines) + "\n"

    if args.output:
        with open(args.output, "w") as out:
            out.write(text)
    else:
        sys.stdout.write(text)


if __name__ == "__main__":
    main()
