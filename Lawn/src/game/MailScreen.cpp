#include "MailScreen.h"
#include "Font.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "Image.h"
#include "LawnApp.h"
#include "LawnPlayerInfo.h"
#include "Mailbox.h"
#include "WidgetManager.h"
#include <algorithm>

MailScreen::MailScreen(LawnApp* theApp)
    : LawnDialog(theApp, nullptr, 60, true, Sexy::String(), Sexy::String(), Sexy::String(), 0),
      mInited(false), mCurrentListPos(0), mCurrentMessageIndex(-1), mPage(MAIL_PAGE_INBOX),
      mNumUnread(0), mNumRead(0), mRightHintAnimTime(0.0f), mLeftHintAnimTime(0.0f),
      mRightHintAnimValue(0.0f), mLeftHintAnimValue(0.0f)
{
    mClip = false;
    Resize(0, 0, 800, 600);
}

// PvZ TV Touch Team's swipe/page gesture, kept local to this dialog.
void MailScreen::MouseDown(int x, int y, int theClickCount)
{
    mMousePressed = theClickCount > 0 && theClickCount != 3;
    mMouseDownX = x;
}

void MailScreen::MouseUp(int x, int y, int theClickCount)
{
    const bool pressed = mMousePressed;
    mMousePressed = false;
    if (!pressed || theClickCount <= 0 || theClickCount == 3
        || x < 0 || x >= mWidth || y < 0 || y >= mHeight)
        return;
    const int distance = x - mMouseDownX;
    KeyDown(distance > 20 || (distance >= -20 && mMouseDownX < 400)
        ? Sexy::KEYCODE_LEFT : Sexy::KEYCODE_RIGHT);
}

MailScreen::~MailScreen()
{
    mApp->mMailBox->ReleaseMessageImages();
}

std::vector<MailScreen::MessageDrawInfo>& MailScreen::CurrentMessages()
{
    return mPage == MAIL_PAGE_READ ? mReadList : mUnreadList;
}

// Draw @0x1E8D58: background offsets are absolute; message offsets are relative.
void MailScreen::Draw(Sexy::Graphics* g)
{
    g->DrawImage(IMAGE_MAILBOX_TABLE, -240, -60);
    Sexy::String title = TodStringTranslate("[MAILBOX]");
    g->SetColor(Sexy::Color::White);
    g->SetFont(FONT_HOUSEOFTERROR28);
    g->DrawString(title, 400 - FONT_HOUSEOFTERROR28->StringWidth(title) / 2, 50);
    Sexy::String pageName = TodStringTranslate(mPage == MAIL_PAGE_READ ? "[READ_MESSAGES]" : "[INBOX]");
    g->SetFont(FONT_HOUSEOFTERROR20);
    g->DrawString(pageName, 400 - FONT_HOUSEOFTERROR20->StringWidth(pageName) / 2, 100);

    const std::vector<MessageDrawInfo>& messages = CurrentMessages();
    if (messages.empty())
    {
        Sexy::String emptyText = TodStringTranslate(mPage == MAIL_PAGE_READ ? "[NO_READ_MESSAGES]" : "[MAILBOX_EMPTY]");
        g->SetFont(FONT_HOUSEOFTERROR28);
        g->DrawString(emptyText, 400 - FONT_HOUSEOFTERROR28->StringWidth(emptyText) / 2,
                      300 - FONT_HOUSEOFTERROR28->GetAscent());
    }
    else
    {
        for (auto entry = messages.rbegin(); entry != messages.rend(); ++entry)
        {
            Mailbox::MailMessage* message = mApp->mMailBox->GetMessageByIndex(entry->mMessageIndex, true);
            int backgroundX = static_cast<int>(static_cast<int>(entry->mDrawXOffset) + message->mBackgroundOffsetX);
            int backgroundY = static_cast<int>(message->mBackgroundOffsetY);
            Sexy::Image* background = message->mBackgroundImage;
            if (background != nullptr)
                g->DrawImage(background, backgroundX, backgroundY);
            Sexy::Image* body = message->mMessageImage;
            if (body != nullptr)
                g->DrawImage(body, backgroundX + static_cast<int>(message->mMessageOffsetX),
                              backgroundY + static_cast<int>(message->mMessageOffsetY));
        }
    }
    g->DrawImage(IMAGE_MAILBOX_TABLE_LEAVESSHADOWS, -240, -60);
}

