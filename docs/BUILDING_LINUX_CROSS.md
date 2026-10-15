# Cross-compiling GoldenRecomp.exe for Windows from Linux (WSL)

Builds `GoldenRecomp.exe` for Windows using Linux tooling only: CMake, Ninja and MinGW-w64 GCC. The code generators and the shader compiler run on Linux; the resulting `.exe` runs on Windows.

This is an alternative to the Visual Studio route in [BUILDING_WINDOWS.md](BUILDING_WINDOWS.md), which remains the reference build. Both produce a working game.

Tested once, by one person, on a clean clone in WSL (Kali). The cross-built Release exe loads into gameplay and runs at 60 FPS on both Vulkan and D3D12. Long sessions, every mission and audio on every backend have not been checked.

Do not upload or commit the ROM or the ELF. Do not distribute the built `.exe`: it embeds code translated from the game.

## 1. Host tools versus target tools

Anything *executed during the build* must be a native Linux binary:

| Tool | Role |
|---|---|
| `N64Recomp`, `RSPRecomp` | generate `RecompiledFuncs/`, `RecompiledPatches/`, `rsp/aspMain.cpp` |
| `file_to_c` | embeds assets as C arrays |
| `dxc` (`dxc-linux` + `libdxcompiler.so` / `libdxil.so`) | compiles RT64 shaders |
| `clang` (MIPS target) + `ld.lld` | builds `patches/patches.elf` |

The CMake files used to choose some of these by `WIN32`, which refers to the *target*, so a MinGW build picked Windows tools. They now choose by build host, and Windows-host (Visual Studio) builds are unchanged:

- **N64Recomp for the patches step.** Windows host: `n64recomp-src/out/build/x64-Debug/N64Recomp.exe`. Any other host: `n64recomp-src/build-linux/N64Recomp`.
- **DXC.** Target Windows on a Linux host with `lib/rt64/src/contrib/dxc/bin/x64/dxc-linux` present: the Linux DXC. Otherwise `dxc.exe`, as before.
- **`wslpath` path conversion** (for calling a Windows `dxc.exe` from WSL): skipped when `dxc-linux` is present.
- **Subsystem.** Under MinGW GCC, non-Debug configurations link as a GUI application (`-mwindows`), matching the MSVC `/SUBSYSTEM:WINDOWS` setting. There is no console window. A Debug build keeps the console.

If a build fails with "cannot execute binary file" or "No such file" for a `.exe`, a Windows tool was selected where a Linux one is needed.

## 2. Packages and host tools

```sh
sudo apt install git cmake ninja-build g++ python3 mingw-w64 clang lld xdelta3
```

Build the Linux `N64Recomp` and `RSPRecomp` into `n64recomp-src/build-linux` (the path the patches step expects). Standard CMake invocation; adjust if your layout differs:

```sh
cmake -S n64recomp-src -B n64recomp-src/build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C n64recomp-src/build-linux N64Recomp RSPRecomp
cp n64recomp-src/build-linux/N64Recomp n64recomp-src/build-linux/RSPRecomp .
```

Build a host `file_to_c` and put it on `PATH`:

```sh
mkdir -p build-mingw
g++ -std=c++17 lib/rt64/src/tools/file_to_c/file_to_c.cpp -o build-mingw/file_to_c
export PATH="$PWD/build-mingw:$PATH"
```

Tested compiler: a pre-release MinGW GCC ("16.0.0"). A stable GCC or `llvm-mingw` is untested and may behave differently (see section 6, static initialisation).

## 3. Inputs and generation order (do this BEFORE configuring)

The ROM, TLB-free ROM and generated code are described in [BUILDING_INPUTS.md](BUILDING_INPUTS.md). In short, you need `ge007.tlbfree.z64` in the project root. No ELF is needed for a normal build.

The root `CMakeLists.txt` globs `RecompiledFuncs/*.c` at configure time, so configuring first fails with `No SOURCES given to target: RecompiledFuncs`. Generate first:

