// ReanimationButton -- reconstructed from libGameMain.so.
//
// Inheritance: `class ReanimationButton : public Sexy::ButtonWidget`.
// _ZTV17ReanimationButton @0x67xxx is _ZTVN4Sexy12ButtonWidgetE with only slots
// 0/1 (destructor), 29 (AddedToManager), 31 (Update) and 36 (Draw) replaced --
// slots 48, 71..80 still hold ButtonWidget's own implementations -- and the ctor
// at ReanimationButton.cpp:61 does `new ((Sexy::ButtonWidget *)a1) Sexy::ButtonWidget(a2, a4)`
// in place. (build/phase1-groups.md guessed `Sexy::WidgetContainer` from the
// header's vtable comment block; the binary and the ctor both say ButtonWidget.)
//
// SIZE 0x348 = ButtonWidget (0x330, 204 ints) + 0x18. Every field the ctor writes
// is at word 204 or above, and the last one is word 209 = +0x344.
//
// ---------------------------------------------------------------------------
// VTABLE, 137 slots in the dump, of which ReanimationButton owns:
//   [  0] 0x19d70c  ~ReanimationButton       override
//   [  1]           (ARM D0; MSVC x86 keeps ONE dtor slot)
//   [ 29] 0x19d7d4  AddedToManager           override
//   [ 31] 0x19d740  Update                   override
//   [ 36] 0x19d750  Draw                     override

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_REANIMATIONBUTTON_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_REANIMATIONBUTTON_H

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "ButtonWidget.h"
#include "Graphics.h"
#include "Image.h"
#include "Reanimation.h"
#include "WidgetContainer.h"

// LawnApp is reached only through `LawnApp *mApp` at +0x330, so it is declared
// rather than included: the include was half of the LawnApp.h <->
// ReanimationButton.h cycle. Sexy\ReconTypes.h already carries it.
class LawnApp;

class ReanimationButton : public Sexy::ButtonWidget
{
public:
    // ---- own members, +0x330.. (reconstructor at ReanimationButton.cpp:57) ----
    LawnApp *mApp; // +0x330  the ctor stores gSexyAppBase
    float mScaleX; // +0x334  initialised to 1.0f
    float mScaleY;
    ReanimationType mReanimationType;
    ReanimationID mReanimationID;
    // size 0x348

    virtual ~ReanimationButton(); // [  0]
    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager); // [ 29]
    virtual void Update(); // [ 31]
    virtual void Draw(Sexy::Graphics *theGraphics);

    // _ZN17ReanimationButtonC1Ei15ReanimationTypePN4Sexy14ButtonListenerEPNS1_6WidgetE
    // -- four parameters. The transcription's extra leading `Sexy::ButtonWidget *`
    // was the decompiler printing its own `this` register as a first argument.
    ReanimationButton(
        int theId, ReanimationType theReanimationType, Sexy::ButtonListener *theListener, Sexy::Widget *theParent);
    void SetScale(float theScaleX, float theScaleY);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_REANIMATIONBUTTON_H
