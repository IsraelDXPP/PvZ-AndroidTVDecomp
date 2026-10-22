# The fork, the upstream, and the licenses

This repository is a fork of **LawnTVDecomp** by `wanxiace`
(`https://cnb.cool/wanxiace/LawnTVDecomp`, remote `origin`). It is published
as **PvZ-AndroidTVDecomp** (`https://github.com/IsraelDXPP/PvZ-AndroidTVDecomp`,
remote `github`).

The upstream pushes into this repository on a schedule. Everything below exists
so that merge stays possible.

## Remotes

```sh
git remote -v
# origin  https://cnb.cool/wanxiace/LawnTVDecomp.git
# github  https://github.com/IsraelDXPP/PvZ-AndroidTVDecomp.git
```

`origin` is the upstream, read-only by convention. `github` is this fork.

## The sync job

`.github/workflows/sync-upstream.yml` runs every six hours
(`cron: '17 */6 * * *'`) and can be started by hand
(`workflow_dispatch`, with an optional `upstream_branch` input).

What it does:

1. Fetches the upstream branch.
2. Merges it into a fresh `upstream-sync` branch based on `main`.
3. If the merge is clean, it restores the fork's `.github/workflows` (the
   upstream's versions are discarded: the `GITHUB_TOKEN` cannot write
   workflows) and opens or updates a pull request `upstream-sync` into
   `main`.
4. If the merge conflicts, it aborts, publishes the exact upstream state as
   the `upstream-mirror` branch, and opens an issue asking for a manual
   resolution.

It never pushes to `main` directly. The fork's own commits are never
overwritten in silence.

## What this fork actually changes

Measured with `git diff origin/main..main --stat` against the upstream commit
`813516b` (2026-10-18): 21 files, 694 insertions, 8 deletions.

| File | Change |
| --- | --- |
| `.github/workflows/build.yml` | New: Windows, macOS, Linux, Android and iOS builds |
| `.github/workflows/sync-upstream.yml` | New: the upstream sync |
| `.gitignore` | `*.ipa`, `Payload/` |
| `CMakeLists.txt`, `CMakePresets.json` | iOS target and iOS presets |
| `Lawn/src/main.cpp` | SDL entry point tweak |
| `README.md` | Fork sections |
| `Sexy/CMakeLists.txt` | iOS sources |
| `Sexy/src/AtlasParser.cpp` | Atlas failures reported on stderr instead of dropped |
| `Sexy/src/EncodingParser.cpp` | UTF-8 truncation fix for accented characters |
| `Sexy/src/GamepadApp.cpp` | Gamepad logging |
| `Sexy/src/Platform/Bgfx/BgfxRenderDevice.cpp` | iOS rendering device |
| `Sexy/src/Platform/Bgfx/BgfxRenderDevice.ios.mm` | New: `CAMetalLayer` handle for bgfx |
| `cmake/IosVcpkg.cmake`, `cmake/ios.toolchain.cmake`, `cmake/triplets/lawn-arm64-ios.cmake` | New: iOS toolchain and triplet |
| `ios/Info.plist`, `ios/LaunchScreen.storyboard`, `ios/Assets.xcassets/**` | New: iOS bundle |

The console content (the twelve achievements, the house and records scene,
the leaderboard sign, the trash piles, the house blueprints) is **already in
the upstream**: `origin/main` carries `Lawn/src/game/AchievementSystem.cpp`
with the same `PvZ TV Touch Team` GPL header this fork has. It is not a fork
delta, and the sync will keep carrying it.

## Rules that keep the merge possible

- **Additive changes in their own files.** A new file never conflicts.
- **Never reformat a file you are not fixing.** The upstream's formatting is
  its own; a whitespace-only diff across 5,000 lines is a conflict waiting to
  happen. `clang-format` runs only on the files you actually changed.
- **Do not touch `.github/workflows/` from the upstream.** The sync restores
  the fork's versions, but a change there can still break the job.
- **Check what is ours before committing:**

  ```sh
  git diff origin/main..main --stat
  ```

  Every entry should be explained by the fork's purpose. If something shows
  up that is not yours, it is not yours to keep.

- **When the upstream changes a ported function**, re-read the binary and
  update the port. The upstream's version is a second opinion, not the
  authority: `docs/metodo.md` gives the order of confidence.
- **When the sync reports a conflict**, resolve it by hand and keep both
  sides' intent. The `upstream-mirror` branch holds the upstream state for
  exactly that case.

## Licensing

Two licenses live in this repository at the same time.

| Part | License |
| --- | --- |
| The decompiled and reimplemented code | MIT (declared in `vcpkg.json`) |
| The console content and part of the mouse controls, adapted from [PlantsVsZombies-AndroidTV](https://github.com/ZombieYetis/PlantsVsZombies-AndroidTV) | GPL-3.0-or-later |

The adapted files keep their header:

```cpp
// Copyright (C) 2023-2026 PvZ TV Touch Team. GPL-3.0-or-later.
// Adapted to native C++ objects and local profiles on 2026-10-09.
```

`ThirdParty/PvZTVTouch/NOTICE.md` records the source, the copyright and the
adaptation details; `ThirdParty/PvZTVTouch/LICENSE` holds the licence text.
Distributing a combined program with those parts means distributing under the
GPL.

Consequences for the work:

- Adapted code stays adapted code. Do not merge it into a MIT-only file
  without moving the header and updating the notice.
- The `libGameMain.so` used as the oracle comes from the same GPL project.
  The binary is not committed to this repository, and the repository ships no
  game assets.
- The game name, artwork, music and level data belong to PopCap / EA. The
  repository contains code and instructions, not assets.
