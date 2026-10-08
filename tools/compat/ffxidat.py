"""File-id lookup through VTABLE/FTABLE for a FINAL FANTASY XI install (PC or the PS2 dump), and the zone dialog format."""
import os, struct

# The older install the maps are built for; XI_OLD=<path> picks another (the 2016 PS2 / Xbox dumps) and
# XI_OLD_TAG names its caches (default: ps2)
PS2 = os.environ.get('XI_OLD', '')    # required: the root of the older install (the folder holding VTABLE.DAT and ROM/)
OLD_TAG = os.environ.get('XI_OLD_TAG', 'ps2')
# Zone files for zones 256+ (dialog, NPC list, events) sit this many ids lower in the older install than on PC:
# 1320 in the final PS2 install (its client: NPC list zone + 84915, events zone + 83415). XI_OLD_HIGH_SHIFT.
OLD_HIGH_SHIFT = int(os.environ.get('XI_OLD_HIGH_SHIFT', '0'))

def old_id(pc_id, zone):
    """a zone file's id in the older install, from its PC id"""
    return pc_id - OLD_HIGH_SHIFT if zone >= 256 else pc_id
PC = os.environ.get('XI_PC', 'C:/Program Files (x86)/PlayOnline/SquareEnix/FINAL FANTASY XI')   # today's install

class Install:
    def __init__(self, root):
        if not root:
            raise SystemExit('set XI_OLD=<root of the older FINAL FANTASY XI install> (and XI_PC for the current install, if it is not in the default place)')
        self.root, self.tables = root, {}
        for n in range(1, 10):
            d = 'ROM' if n == 1 else 'ROM%d' % n
            v = os.path.join(root, 'VTABLE.DAT' if n == 1 else os.path.join(d, 'VTABLE%d.DAT' % n))
            f = os.path.join(root, 'FTABLE.DAT' if n == 1 else os.path.join(d, 'FTABLE%d.DAT' % n))
            if os.path.exists(v):
                vt = open(v, 'rb').read(); ft = open(f, 'rb').read()
                self.tables[n] = (vt, struct.unpack('<%dH' % (len(ft) // 2), ft), d)

    def path(self, fid):
        for n, (vt, ft, d) in self.tables.items():
            if fid < len(vt) and vt[fid] == n:
                e = ft[fid]
                return os.path.join(self.root, d, str(e >> 7), '%d.DAT' % (e & 0x7F))
        return None

    def read(self, fid):
        p = self.path(fid)
        return open(p, 'rb').read() if p and os.path.exists(p) else None

def dialog(raw):
    """Zone dialog DAT: u32 size|0x10000000, then everything XOR 0x80; u32 offset table, then the strings."""
    if not raw or len(raw) < 8: return None
    if struct.unpack_from('<I', raw, 0)[0] != ((len(raw) - 4) | 0x10000000): return None   # not a dialog file
    b = bytes(x ^ 0x80 for x in raw[4:])
    first = struct.unpack_from('<I', b, 0)[0]
    n = first // 4
    offs = list(struct.unpack_from('<%dI' % n, b, 0)) + [len(b)]
    return [b[offs[i]:offs[i + 1]] for i in range(n)]

def en_dialog_id(zone):
    return 6420 + zone if zone < 256 else 85590 + zone - 256
