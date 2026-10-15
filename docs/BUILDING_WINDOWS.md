# Building on Windows (DRAFT: check each step on a clean checkout)

## You need

- Windows 10/11, Visual Studio 2022 with the C++ and LLVM/clang-cl components, CMake and Ninja
- Your own GoldenEye 007 (NTSC-U) ROM as `baserom.u.z64` in the project root. The repository contains no ROM data.
- WSL (or any Linux) for the one-off `ge007.tlbfree.elf` generation, see `docs/TLBFREE_ELF.md`

## 1. TLB-free ROM and ELF

Follow `docs/TLBFREE_ELF.md` to produce `ge007.tlbfree.elf` and `ge007.tlbfree.z64`, then copy both into the project root.

## 2. Dependencies that are not in this repository

**RT64's shader compiler and Windows dependencies.** Clone [rt64/rt64](https://github.com/rt64/rt64) with `--recursive` (this fork was built against commit `43373749`) and copy these two folders into `lib/rt64/src/contrib/`:

| Folder | Upstream submodule | Commit used |
|---|---|---|
| `dxc` | https://github.com/rt64/dxc-bin | `cc15e71` |
| `mupen64plus-win32-deps` | https://github.com/mupen64plus/mupen64plus-win32-deps | `de8111f` |

(`boost-1.81.0` inside the second folder is not needed.)

**FreeType.** Download a prebuilt Windows FreeType 2.10.1 release and unpack it so that `lib/freetype-windows-binaries/include` and `lib/freetype-windows-binaries/release static/vs2015-2022/win64/` exist. `CMakeLists.txt` reads the headers from there.

## 3. Native N64Recomp

Open `n64recomp-src` in Visual Studio and build it (x64-Debug). The root `CMakeLists.txt` runs `n64recomp-src/out/build/x64-Debug/N64Recomp.exe`.

## 4. Build the game

Open the project root in Visual Studio (CMake, clang-cl, Ninja), pick an x64 configuration and build `GoldenRecomp`. Generated sources (`RecompiledFuncs/`, `RecompiledPatches/`, `rsp/aspMain.cpp`) are produced by the build and are not committed.

## 5. Runtime files

Next to `GoldenRecomp.exe` you need `SDL2.dll`, `freetype.dll`, `dxcompiler.dll` and `dxil.dll`. Run from the project root so `baserom.u.z64` and `gamecontrollerdb.txt` are found (the Visual Studio debugger working directory is already set to it).

User settings are stored in `%LOCALAPPDATA%\GoldenRecomp` (`general.json`, `graphics.json`, `controls.json`, `mods/`).

## Known issues

- Debug builds assert in an RmlUi flex layout on the Settings and Setup Controls screens; Release builds are unaffected.
- Release configuration needs an x64 shell and the FreeType arguments described above.

## Why a native Windows build, not WSL/MinGW

Cross-compiling the Windows `.exe` from WSL with MinGW (`mingw-w64.cmake`, kept in the repo for reference) did produce a linked executable, but it was not usable:

- The Release build crashed on launch with `0xC0000005` (access violation).
- The Debug build stopped on an assertion about an out-of-range `std::vector<AreaWarps>` index.

Neither was investigated further. A native build with Visual Studio 2022 and clang-cl ran, and gave a working debugger, so it is the supported route. It also needed a few fixes that MinGW had hidden, for example the Dxc interface lookups in `lib/rt64/src/render/rt64_shader_compiler.cpp`.

Two related points:

- The `N64Recomp` binary built under WSL is a Linux ELF and cannot run on Windows. Build `N64Recomp.exe` natively from `n64recomp-src` (step 3).
- A convenient workflow is to edit files from WSL (the repo is reachable at `/mnt/c/Users/.../GoldenEye64Recomp`) and run the CMake build from Windows.
