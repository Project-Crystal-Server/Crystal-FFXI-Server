"""Find every event-format file in an install and the zone its blocks belong to."""
import struct, sys, os
from ffxidat import *

def parse_events(raw):
    """[(uniqueNo, {event: code bytes})] or None if raw is not an event file (beta layout, gen_events.py)."""
    if not raw or len(raw) < 8: return None
    n = struct.unpack_from('<I', raw, 0)[0]
    if n == 0 or n > 4096 or 4 + 4 * n > len(raw): return None
    sizes = struct.unpack_from('<%dI' % n, raw, 4)
    if sum(sizes) + 4 + 4 * n != len(raw): return None
    off, out = 4 + 4 * n, []
    for size in sizes:
        blk = raw[off:off + size]; off += size
        if len(blk) < 12: return None
        uid, cnt = struct.unpack_from('<II', blk, 0); cnt &= 0xFFFF
        p = 8
        if p + 4 * cnt > len(blk): return None
        offs = struct.unpack_from('<%dH' % cnt, blk, p); nos = struct.unpack_from('<%dH' % cnt, blk, p + 2 * cnt); p += 4 * cnt
        n2 = struct.unpack_from('<I', blk, p)[0] & 0xFFFF
        if p + 4 + 4 * n2 > len(blk): return None
        work = struct.unpack_from('<%dI' % n2, blk, p + 4); p += 4 + 4 * n2
        if p + 4 > len(blk): return None
        csize = struct.unpack_from('<I', blk, p)[0]; code = blk[p + 4:p + 4 + csize]
        ev = {}
        order = sorted(set(offs) | {csize})
        for o, e in zip(offs, nos):
            if e == 0xFFFF or o > csize: continue
            end = next(x for x in order if x > o) if any(x > o for x in order) else csize
            ev[e] = code[o:end]
        ev['__work__'] = work
        out.append((uid, ev))
    return out

def scan(inst, max_id=120000):
    found = {}
    for fid in range(max_id):
        p = inst.path(fid)
        if not p or not os.path.exists(p): continue
        sz = os.path.getsize(p)
        if sz < 16 or sz > 8 << 20: continue
        with open(p, 'rb') as f: head = f.read(8)
        n = struct.unpack_from('<I', head, 0)[0]
        if n == 0 or n > 4096: continue
        blocks = parse_events(open(p, 'rb').read())
        if blocks: found[fid] = blocks
    return found

if __name__ == '__main__':
    import pickle
    for name, root in (('ps2', PS2), ('pc', PC)):
        f = scan(Install(root))
        pickle.dump({k: [(u, {e: bytes(c) for e, c in ev.items()}) for u, ev in v] for k, v in f.items()}, open(os.environ['TMP'] + '/events_%s.pkl' % name, 'wb'))
        z50 = [k for k, v in f.items() if any((u >> 12) & 0xFFF == 50 and u >> 24 == 1 for u, _ in v)]
        print(name, 'event files', len(f), '; with zone-50 entities:', z50)