```sh
./N64Recomp us.toml                       # 1. RecompiledFuncs/
# 2. build patches/patches.elf (clang -target mips + ld.lld), then:
#    ./N64Recomp patches.toml             #    -> RecompiledPatches/patches.c
python3 tools_weaken_patched.py           # 3. only after patches.c exists
./RSPRecomp aspMain.us.toml               # 4. rsp/aspMain.cpp
```

Order matters:

- `tools_weaken_patched.py` reads `RecompiledPatches/patches.c` to learn which functions are patched, then marks the originals in `RecompiledFuncs/funcs_*.c` as `weak`. If it runs before `patches.c` exists, nothing is weakened and the link fails with duplicate symbols. It also applies the `cosf` to `sinf` fallthrough fix (idempotent).
- The script edits generated files in place. Re-running `N64Recomp us.toml` discards its edits, so run the script again afterwards.
- `build_linux.sh` runs the script straight after `N64Recomp us.toml`, before `patches.c` exists. This is a known ordering bug in that script, not yet fixed.
- `RSPRecomp` resolves `rom_file_path` relative to the toml's directory.

### aspMain offsets

`aspMain.us.toml` must contain:

```
text_offset   = 0x22C80
text_size     = 0xDC0
text_address  = 0x04001080
rom_file_path = "ge007.tlbfree.z64"
```

`0x22C80` is verified: the ELF symbol `aspMainTextStart` (file offset `0x24080` in the TLB-free ELF) matches the TLB-free ROM bytes at `0x22C80`. The value `0x22DA0` entered the fork in `7c8d5aa` by mistake and is fixed on the cross-build branch.

## 4. Configure and build

```sh
cmake -S . -B build-mingw -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/mingw-w64.cmake" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5
ninja -C build-mingw GoldenRecomp
```

`CMAKE_POLICY_VERSION_MINIMUM=3.5` is needed because bundled `lunasvg` declares an old minimum CMake version. Expect about 900 steps. Check the result:

```sh
file build-mingw/GoldenRecomp.exe        # PE32+ x86-64
x86_64-w64-mingw32-objdump -p build-mingw/GoldenRecomp.exe | grep -i "^Subsystem"   # Windows GUI
```

## 5. The DXC rule (the main runtime lesson)

RT64 compiles part of its D3D12 shaders at run time, linking a run-time shader against a library precompiled at build time. DXC refuses to link libraries from different compiler builds. The build-time compiler is the Linux `dxc` and the run-time compiler is the Windows `dxcompiler.dll`, so **both must come from the same bundle**.

What went wrong in testing: the Linux `dxc-linux`, `libdxcompiler.so` and `libdxil.so` in `lib/rt64/src/contrib/dxc/` had been replaced with a newer build (1.9, commit 4946) while the Windows DLLs stayed on the bundled one. The committed Linux files report `1.8 (4461)`. D3D12 then aborted at start-up:

```
std::runtime_error: Shader compilation error:
Cannot link libraries with conflicting compiler versions
  library "RasterVSEntry":   No version info available
  library "RasterVSLibrary": Version(1.9) commits(4946) ...
```

Fix:

```sh
git checkout -- lib/rt64/src/contrib/dxc          # restore the committed Linux DXC
find build-mingw -name '*.dxil.h' -delete          # force the shader headers to regenerate
ninja -C build-mingw GoldenRecomp
```

Check the version with `LD_LIBRARY_PATH=lib/rt64/src/contrib/dxc/lib/x64 lib/rt64/src/contrib/dxc/bin/x64/dxc-linux --version`.

Rules:

- Never swap only the Linux DXC, or only the Windows DLLs. Change both as a matched set and regenerate every `*.dxil.h`.
- After changing any DXC file, delete the generated `.dxil.h` files before rebuilding.
- Replacing DLLs in the release folder cannot fix a build-time mismatch, because the library is compiled into the exe.

Hardening: `rt64_raster_shader_cache.cpp` catches a failed specialised-shader compile in its worker thread, logs `RT64: specialised shader compile failed, ubershader will be used`, and carries on instead of calling `std::terminate`. RT64 already defaults to `ubershadersOnly = true`, so specialised shaders are never drawn with. It is not known whether this catch is still needed once the DXC files match.

