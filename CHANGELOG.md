# Changelog

Changes in this fork relative to cblock85/GoldenEye64Recomp.

## October 2026

### Windows build
- Native Windows build with Visual Studio 2022 (clang-cl, Ninja, CMake). The WSL/MinGW cross-compile is kept only as a diagnostic path.
- The root `CMakeLists.txt` builds the nine patch sources directly with CMake custom commands (no GNU make) and runs the native `N64Recomp.exe`.
- `tools_weaken_patched.py` only weakens patch functions that are actually emitted into `RecompiledPatches/patches.c`.
- RT64: standard `__uuidof` lookups in `rt64_shader_compiler.cpp` for the Dxc interfaces; several small MSVC/clang-cl compatibility fixes.
- RT64 D3D12 debug layer: copy-queue barriers keep textures in `COMMON`, `FENCE_ZERO_WAIT` is filtered.
- RmlUi renderer: texture upload footprints use the texture's real format (BGRA for TGA thumbnails). This fixed a D3D12 crash when opening the Mods tab.

### Features
- **FPS overlay** (F9, or the QOL setting): shows the active graphics backend, display FPS and game FPS.
- **QOL Settings tab**: Dual-Stick and Show FPS Counter.
- **Dual-Stick** (one gamepad presented as controller 1 and 2): works for all four GoldenEye 2.x control styles, right- and left-handed. D-pad Up/Down for crouch and zoom, D-pad Right/Left for next/previous weapon. Off by default; can be switched while playing.
- **Graphics API option** (Auto / D3D12 / Vulkan) on the Graphics tab. Takes effect on the next launch.
- **Mods tab**: enable or disable `.rtz` texture packs from the launcher and in game.

### Tooling
- `tools/tlbfree/`: reproducible generation of `ge007.tlbfree.elf` and the matching ROM from your own ROM. See `docs/TLBFREE_ELF.md`.
- `.gitattributes` for consistent line endings.
