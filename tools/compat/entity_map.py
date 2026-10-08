"""Per-zone map of today's static entity index (id & 0xFFF) -> the 2010 PS2 index, aligning the zones' NPC name lists.
PC records are 32 bytes (name 28, u32 id), PS2 records 28 bytes (name 24, u32 id)."""
import sys, struct
from ffxidat import *
from align import align

def ents(raw, rec, nl):
    return [(struct.unpack_from('<I', raw, k * rec + nl)[0], raw[k * rec:k * rec + nl].split(b'\0')[0]) for k in range(len(raw) // rec)]

def rec_size(raw):
    """28-byte (2010 PS2) or 32-byte (PC, later consoles) records: the one whose ids look like static entities"""
    best = None
    for rec, nl in ((32, 28), (28, 24)):
        if not raw or len(raw) % rec: continue
        ids = [struct.unpack_from('<I', raw, k * rec + nl)[0] for k in range(min(len(raw) // rec, 64))]
        good = sum(1 for i in ids if i >> 24 == 1 or i == 0)
        if best is None or good > best[0]: best = (good, rec, nl)
    return best[1:] if best else (32, 28)

def names_by_index(lst):
    by = {}
    for eid, n in lst:
        if eid >> 24 == 1: by[eid & 0xFFF] = n     # static entities: 0x01000000 | zone << 12 | index
    top = max(by) + 1 if by else 0
    return [by.get(i, b'') for i in range(top)]

def ent_id(zone):
    return 6720 + zone if zone < 256 else 86491 + zone - 256

def event_id(zone):
    return 5820 + zone if zone < 256 else 85891 + zone - 256

def events_by_index(raw):
    """zone event file -> {entity index: tuple(sorted event numbers)} (blocks keyed by UniqueNo)."""
    out = {}
    if not raw or len(raw) < 8: return out
    n = struct.unpack_from('<I', raw, 0)[0]
    if 4 + 4 * n > len(raw): return out
    off = 4 + 4 * n
    for size in struct.unpack_from('<%dI' % n, raw, 4):
        blk = raw[off:off + size]; off += size
        if len(blk) < 8: continue
        uid, cnt = struct.unpack_from('<II', blk, 0); cnt &= 0xFFFF
        if uid >> 24 != 1 or 8 + 4 * cnt > len(blk): continue
        out[uid & 0xFFF] = tuple(sorted(x for x in struct.unpack_from('<%dH' % cnt, blk, 8 + 2 * cnt) if x != 0xFFFF))
    return out

def keys(names, events):
    # Named entities align on their name; unnamed ones (cutscene actors, doors...) on the events they
    # play, so a blank only matches the blank that carries the same scripts.
    return [n[:24] if n else (b'', events.get(i, ())) for i, n in enumerate(names)]

if __name__ == '__main__':
    ps2, pc = Install(PS2), Install(PC)
    out, total, mapped, dropped_named, blanks = [], 0, 0, 0, 0
    for z in range(0, 300):
        rb = pc.read(ent_id(z))
        if not rb: continue
        ra = ps2.read(old_id(ent_id(z), z))
        if not ra: continue
        b = names_by_index(ents(rb, 32, 28)); a = names_by_index(ents(ra, *rec_size(ra)))
        if not any(a): continue   # not an entity list (a zone this install does not have)
        # PC names are up to 28 characters, PS2 24: compare on the first 24
        m = align(keys(b, events_by_index(pc.read(event_id(z)))), keys(a, events_by_index(ps2.read(old_id(event_id(z), z)))))
        out.append((z, m))
        named = [i for i, n in enumerate(b) if n]
        total += len(named); mapped += sum(1 for i in named if m[i] is not None)
        blanks += sum(1 for i, n in enumerate(b) if not n and m[i] is not None)
    with open(sys.argv[1] if len(sys.argv) > 1 else 'entity_map.bin', 'wb') as f:
        f.write(b'PS2ENT01' + struct.pack('<I', len(out)))
        for z, m in out:
            f.write(struct.pack('<HI', z, len(m)))
            f.write(struct.pack('<%dH' % len(m), *[0xFFFF if x is None else x for x in m]))
    print('zones %d, named static entities today %d, with a 2010 index %d (%.1f%%); unnamed entities mapped %d' % (len(out), total, mapped, 100.0 * mapped / total, blanks))
