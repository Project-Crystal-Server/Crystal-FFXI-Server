"""Order-preserving alignment of a current (PC) id table onto the 2010 (PS2) one: pc index -> ps2 index or None."""
import difflib

def align(pc, ps2):
    sm = difflib.SequenceMatcher(None, pc, ps2, autojunk=False)
    m = [None] * len(pc)
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == 'equal':
            for k in range(i2 - i1): m[i1 + k] = j1 + k
    return m
