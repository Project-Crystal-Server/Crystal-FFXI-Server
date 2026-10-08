"""Per-zone map of current (PC) English dialog line -> 2010 PS2 line, by order-preserving alignment then exact-text fallback."""
import sys, struct, collections
from ffxidat import *
from align import align

def zone_map(pc_lines, ps2_lines):
    m = align(pc_lines, ps2_lines)
    first = {}
    for j, s in enumerate(ps2_lines): first.setdefault(s, j)
    fb = 0
    for i, s in enumerate(pc_lines):
        if m[i] is None and s in first:
            m[i] = first[s]; fb += 1
    return m, fb

if __name__ == '__main__':
    ps2, pc = Install(PS2), Install(PC)
    rows, out = [], []
    for z in range(0, 300):
        b = dialog(pc.read(en_dialog_id(z)))
        if not b: continue
        a = dialog(ps2.read(old_id(en_dialog_id(z), z)))
        if not a:
            rows.append((z, len(b), 0, 0, 0, 'zone not in PS2 install')); continue
        m, fb = zone_map(b, a)
        n = sum(1 for x in m if x is not None)
        rows.append((z, len(b), len(a), n, fb, ''))
        out.append((z, m))
    with open(sys.argv[1] if len(sys.argv) > 1 else 'dialog_map.bin', 'wb') as f:
        f.write(b'PS2DLG01' + struct.pack('<I', len(out)))
        for z, m in out:
            f.write(struct.pack('<HI', z, len(m)))
            f.write(struct.pack('<%dH' % len(m), *[0xFFFF if x is None else x for x in m]))
    tot = sum(r[1] for r in rows if r[2]); mp = sum(r[3] for r in rows)
    print('zones with dialog on PC: %d, also on PS2: %d, missing on PS2: %d' % (len(rows), len(out), sum(1 for r in rows if not r[2])))
    print('lines in shared zones: %d, mapped %d (%.1f%%), of which by text fallback %d' % (tot, mp, 100.0 * mp / tot, sum(r[4] for r in rows)))
    print('missing zones:', [r[0] for r in rows if not r[2]])
    worst = sorted((r for r in rows if r[2]), key=lambda r: r[3] / r[1])[:8]
    print('lowest coverage:', [(r[0], '%d/%d' % (r[3], r[1])) for r in worst])
