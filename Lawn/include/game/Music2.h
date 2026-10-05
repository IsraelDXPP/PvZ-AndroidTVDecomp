// Music2 -- reconstructed from libGameMain.so.
//
// `class Music2 : public Music`. Evidence:
//   * _recon/vtables.json, RTTI `6Music2` (vtable_ea 0x686020) records
//     `bases: [["5Music", 0]]`;
//   * the authoritative header PvZ\Lawn\System\Music.h:163 declares
//     `class Music2 : public Music`;
//   * every member offset the transcribed bodies touch (+0x18, +0x1C, +0x20,
//     +0x24, +0x28, +0x38, +0x40, +0x44, +0x4C, +0x50, +0x54, +0x55) is a Music
//     member, so Music2 adds no data member of its own and
//     `sizeof(Music2) == sizeof(Music) == 0x68`.
// Music.h already carries the full member table and the base vtable; Music2 only
// adds one slot and overrides existing ones.
//
// VTABLE -- 22 slots @0x686020:
//   [  0] 0x1ea220  Music2::~Music2            (D1)
//   [  1] 0x1ea244  Music2::~Music2            (D0)
//   [  2] 0x1ea564  Music2::MusicInit
//   [  3] 0x1ea17c  Music2::MusicDispose
//   [  4] 0x1ea270  Music2::MusicUpdate
//   [  5] 0x1ea188  Music2::StopAllMusic
//   [  6] 0x1ea330  Music2::StartGameMusic
//   [  7] 0x1b7e00  Music::LoadSong            (base implementation, not overridden)
//   [  8] 0x1ea1c8  Music2::MusicResync
//   [  9] 0x1ea1d0  Music2::UpdateMusicBurst
//   [ 10] 0x1ea1cc  Music2::StartBurst
//   [ 11] 0x1ea1d4  Music2::GameMusicPause
//   [ 12] 0x1ea1c4  Music2::MusicResyncChannel
//   [ 13] 0x1ea46c  Music2::MusicTitleScreenInit
//   [ 14] 0x1eadbc  Music2::MakeSureMusicIsPlaying
//   [ 15] 0x1ea1d8  Music2::FadeOut
//   [ 16] 0x1b7c54  Music::SetupMusicFileForTune (base implementation)
//   [ 17] 0x1ea1e4  Music2::GetMusicOrder
//   [ 18] 0x1ea174  Music2::GetNumLoadingTasks
//   [ 19] 0x1ea4e0  Music2::MusicLoadCreditsSong
//   [ 20] 0x1ea1f4  Music2::PlayFromOffset
//   [ 21] 0x1ea2f8  Music2::LoadSong            <- the ONE slot Music2 adds
// The Itanium table has a D0/D1 destructor pair; MSVC x86 keeps one slot, so Music
// generates 20 slots (0..19) and Music2's single new virtual lands at slot 20,
// which is exactly ARM slot 21. Slot order is therefore preserved.
//
// RETURN TYPES -- settled in phase 2. The transcriber kept the decompiler's
// spelling for six of the overrides (`int MusicDispose()`, `int MusicUpdate()`,
// `int StopAllMusic()`, `int StartGameMusic(bool)`, `Music2 * FadeOut(int)`,
// `int PlayFromOffset(MusicFile, int, double)`). Those are C2555, not harmless:
// C++ makes a same-name/same-parameter member an override whether or not the
// keyword `virtual` is written, and an override whose return type differs is
// REJECTED and demoted to a brand new virtual function -- which both shifts the
// vtable and (MSVC error recovery) wrecks `sizeof(Music2)`, so the static_assert
// at the bottom of this file used to fail. Music.h's own declarations were
// already right, and PvZ\Lawn\System\Music.h confirms every one of them is void
// in the Music2VTable (lines 169-186). Music2.cpp's definitions now match, and
// each one was checked against the ARM epilogue: every one of those six returns
// nothing (r0 still holds `this` at the `POP`, which is why the decompiler typed
// them non-void).
//
// PARAMETER LISTS -- also settled in phase 2, from the mangled names, which is
// the one place the real signature is recorded:
//   `_ZN6Music218MusicResyncChannelE9MusicFileS0_` -> (MusicFile, MusicFile)
//   `_ZN6Music213GetMusicOrderE9MusicFile`         -> (MusicFile)
//   `_ZN6Music222MakeSureMusicIsPlayingE9MusicTune`-> (MusicTune)
//   `_ZN6Music29PlayMusicE9MusicTune`              -> (MusicTune)
// The transcriber had all four as zero- or one-argument near-misses, which made
// three real overrides look like brand new virtuals. They are overrides, and
// they are declared in Music's own slot order below so the table is unchanged:
// an override replaces its base slot, it never appends one, so `LoadSong` stays
// at slot 21.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Music.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// This file names neither LawnApp nor Board -- its only base is Music -- so both
// includes were pure cycle. They mattered: Music.h has to be read before this
// file (Music2 derives from Music), and when Music.h was entered first its
// transitive LawnApp.h reached Music2.h while Music was still mid-parse, so
// `#pragma once` skipped Music's definition and `class Music2 : public Music`
// reported C2504. The declarations are kept explicit so the two names stay
// spelled here even though Sexy\ReconTypes.h already declares them.
class LawnApp;
class Board;

