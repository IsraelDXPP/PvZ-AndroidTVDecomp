# LawnTVDecomp

A C++ decompilation of `libGameMain.so` from the Plants vs. Zombies TV /
Android edition, with the console content restored and the platform layer
replaced by SDL3 and bgfx.

The game logic is recovered from the original ARM binary. The platform layer
is native: SDL3 for windows, input, gamepads, audio, files and threads, bgfx
for rendering. The project builds `LawnTV.exe` and the Android/iOS
equivalents, and it is still a work in progress: many functions remain
approximate.

The original game is copyrighted by PopCap / EA. This repository contains
decompiled and reimplemented code and build instructions, without game assets.

## Start here

1. `docs/estado.md` — where we are: what is done, what is in progress, what is
   known to be wrong.
2. `docs/metodo.md` — how a change is made. The binary decides.
3. `docs/binario.md` — the binary itself: the builds, the addresses, the
   tools used to look at it.
4. `docs/upstream.md` — what belongs to this fork and what belongs to the
   upstream, how the sync works, and the licensing obligations.
5. `docs/verificacion.md` — how to build, how to smoke test, and what
   "verified" means here.
6. `docs/decisiones.md` — decisions already taken. Do not re-open them without
   new data.
7. `AGENTS.md` — the hard rules for any agent working in this repository.

## The one rule

**The binary is the source of truth.** Not PC PvZ, not the HD edition, not
what the game "should" do. The binary that this port follows is the **v115**
build of `libGameMain.so`:

```text
https://github.com/ZombieYetis/PlantsVsZombies-AndroidTV
  app/src/v115/jniLibs/armeabi-v7a/libGameMain.so
```

Every function in the port carries the address it was recovered from:

```cpp
// ==========================================================================
// LawnApp::HasSeedType(SeedType,bool)
// @002B756C  238 bytes
// ==========================================================================
```

The banner is `// Class::Method(Args)`, then `// @00XXXXXX  N bytes`, with the
size taken from the v115 symbol table. Inline notes use `// @0x2B756C: ...`.

If a function cannot be found, or the pseudo-C does not make sense, write no
code: leave `// NOT IN BINARY: <what is missing>` and report it.

### Two builds, one set of addresses

`v115` and `w111` are two builds of the same library, with different layouts.
The addresses quoted in this repository are **v115** ones. The Hex-Rays export
kept on this machine (`C:\Users\nydil\Documents\libGameMain.so.c`) is the
**w111** build, so its addresses do not match the quotes. A mismatch is not
evidence that the code here is wrong. `docs/binario.md` has the fingerprint
table and the commands to check a `.so`.

## Repository layout

| Path | Contents |
| --- | --- |
| `Lawn` | Game logic: `LawnApp`, `Board`, plants, zombies, levels, modes, dialogs |
| `Sexy` | Decompiled PopCap SexyApp framework: widgets, resources, strings, rendering interfaces |
| `Sexy/src/Platform` | SDL3 windows, input, gamepads, audio, files, threads, and bgfx rendering |
| `Sexy/include/Runtime` | Runtime compatibility helpers for the ARM decompilation and `Sexy::String` |
| `Sexy/include/ReconTypes.h` | The enum tables (`SEED_*`, `STORE_ITEM_*`, `REANIM_*`, `GAMEMODE_*`) |
| `TodLib` | `Tod*` helpers: particles, foley, curves, hesitation brackets, text drawing |
| `Levels` | Standalone TV and classic level data, installed as `properties/levels.xml` |
| `Lawn/src/main.cpp` | SDL3 application callbacks and frame updates |
| `CMakeLists.txt`, `CMakePresets.json` | Cross-platform builds and Debug / Release presets |
| `cmake` | Dependency discovery, iOS toolchain, and Android vcpkg triplets |
| `android` | SDLActivity launcher; Gradle builds the C++ libraries and the APK |
| `ios` | iOS app bundle: `Info.plist`, launch storyboard, app icon |
| `vcpkg.json` | Third-party dependency manifest |
| `Tools` | SCBP shader compiler, packer, and format documentation |
| `Shaders` | Game shader sources and multiplatform SCBP packages |
| `ThirdParty` | GPL-3.0-or-later notice and license for the adapted console code |
| `docs` | This documentation set |

## Build and run

Requirements: CMake 3.25+, Ninja, a C++17 compiler, and vcpkg (`$env:VCPKG_ROOT`
points at the vcpkg root; the presets use
`$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`).

```powershell
cmake --fresh --preset windows-x64-release
cmake --build --preset windows-x64-release --parallel
```

Output goes to `build/windows-x64-release/bin/Release/`. Assets are not in git:
put them in `assets/` beside the executable (the local build already has them
in `build/windows-x64-release/bin/Release/assets/files`).

Smoke test, from the output directory:

```powershell
.\LawnTV.exe --frames 5 --screenshot smoke.png --headless
```

It must exit with code 0 and write a 1280x720 PNG. `LawnTV-diagnostics.log`
next to the executable records the session; a `fatal_signal` or
`fatal_exception` line means the run crashed.

There is no test suite in this project. Build plus smoke test plus reading the
diagnostics log is the verification. `docs/verificacion.md` has the details
and the per-platform commands.

Runtime options live in `assets/setup.env`; see `setup.env.example`.

## The fork and the upstream

The upstream is `https://cnb.cool/wanxiace/LawnTVDecomp` (remote `origin`).
This fork is published at `https://github.com/IsraelDXPP/PvZ-AndroidTVDecomp`
(remote `github`) and adds the CI, the iOS target, the upstream sync job, and
a few platform fixes.

A scheduled job merges the upstream into a branch and opens a pull request
into `main`; it never pushes to `main` directly. That means the tree has to
stay mergeable: prefer additive changes, and never reformat files you are not
fixing. `docs/upstream.md` has the file-level map and the policy.

## Conventions

- All project documentation, comments, diagnostics and configuration text are
  in English. Identifiers are English.
- Four spaces, no tabs, Allman braces, 120 columns (`.clang-format`).
- `clang-format -i` on the files you changed, and only on those.
- One topic per commit, in English and imperative, saying what changed, in
  which function, and the address.
- Nothing proprietary enters git: no binaries, no assets, no `.pak`, no IDA
  databases, no `build/`.
- The game name, artwork, music and level data are not covered by this
  repository. Use original assets in accordance with their owners' permissions.

## Documentation

| File | Contents |
| --- | --- |
| `docs/estado.md` | Where we are: done, in progress, known gaps, backlog |
| `docs/metodo.md` | How to take a function from the binary into the port |
| `docs/binario.md` | The binary: builds, addresses, tools, fingerprints |
| `docs/upstream.md` | Fork versus upstream, sync policy, licensing |
| `docs/verificacion.md` | Build, smoke test, and what "verified" means |
| `docs/decisiones.md` | Decisions already taken, with the reason |
| `docs/achievement-conditions.md` | Audit of the twelve console achievements |
| `README.md` | Project overview, scope, build instructions, runtime options |
