// Mailbox holds the TV mail list: message ids split into unread and read,
// and the message records keyed by id. The read and seen bits live on the
// player profile, 32 bytes of 8 messages each.

#pragma once

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
        Sexy::SharedImageRef mPreviewImage; // node + 24
        Sexy::SharedImageRef mBodyImage;    // node + 36
        Sexy::String mImagePath;            // node + 48  the note image
        Sexy::String mAlphaGridPath;        // node + 52
        Sexy::String mBackgroundPath;       // node + 56
        float mBackgroundOffsetX;           // node + 60  default 100
        float mBackgroundOffsetY;           // node + 64  default 150
        float mMessageOffsetX;              // node + 68  default 51
        float mMessageOffsetY;              // node + 72  default 52
        unsigned char mHasOffer;            // node + 76  offerID parsed
        long long mOfferId;                 // node + 80
        int mMinLevel;                      // node + 88  deliveryLevel
        unsigned char mFromRemoteManifest;  // node + 92  path is not the local file
        unsigned char mIsRead;              // node + 93
        unsigned char mIsValid;             // node + 94
        unsigned char mPad95;
    };

    LawnApp* mApp;
    std::vector<int> mUnreadMessages;
    std::vector<int> mReadMessages;
    std::map<int, MailMessage> mMessages;
    int mLoadState; // 3 means the manifest has been parsed

    Mailbox(LawnApp* theApp);
    void Update();
    bool MessagesLoaded();
    bool CheckMessageRead(int theMessageId);
    Mailbox* MarkMessageSeen(int theMessageId);
    int GetNumReadMessages();
    int GetReadMessage(int theIndex);
    int GetNumUnreadMessages();
    int GetUnreadMessage(int theIndex);
    void ReleaseMessageImages();
    void Cleanup();
    void MarkMessageRead(int theMessageId);
    MailMessage* GetMessageByIndex(int theIndex, bool theReadPage);
    int GetNumUnseenMessages();
    void ParseManifest(const Sexy::String& thePath);
    void RefreshMessages();

    // Returned when an id is not in the map. The binary sentinel is zeroes.
    static MailMessage INVALID_MESSAGE;
};
