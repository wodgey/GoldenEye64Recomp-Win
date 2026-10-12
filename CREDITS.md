# Credits

This is a Windows-focused fork. Almost everything in it is other people's work.

## Projects this fork builds on

| Project | Role |
|---|---|
| [cblock85/GoldenEye64Recomp](https://github.com/cblock85/GoldenEye64Recomp) | The macOS/Linux GoldenEye recompilation this fork is based on |
| GoldenRecomp | The earlier Windows-only GoldenEye recompilation that the macOS/Linux port was adapted from. The game patches in `patches/` (for example `workbench_theboy.c`) are credited to @theboy181 |
| [n64decomp/007](https://github.com/n64decomp/007) | The GoldenEye 007 decompilation. `tools/tlbfree/` rebuilds the TLB-free ELF from it |
| Kholdfuzion ([goldeneye_src](https://gitlab.com/kholdfuzion/goldeneye_src)) | The TLB-free, no-compression GoldenEye decomp work and the TLB-free ROM layout this port depends on |
| [N64Recomp](https://github.com/N64Recomp/N64Recomp) and [N64ModernRuntime](https://github.com/N64Recomp/N64ModernRuntime) | Static recompiler and runtime (`lib/N64ModernRuntime`, `n64recomp-src`) |
| [RT64](https://github.com/rt64/rt64) | Renderer (`lib/rt64`); Windows uses its D3D12 and Vulkan backends |
| [Zelda64Recomp](https://github.com/Zelda64Recomp/Zelda64Recomp) | The launcher, settings menu and UI structure this port's UI descends from |
| [RmlUi](https://github.com/mikke89/RmlUi) | UI library |
| [GamepadMotionHelpers](https://github.com/JibbSmart/GamepadMotionHelpers) | Controller motion helpers |
| [SDL2](https://www.libsdl.org/) | Windowing and input |
| [FreeType](https://freetype.org/) | Font rendering (Windows prebuilt binaries, see `docs/BUILDING_WINDOWS.md`) |
| [rt64/dxc-bin](https://github.com/rt64/dxc-bin) and [mupen64plus-win32-deps](https://github.com/mupen64plus/mupen64plus-win32-deps) | Prebuilt shader compiler and Windows dependencies used by RT64 |

Each project keeps its own licence; see its repository and the licence files under `lib/`.

## What this fork adds

See `CHANGELOG.md`. Much of the Windows build investigation and code was done with Claude (Anthropic) as an assistant.

## What is not included

No ROM, ELF or other Nintendo, Rare, Danjaq or EON material is in this repository. You supply your own legally obtained copy of GoldenEye 007 (NTSC-U) and generate the other files from it (`docs/TLBFREE_ELF.md`).

