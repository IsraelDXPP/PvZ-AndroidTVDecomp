// MessageWidget -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class MessageWidget : public SyncObject`
// (RTTI "13MessageWidget", primary base "10SyncObject" at offset 0,
// vtable 0x684068). The base occupies +0x00..+0x0F, so the derived members start
// at +0x10 -- exactly where the authoritative header
// PvZ\Lawn\Board\MessageWidget.h puts mApp (its "ordinal 4").
//
// Object size 0x330 = 204 ints, confirmed twice: the authoritative header ends
// with "size = 204 ints", and Board.cpp:18279 allocates the advice widget with a
// 0x330-byte block. The transcribed object was 0x340, sixteen bytes too big; its
// constructor's memset already stops at +0x330.
//
// vtable order -- only the two destructor slots, nothing is overridden:
//   [  0] 0x1a3b90  MessageWidget::~MessageWidget   (D1)
//   [  1] 0x1a3bd8  MessageWidget::~MessageWidget   (D0)
// MSVC x86 emits ONE slot for a virtual destructor where Itanium emits two.

#pragma once

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "Graphics.h"
#include "Reanimation.h"
#include "ReanimatorTransform.h"
#include "SexyAppBase.h"
#include "SyncObject.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// Board is not named in this file at all, and LawnApp is reached only through
// `LawnApp *mApp` / the constructors' pointer parameter, so both includes were
// pure cycle (MessageWidget.h <-> Board.h and MessageWidget.h <-> LawnApp.h).
// LawnApp.h -> Board.h -> MessageWidget.h was one of the shortest paths into
// LawnApp.h's cycle cluster. Sexy\ReconTypes.h already declares LawnApp.
class Board;
class LawnApp;

// Constants of the authoritative header; the transcription had neither.
inline constexpr int MAX_MESSAGE_LENGTH = 128;
inline constexpr int MAX_REANIM_LINES = 5;

class MessageWidget : public SyncObject
{
public:
    virtual ~MessageWidget();

    LawnApp       *mApp;                     // +0x10
    char           mLabel[MAX_MESSAGE_LENGTH]; // +0x14 ~ +0x93
    int            mDisplayTime;             // +0x94
    int            mDuration;                // +0x98
    MessageStyle   mMessageStyle;            // +0x9C
    // The array holds ReanimationID VALUES, not pointers: the constructor
    // zeroes all 128 words, SetLabel/LayoutReanimText store the result of
    // `mApp->ReanimationGetID(...)` into them, and the readers hand each word
    // straight back to `mApp->ReanimationTryToGet(ReanimationID)`. It was
    // declared `Reanimation *` in stage 1, which made every access need a cast.
    ReanimationID  mTextReanimID[MAX_MESSAGE_LENGTH]; // +0xA0 ~ +0x29F
    ReanimationType mReanimType;             // +0x2A0
    int            mSlideOffTime;            // +0x2A4  (the constructor writes 100)
    float          mFontScale;   // +0x2A8  1.0 normally; the huge-wave style uses 1.6 in zh_CN
    char           mLabelNext[MAX_MESSAGE_LENGTH]; // +0x2AC ~ +0x32B
    MessageStyle   mMessageStyleNext;        // +0x32C
    // Size 0x330 (204 ints).

    MessageWidget(LawnApp *theApp);

    Reanimation * ClearReanim();
    MessageWidget* ClearLabel();
    // The three below are void in the binary; the sibling class
    // SimpleMessageWidget.h declares the same three the same way.
    Sexy::Font * GetFont();
    bool IsBeingDisplayed();
    void LayoutReanimText();
    void SetLabel(Sexy::String const& theLabel, MessageStyle theStyle);
    void Update();
    void DrawReanimatedText(Sexy::Graphics *theGraphics, Sexy::Font *theFont, Color const& theColor, float thePos);
    void Draw(Sexy::Graphics *theGraphics);
};

// The authoritative header declares a second class in the same file. The
// transcription had omitted it entirely, although the IDA dump
// (libGameMain115.h:7890) and the recon tree both carry it, and the recon
// Board.cpp constructs it for `Board::mAdvice`.
//
// It adds exactly one member, `mIcon`, at +0x330, so the object is 0x334; and it
// overrides ClearLabel/SetLabel/Update/Draw, each of which clears mIcon first.
class CustomMessageWidget : public MessageWidget
{
public:
    virtual ~CustomMessageWidget();

    Sexy::Image *mIcon;   // +0x330
    // Size 0x334 (205 ints).

    CustomMessageWidget(LawnApp *theApp);

    void ClearLabel();
    void SetLabel(Sexy::String const& theLabel, MessageStyle theStyle);
    void Update();
    void Draw(Sexy::Graphics *theGraphics);
};

