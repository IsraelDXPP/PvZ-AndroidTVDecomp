// CursorPreview -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class CursorPreview : public GameObject` (GameObject : SyncObject).
// The base occupies +0x00..+0x33, so the derived members start at +0x34.
// Total size 0x40, from the allocation site `operator new(0x40u)` at Board.cpp:18127;
// the authoritative header PvZ\Lawn\Board\CursorObject.h declares CursorPreview in the
// same file and agrees ("size = 16 ints").
//
// vtable order -- this is what the binary actually holds, and `virtual` must be
// declared in this order:
//   [  0] 0x146b20  CursorPreview::~CursorPreview
//   [  1] 0x146b40  CursorPreview::~CursorPreview
//   [  2] 0x1b7b80  GameObject::BeginDraw
//   [  3] 0x1b7af0  GameObject::EndDraw
//   [  4] 0x1b7b1c  GameObject::MakeParentGraphicsFrame
// Slots 2/3/4 point at GameObject's own implementations, so this class does not
// override them. Update/Draw are not virtual in the binary.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_CURSORPREVIEW_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_CURSORPREVIEW_H

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "BaseGamepadControls.h"
#include "GameObject.h"
#include "Graphics.h"
#include "ZenGarden.h"

// Board and LawnApp are not named in this file, so both includes were pure cycle
// (CursorPreview.h <-> Board.h and CursorPreview.h <-> LawnApp.h). Board.h in
// particular is a hub with fifteen mutual includes, and this edge was one of
// them. Sexy\ReconTypes.h already declares LawnApp.
class Board;
class LawnApp;

class CursorPreview : public GameObject
{
public:
    virtual ~CursorPreview();

    CursorPreview(int thePlayerIndex);

    unsigned int Update();
    int Draw(Sexy::Graphics *g);

    // Derived member area +0x34..+0x3F (12 bytes); the constructor registers the
    // whole block as one sync block.
    int mGridX; // +0x34
    int mGridY; // +0x38
    int mPlayerIndex; // +0x3C
};

// Layout check: 0x34 (GameObject) + 3 four-byte members.

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_CURSORPREVIEW_H
