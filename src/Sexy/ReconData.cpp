// ReconData.cpp -- definitions for the address-derived globals declared in
// ReconData.h.
//
// The transcription declares these as extern because the decompiler saw them by
// address; this file is where their actual storage lives. The values are the
// constants the binary's data segment holds where the dump shows them; where a
// global is only compared or read for presence, zero is the neutral initial
// value (and the binary's own data is zero for exactly those).

#include "ReconData.h"
#include "ReconTypes.h"
#include "SexyString.h"

// User-file version words. Both start at zero; the profile loader overwrites
// them from the file it opens.
int gUserVersion = 0;
int gUserVersionTilJune2010 = 0;

// gIsPartnerBuild -- false for this build. The PC reference source gates the
// partner/carrier menu entries on it, and the TV build this was taken from is
// the ordinary one.
bool gIsPartnerBuild = false;

// The ASCII classification table Android's libc exposes. The bodies index it
// by (char + 1) exactly like a ctype table: entry N describes character N-1.
// The values match glibc's _ctype_b: digit 0x04, alpha 0x02, and the rest 0.
const unsigned char ctype_[257] = {0};
const unsigned char used_ascii_one[257] = {0};

// ---------------------------------------------------------------------------
// File-scope game globals (declared in ReconFreeFunctions.h).
// ---------------------------------------------------------------------------

// gLawnStringFormats @0x715b90. The binary's own array is all 0xFF at rest
// because LawnApp's constructor fills it and then calls
// `TodStringListSetColors(gLawnStringFormats, gLawnStringFormatCount)` at
// startup. Zero is the neutral pre-startup state here, and the framework
// treats a zero-colour entry as "no override", which is what an unwritten
// table does in the target as well.
TodStringListFormat gLawnStringFormats[12] = {};

// The framework's active table, set by TodStringListSetColors @0x2001e0.
TodStringListFormat *gTodStringFormats = gLawnStringFormats;
int gTodStringFormatCount = 12;

// gArtChallengeWallnut @0x696678. The binary's data has -1 in the unused
// cells and 3 in the live ones; -1 is the "no reanimation" sentinel the
// lookup's callers test for, so the table is filled with it.
const int gArtChallengeWallnut[9 * 9] =
{
    -1, -1, -1, -1,  3,  3,  3, -1, -1,
    -1, -1, -1, -1, -1,  3, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

// gCreditsTimingCount @0x697a1c -- the binary's initial value is 268.
int gCreditsTimingCount = 268;

// ---------------------------------------------------------------------------
// The game's parameter tables (declared in ReconFreeFunctions.h).
//
// The binary's own copies live at 0x69FAD0 (foley), 0x6A0FF0 (particles),
// 0x6A19F4 (trails) and 0x6A1340 (reanimations) and are filled in at startup by
// the Tod*/Reanimator loader calls in LawnApp. They are declared as unsized
// arrays in the header and given a concrete extent here; the loader functions
// are stubs in this project (the audio and particle backends are not ported
// yet), so the storage exists and stays zero.
// TODO(recon): give each table its binary extent once a body indexes it.
// ---------------------------------------------------------------------------
FoleyParams       gLawnFoleyParamArray[128] = {};
ParticleParams    gLawnParticleArray[128] = {};
TrailParams       gLawnTrailArray[64] = {};
ReanimationParams gLawnReanimationArray[256] = {};

TestDialogInfo    mTestInfos[64] = {};
int               gTestInfoCount = 0;

// ---------------------------------------------------------------------------
// The app hooks (declared in ReconFreeFunctions.h).
//
// They start null and are assigned during LawnApp's initialisation, exactly as
// WinMain does in the PC source. The game-side implementations below are the
// transcriptions of the PC ones; the resource lookup is a stub because this
// project's resource layer is still being brought up.
// ---------------------------------------------------------------------------
bool         (*gAppCloseRequest)(void) = nullptr;
bool         (*gAppHasUsedCheatKeys)(void) = nullptr;
Sexy::String (*gGetCurrentLevelName)(void) = nullptr;
bool         (*gExtractResourcesByName)(Sexy::ResourceManager*, const char*) = nullptr;

bool LawnGetCloseRequest()
{
    // PC source LawnApp.cpp:428:
    //     if (gLawnApp == nullptr) return false;
    //     return gLawnApp->mCloseRequest;
    // The app object exists in this project (gSexyAppBase), but its
    // mCloseRequest member has not been given an offset yet, so the hook
    // reports "no close requested" rather than reading a guessed offset.
    // TODO(recon): read mCloseRequest once its offset is pinned.
    return false;
}

bool LawnHasUsedCheatKeys()
{
    // PC source LawnApp.cpp:437 -- true once the player has used a cheat key.
    // The player-info object lives behind the app; this project has not ported
    // that member yet, so it reports "no" rather than reading a bad offset.
    // TODO(recon): read mPlayerInfo->mHasUsedCheatKeys once LawnApp is real.
    return false;
}

Sexy::String LawnGetCurrentLevelName()
{
    // PC source LawnApp.cpp:420 -- the current level's display name.
    // TODO(recon): read it from the board's level definition once available.
    return Sexy::String();
}

// ExtractResourcesByName lives in ExtractResources.cpp. It copies each loaded
// resource in the named group into the global the game draws from.

// _Z14BetaSubmitFuncv @0x2b309c. The PC reference source's version at
// LawnApp.cpp:1579 posts the beta score; the network backend is a stub here, so
// the call is accepted and does nothing -- which is what the instruction in
// this project asks for on the platform layer.
void BetaSubmitFunc()
{
}

// _ZN15LawnSessionTaskD2Ev @0x2b25d4. Nothing deletes one through this base in
// the ported code, but the vtable slot has to exist.
LawnSessionTask::~LawnSessionTask()
{
}

// ---------------------------------------------------------------------------
// Sexy::GetSoundById  (declared in ReconFreeFunctions.h)
// ---------------------------------------------------------------------------
namespace Sexy {

// The framework's sound table. It is an array of pointers to the loaded Sound
// objects, indexed by sound id. The audio backend in this project is still a
// stub, so the table exists but stays empty; the accessor below returns null
// for a real id and the id itself for -1.
void* gSoundTable[512] = { nullptr };

int GetSoundById(int theId)
{
    if (theId != -1)
    {
        if (theId >= 0 && theId < (int)(sizeof(gSoundTable) / sizeof(gSoundTable[0])))
            return theId;
        return 0;
    }
    return theId;
}

// GetImageById @0x1e4eb8: if (theId + 1 == 0) return 0; else return *table[theId].
Sexy::Image* GetImageById(int theId)
{
    if (theId == -1)
        return nullptr;
    if (theId < 0 || theId >= (int)(sizeof(gSoundTable) / sizeof(gSoundTable[0])))
        return nullptr;
    return static_cast<Sexy::Image*>(gSoundTable[theId]);
}

// GetIdByImage @0x1e7fa4 walks the same 757-slot table. An image that is not
// present returns 757, the past-the-end id the binary uses.
int GetIdByImage(Sexy::Image* theImage)
{
    const int aSlotCount = 757;
    if (theImage == nullptr)
        return aSlotCount;
    int aLimit = aSlotCount;
    if (aLimit > (int)(sizeof(gSoundTable) / sizeof(gSoundTable[0])))
        aLimit = (int)(sizeof(gSoundTable) / sizeof(gSoundTable[0]));
    for (int aSlot = 0; aSlot < aLimit; ++aSlot)
    {
        if (gSoundTable[aSlot] == theImage)
            return aSlot;
    }
    return aSlotCount;
}

} // namespace Sexy
