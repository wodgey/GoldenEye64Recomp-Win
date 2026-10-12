#!/usr/bin/env bash
# Rebuild ge007.tlbfree.elf (and the matching ROM) from YOUR OWN legally obtained
# GoldenEye 007 (U) ROM. See docs/TLBFREE_ELF.md for what this does and why.
#
# Usage:  tools/tlbfree/make_tlbfree_elf.sh [path/to/baserom.u.z64]
# Env:    RESUME=1   reuse an existing $WORK/007 that already built the ELF (skips clone/patch/build)
#         MIPS_BIN   dir holding the mips64-elf toolchain  (default ~/mips64-elf/bin)
#         WORK       scratch dir, must not exist yet       (default ~/ge007-tlbfree-build)
#         OUT        where results are written             (default $WORK/out)
#         COMPARE_ELF  optional known-good ELF to compare the result against
#
# Nothing in your repo is overwritten; copy the results out of $OUT yourself.
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)
ROM=${1:-$REPO/baserom.u.z64}
WORK=${WORK:-$HOME/ge007-tlbfree-build}
OUT=${OUT:-$WORK/out}
MIPS_BIN=${MIPS_BIN:-$HOME/mips64-elf/bin}
BASE_COMMIT=c4356466796c697dfd298010b9bed261f9ed8c6a   # n64decomp/007 master the patch was made against
VANILLA_SHA1=abe01e4aeb033b6c0836819f549c791b26cfde83  # clean NTSC-U ROM

die() { echo "ERROR: $*" >&2; exit 1; }
step() { echo; echo "== $*"; }

[ -f "$ROM" ] || die "ROM not found: $ROM (supply your own baserom.u.z64)"
[ ! -e "$WORK" ] || [ "${RESUME:-0}" = 1 ] || die "$WORK already exists; remove it, set WORK=..., or use RESUME=1"
for t in git patch xdelta3 python3 make sha1sum; do command -v "$t" >/dev/null || die "missing tool: $t"; done
OBJCOPY=$(ls "$MIPS_BIN"/*objcopy 2>/dev/null | head -1) || true
[ -n "${OBJCOPY:-}" ] || die "no MIPS objcopy in $MIPS_BIN (set MIPS_BIN)"
export PATH="$MIPS_BIN:$PATH"

step "Checking ROM"
got=$(sha1sum "$ROM" | cut -d' ' -f1)
[ "$got" = "$VANILLA_SHA1" ] || die "ROM SHA-1 is $got, expected $VANILLA_SHA1 (NTSC-U, big-endian .z64)"

mkdir -p "$WORK" "$OUT"
if [ "${RESUME:-0}" = 1 ]; then
  step "RESUME: reusing $WORK/007"
  cd "$WORK/007"
else
  step "Cloning n64decomp/007 @ ${BASE_COMMIT:0:8}"
  git clone https://github.com/n64decomp/007 "$WORK/007"
  cd "$WORK/007"
  git checkout -q "$BASE_COMMIT"
  cp "$ROM" baserom.u.z64

  step "Applying tlbfree.patch"
  patch -p1 --dry-run < "$HERE/tlbfree.patch" >/dev/null || die "patch does not apply cleanly"
  patch -p1 < "$HERE/tlbfree.patch"
fi

step "Building ELF (make VERSION=US)"
# Plain make builds the helper tools (armips, n64cksum, ...) first. The first parallel
# pass can race on those tools, so retry serially. A failure in the final ROM packaging
# step is tolerated; only the ELF is needed.
ELF=build/u/ge007.u.elf
if [ "${RESUME:-0}" != 1 ]; then
  make VERSION=US COMPARE=0 -j"$(nproc)" || make VERSION=US COMPARE=0 || echo "(make reported an error; continuing if the ELF was produced)"
fi
[ -f "$ELF" ] || die "build did not produce $ELF"

step "Verifying against the TLB-free ROM"
xdelta3 -d -f -s "$ROM" "$REPO/vanilla_to_tlbfree.xdelta" "$OUT/ge007.tlbfree.z64"
"$OBJCOPY" -O binary --gap-fill=0xff "$ELF" "$WORK/built.bin"
cp "$WORK/built.bin" "$WORK/built_ck.z64"
tools/n64cksum "$WORK/built_ck.z64" >/dev/null   # fixes the header CRCs, as the normal build does
bs=$(stat -c %s "$WORK/built_ck.z64"); ts=$(stat -c %s "$OUT/ge007.tlbfree.z64")
echo "built ROM image: $bs bytes; target ROM: $ts bytes"
n=$(( bs < ts ? bs : ts ))
cmp -n "$n" "$WORK/built_ck.z64" "$OUT/ge007.tlbfree.z64" && echo "ROM bytes match over $n bytes" \
  || die "built image does not match the TLB-free ROM"
[ "$bs" -eq "$ts" ] || echo "WARNING: sizes differ; the ELF image is shorter/longer than the ROM (trailing padding?)"

step "Fixing symbol sizes"
python3 "$HERE/elf_sizes.py" apply "$ELF" "$HERE/tlbfree_func_sizes.txt" "$OUT/ge007.tlbfree.elf"

if [ -n "${COMPARE_ELF:-}" ]; then
  step "Comparing with $COMPARE_ELF"
  cmp "$OUT/ge007.tlbfree.elf" "$COMPARE_ELF" && echo "IDENTICAL to reference ELF"
fi

echo
echo "Done. Results in $OUT:"
ls -l "$OUT"
echo "Copy ge007.tlbfree.elf and ge007.tlbfree.z64 into the GoldenRecomp project root."