void MailScreen::SetCurrentMessageIndex(int theIndex)
{
    const std::vector<MessageDrawInfo>& messages = CurrentMessages();
    if (messages.empty())
    {
        mCurrentListPos = 0;
        mCurrentMessageIndex = -1;
        return;
    }
    mCurrentListPos = std::clamp(theIndex, 0, static_cast<int>(messages.size()) - 1);
    mCurrentMessageIndex = messages[mCurrentListPos].mMessageIndex;
}

void MailScreen::MessagesShiftLeft()
{
    std::vector<MessageDrawInfo>& messages = CurrentMessages();
    if (messages.empty())
        return;
    if (mCurrentListPos > 0)
    {
        messages[mCurrentListPos - 1].mAnimTime = 0.0f;
        messages[mCurrentListPos - 1].mAnimState = MESSAGE_SLIDE_OUT;
    }
    int offset = 15;
    for (int index = mCurrentListPos; index < static_cast<int>(messages.size()); ++index)
    {
        MessageDrawInfo& entry = messages[index];
        if (entry.mAnimTime < 0.5f)
            entry.mDrawXOffset = static_cast<float>(offset);
        entry.mAnimState = MESSAGE_SHIFT_LEFT;
        entry.mAnimTime = 0.0f;
        offset += 15;
    }
}

void MailScreen::MessagesShiftRight()
{
    std::vector<MessageDrawInfo>& messages = CurrentMessages();
    if (messages.empty())
        return;
    messages[mCurrentListPos].mAnimTime = 0.0f;
    messages[mCurrentListPos].mAnimState = MESSAGE_SLIDE_IN;
    int offset = 0;
    for (int index = mCurrentListPos + 1; index < static_cast<int>(messages.size()); ++index)
    {
        MessageDrawInfo& entry = messages[index];
        if (entry.mAnimTime != 0.0f)
            entry.mDrawXOffset = static_cast<float>(offset);
        entry.mAnimState = MESSAGE_SHIFT_RIGHT;
        entry.mAnimTime = 0.0f;
        offset += 15;
    }
}

void MailScreen::InitMessages()
{
    mUnreadList.clear();
    mReadList.clear();
    LawnPlayerInfo* player = mApp->mPlayerInfo;
    if (player != nullptr)
    {
        bool completedAdventure = player->GetFlag(PLAYER_FLAG_ADVENTURE_COMPLETE);
        for (int index = 0; index < mApp->mMailBox->GetNumUnreadMessages(); ++index)
        {
            int messageId = mApp->mMailBox->GetUnreadMessage(index);
            Mailbox::MailMessage* message = mApp->mMailBox->GetMessageByIndex(messageId, false);
            if (message->mIsValid && (completedAdventure || message->mMinLevel <= player->mLevel))
            {
                mApp->mMailBox->GetMessageByIndex(messageId, true);
                mApp->mMailBox->MarkMessageSeen(messageId);
                mUnreadList.push_back({0.0f, MESSAGE_STATIC, messageId, 15.0f * mUnreadList.size()});
            }
        }
        for (int index = 0; index < mApp->mMailBox->GetNumReadMessages(); ++index)
        {
            int messageId = mApp->mMailBox->GetReadMessage(index);
            Mailbox::MailMessage* message = mApp->mMailBox->GetMessageByIndex(messageId, false);
            if (message->mIsValid && (completedAdventure || message->mMinLevel <= player->mLevel))
            {
                mApp->mMailBox->GetMessageByIndex(messageId, true);
                mReadList.push_back({0.0f, MESSAGE_STATIC, messageId, 15.0f * mReadList.size()});
            }
        }
    }
    mNumUnread = static_cast<int>(mUnreadList.size());
    mNumRead = static_cast<int>(mReadList.size());
    SetCurrentMessageIndex(mCurrentListPos);
    MarkDirty();
}

