"""Pair 2010 NPCs with today's numbered duplicates ("Home Point" <-> "Home Point #1") in an existing entity_map.bin.

entity_map.py aligns the two NPC lists on the exact name, so a 2010 NPC that today is split into "<name> #1" .. "#N"
gets no counterpart. The 2010 opening cutscenes still list that NPC as an actor they need (the Home Point crystals in the
three San d'Oria cities), the client waits for it, and the server cannot answer (actIndexFromClient finds no index).

For every zone this takes the 2010 NPCs that nothing maps to and the unmapped NPCs of today whose name, without the
trailing " #N", is the same, and pairs them in index order. The rest of the map is not touched.

    python entity_map_suffix.py <entity_map.bin in> [<entity_map.bin out>]     (no out = preview only)
    XI_OLD_HIGH_SHIFT=1320 for the final PS2 install, as for entity_map.py
"""
import re
import struct
import sys

from ffxidat import *
from entity_map import ent_id, ents, names_by_index, rec_size

SUFFIX = re.compile(rb'\s*#\d+$')


def read_map(path):
    raw = open(path, 'rb').read()
    assert raw[:8] == b'PS2ENT01', 'not an entity map'
    count = struct.unpack_from('<I', raw, 8)[0]
    off, zones = 12, []
    for _ in range(count):
        zone, n = struct.unpack_from('<HI', raw, off)
        zones.append((zone, list(struct.unpack_from('<%dH' % n, raw, off + 6))))
        off += 6 + 2 * n
    return zones


def write_map(path, zones):
    with open(path, 'wb') as f:
        f.write(b'PS2ENT01' + struct.pack('<I', len(zones)))
        for zone, m in zones:
            f.write(struct.pack('<HI', zone, len(m)))
            f.write(struct.pack('<%dH' % len(m), *m))


def main():
    src = sys.argv[1]
    dst = sys.argv[2] if len(sys.argv) > 2 else None
    pc, ps2 = Install(PC), Install(PS2)
    zones, changes = read_map(src), 0
    for zi, (zone, m) in enumerate(zones):
        rb = pc.read(ent_id(zone))
        ra = ps2.read(old_id(ent_id(zone), zone))
        if not rb or not ra:
            continue
        today = names_by_index(ents(rb, 32, 28))
        old = names_by_index(ents(ra, *rec_size(ra)))
        mapped = {j for j in m if j != 0xFFFF}
        free_old = {}
        for j, n in enumerate(old):
            if n and j not in mapped:
                free_old.setdefault(n[:24], []).append(j)
        for i, n in enumerate(today):
            if not n or i >= len(m) or m[i] != 0xFFFF:
                continue
            base = SUFFIX.sub(b'', n)[:24]
            if base == n[:24] or base not in free_old or not free_old[base]:
                continue
            j = free_old[base].pop(0)
            m[i] = j
            changes += 1
            print('zone %3d: %-26s (today %#05x) -> 2010 %#05x' % (zone, n.decode('latin1'), i, j))
        zones[zi] = (zone, m)
    print('%d mapping(s) added' % changes)
    if dst:
        write_map(dst, zones)
        print('wrote', dst)


if __name__ == '__main__':
    main()
