# Generating `ge007.tlbfree.elf`

N64Recomp turns a game into C from two inputs: the ROM and an **ELF** of the same build that carries the section layout and symbol names (`us.toml` points at both: `ge007.tlbfree.z64` and `ge007.tlbfree.elf`). The ROM comes from `vanilla_to_tlbfree.xdelta`. The ELF cannot come from a ROM patch, so it has to be produced by a build of the GoldenEye decompilation that matches the TLB-free ROM.

Neither file is distributed here. Both contain compiled game code and data, so you build them yourself from your own ROM. This repository only ships the recipe: a small source patch, a size table and two scripts.

## Why the stock decomp ELF is not enough

[n64decomp/007](https://github.com/n64decomp/007) builds `ge007.u.elf`, an ordinary build. It relies on TLB demand-paging for the game segment at `0x7F000000` and on boot-time decompression of the data segment. The TLB-free ROM does not, so its code and data are laid out differently. An ELF from the stock build does not describe that ROM, so renaming it does not work.

## How the matching ELF was found

The original TLB-free decomp branch could not be recovered, so the build was reconstructed from the ROM:

1. Build the stock decomp at commit `c4356466` (`make VERSION=US`).
2. Create the target ROM from a vanilla ROM with `xdelta3` and `vanilla_to_tlbfree.xdelta`.
3. Convert the freshly built ELF to a flat image (`objcopy -O binary --gap-fill=0xff`) and compare it with the target ROM, region by region (`regions.py`) and instruction by instruction. The instruction comparison (`align.py`, `alignseg.py`) fingerprints each MIPS word, ignoring register and immediate operands, and resynchronises after each divergence. That shows where the source builds a different number of instructions, and so which function is at fault.
4. Edit the decomp source (linker script, startup code and the TLB hooks) until the image matched the target ROM. The CPU can no longer fault on game pages, so the TLB code goes. Only the ROM header CRCs differed in the end, and `n64cksum` recomputes those.
5. The assembler leaves many asm functions with a symbol size of 0. N64Recomp needs sizes, so they were filled in. Sizes were first estimated from the gap to the next symbol, then corrected using the function table from an earlier N64Recomp run.

The ELF is therefore a build whose code and data are byte-identical to the TLB-free ROM, with symbols from the public decomp.

## What is in this repo

| File | Purpose |
|---|---|
| `tools/tlbfree/tlbfree.patch` | Source patch against n64decomp/007 `c4356466` (`ge007.ld`, `src/boot.s`, `src/boss.c`, `src/crash.c`, `src/game/lv.c`, `src/init.c`). Text only. |
| `tools/tlbfree/tlbfree_func_sizes.txt` | `address name size` for the symbols whose size needs fixing. |
| `tools/tlbfree/elf_sizes.py` | Writes those sizes into the freshly built ELF. |
| `tools/tlbfree/make_tlbfree_elf.sh` | Does everything below in one go. |
| `vanilla_to_tlbfree.xdelta` | Vanilla ROM to TLB-free ROM. |

## Usage

You need your own NTSC-U ROM (SHA-1 `abe01e4aeb033b6c0836818f549c791b26cfde83`, big-endian `.z64`), the MIPS cross toolchain the decomp uses, `git`, `patch`, `xdelta3`, `python3` and `make`.

```sh
MIPS_BIN=$HOME/mips64-elf/bin tools/tlbfree/make_tlbfree_elf.sh /path/to/baserom.u.z64
```

The script:

1. checks the ROM hash;
2. clones n64decomp/007 into a fresh scratch directory and checks out `c4356466`;
3. applies `tlbfree.patch`;
4. builds `build/u/ge007.u.elf`. Despite the name, with the patch applied this is the TLB-free ELF;
5. converts the ELF to an image, fixes the CRCs and compares it with the xdelta-built ROM. It stops if they differ;
6. applies `tlbfree_func_sizes.txt` to the ELF;
7. writes `ge007.tlbfree.elf` and `ge007.tlbfree.z64` to `$WORK/out` (default `~/ge007-tlbfree-build/out`). Copy both into the project root next to `us.toml`.

It never overwrites anything in your repository.

## Notes and caveats

- `us.toml` may need `initTLBPrepareContext`, `resolve_TLBaddress_for_InvalidHit` and `tlbRandomGetNext` in its `ignored` list. These TLB helpers have no body in the TLB-free build, so N64Recomp's output for them is empty.
- The size table was generated from the maintainer's working ELF: `elf_sizes.py make-table <unsized.elf> <sized.elf>`. If the patch is ever changed, regenerate it.
- Before publishing, `make_tlbfree_elf.sh` was checked with `bash -n` and `elf_sizes.py` was round-tripped on synthetic ELFs, but the script has not yet been run end to end from a fresh clone. Set `COMPARE_ELF` to a known-good ELF to confirm that the result is byte-identical.

## Credits

The TLB-free ROM and the original TLB-free decomp work are Kholdfuzion's. The decompilation is [n64decomp/007](https://github.com/n64decomp/007). The source patch here was reconstructed by matching the build against the TLB-free ROM; it is not a copy of the original branch.