## 6. MinGW-specific fixes

**DXC interface IDs.** MinGW ignores `__declspec(uuid)`, so `__uuidof(IDxcCompiler)` and friends fail to link (undefined `__mingw_uuidof<...>`). `lib/rt64/src/render/rt64_shader_compiler.cpp` defines explicit IIDs near the top (`IID_IDxcCompiler_RT64` and the Utils and Linker equivalents) and uses them under `#ifdef __MINGW32__`. The MSVC path keeps `__uuidof`.

**Static initialisation order.** The first MinGW exe crashed at start-up with an access violation. `src/ui/ui_config.cpp` had a global `DebugContext debug_context` whose constructor indexes `zelda64::game_warps`, a global vector defined in another file (`src/game/scene_table.cpp`). Initialisation order across files is unspecified: MSVC and clang-cl happened to work, and MinGW GCC ran the constructor first and read an empty vector. `debug_context` is now a function-local static, built on first use. A `RelWithDebInfo` build with `-D_GLIBCXX_ASSERTIONS` turns this class of bug into a readable assertion message instead of a silent crash. The debug context is a leftover from Zelda64Recomp and has no use in GoldenEye.

## 7. Run-time DLLs

The build already copies `SDL2.dll`, `dxil.dll` and `dxcompiler.dll` into `build-mingw/`. Copy those three next to the exe, and add:

| DLL | Source |
|---|---|
| `freetype.dll` | `lib/freetype-windows-binaries/win64/` |
| `libwinpthread-1.dll`, `libgcc_s_seh-1.dll`, `libstdc++-6.dll` | `x86_64-w64-mingw32-gcc -print-file-name=<name>` |

The `SDL2.dll` the build copies is the version CMake fetches. It can differ from the 2.26.3 copy under `lib/rt64/src/contrib/mupen64plus-win32-deps/`. Both are expected to work. If the game misbehaves around input or windowing, compare them first.

Do not copy Windows system DLLs (`d3d12.dll`, `dxgi.dll`, ...).

Exit codes: `0xC0000005` (-1073741819) is an access violation, `0xC0000135` is a missing DLL, and exit code 3 is `abort()` or `std::terminate`, so look for `what():` in the console output. The Release exe has no console, so for a failed start use a Debug build or run it with output redirected.

## 8. ROM and ELF

See [BUILDING_INPUTS.md](BUILDING_INPUTS.md) and [TLBFREE_ELF.md](TLBFREE_ELF.md). The TLB-free ROM (`ge007.tlbfree.z64`) is required. The ELF is optional and only for developers. Keep both out of git and out of release archives.

## 9. Debug tracing can fake a slow game

Upstream ships `debug_printf` in `lib/N64ModernRuntime/ultramodern/include/ultramodern/ultramodern.hpp` as a live `printf`. In a console-attached build, every thread-queue and scheduling event prints a line, and Windows console output is slow enough to stall the game at about 1 FPS and click the audio. This fork sets it to the empty define:

```cpp
#define debug_printf(...)
//#define debug_printf(...) printf(__VA_ARGS__);
```

Never judge performance with tracing on.

## 10. Known issues

- `include/recomp_input.h:146`: `get_default_mapping_for_input` returns a reference to a temporary (`-Wreturn-local-addr`). Undefined behaviour on the `default:` branch only. Not fixed.
- `build_linux.sh` runs `tools_weaken_patched.py` before `patches.c` exists (section 3). Not fixed.
- `lib/FindFreetype.cmake` hardcodes the Windows `freetype.lib` path (`vs2015-2022/win64`).
- `BUILDING_WINDOWS.md` says FreeType "2.13.x". The bundled binaries are 2.10.1.
- The `lib/rt64` first branch selects `dxc-linux` without checking that it exists. The root file does check.

## 11. Validation checklist

- [x] Release exe starts, reaches gameplay, 60 FPS on Vulkan
- [x] Same on D3D12, no `Cannot link libraries` error
- [ ] A full mission, long-session stability, audio on both backends
- [ ] A fresh clone of this branch builds and runs following only this guide
- [ ] The Visual Studio build still configures and builds from this branch