// Update @0x1E9640 ends in MarkDirty (vtable byte 100), not Widget::Update.
void MailScreen::Update()
{
    if (!mInited)
    {
        mInited = true;
        InitMessages();
    }
    if (mRightHintAnimValue != 0.0f)
    {
        mRightHintAnimTime += 0.016f;
        mRightHintAnimValue = TodAnimateCurveFloatTime(0.0f, 0.5f, mRightHintAnimTime, 4.0f, 0.0f, CURVE_LINEAR);
    }
    if (mLeftHintAnimValue != 0.0f)
    {
        mLeftHintAnimTime += 0.016f;
        mLeftHintAnimValue = TodAnimateCurveFloatTime(0.0f, 0.5f, mLeftHintAnimTime, 4.0f, 0.0f, CURVE_LINEAR);
    }
    for (MessageDrawInfo& entry : CurrentMessages())
    {
        entry.mAnimTime += 0.016f;
        switch (entry.mAnimState)
        {
        case MESSAGE_SLIDE_OUT:
            entry.mDrawXOffset = TodAnimateCurveFloatTime(0.0f, 0.5f, entry.mAnimTime, 0.0f, -1000.0f, CURVE_LINEAR);
            break;
        case MESSAGE_SLIDE_IN:
            entry.mDrawXOffset = TodAnimateCurveFloatTime(0.0f, 0.5f, entry.mAnimTime, -1000.0f, 0.0f, CURVE_LINEAR);
            break;
        case MESSAGE_SHIFT_LEFT:
            if (entry.mAnimTime > 0.4f && entry.mAnimTime <= 0.5f)
                entry.mDrawXOffset -= 2.4f;
            break;
        case MESSAGE_SHIFT_RIGHT:
            if (entry.mAnimTime > 0.4f && entry.mAnimTime <= 0.5f)
                entry.mDrawXOffset += 2.4f;
            break;
        default:
            break;
        }
    }
    MarkDirty();
}

int MailScreen::KeyDown(Sexy::KeyCode theKey)
{
    if (!mInited)
    {
        mInited = true;
        InitMessages();
    }
    int count = static_cast<int>(CurrentMessages().size());
    switch (theKey)
    {
    case Sexy::KEYCODE_RIGHT:
    case Sexy::KEYCODE_GAMEPAD_RIGHT:
    case Sexy::KEYCODE_GAMEPAD_DPAD_RIGHT:
        if (mCurrentListPos + 1 < count)
        {
            SetCurrentMessageIndex(mCurrentListPos + 1);
            MessagesShiftLeft();
        }
        mRightHintAnimTime = 0.0f;
        mRightHintAnimValue = 4.0f;
        return 1;
    case Sexy::KEYCODE_LEFT:
    case Sexy::KEYCODE_GAMEPAD_LEFT:
    case Sexy::KEYCODE_GAMEPAD_DPAD_LEFT:
        if (mCurrentListPos > 0 && count > 0)
        {
            SetCurrentMessageIndex(mCurrentListPos - 1);
            MessagesShiftRight();
        }
        mLeftHintAnimTime = 0.0f;
        mLeftHintAnimValue = 4.0f;
        return 1;
    case Sexy::KEYCODE_RETURN:
    case KEYCODE_A:
    case Sexy::KEYCODE_GAMEPAD_A:
        if (mPage == MAIL_PAGE_INBOX && count > 0)
        {
            mApp->mMailBox->MarkMessageRead(mCurrentMessageIndex);
            mCurrentListPos = 0;
            InitMessages();
        }
        return 1;
    case Sexy::KEYCODE_SPACE:
    case Sexy::KEYCODE_GAMEPAD_Y:
        if (count > 0)
            mApp->mMailBox->GetMessageByIndex(mCurrentMessageIndex, true);
        return 1;
    case KEYCODE_X:
    case Sexy::KEYCODE_GAMEPAD_X:
        mPage = mPage == MAIL_PAGE_READ ? MAIL_PAGE_INBOX : MAIL_PAGE_READ;
        mCurrentListPos = 0;
        InitMessages();
        UpdateHelpButtons();
        return 1;
    case Sexy::KEYCODE_ESCAPE:
    case KEYCODE_B:
    case Sexy::KEYCODE_GAMEPAD_B:
    case Sexy::KEYCODE_GAMEPAD_SELECT:
        mApp->TryToSaveProfile();
        mButtonDelay = 1000;
        mApp->KillDialog(mId);
        return 1;
    default:
        return Widget::KeyDown(theKey);
    }
}

