# Verification

There is no test suite in this project: no `enable_testing()`, no `add_test()`,
no `ctest` target anywhere in the CMake files. The verification of a change is
therefore the build, a smoke run, and reading the diagnostics log. This file
says exactly what to run and what each result means.

## Build

Windows, from a **Developer PowerShell / Native Tools Command Prompt** (MSVC
and the Windows SDK are required):

```powershell
$env:VCPKG_ROOT = 'C:/vcpkg'
cmake --fresh --preset windows-x64-release
cmake --build --preset windows-x64-release --parallel
```

`--fresh` clears the cached dependency paths. vcpkg installs the
dependencies of `vcpkg.json` into `vcpkg_installed/<preset>`, so a preset
switch does not remove the dependencies of another platform.

macOS and Linux:

```sh
export VCPKG_ROOT="$HOME/vcpkg"
cmake --preset macos-arm64-release
cmake --build --preset macos-arm64-release --parallel
```

Android needs the NDK, and iOS needs Xcode:

```powershell
$env:ANDROID_NDK_HOME = 'C:/Android/Sdk/ndk/<installed-version>'
cmake --preset android-arm64-release
cmake --build --preset android-arm64-release --parallel
```

```sh
export VCPKG_ROOT="$HOME/vcpkg"
cmake --preset ios-arm64-release
cmake --build --preset ios-arm64-release --parallel
```

The preset list is in `CMakePresets.json`; every platform prefix has a
`-debug` and a `-release` variant.

The build produces no assets. A missing asset never breaks the build; it
breaks the run, and it breaks it at run time, not at compile time.

## Smoke test

From the output directory (`build/windows-x64-release/bin/Release`):

```powershell
.\LawnTV.exe --frames 5 --screenshot smoke.png --headless
```

Expected, and measured on 2026-10-21:

```text
exit code: 0
screenshot: wrote (1280x720) -> smoke.png
Lawn: exited after 5 frame(s)
```

The run loads the resources, builds the title screen, renders five frames and
exits. A longer run reaches the menu and the gameplay:

```powershell
.\LawnTV.exe --frames 600 --screenshot menu.png --headless
```

Useful flags: `--assets <dir>` selects another resource root, `--width` /
`--height` set the window, `--headless` hides the window while keeping GPU
rendering, `--screenshot <file>` writes the last frame and exits.

### The diagnostics log

`LawnTV-diagnostics.log` is written next to the executable. One line per
session, plus one line per crash:

```text
session_start code=0 phase=hardware_init mode=-1 level=-1 scene=-1 frame=0 ...
fatal_signal code=11 phase=game_init mode=-1 level=-1 scene=-1 frame=0 ...
```

`fatal_signal` and `fatal_exception` mean the run died: `code=11` is a
segfault, `code=3221225477` is an access violation. A clean run ends with
`session_start` and no fatal line. When a run crashes, the log is the first
thing to read and the first thing to quote.

## What counts as verified

| Claim | What it requires |
| --- | --- |
| "It compiles" | The build command above, on the platform that matters, with no error |
| "It runs" | The smoke test, exit code 0, a PNG written, no fatal line in the log |
| "It works" | The same, plus the behaviour exercised by hand and described |
| "It matches the binary" | The comparison of `docs/metodo.md`, against the v115 build, with the address |

Never report a weaker claim than the one you actually measured. If the build
was not run, say the build was not run.

## Checking a change by hand

With `-tod` (or `--debug-keys`) the debug shortcuts are enabled, in Debug and
in Release:

- `u` at the main menu marks Adventure completed, unlocks the game modes and
  all twelve achievements, and saves the profile.
- `l` during gameplay opens the level-skip dialog.
- `F12` saves the offscreen buffer to `screenshots/shot_NNNN.png`.

The twelve achievements and their conditions are audited in
`docs/achievement-conditions.md`. The shop and the house scene are the parts
with the most hand-written code, so they are the first to try by hand.

## Runtime configuration

Gameplay options live in the asset root's `setup.env`, and the file must be
outside the `.pak` archives. `setup.env.example` documents every option;
`README.md` explains the ones that change the game (`LAWN_LOW_VIEW`,
`LAWN_AUTO_COLLECT`, `LAWN_HIDE_COVER_LAYER`, `LAWN_MUSIC2`, `LAWN_NO_ATLAS`,
`LAWN_LANGUAGE`).

Changing `setup.env` requires a restart: the settings are read once.

## Before committing

```powershell
cmake --fresh --preset windows-x64-release
cmake --build --preset windows-x64-release --parallel
cd build\windows-x64-release\bin\Release
.\LawnTV.exe --frames 5 --screenshot smoke.png --headless
Get-Content LawnTV-diagnostics.log -Tail 4
```

Record the exact output in the commit message or in `docs/estado.md`. If the
run crashed, the commit says so, with the fatal line.