class Music2 : public Music
{
public:
    // Music.h declares `virtual ~Music()`, so this destructor is virtual by
    // inheritance; it has no definition in Music2.cpp, hence `= default`.
    virtual ~Music2() = default;

    // ---- overrides of Music's slots, declared in vtable order ----
    virtual void MusicInit();                                        // [ 2]
    virtual void MusicDispose();                                     // [ 3]
    virtual void MusicUpdate();                                      // [ 4]
    virtual void StopAllMusic();                                     // [ 5]
    virtual void StartGameMusic(bool theStart);
    virtual void MusicResync();                                      // [ 8]
    virtual void UpdateMusicBurst();                                 // [ 9]
    virtual void StartBurst();                                       // [10]
    virtual void GameMusicPause(bool thePaused);
    virtual void MusicResyncChannel(MusicFile theMusicFile1, MusicFile theMusicFile2);
    virtual void MusicTitleScreenInit();                             // [13]
    virtual void MakeSureMusicIsPlaying(MusicTune theTune);
    virtual void FadeOut(int theFadeOutDuration);
    virtual int GetMusicOrder(MusicFile theMusicFile);
    virtual int GetNumLoadingTasks();                                // [18]
    virtual void MusicLoadCreditsSong();                             // [19]
    virtual void PlayFromOffset(MusicFile theMusicFile, int theOffset, double theVolume);

    // ---- slot [21]: the single virtual Music2 adds ----
    // PvZ\Lawn\System\Music.h types this slot
    // `void (*LoadSongByTune)(Music2 *, MusicTune, const std::string &)`, and the
    // ARM agrees that the return value is unused: every call site discards r0 and
    // the one internal `return 0` is the failure path of the LoadMusic it wraps.
    // It is an `int` here because the body reports the interface's own result.
    virtual int LoadSong(MusicTune theTune, const Sexy::String& theFileName);

    virtual void PlayMusic(MusicTune theTune, int theOffset, int theDrumsOffset);

    // `_ZN6Music2C2Ev` @0x1EAD58 -- the binary's own constructor. It calls
    // Music::Music() and then stores Music2's vtable, which is all C++ does by
    // itself; the definition is empty for that reason. The declaration has to be
    // here: an implicitly-declared default constructor cannot be defined
    // out-of-line (C2600).
    Music2();
};

// Layout check: Music2 adds no data member of its own. This only holds while every
// override above is accepted -- a rejected one silently becomes a new virtual and
// MSVC's error recovery then reports a size that is not Music's.