void MailScreen::GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags)
{
    (void)thePlayerIndex;
    switch (theButton)
    {
    case GAMEPAD_BUTTON_RIGHT:
        if ((theFlags & 1u) == 0)
            KeyDown(Sexy::KEYCODE_RIGHT);
        break;
    case GAMEPAD_BUTTON_LEFT:
        if ((theFlags & 1u) == 0)
            KeyDown(Sexy::KEYCODE_LEFT);
        break;
    case GAMEPAD_BUTTON_A:
        KeyDown(Sexy::KEYCODE_RETURN);
        break;
    case GAMEPAD_BUTTON_Y:
        KeyDown(Sexy::KEYCODE_GAMEPAD_Y);
        break;
    case GAMEPAD_BUTTON_X:
        KeyDown(Sexy::KEYCODE_GAMEPAD_X);
        break;
    case GAMEPAD_BUTTON_B:
    case GAMEPAD_BUTTON_SELECT:
        KeyDown(Sexy::KEYCODE_ESCAPE);
        break;
    default:
        break;
    }
}

// Help buttons @0x1E9FB0: left stick, A, X, B.
void MailScreen::UpdateHelpButtons()
{
    HelpBarWidget* help = mApp->mHelpBarWidget;
    help->ClearButtons(0);
    help->AddButton(GAMEPAD_BUTTON_THUMBL, "[SWITCH_MESSAGE]", HelpBarWidget::HELP_ALIGN_RIGHT);
    if (mPage == MAIL_PAGE_INBOX)
        help->AddButton(GAMEPAD_BUTTON_A, "[MARK_MESSAGE_READ]", HelpBarWidget::HELP_ALIGN_RIGHT);
    help->AddButton(GAMEPAD_BUTTON_X, mPage == MAIL_PAGE_READ ? "[GO_TO_INBOX]" : "[GO_TO_READ_MAIL]",
                    HelpBarWidget::HELP_ALIGN_LEFT);
    help->AddButton(GAMEPAD_BUTTON_B, "[BACK]", HelpBarWidget::HELP_ALIGN_LEFT);
}

void MailScreen::AddedToManager(Sexy::WidgetManager* theManager)
{
    LawnDialog::AddedToManager(theManager);
    // Original content is 800x600; the 1280x720 table extends past its edges.
    Widget::Resize((theManager->mWidth - 800) / 2, (theManager->mHeight - 600) / 2, 800, 600);
    UpdateHelpButtons();
    mApp->mHelpBarWidget->mDrawShade = false;
    mApp->ShowHelpBarWidget();
}

void MailScreen::RemovedFromManager(Sexy::WidgetManager* theManager)
{
    LawnDialog::RemovedFromManager(theManager);
    mApp->HideHelpBarWidget();
}
