# The binary

Everything in this port is recovered from one file: `libGameMain.so`, the
ARM32 Thumb shared library of the Plants vs. Zombies TV / Android edition.
The library is **not stripped**: it ships with 15,509 defined dynamic symbols
in the v115 build, including C++ mangled names, so functions can be found by
name and not only by address.

## The two builds

The AndroidTV project carries two builds of the same library. They are
different builds, not different versions of the file: the layouts differ and
the same function sits at a different address in each.

| Build | Size | MD5 | `LawnApp::HasSeedType` |
| --- | --- | --- | --- |
| **v115** | 6,936,968 bytes | `FE7E5801B6B0B6812CD40FFCBA5E6959` | `0x2B756C` |
| w111 | 6,923,240 bytes | `32E6B970FFF4BDB63AD1A14EB3B227FA` | `0x2B49A8` |

**The addresses quoted in this repository are v115 addresses.** That build is
the one to use as the oracle:

```text
https://github.com/ZombieYetis/PlantsVsZombies-AndroidTV
  app/src/v115/jniLibs/armeabi-v7a/libGameMain.so
```

The AndroidTV project is GPL-3.0-or-later and is the source of the console
content restored in this fork. Its `libGameMain.so` is the only binary this
port follows.

## Fingerprint

If you need to know which build a `.so` is, ask the symbol table. These nine
functions were checked against the v115 build and against the addresses
quoted in the code:

| Function | Address | Size (bytes) |
| --- | --- | --- |
| `LawnApp::SaveFileExists` | `0x2B33B8` | 136 (the banner in `LawnApp.cpp` says 112) |
| `LawnApp::IsIceDemo` | `0x2B3440` | 4 |
| `LawnApp::TransitionToHouseScreen` | `0x2B3444` | 2 |
| `LawnApp::ShowLeaderboardScreen` | `0x2B3448` | 2 |
| `LawnApp::ShowHouseScreen` | `0x2B344C` | 2 |
| `LawnApp::KillHouseScreenFromGame` | `0x2B3450` | 2 |
| `LawnApp::HasSeedType` | `0x2B756C` | 238 |
| `Board::SurvivalSaveScore` | `0x15AF08` | 122 |
| `VSResultsMenu::InitFromBoard` | `0x1916FC` | 664 |
| `Plant::DoSpecial` | `0x16DDB8` | 804 |
| `Zombie::DieNoLoot` | `0x1B1660` | 344 |
| `Challenge::TreeOfWisdomGrow` | `0x1853D4` | 188 |

`ShowHouseScreen` is two bytes because the TV build stubs the console house
screen out. The run of two-byte functions around it is the fingerprint.

### Reading the symbol table

```sh
nm -C -D --defined-only -S libGameMain.so | grep -E 'ShowHouseScreen|HasSeedType'
```

```text
002b344d 00000002 T LawnApp::ShowHouseScreen()
002b756d 000000ee T LawnApp::HasSeedType(SeedType, bool)
```

The value is the symbol address **plus one**: the low bit is the Thumb bit.
`0x2b344d` means the function starts at `0x2B344C`, and `0xee` is 238 bytes.
The same works with `readelf -sW` on a toolchain without `nm`.

`nm -C -D --defined-only -S` also gives the demangled name, which is how the
`Class::Method(Args)` banner in the code is written.

## The local Hex-Rays exports

Two Hex-Rays exports of this library exist on this machine. Neither is the
v115 build, so **their addresses do not match the quotes in the code**:

| File | Size | Build | `LawnApp::HasSeedType` |
| --- | --- | --- | --- |
| `C:\Users\nydil\Documents\libGameMain.so.c` | 32,947,311 bytes | w111 | `0x2B49A8` |
| `C:\Users\nydil\Documents\antigravity\optimistic-lovelace\pseudocode\libGameMain.so.c` | 25,619,041 bytes | a third build | `0x1AC0BC` |

The first one is the freshest (written on 2026-10-21) and the most complete:
13,525 function blocks. The IDA database of the library is next to the second
one (`libGameMain.so.i64`, plus the `.id0` / `.id1` / `.id2` / `.nam` / `.til`
components of the session).

Use the exports to read the *logic*. Use the v115 symbol table to fix the
*address*. When they disagree about the address, the export is from another
build and the code is not wrong.

### Reading the export

The export is the output of Hex-Rays 9.3.0.260213. Each function is
introduced by a header with its address:

```c
//----- (000E9440) --------------------------------------------------------
int __fastcall AlmanacDialog::GetSeedPosition(int a1, int a2, int *a3, _DWORD *a4)
```

Reading rules:

- `a1` is `this`. `*(LawnApp **)(a1 + 776)` is the field at offset 776, that
  is `*((LawnApp **)this + 194)`. The port has named fields: use the field.
- `__fastcall`, `_DWORD`, `_BOOL4`, `JUMPOUT(0)`, `// idb` and
  `// Alternative name is '_ZN...'` are decompiler artefacts, not code.
- `sub_XXXXXX` and `dword_XXXXXX` are not names. The real name is in the
  symbol table.
- Wide strings come out as `dword_XXXXXX` pointers: read them from the
  binary, never guess them.
- `LODWORD` and `COERCE_FLOAT` are the same value mistyped. Check the asm.
- Unsigned comparisons: `(unsigned int)(x - 19) <= 1` is
  `x == 19 || x == 20`. Keep that shape in the port.

## Tools

- **IDA Pro 9.3** with Hex-Rays is the main tool. The local install is at
  `C:\Program Files\IDA Professional 9.3`.
- **binutils** (`nm`, `readelf`, `objdump` from
  `C:\ProgramData\mingw64\mingw64\bin`) read the symbol table of any `.so` and
  settle every address question.
- The Hex-Rays export is the searchable copy of the whole library, 800k lines
  of pseudo-C. Grep it by function name, never by address.
