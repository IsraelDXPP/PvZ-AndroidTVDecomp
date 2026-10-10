// Reconstructed from libGameMain.so @0x1E8CC8-0x1EA16C.
#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_MAILSCREEN_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_MAILSCREEN_H
#include "LawnDialog.h"
#include <vector>

class LawnApp;

class MailScreen : public LawnDialog
{
public:
    enum MailPage
    {
        MAIL_PAGE_INBOX,
        MAIL_PAGE_READ
    };
    enum MessageAnimation
    {
        MESSAGE_SLIDE_OUT,
        MESSAGE_SLIDE_IN,
        MESSAGE_SHIFT_LEFT,
        MESSAGE_SHIFT_RIGHT,
        MESSAGE_STATIC
    };
    struct MessageDrawInfo
    {
        float mAnimTime;
        MessageAnimation mAnimState;
        int mMessageIndex;
        float mDrawXOffset;
    };

    bool mInited;
    int mCurrentListPos;
    int mCurrentMessageIndex;
    MailPage mPage;
    std::vector<MessageDrawInfo> mUnreadList;
    std::vector<MessageDrawInfo> mReadList;
    int mNumUnread;
    int mNumRead;
    float mRightHintAnimTime;
    float mLeftHintAnimTime;
    float mRightHintAnimValue;
    float mLeftHintAnimValue;

    explicit MailScreen(LawnApp *theApp);
    ~MailScreen() override;
    void AddedToManager(Sexy::WidgetManager *theWidgetManager) override;
    void RemovedFromManager(Sexy::WidgetManager *theWidgetManager) override;
    void Update() override;
    void Draw(Sexy::Graphics *g) override;
    int KeyDown(Sexy::KeyCode theKey) override;
    void MouseDown(int x, int y, int theClickCount) override;
    void MouseUp(int x, int y, int theClickCount) override;
    void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags);
    void SetCurrentMessageIndex(int theIndex);
    void MessagesShiftLeft();
    void MessagesShiftRight();
    void InitMessages();

private:
    int mMouseDownX = 0;
    bool mMousePressed = false;
    std::vector<MessageDrawInfo> &CurrentMessages();
    void UpdateHelpButtons();
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_MAILSCREEN_H
