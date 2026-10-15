# TODO

Open items for this fork. Roughly ordered by usefulness, not by urgency.

## Build and docs
- [ ] Verify that a fresh clone builds and runs using only `docs/BUILDING_LINUX_CROSS.md` (the guide flags this as unverified).
- [ ] Run `tools/tlbfree/make_tlbfree_elf.sh` from scratch (no `RESUME=1`) with `COMPARE_ELF` set, and confirm the result is byte-identical to a known-good ELF.
- [ ] Validate `docs/BUILDING_WINDOWS.md` on a clean checkout, including the Clean build (`-DLIVE_GAMECODE=ON`).
- [ ] `build_linux.sh` runs `tools_weaken_patched.py` before `RecompiledPatches/patches.c` exists. Move it after the patches step.
- [ ] `include/recomp_input.h:146`: `get_default_mapping_for_input` returns a reference to a temporary (`-Wreturn-local-addr`). Undefined behaviour on the `default:` branch.
- [ ] `lib/FindFreetype.cmake` hardcodes the Windows `freetype.lib` path.
- [ ] `lib/rt64/CMakeLists.txt`: the Linux DXC branch should check that `dxc-linux` exists, as the root file does.
- [ ] Try a stable GCC and `llvm-mingw` for the Linux cross-build (only a pre-release GCC has been tested).
- [ ] Check whether the `try/catch` around the specialised shader compile in `rt64_raster_shader_cache.cpp` is still needed now that the DXC versions match.

## Upstream
- [ ] Tell upstream (cblock85/GoldenEye64Recomp) that `debug_printf` ships as a live `printf` in `ultramodern.hpp`, which floods console-attached builds.

## Game
- [ ] Play a full mission and check long-session stability on both D3D12 and Vulkan.
- [ ] Check audio on D3D12.
- [ ] Controller (D-pad / Accept) navigation of the Mods list.
- [ ] Blank mod thumbnails in the Mods tab.
- [ ] RmlUi flex assertion on Settings / Setup Controls in Debug builds.
