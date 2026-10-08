"""Every event number each zone's 2010 event files carry -> res/compat/ps2-20100904/events.bin.

Event files (event_files.parse_events): 5820 + zone, and extra files at 56911 + zone and 65691 + zone.
A file's zone comes from its entity blocks (0x01000000 | zone << 12 | index), else from its id.
Format: "PS2EVT01", u32 zones, then per zone u16 zone, u32 n, u16 events[n] (sorted)."""
import os, pickle, struct, sys, collections
from ffxidat import *
from event_files import scan

def file_zone(fid, blocks):
    z = {(u >> 12) & 0xFFF for u, _ in blocks if u >> 24 == 1}
    if len(z) == 1: return z.pop()
    for base in (5820, 56911, 65691):
        if base < fid < base + 0x400: return fid - base
    return None

if __name__ == '__main__':
    cache = os.environ.get('TMP', '.') + '/events_%s.pkl' % OLD_TAG
    files = pickle.load(open(cache, 'rb')) if os.path.exists(cache) else scan(Install(PS2))
    zones = collections.defaultdict(set)
    for fid, blocks in files.items():
        z = file_zone(fid, blocks)
        if z is None: print('no zone for file', fid); continue
        for _, ev in blocks:
            zones[z].update(e for e in ev if e != '__work__' and e != 0xFFFF)
    out = sys.argv[1] if len(sys.argv) > 1 else 'events.bin'
    with open(out, 'wb') as f:
        f.write(b'PS2EVT01' + struct.pack('<I', len(zones)))
        for z in sorted(zones):
            ev = sorted(zones[z]); f.write(struct.pack('<HI', z, len(ev)) + struct.pack('<%dH' % len(ev), *ev))
    print('zones %d, events %d -> %s' % (len(zones), sum(len(v) for v in zones.values()), out))
    print('zone 50: %d events; has 4: %s, 3001: %s, 166: %s' % (len(zones[50]), 4 in zones[50], 3001 in zones[50], 166 in zones[50]))
