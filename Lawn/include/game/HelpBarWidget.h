// HelpBarWidget -- reconstructed from libGameMain.so.
//
// `class HelpBarWidget : public Sexy::Widget`.
//   RTTI: `_ZTI13HelpBarWidget` @0x67F5C0 records `_ZTIN4Sexy6WidgetE` @0x68B958
//   as its base, and the authoritative PvZ\Lawn\Widget\HelpBarWidget.h says
//   `class HelpBarWidget : public Sexy::Widget` as well.
//
// SIZE 0x12C, NOT 0x330.
//   The authoritative header only recovers `char mUnk[40]; // 65 ~ 74`, i.e. the
//   object ends around +0x12C, so this is one of the classes map-lawn.md flags as
//   `this`-extent inflated. The 0x12C is confirmed word by word from
//   HelpBarWidget.cpp, which names every field the authoritative header leaves
//   opaque:
//     +0x100 float          the help bar's fade alpha (raised by 0.02 per frame,
//                           capped at 1.0, and passed to DrawButtons)
//     +0x104 vector<ButtonDef>  first button array (begin/end/capacity)
//     +0x110 vector<ButtonDef>  second button array
//     +0x11C bool           the constructor sets it to 1; HelpTextScreen.cpp:188
//                           and ImitaterDialog.cpp:174 clear it from outside
//     +0x120 int            font id (FONT_HOUSEOFTERROR20 / FONT_HOUSEOFTERROR16)
//     +0x124 int            state; both the constructor and Update write -1
//     +0x128 int            animation counter, 0..10 (TodAnimateCurve(0, 10, ...))
//
// VTABLE -- 122 slots @0x67F5D0; only four belong to this class, the rest are
// WidgetContainer's and Widget's, so they are inherited and not redeclared:
//   [  0] 0x146f8c  ~HelpBarWidget (deleting)
//   [ 29] 0x149048  AddedToManager
//   [ 30] 0x14904c  RemovedFromManager
//   [ 31] 0x148f10  Update
//   [ 36] 0x148608  Draw
// The Itanium table has a D0/D1 destructor pair; MSVC x86 keeps one slot, so only
// one `virtual ~HelpBarWidget()` is declared and the relative order is unchanged.
// NOTE: the transcribed .cpp spells Draw and the two manager hooks with `int`
// returns; the base class declares `void`, which is what an override needs, so the
// declarations below use void. Phase 2 aligns the bodies.

#pragma once

#include "ReconTypes.h"
// The bodies call methods on these complete types.
#include "GamepadApp.h"
#include "Graphics.h"
#include "SexyAppBase.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include <memory>
#include <vector>

class HelpBarWidget : public Sexy::Widget
{
public:
    // HelpBarWidget::HelpAlign -- a class-scope enum in the binary
    // (`HelpBarWidget::HelpAlign` is the mangled parameter type of AddButton).
    // Alignment values are recovered from AddButton and DrawButtons.
    enum HelpAlign
    {
        HELP_ALIGN_NONE = -1,
        HELP_ALIGN_LEFT = 0,
        HELP_ALIGN_RIGHT = 1,
    };

    virtual ~HelpBarWidget();

    // slot 29
    virtual void AddedToManager(Sexy::WidgetManager * theWidgetManager);
    // slot 30
    virtual void RemovedFromManager(Sexy::WidgetManager * theWidgetManager);
    // slot 31
    virtual void Update();
    // slot 36
    virtual void Draw(Sexy::Graphics * theGraphics);
    bool IsPointVisible(int x, int y) override;
    void MouseDown(int x, int y, int theClickCount) override;
    void MouseUp(int x, int y, int theClickCount) override;

    // One help-button entry, including its column, row and animation state.
    struct ButtonDef
    {
        Sexy::GamepadButton mButton;
        Sexy::String mLabel;
        HelpAlign mAlign;
        bool mAnimating;
        int mRow;
    };

    // Initialized as two float points by the original startup @0x12F838.
    static Sexy::TPoint<float> DEFAULT_POS;
    static const Sexy::TPoint<float> OFFSCREEN_POS;

    // ---- members, absolute offsets (base Sexy::Widget = 0x100) ----
    // Both button arrays are libstdc++ vectors of ButtonDef: the binary inserts
    // through std::vector<ButtonDef>::_M_insert_aux, which reserves three words
    // at each offset -- the same size the opaquely-typed int[3] pairs had.
    float mAlpha;
    std::vector<ButtonDef> mButtons;
    std::vector<ButtonDef> mButtons2;
    bool mDrawShade;
    Sexy::Font *mFontId;         // +0x120
    int mState;                  // +0x124
    int mAnimCounter;            // +0x128
    std::unique_ptr<Sexy::Font> mOutlineFont;
    Sexy::GamepadButton mMousePressedButton = GAMEPAD_BUTTON_NONE;

    // ---- methods that are not vtable slots (kept from the transcription) ----
    // Button renderer @0x1470BC: incoming/outgoing set and fade alpha.
    void DrawButtons(Sexy::Graphics* theGraphics, std::vector<ButtonDef>& theButtons, bool theIncoming, float theAlpha);
    HelpBarWidget();
    void AddButton(Sexy::GamepadButton theButton, Sexy::String theLabel, HelpAlign theAlign);
    void ClearButtons(unsigned int theFirstKept);
    Sexy::GamepadButton HitTestButton(int x, int y) const;
};
