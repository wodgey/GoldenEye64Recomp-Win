# GoldenEye64Recomp-Win

A Windows-focused fork of [GoldenEye64Recomp](https://github.com/cblock85/GoldenEye64Recomp), with native Windows builds, RT64 rendering, high-resolution texture-pack support, and some quality-of-life improvements.

**This is not an emulator.** The game's original MIPS code is translated into C and runs natively on your CPU.

**Status: working and playable.** The Windows `GoldenRecomp.exe` builds and runs (Direct3D 12 and Vulkan) with **both** a Windows toolchain (Visual Studio 2022, clang-cl) **and** a Linux toolchain (WSL, CMake, Ninja and MinGW-w64, cross-compiling to Windows). See [docs/BUILDING_WINDOWS.md](docs/BUILDING_WINDOWS.md) and [docs/BUILDING_LINUX_CROSS.md](docs/BUILDING_LINUX_CROSS.md). The Visual Studio route is the reference build; the Linux cross-build has had much less testing.

## About this fork

This is my attempt at getting the macOS/Linux-oriented GoldenEye recompilation project running as a native Windows PC game.

The motivation was simple: I wanted to play GoldenEye 007 with 4K high-resolution texture packs, similar to what has been possible with emulators. The first playable PC projects I tried used SDL2 rendering, but I knew that an RT64-based renderer would give me a better route towards the texture-pack support I wanted.

With very little coding experience, and a lot of help from Claude, I gradually worked through the build problems and got the game running in a state I'm happy with. It involved plenty of missteps, workarounds and debugging along the way; hopefully documenting what I learned will save someone else some of the trouble.

## Features and improvements

- **Native Windows build**, developed with WSL for much of the source work, with the CMake configuration and compilation done natively on Windows (Visual Studio 2022, clang-cl). The same Windows exe can also be cross-compiled from Linux with MinGW-w64; see [docs/BUILDING_LINUX_CROSS.md](docs/BUILDING_LINUX_CROSS.md).
- **RT64 rendering** with Direct3D 12 and Vulkan. The Graphics tab has a Graphics API option (Auto, D3D12 or Vulkan) that takes effect on the next launch.
- **High-resolution texture packs**: import `.rtz` packs and enable or disable them from the Mods tab.
- **Dual-Stick controller support** (at the expense of local multiplayer): a workaround that lets a single modern gamepad use GoldenEye's twin-stick 2.x control styles.
- **Classic GoldenEye control schemes**: the patch presents the gamepad to the game as two N64 controllers. One stick and its associated shoulder button go through port 0, the other stick through port 1, so the in-game Controls menu offers the original 2.1 to 2.4 schemes, including the left-handed ones. D-pad Up/Down crouch and zoom; D-pad Right/Left cycle weapons.
- **FPS counter** (F9, or the QOL Settings tab) showing the active graphics backend, display FPS and the game's own frame rate.
- **QOL Settings tab** in the launcher for the Dual-Stick and FPS counter options.

The dual-stick implementation is admittedly a little hacky, but it works. It relies on the original game's support for multiple controllers and keeps its existing control-scheme options. Through the in-game Controls menu you can still swap which shoulder button is AIM or FIRE and which stick controls MOVE or LOOK.

See [CHANGELOG.md](CHANGELOG.md) for the full list of changes against upstream.

## No Nintendo assets

This repository contains **no game code, no game assets, and no ROM data.** You
must own a retail GoldenEye 007 NTSC-U cartridge dump.

What *is* committed is factual symbol data — function names, addresses and sizes
(`dump.toml`, `data_dump.toml`) — plus original port code and a delta patch that
rearranges *your own* dump into the modified "TLBFREE" ROM layout the
recompilation needs. Everything game-derived is produced on your machine, from
your ROM.

## Requirements

**All platforms**

- Your own GoldenEye 007 **NTSC-U** ROM dump, big-endian `.z64`,
  sha1 `abe01e4aeb033b6c0836819f549c791b26cfde83`
  (byteswapped `.v64` / `.n64` dumps are accepted and converted too)

**Windows**

- Windows 10/11 and Visual Studio 2022 with the C++ and LLVM/clang-cl components, plus CMake and Ninja
- `xdelta3` (for example inside WSL) to turn your ROM into the TLBFREE ROM
- A few third-party dependencies that are not stored in this repository (RT64's shader compiler and Windows dependency pack, and a prebuilt FreeType); see [docs/BUILDING_WINDOWS.md](docs/BUILDING_WINDOWS.md)

**macOS**

- macOS 13+ (Apple Silicon or Intel)
- Full Xcode, for the Metal shader compiler:
  `sudo xcode-select -s /Applications/Xcode.app`
  (Command Line Tools alone are not enough)
- [Homebrew](https://brew.sh) — the build script installs the rest

**Linux**

- A Vulkan-capable GPU and drivers
- `cmake ninja-build clang lld python3 libsdl2-dev libfreetype-dev libgtk-3-dev xdelta3`
  (Debian/Ubuntu names; the build script checks and tells you what's missing)

## Building on Windows

Step-by-step instructions, including where each external dependency comes from, are in
[docs/BUILDING_WINDOWS.md](docs/BUILDING_WINDOWS.md). In outline:

1. Put your ROM in the project root as `baserom.u.z64` and create the TLBFREE ROM with
   `xdelta3 -d -s baserom.u.z64 vanilla_to_tlbfree.xdelta ge007.tlbfree.z64`.
2. Add the dependencies that are not in the repository (`dxc`, `mupen64plus-win32-deps`, FreeType).
3. Build the native `N64Recomp.exe` from `n64recomp-src`.
4. Open the project root in Visual Studio and build `GoldenRecomp`.

No `ge007.tlbfree.elf` is needed. Developers who want to recompile from an ELF instead of the committed
symbol data can generate one from their own ROM; see [docs/TLBFREE_ELF.md](docs/TLBFREE_ELF.md).

## Build (macOS and Linux)

There are two build modes. Both need your ROM; they differ in *when* the game
code is generated.

### Standard build

The game code is generated from your ROM at build time and compiled into the
executable. Fastest to launch; the resulting binary is personal to you and
should not be redistributed.

```bash
git clone --recurse-submodules https://github.com/cblock85/GoldenEye64Recomp.git
cd GoldenEye64Recomp
cp /path/to/your/goldeneye.z64 baserom.u.z64

./build_macos.sh          # macOS
./build_linux.sh          # Linux
```

(If you already cloned without `--recurse-submodules`, don't worry — the build
scripts fetch what's missing. The `lib/ge` submodule is the decompilation, whose
headers the gameplay patches compile against.)

### Clean build

The executable ships with **zero game-derived CPU code**. At first launch it
recompiles all ~3,000 game functions from your ROM in memory — about two seconds —
using N64Recomp's live recompiler. This is the variant suitable for distributing
binaries, since the app itself contains only original code plus symbol facts.

```bash
./build_macos.sh --clean  # macOS
./build_linux.sh --clean  # Linux
```

(The audio microcode is still translated at build time, so a ROM is needed once
even for a clean build.)

### macOS app bundle

Optional, wraps the binary into a double-clickable `.app` with an icon:

```bash
./make_macos_app.sh                          # standard build
./make_macos_app.sh build-clean/GoldenRecomp # clean build
```

## Running

```bash
./build/GoldenRecomp          # standard build
./build-clean/GoldenRecomp    # clean build
```

or open **GoldenEye 007 Recompiled.app** on macOS.

On first launch, pick your ROM in the picker — a plain retail NTSC-U dump works
directly, and the app converts it for you. Settings and the stored ROM live in
`%LOCALAPPDATA%\GoldenRecomp` (Windows), `~/Library/Application Support/GoldenRecomp` (macOS) or
`~/.config/GoldenRecomp` (Linux); delete the stored `.z64` there if you ever want the ROM picker back.

## State

**Working:** the full intro (Nintendo and Rareware logos, gunbarrel), menus,
file select, briefings, missions, audio, controller support, frame interpolation
to your display's refresh rate, and widescreen.

**Known issues**, inherited from upstream GoldenRecomp and present in the Windows
build too: skyboxes render black and water is flat (the game draws them with
custom microcode commands RT64 doesn't implement); the multiplayer UI is
incomplete; some weapons fire too fast because the game runs at native 60Hz.

**Region:** only NTSC-U (US) is supported. Other regions are rejected — the
symbol data here is generated against the US ROM. The decompilation supports EU,
so PAL support is possible future work.

## Status and maintenance

This fork has reached a state where I'm happy with it for my own use. I don't currently intend to maintain it actively, unless significant progress upstream gives me a reason to revisit it. I will, however, try to keep the build notes accurate in case they help someone else.

## What was fixed to get here (upstream)

Beyond the macOS and Linux platform glue, the upstream port surfaced and fixed a number
of bugs affecting all platforms. Drafted upstream reports live in
`docs/upstream-issues/`.

**N64ModernRuntime**

1. SP/DP task-completion messages were silently dropped when the game's scheduler
   queue was full, wedging GoldenEye's renderer. Fixed with a FIFO pending queue
   in `ultramodern/src/mesgqueue.cpp`.

**RT64**

2. Specialized (spec-constant) raster shaders produce broken output on the
   SPIRV/Metal shader paths, so any geometry using them silently vanished — no
   character models, no weapon in hand. Worked around by forcing the ubershader
   path (`ubershadersOnly` in `rt64_workload_queue.h`).
3. Frame interpolation interpolated *rotation* unconditionally (a `FIXME` in
   tree), so mismatched transform pairs swept garbage geometry across the screen
   for a frame every few seconds while moving. Added a teleport guard mirroring
   the existing translation heuristic (`rt64_rigid_body.cpp`).

**N64Recomp (static)**

4. The game's `cosf` is a 3-instruction stub that *falls through* into `sinf`
   (`cos(x) = sin(x + pi/2)`). N64Recomp truncates functions at the fallthrough
   boundary, so `cosf` returned stale garbage — breaking every CPU-built rotation
   matrix in the game (the Nintendo logo zoomed past the camera; Bond was a white
   triangle in the gunbarrel). Repaired post-generation in
   `tools_weaken_patched.py`.

**N64Recomp (live recompiler)** — found while making the clean build work:

5. Odd-indexed single FPR accesses (`mtc1 $f5`, …) hit a `TODO`/`assert` that
   compiles out in release builds, returning context offset `-1` and silently
   corrupting memory. Implemented properly via the runtime `f_odd` pointer.
6. `mtc1` emitted a full-width 64-bit move into a 32-bit float-word slot,
   clobbering the adjacent half of the FPR pair — corrupting any double built
   high-half-first, which is what GoldenEye's audio synthesizer does.
7. Jump-table analysis assumed jump tables live in the same section as the
   function using them; GoldenEye keeps game-code jump tables in its data
   segment, which crashed the analyzer.

## Credits

This project stands on the shoulders of giants. See [CREDITS.md](CREDITS.md) for the full list. In short:

- [cblock85/GoldenEye64Recomp](https://github.com/cblock85/GoldenEye64Recomp) — the macOS/Linux port this fork is based on
- The [GoldenEye decompilation](https://gitlab.com/kholdfuzion/goldeneye_src) team
- [kholdfuzion](https://github.com/kholdfuzion)'s [GoldenRecomp](https://github.com/kholdfuzion/goldenrecomp) — the Windows recomp the port builds on
- [Mr-Wiseguy](https://github.com/Mr-Wiseguy) — N64Recomp, N64ModernRuntime, and the recomp technique
- [RT64](https://github.com/rt64/rt64) — the renderer

Claude also played a substantial role in helping me investigate build failures, work through code changes and develop the Windows-specific modifications.

## License

The port code and build scripts follow the licenses of the projects they extend
(see `COPYING` and the license files under `lib/`). No Nintendo, Rare, Danjaq, or
EON material is included; GoldenEye 007 is their property. This project exists for
preservation and interoperability, and requires you to supply your own legally
obtained ROM.
