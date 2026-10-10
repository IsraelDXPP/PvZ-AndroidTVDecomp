// HitResult -- what Board::MouseHitTest fills in (authoritative Board.h:68).
//
// It lives in its own header rather than in Game\Board.h because
// BaseGamepadControls::GetHitResult returns one BY VALUE (ARM sret, the
// destination arrives in r0 -- see the note on the declaration), and a by-value
// return needs the complete type. Board.h includes BaseGamepadControls.h, so
// BaseGamepadControls.h cannot include Board.h back; this header is the way out
// of that cycle. It is a pure addition: the layout is unchanged and nothing that
// included Board.h loses anything, because Board.h includes this.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_HITRESULT_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_HITRESULT_H

#include "ReconTypes.h"

class HitResult
{
public:
    void *mObject; // +0x00
    GameObjectType mObjectType; // +0x04
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_HITRESULT_H
