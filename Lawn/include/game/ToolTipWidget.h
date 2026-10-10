// ToolTipWidget -- reconstructed from libGameMain.so.
//
// The layout matches the authoritative header `PvZ\Lawn\Board\ToolTipWidget.h`
// field for field, and the object is **0x48** bytes.
// Allocation-site evidence: SeedBank.cpp:605, Board.cpp:18275,
// ChallengeScreen.cpp:1405, ImitaterDialog.cpp:127, SeedChooserScreen.cpp:4537
// and :4540 -- every one of them a 0x48-byte heap allocation of this type.
//
// The class has **no vtable** (neither side is polymorphic): the constructor
// stores `Sexy::gEmptyStringData` into +0/+4/+8, and that global is the data pointer of the
// shared empty string rep (dword_71BB54 + 12), not a vtable pointer.
//
// The first transcription kept the whole object as an opaque byte array; this
// header names all 16 members.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_TOOLTIPWIDGET_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_TOOLTIPWIDGET_H

#include "ReconTypes.h"
// The member types have to be complete, so these are included rather than
// forward-declared.
#include "Font.h"
#include "Graphics.h"
#include "SexyAppBase.h"
#include <string>
#include <vector>

class ToolTipWidget
{
public:
    Sexy::String mTitle; // +0x00
    Sexy::String mLabel; // +0x04
    Sexy::String mWarningText; // +0x08
    int mX; // +0x0C
    int mY; // +0x10
    int mWidth; // +0x14
    int mHeight; // +0x18
    bool mVisible; // +0x1C
    bool mCenter; // +0x1D
    int mMinLeft; // +0x20
    int mMaxBottom; // +0x24
    int mGetsLinesWidth; // +0x28
    int mWarningFlashCounter; // +0x2C
    Sexy::Font *mWarningTextFont; // +0x30
    Sexy::Font *mTitleFont; // +0x34
    Sexy::Color mTitleTextColor; // +0x38, 16 bytes
    // Total size 0x48.

    ToolTipWidget();
    void Update();
    void FlashWarning();
    void CalculateSize();
    void SetLabel(const Sexy::String &theLabel);
    void SetTitle(const Sexy::String &theTitle);
    void SetWarningText(const Sexy::String &theWarningText);
    void GetLines(std::vector<Sexy::String> &theLines);
    void Draw(Sexy::Graphics *theGraphics, bool theLocalCoordinates = false);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_TOOLTIPWIDGET_H
