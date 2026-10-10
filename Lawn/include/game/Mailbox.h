// Mailbox holds the TV mail list: message ids split into unread and read,
// and the message records keyed by id. The read and seen bits live on the
// player profile, 32 bytes of 8 messages each.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_MAILBOX_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_MAILBOX_H

#include "ReconTypes.h"
#include "LawnPlayerInfo.h"
#include "SharedImageRef.h"
#include "XMLParser.h"
#include <map>
#include <vector>

class LawnApp;

class Mailbox
{
public:
    // One mail record. Offsets past the images are the values ParseManifest
    // writes into the tree node, and the values MailScreen draws with.
    struct MailMessage
    {
        Sexy::SharedImageRef mMessageImage; // node + 24
        Sexy::SharedImageRef mBackgroundImage; // node + 36
        Sexy::String mImagePath; // node + 48  the note image
        Sexy::String mAlphaGridPath; // node + 52
        Sexy::String mBackgroundPath; // node + 56
        float mMessageOffsetX = 0.0f; // node + 60, relative to background
        float mMessageOffsetY = 0.0f; // node + 64
        float mBackgroundOffsetX = 0.0f; // node + 68, absolute position
        float mBackgroundOffsetY = 0.0f; // node + 72
        bool mHasOffer = false;
        long long mOfferId = 0;
        int mMinLevel = 0;
        bool mFromLocalManifest = false;
        bool mIsRead = false;
        bool mIsValid = false;
    };

    LawnApp *mApp;
    std::vector<int> mUnreadMessages;
    std::vector<int> mReadMessages;
    std::map<int, MailMessage> mMessages;
    int mLoadState; // 3 means the manifest has been parsed

    Mailbox(LawnApp *theApp);
    void Update();
    bool MessagesLoaded();
    bool CheckMessageRead(int theMessageId);
    void MarkMessageSeen(int theMessageId);
    int GetNumReadMessages();
    int GetReadMessage(int theIndex);
    int GetNumUnreadMessages();
    int GetUnreadMessage(int theIndex);
    void ReleaseMessageImages();
    void Cleanup();
    void MarkMessageRead(int theMessageId);
    MailMessage *GetMessageByIndex(int theIndex, bool theLoadImages);
    int GetNumUnseenMessages();
    void ParseManifest(const Sexy::String &thePath);
    void RefreshMessages();

    // Returned when an id is not in the map. The binary sentinel is zeroes.
    static MailMessage INVALID_MESSAGE;
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_MAILBOX_H
