// ProfileEventListener -- reconstructed from libGameMain.so.
//
// Sexy::ProfileMgr / Sexy::DefaultProfileMgr hand a profile change back through
// this interface. It is four bytes -- one vptr and no data -- so it does not
// shift any field of the class that implements it.
//
// The binary's own table (_ZTVN4Sexy20ProfileEventListenerE @0x687968) holds
// exactly FOUR slots, all `__cxa_pure_virtual`, and no destructor slot: a pure
// interface, which is how it is declared here.
//
// The four slots are named from the concrete table the compiler emitted for
// LawnApp (_off_tables.txt line 223, @0x687DF0):
//   [0] _ZThn2200_N7LawnApp17GetProfileVersionEv
//   [1] _ZThn2200_N7LawnApp16CreatePlayerInfoEv
//   [2] _ZThn2200_N7LawnApp20NotifyProfileChangedEPN4Sexy10PlayerInfoE
//   [3] _ZThn2200_N7LawnApp18OnProfileSaveErrorEPN4Sexy10PlayerInfoE
// The `2200` in every thunk is 0x898, i.e. this sub-object's offset inside
// LawnApp, and slot 1 is the one Sexy::ProfileMgr::CreatePlayerInfo() calls.
//
// TODO(recon): the authoritative header models this as a raw `void **vTable;`
// member (PvZ\SexyAppFramework\Widget\ProfileEventListener.h:25), which rule R4b
// forbids in this tree, so the four callbacks are real virtuals instead.
//
// This declaration used to live inline in Game\LawnApp.h, which asked for it to
// be moved here "once that file exists"; Sexy\ProfileMgr.cpp needs the complete
// type to dispatch through the listener, so the move is done and LawnApp.h now
// includes this header instead of keeping a second copy.

#pragma once

namespace Sexy {

class PlayerInfo;

class ProfileEventListener
{
public:
    virtual int GetProfileVersion() = 0;                              // [0]
    virtual PlayerInfo *CreatePlayerInfo() = 0;                       // [1]
    virtual void NotifyProfileChanged(PlayerInfo *thePlayerInfo) = 0; // [2]
    virtual void OnProfileSaveError(PlayerInfo *thePlayerInfo) = 0;   // [3]
};

} // namespace Sexy
