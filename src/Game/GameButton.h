// GameButton -- the TV lawn button. It derives from DialogButton and adds the
// parent widget, the overlay image, the stone-button flag, the press-wobble
// state and the scale the Draw path pulses with.

#pragma once

#include "ReconTypes.h"
#include "ButtonWidget.h"
#include "DialogButton.h"
#include "Font.h"
#include "GamepadApp.h"
#include "Graphics.h"
#include "Image.h"
#include "Widget.h"

class GameButton : public Sexy::DialogButton
{
public:
    virtual ~GameButton();

    virtual void Update();
    virtual void Draw(Sexy::Graphics* theGraphics);
    virtual void SetDisabled(bool isDisabled);
    virtual void Resize(int theX, int theY, int theWidth, int theHeight);
    virtual void FocusDirectionHint(Sexy::WidgetFocusDir theDir);
    virtual int KeyDown(Sexy::KeyCode theKey);
    virtual void DrawButtonImage(Sexy::Graphics* theGraphics, Sexy::Image* theImage, const TRect<int>& theRect, int theX, int theY);
    virtual void SetFont(Sexy::Font* theFont);
    virtual void SetLabel(const Sexy::String& theLabel);
    virtual bool IsButtonDown();
    virtual void SetPadding(int theX, int theY);

    GameButton(int theId, Sexy::ButtonListener* theListener);

    bool IsMouseOver();
    int OnPressed();
    bool HaveButtonImage(Sexy::Image* theImage, const TRect<int>& theRect);

    Sexy::Widget* mParentWidget;
    // Last gamepad that pressed this button. -1 until KeyDown records one.
    int mGamepadIndex;
    // When set, Draw pins the graphics translation to the position Resize stored.
    bool mUseFixedOrigin;
    float mAnchorX;
    float mAnchorY;
    Sexy::Image* mOverOverlayImage;
    bool mDrawStoneButton;
    // Extra label offset, on top of the centred or justified position.
    int mTextOffsetX;
    int mTextOffsetY;
    // Written 0 by the constructor. Draw never reads them.
    int mButtonOffsetX;
    int mButtonOffsetY;
    // Scale Draw starts from (1, 1) and then adds the press pulse.
    float mScaleX;
    float mScaleY;
    // The scale Draw last handed to Graphics::SetScale.
    float mAppliedScaleX;
    float mAppliedScaleY;
    // Press-wobble amount, 0..1, animated over 15 frames.
    float mWobble;
    // Focus direction last hinted. -1 until FocusDirectionHint records one.
    int mFocusDirection;
    // Counts frames while the button is hot. Reset when mPulseTime reaches 15.
    int mPulseFrame;
    // Counts up to 15 while the button is highlighted, then restarts the pulse.
    int mPulseTime;
    // Sound id passed to GetSoundById. -1 plays the ceramic or grave default.
    int mPressSoundId;
};
