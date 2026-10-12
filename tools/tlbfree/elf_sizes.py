#!/usr/bin/env python3
"""Symbol-size fix-up for the TLB-free GoldenEye ELF.

The decomp build leaves many hand-written assembly functions with st_size == 0.
N64Recomp needs real sizes, so after the build we patch them in.

  elf_sizes.py make-table  UNSIZED.elf SIZED.elf   > tlbfree_func_sizes.txt
  elf_sizes.py apply       IN.elf TABLE OUT.elf

make-table records every symbol whose size differs between two ELFs as
"<addr> <name> <size>" (hex, one per line). apply writes those sizes back into
a fresh build, matching on (address, name). Only the symbol table is touched.
Expects ELF32 big-endian (MIPS).
"""
import struct
import sys


def load(path):
    d = bytearray(open(path, 'rb').read())
    if d[:4] != b'\x7fELF' or d[4] != 1 or d[5] != 2:
        sys.exit(f'{path}: not an ELF32 big-endian file')
    shoff, = struct.unpack_from('>I', d, 0x20)
    shentsize, shnum = struct.unpack_from('>HH', d, 0x2e)
    secs = [struct.unpack_from('>IIIIIIIIII', d, shoff + i * shentsize) for i in range(shnum)]
    sym = next((s for s in secs if s[1] == 2), None)  # SHT_SYMTAB
    if sym is None:
        sys.exit(f'{path}: no symbol table')
    strtab = secs[sym[6]]
    off, size = sym[4], sym[5]
    rows = []
    for i in range(size // 16):
        nm, val, sz, info, other, shn = struct.unpack_from('>IIIBBH', d, off + i * 16)
        s = strtab[4] + nm
        name = d[s:d.index(0, s)].decode(errors='replace')
        rows.append((i, name, val, sz, info, shn))
    return d, off, rows


def make_table(a, b):
    _, _, ra = load(a)
    _, _, rb = load(b)
    if len(ra) != len(rb):
        sys.exit(f'symbol counts differ ({len(ra)} vs {len(rb)}); ELFs are not from the same build')
    n = 0
    for x, y in zip(ra, rb):
        if (x[1], x[2]) != (y[1], y[2]):
            sys.exit(f'symbol {x[0]} differs in name/address; ELFs are not from the same build')
        if x[3] != y[3]:
            print(f'{y[2]:08x} {y[1]} {y[3]:#x}')
            n += 1
    print(f'{n} sizes recorded', file=sys.stderr)


def apply(src, table, dst):
    want = {}
    for line in open(table):
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        addr, name, size = line.split()
        want[(int(addr, 16), name)] = int(size, 0)
    d, off, rows = load(src)
    done = set()
    for i, name, val, sz, info, shn in rows:
        k = (val, name)
        if k in want:
            struct.pack_into('>I', d, off + i * 16 + 8, want[k])
            done.add(k)
    missing = set(want) - done
    open(dst, 'wb').write(d)
    print(f'applied {len(done)} of {len(want)} sizes -> {dst}')
    if missing:
        for a, n in sorted(missing)[:20]:
            print(f'  not found: {a:08x} {n}', file=sys.stderr)
        sys.exit(f'{len(missing)} table entries did not match a symbol; this build differs from the reference')


if __name__ == '__main__':
    if len(sys.argv) == 4 and sys.argv[1] == 'make-table':
        make_table(sys.argv[2], sys.argv[3])
    elif len(sys.argv) == 5 and sys.argv[1] == 'apply':
        apply(sys.argv[2], sys.argv[3], sys.argv[4])
    else:
        sys.exit(__doc__)
