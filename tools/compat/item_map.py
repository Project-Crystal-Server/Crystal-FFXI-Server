"""Bitmap of the item ids the 2010 PS2 install really has (a record with a name other than the "." placeholder).

Item DATs: 0xC00-byte records, bytes rotated by 5; u32 item id first. Each record holds a string table
{u32 count; {u32 offset, u32 type}[count]; ...} whose first entry is the name, at table + offset + 0x1C.
Output: res/compat/ps2-20100904/items.bin = "PS2ITM01" + 0x10000 bits (bit = item id). Gil (0xFFFF) is always set."""
import os, struct, sys
from ffxidat import *

ITEM_FILES = [73, 74, 75, 76, 77, 93]   # 0000-0FFF, 1000-17FF, 4000-4DFF, 2800-3FFF, 2000-21FF, 7000-73FF

def rot5(b): return bytes(((x >> 5) | (x << 3)) & 0xFF for x in b)

def name(rec):
    for base in range(4, 0x100, 4):
        n, first = struct.unpack_from('<II', rec, base)
        if 0 < n < 16 and first == 4 + 8 * n:
            p = base + first + 0x1C
            return rec[p:rec.find(b'\0', p, p + 0x40)]
    return None

def item_files(inst):
    """every file of 0xC00-byte records whose first two records are consecutive item ids (the fixed 2010
    list misses the ranges later installs added)"""
    out = []
    for n, (vt, ft, d) in inst.tables.items():
        for fid in range(min(len(vt), 120000)):
            if vt[fid] != n: continue
            p = inst.path(fid)
            try: sz = os.path.getsize(p)
            except OSError: continue
            if sz < 0xC00 * 8 or sz % 0xC00: continue
            with open(p, 'rb') as f: head = rot5(f.read(0xC00 * 2))
            a, b = struct.unpack_from('<I', head, 0)[0], struct.unpack_from('<I', head, 0xC00)[0]
            if b == a + 1 and a < 0x10000 and name(head[:0xC00]) is not None: out.append(fid)
    return sorted(set(out))

def build(inst):
    known, placeholder, unparsed = set(), 0, 0
    files = item_files(inst) if os.environ.get('XI_OLD') else ITEM_FILES
    print('item files', files)
    for fid in files:
        raw = inst.read(fid)
        for i in range(len(raw) // 0xC00):
            rec = rot5(raw[i * 0xC00:(i + 1) * 0xC00])
            iid = struct.unpack_from('<I', rec, 0)[0]
            nm = name(rec)
            if nm is None: unparsed += 1
            elif nm in (b'', b'.'): placeholder += 1
            elif iid < 0x10000: known.add(iid)
    return known, placeholder, unparsed

if __name__ == '__main__':
    known, ph, bad = build(Install(PS2))
    known.add(0xFFFF)
    bits = bytearray(0x2000)
    for i in known: bits[i >> 3] |= 1 << (i & 7)
    out = sys.argv[1] if len(sys.argv) > 1 else 'items.bin'
    open(out, 'wb').write(b'PS2ITM01' + bytes(bits))
    print('known items %d, placeholders %d, unparsed records %d -> %s' % (len(known), ph, bad, out))
