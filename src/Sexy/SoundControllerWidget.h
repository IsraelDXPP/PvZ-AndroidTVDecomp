// SoundControllerWidget is the volume overlay the common app owns.
//
// It is a Widget plus four state images, the level, the drawn state, the show
// flag and the tick the overlay was last touched. The overlay stays up for two
// seconds after the last volume key.

#pragma once

#include "Widget.h"

namespace Sexy {

class SoundControllerWidget : public Widget
{
public:
    enum State
    {
        STATE_OPEN = 0,
        STATE_CLOSED = 1,
        STATE_RAISED = 2,
        STATE_LOWERED = 3,
    };

    SoundControllerWidget();
    virtual ~SoundControllerWidget();

    virtual void Draw(Sexy::Graphics* g);
    virtual void Update();
    virtual int KeyUp(Sexy::KeyCode theKey);

    void openSound();
    void closeSound();
    int setSoundSize(int theSize);
    void addSoundSize();
    void reduceSoundSize();
    void setSoundStatus(int theState);
    SoundControllerWidget* setSoundControllerImage(Sexy::Image* theOpen,
                                                   Sexy::Image* theClosed,
                                                   Sexy::Image* theRaised,
                                                   Sexy::Image* theLowered);

    Sexy::Image* mOpenImage;     // +0x100
    Sexy::Image* mClosedImage;   // +0x104
    Sexy::Image* mRaisedImage;   // +0x108
    Sexy::Image* mLoweredImage;  // +0x10C
    int mSoundSize;              // +0x110
    int mState;                  // +0x114
    bool mShowing;               // +0x118
    unsigned int mShownTick;     // +0x11C
};

} // namespace Sexy
