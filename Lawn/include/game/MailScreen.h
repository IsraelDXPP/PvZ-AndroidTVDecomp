// MailScreen -- reconstructed from libGameMain.so.
//
// Inheritance: `class MailScreen : public LawnDialog`. The authoritative header
// says so, _ZTV10MailScreen @0x680xxx repeats LawnDialog's 133 primary slots and
// adds none, and the body at MailScreen.cpp:831 uses +0x2E0 as a `LawnApp *`,
// which is exactly LawnDialog::mApp.
//
// SIZE 0x33C -- three independent proofs, and NOT the transcriber's 0x348:
//   * the largest own offset the bodies reach is +0x338 plus one word = 0x33C;
//   * MainMenu.cpp:304 and :2562 both allocate `operator new(0x33Cu)`;
//   * the authoritative header closes with "115: 207" = 207 ints = 0x33C.
//
// ---------------------------------------------------------------------------
// VTABLE @0x680xxx, 133 primary slots, all inherited except:
//   [  0] 0x1e8cc8  ~MailScreen              override
//   [  1]           (ARM D0; MSVC x86 keeps ONE dtor slot)
//   [ 29] 0x1e9fb0  AddedToManager           override
//   [ 30] 0x1ea154  RemovedFromManager       override
//   [ 31] 0x1e9640  Update                   override
//   [ 36] 0x1e8d58  Draw                     override
//   [ 71] 0x1e97e8  KeyDown(KeyCode)         override
// Slots [76]..[79] (MouseDown/MouseUp) still hold Sexy::Dialog's implementations
// in this .so, so this class does not override them and they are not redeclared,
// even though the authoritative header declares them (that header models the hook
// project, where the hooks are installed on Dialog's slots).

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Font.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "LawnDialog.h"
#include "LawnPlayerInfo.h"
#include "Mailbox.h"
#include "Widget.h"
#include <vector>

// LawnApp is only the constructor's pointer parameter (`MailScreen(LawnApp *)`),
// so it is declared rather than included: the include was half of the
// LawnApp.h <-> MailScreen.h cycle, which is what made this header report
// C2504 at the `class MailScreen : public LawnDialog` line. Sexy\ReconTypes.h
// already carries the declaration.
class LawnApp;

class MailScreen : public LawnDialog
{
public:
    // The element type of the two vector<MessageDrawInfo> lists. 16 bytes.
    struct MessageDrawInfo
    {
        float mAnimTime;    // +0x00  animation timer
        int mAnimState;     // +0x04  0 left out, 1 left in, 2 shift left, 3 shift right, 4 static
        int mMessageIndex;  // +0x08  mailbox message index
        float mDrawXOffset; // +0x0C  draw X offset
    };

    // ---- members, absolute offsets (authoritative ordinals 191..206 * 4) ----
    // mInited is a single byte in the bodies (MailScreen.cpp:274 clears it, :551
    // sets it); the authoritative header packs it into the int slot, which is
    // what the offsets require.
    int mInited;                // +0x2FC
    int mCurrentListPos;        // +0x300
    int mCurrentMessageIndex;   // +0x304
    int mPage;                  // +0x308  0 = inbox/unread, 1 = read
    // Unread inbox and the already-read page. Draw walks the active page.
    std::vector<MessageDrawInfo> mUnreadList;
    std::vector<MessageDrawInfo> mReadList;
    int mNumUnread;             // +0x324
    int mNumRead;               // +0x328
    float mRightHintAnimTime;   // +0x32C
    float mLeftHintAnimTime;    // +0x330
    float mRightHintAnimValue;  // +0x334
    float mLeftHintAnimValue;   // +0x338
    // size 0x33C

    // ---- the LawnDialog/Dialog slots this class overrides, in binary order ----
    virtual ~MailScreen();                                                     // [  0]
    virtual void AddedToManager(Sexy::WidgetManager * theWidgetManager);       // [ 29]
    virtual void RemovedFromManager(Sexy::WidgetManager * theWidgetManager);   // [ 30]
    virtual void Update();                                                    // [ 31]
    virtual void Draw(Sexy::Graphics * g);                                    // [ 36]
    virtual int KeyDown(Sexy::KeyCode theKey);                               // [ 71]

    MailScreen(LawnApp * theApp);
    MailScreen * SetCurrentMessageIndex(int theIndex);
    // Both return nothing: the value the dump returns is whichever of `this`,
    // a MessageDrawInfo* or the loop-end pointer the register happened to hold,
    // and every call site discards it.
    void MessagesShiftLeft();
    void MessagesShiftRight();
    int InitMessages();
    void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags);
};

