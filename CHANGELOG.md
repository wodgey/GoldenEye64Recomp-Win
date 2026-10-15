# Changelog

Changes in this fork relative to cblock85/GoldenEye64Recomp.

## October 2026

### Windows build
- Native Windows build with Visual Studio 2022 (clang-cl, Ninja, CMake). A Linux-hosted MinGW-w64 cross-compile of the same Windows exe is also supported; see `docs/BUILDING_LINUX_CROSS.md`.
- The root `CMakeLists.txt` builds the nine patch sources directly with CMake custom commands (no GNU make) and runs `N64Recomp` (the Windows build on a Windows host, `n64recomp-src/build-linux/N64Recomp` otherwise).
- `tools_weaken_patched.py` only weakens patch functions that are actually emitted into `RecompiledPatches/patches.c`.
- RT64: standard `__uuidof` lookups in `rt64_shader_compiler.cpp` for the Dxc interfaces (explicit IIDs under MinGW); several small MSVC/clang-cl compatibility fixes.
- RT64 D3D12 debug layer: copy-queue barriers keep textures in `COMMON`, `FENCE_ZERO_WAIT` is filtered.
- RmlUi renderer: texture upload footprints use the texture's real format (BGRA for TGA thumbnails). This fixed a D3D12 crash when opening the Mods tab.

### Linux cross-build (MinGW-w64)
- The Windows exe can be built from Linux/WSL with MinGW-w64 and runs at 60 FPS on D3D12 and Vulkan. See `docs/BUILDING_LINUX_CROSS.md`.
- CMake picks its tools by build host: the patches step uses the Linux `N64Recomp` off Windows, the bundled Linux DXC is used when cross-compiling from Linux, and `wslpath` conversion is skipped when it is present. Windows-host builds are unchanged.
- MinGW Release builds link as a GUI application (no console window), matching the MSVC `/SUBSYSTEM:WINDOWS` setting.
- `ui_config.cpp`: `debug_context` is now a function-local static. Its constructor read `game_warps` before that global was initialised, which crashed at start-up under MinGW GCC.
- `ultramodern.hpp`: `debug_printf` is the empty define again. Upstream ships a live `printf` that flooded the console and slowed console-attached builds.
- RT64: a failed specialised-shader compile is caught and logged instead of terminating the process.
- `aspMain.us.toml`: `text_offset` restored to `0x22C80` (`0x22DA0` had been introduced by mistake).
- `RSPRecomp` prints the raw opcode for unhandled instructions.

### Features
- **FPS overlay** (F9, or the QOL setting): shows the active graphics backend, display FPS and game FPS.
- **QOL Settings tab**: Dual-Stick and Show FPS Counter.
- **Dual-Stick** (one gamepad presented as controller 1 and 2): works for all four GoldenEye 2.x control styles, right- and left-handed. D-pad Up/Down for crouch and zoom, D-pad Right/Left for next/previous weapon. Off by default; can be switched while playing.
- **Graphics API option** (Auto / D3D12 / Vulkan) on the Graphics tab. Takes effect on the next launch.
- **Mods tab**: enable or disable `.rtz` texture packs from the launcher and in game.

### Tooling
- `tools/tlbfree/`: reproducible generation of `ge007.tlbfree.elf` and the matching ROM from your own ROM. See `docs/TLBFREE_ELF.md`.
- `docs/BUILDING_INPUTS.md` and `docs/BUILDING_LINUX_CROSS.md`: how to produce the build inputs and how to cross-compile from Linux.
- `.gitattributes` for consistent line endings.
