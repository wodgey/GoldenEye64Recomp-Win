# How to produce the build inputs: TLB-free ROM, (optional) ELF, RecompiledFuncs

Everything here is generated from **your own** NTSC-U ROM. Never upload or commit the ROM, the ELF or the generated code, and do not distribute the built `.exe`.

The recipe already lives in the repo: `docs/TLBFREE_ELF.md`, `vanilla_to_tlbfree.xdelta`, `tools/tlbfree/`. This page puts the steps in order.

## What you need to know first

- The **committed `us.toml` needs no ELF.** It recompiles from the symbol data in `dump.toml` plus the TLB-free ROM. For a normal build (including the Linux cross-build) you need steps 1-2 and 4 only.
- The ELF (step 3) is optional, for developers who want to switch `us.toml` to `elf_path = "ge007.tlbfree.elf"`. If you do, do not commit that edit to `us.toml`.
- The stock decomp ELF (`ge007.u.elf`) is a TLB-based build and is not a substitute. Renaming it does not work.

## 1. Get the vanilla ROM

NTSC-U GoldenEye 007, big-endian `.z64`, SHA-1 `abe01e4aeb033b6c0836819f549c791b26cfde83`. Name it `baserom.u.z64`.

```sh
sha1sum baserom.u.z64
```

## 2. Make the TLB-free ROM

```sh
sudo apt install xdelta3
xdelta3 -d -s baserom.u.z64 vanilla_to_tlbfree.xdelta ge007.tlbfree.z64
ls -l ge007.tlbfree.z64      # expect 12,653,664 bytes
```

Put `ge007.tlbfree.z64` in the project root next to `us.toml`. (The app can also convert a retail dump in-app via `vanilla_to_tlbfree.gep1`; the xdelta route is for building.)

## 3. (Optional) Build the TLB-free ELF

Needs: the MIPS cross toolchain the decomp uses, `git`, `patch`, `xdelta3`, `python3`, `make`.

```sh
MIPS_BIN=$HOME/mips64-elf/bin tools/tlbfree/make_tlbfree_elf.sh /path/to/baserom.u.z64
```

The script checks the ROM hash, clones n64decomp/007 at `c4356466` into a scratch directory, applies `tools/tlbfree/tlbfree.patch`, builds the ELF, checks that its image matches the xdelta-built ROM (header CRCs aside; it stops if not), writes the symbol sizes from `tlbfree_func_sizes.txt`, and writes `ge007.tlbfree.elf` and `ge007.tlbfree.z64` to `~/ge007-tlbfree-build/out`. Copy the ELF next to `us.toml`. It never touches your repository.

Verification status: `RESUME=1` mode (reusing an already-built scratch tree) was verified to rebuild a ROM and ELF byte-identical to the maintainer's working ELF. A full from-scratch run (clone, patch, build) on a clean machine has not been confirmed. Set `COMPARE_ELF` to a known-good ELF to check a result byte-for-byte.

## 4. Generate the recompiled code

From the project root, in this order (details in `BUILDING_LINUX_CROSS.md`):

```sh
./N64Recomp us.toml                         # RecompiledFuncs/
# build patches/patches.elf (clang -target mips + ld.lld), then:
./N64Recomp patches.toml                    # RecompiledPatches/patches.c
python3 tools_weaken_patched.py             # only after patches.c exists
./RSPRecomp aspMain.us.toml                 # rsp/aspMain.cpp
```

Generated directories are gitignored. Re-running `N64Recomp us.toml` overwrites the weaken script's edits, so run the script again afterwards.

## Sanity checks

- `aspMain.us.toml` must have `text_offset = 0x22C80` (not `0x22DA0`).
- ELF mode only: keep `initTLBPrepareContext`, `resolve_TLBaddress_for_InvalidHit` and `tlbRandomGetNext` in the `stubs`/`ignored` lists of `us.toml`.

## Credits

The TLB-free ROM and the original TLB-free decomp work are Kholdfuzion's. The decompilation is n64decomp/007. The source patch was reconstructed by matching a build against the TLB-free ROM; it is not a copy of the original branch.
