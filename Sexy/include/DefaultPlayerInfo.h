// DefaultPlayerInfo -- reconstructed from libGameMain.so.
//
// Inheritance: `class Sexy::DefaultPlayerInfo : public Sexy::PlayerInfo`.
// The authoritative header PvZ\Lawn\System\PlayerInfo.h declares it in
// PlayerInfo.h, and the vtable at 0x17D250 continues PlayerInfo's slot layout:
// slots 11/12/13 (0x17d1bc / 0x17d1c0 / 0x17d1c4) are still `Sexy::PlayerInfo::
// IsSaving / IsLoaded / IsLoading`, i.e. this class does not override them.
//
// The class adds NO data members -- the whole 516-int (0x810) payload is
// PlayerInfo's -- so its size is PlayerInfo's.
//
// ---------------------------------------------------------------------------
// VTABLE, 14 slots. This class's own entries:
//   [  0] 0x17d250  ~DefaultPlayerInfo       override
//   [  1]           (ARM D0; MSVC x86 keeps ONE dtor slot)
//   [  2] 0x17d1c8  GetId                    override
//   [  3] 0x17d1cc  GetProfileId             override
//   [  4] 0x17d1d0  GetName                  override
//   [  5] 0x2a9058  Reset                    override
//   [  6] 0x2a9060  SyncSummary              override
//   [  7] 0x2a905c  SyncDetails              override
//   [  8] 0x2a9094  DeleteUserFiles          override
//   [  9] 0x2a921c  LoadDetails              override
//   [ 10] 0x2a9404  SaveDetails              override
//   [ 11] 0x17d1bc  Sexy::PlayerInfo::IsSaving    inherited
//   [ 12] 0x17d1c0  Sexy::PlayerInfo::IsLoaded    inherited
//   [ 13] 0x17d1c4  Sexy::PlayerInfo::IsLoading   inherited

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Buffer.h"
#include "DataReader.h"
#include "DataSync.h"
#include "DataWriter.h"
#include "PlayerInfo.h"
#include "SexyAppBase.h"

namespace Sexy {

class DefaultPlayerInfo : public PlayerInfo
{
public:
    // ---- the PlayerInfo slots this class overrides, in binary order ----
    virtual ~DefaultPlayerInfo();                          // [ 0]
    virtual int GetId();                                   // [ 2]
    virtual int GetProfileId();                            // [ 3]
    virtual Sexy::String GetName();                        // [ 4]
    virtual void Reset();                                  // [ 5]
    virtual void SyncSummary(DataSync & theSync);          // [ 6]
    // The binary's symbol is _ZN4Sexy17DefaultPlayerInfo11SyncDetailsER8DataSynci
    // -- TWO parameters, matching PlayerInfo's pure virtual. The one-argument
    // spelling left the class abstract and could not override slot 7.
    virtual void SyncDetails(DataSync & theSync, int theVersion); // [ 7]
    virtual void DeleteUserFiles();                        // [ 8]
    virtual void LoadDetails();                            // [ 9]
    // Returns whether every userdata file was written, which is what
    // SettingsDialog::ButtonDepress stores into the dialog's save-failed flag.
    virtual bool SaveDetails();                            // [10]

    DefaultPlayerInfo(Sexy::ProfileMgr * theProfileMgr);
};

} // namespace Sexy

// The global spelling, which the transcribed bodies use.
using Sexy::DefaultPlayerInfo;
