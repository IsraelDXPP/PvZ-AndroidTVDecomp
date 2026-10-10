// Mailbox loads the TV mail manifest, splits message ids by the profile's
// read bits, and loads the note and background images when a page asks for them.

#include "Mailbox.h"
#include "ReconData.h"
#include "Image.h"
#include "LawnApp.h"
#include "LawnPlayerInfo.h"
#include "SharedImageRef.h"
#include "XMLElement.h"
#include "XMLParser.h"
#include <algorithm>
#include <cstring>

Mailbox::MailMessage Mailbox::INVALID_MESSAGE;

namespace
{

int AttributeInt(const Sexy::XMLElement &theElement, const char *theName, int theDefault)
{
    if (!theElement.HasAttribute(Sexy::String(theName)))
        return theDefault;
    int aValue = theDefault;
    StringToInt(theElement.GetAttribute(Sexy::String(theName)), &aValue);
    return aValue;
}

float AttributeFloat(const Sexy::XMLElement &theElement, const char *theName, float theDefault)
{
    return (float)AttributeInt(theElement, theName, (int)theDefault);
}

// offerID is a signed integer, decimal or 0x hex, stored in a 64-bit word.
long long ParseOfferId(const Sexy::String &theText)
{
    const char *aText = theText.c_str();
    if (aText == nullptr || aText[0] == 0)
        return 0;

    bool aNegative = aText[0] == '-';
    int aIndex = aNegative ? 1 : 0;
    int aLength = (int)theText.length();
    if (aIndex >= aLength)
        return 0;

    int aBase = 10;
    if (aText[aIndex] == '0' && aIndex + 1 < aLength)
    {
        char aMark = aText[aIndex + 1];
        if (aMark == 'x' || aMark == 'X')
        {
            aBase = 16;
            aIndex += 2;
        }
    }

    long long aValue = 0;
    for (; aIndex < aLength; ++aIndex)
    {
        unsigned char aChar = (unsigned char)aText[aIndex];
        int aDigit = -1;
        if (aChar >= '0' && aChar <= '9')
            aDigit = aChar - '0';
        else if (aBase == 16 && aChar >= 'a' && aChar <= 'f')
            aDigit = aChar - 'a' + 10;
        else if (aBase == 16 && aChar >= 'A' && aChar <= 'F')
            aDigit = aChar - 'A' + 10;
        if (aDigit < 0 || aDigit >= aBase)
            break;
        aValue = aValue * aBase + aDigit;
    }
    if (aNegative)
        aValue = -aValue;
    return aValue;
}

void EnsureImage(Sexy::SharedImageRef &theImage, const Sexy::String &thePath)
{
    Sexy::Image *image = theImage;
    if (image != nullptr)
        return;
    if (thePath.empty())
        return;
    theImage = gSexyAppBase->GetSharedImage(thePath, Sexy::String(), nullptr, true, false);
}

} // namespace

Mailbox::Mailbox(LawnApp *theApp)
{
    mApp = theApp;
    mLoadState = 0;
}

void Mailbox::Update() {}

bool Mailbox::MessagesLoaded()
{
    return mLoadState == 3;
}

bool Mailbox::CheckMessageRead(int theMessageId)
{
    // 32 bytes, 8 messages each. Message 0 is the low bit of byte 0.
    if (theMessageId < 0 || mApp->mPlayerInfo == nullptr)
        return false;
    int aByte = theMessageId / 8;
    int aBit = theMessageId % 8;
    if (aByte >= 32)
        return false;
    return (mApp->mPlayerInfo->mMailMessageRead[aByte] & (unsigned char)(1 << aBit)) != 0;
}

void Mailbox::MarkMessageSeen(int theMessageId)
{
    if (theMessageId < 0 || mApp->mPlayerInfo == nullptr)
        return;
    int aByte = theMessageId / 8;
    int aBit = theMessageId % 8;
    if (aByte >= 32)
        return;
    mApp->mPlayerInfo->mMailMessageSeen[aByte] |= (unsigned char)(1 << aBit);
}

int Mailbox::GetNumReadMessages()
{
    return (int)mReadMessages.size();
}

int Mailbox::GetReadMessage(int theIndex)
{
    return theIndex >= 0 && theIndex < GetNumReadMessages() ? mReadMessages[theIndex] : -1;
}

int Mailbox::GetNumUnreadMessages()
{
    return (int)mUnreadMessages.size();
}

int Mailbox::GetUnreadMessage(int theIndex)
{
    return theIndex >= 0 && theIndex < GetNumUnreadMessages() ? mUnreadMessages[theIndex] : -1;
}

void Mailbox::ReleaseMessageImages()
{
    for (std::map<int, MailMessage>::iterator aMessage = mMessages.begin(); aMessage != mMessages.end(); ++aMessage)
    {
        aMessage->second.mMessageImage.Release();
        aMessage->second.mBackgroundImage.Release();
    }
}

void Mailbox::Cleanup()
{
    mMessages.clear();
    mUnreadMessages.clear();
    mReadMessages.clear();
    mLoadState = 0;
}

void Mailbox::MarkMessageRead(int theMessageId)
{
    if (theMessageId < 0 || mApp->mPlayerInfo == nullptr)
        return;
    int aByte = theMessageId / 8;
    int aBit = theMessageId % 8;
    if (aByte >= 32)
        return;

    auto message = mMessages.find(theMessageId);
    if (message == mMessages.end() || !message->second.mIsValid)
        return;

    mApp->mPlayerInfo->mMailMessageRead[aByte] |= (unsigned char)(1 << aBit);

    std::vector<int>::iterator aFound = std::find(mUnreadMessages.begin(), mUnreadMessages.end(), theMessageId);
    if (aFound != mUnreadMessages.end())
        mUnreadMessages.erase(aFound);

    if (std::find(mReadMessages.begin(), mReadMessages.end(), theMessageId) == mReadMessages.end())
        mReadMessages.push_back(theMessageId);
    message->second.mIsRead = true;
}

Mailbox::MailMessage *Mailbox::GetMessageByIndex(int theIndex, bool theLoadImages)
{
    std::map<int, MailMessage>::iterator aFound = mMessages.find(theIndex);
    if (aFound == mMessages.end())
        return &INVALID_MESSAGE;

    MailMessage &aMessage = aFound->second;
    if (theLoadImages)
    {
        EnsureImage(aMessage.mMessageImage, aMessage.mImagePath);
        EnsureImage(aMessage.mBackgroundImage, aMessage.mBackgroundPath);
    }
    return &aMessage;
}

int Mailbox::GetNumUnseenMessages()
{
    LawnPlayerInfo *aPlayerInfo = mApp->mPlayerInfo;
    if (aPlayerInfo == nullptr)
        return 0;

    int aCount = 0;
    for (int aByte = 0; aByte != 32; ++aByte)
    {
        for (int aBit = 0; aBit != 8; ++aBit)
        {
            MailMessage *aMessage = GetMessageByIndex(aBit + 8 * aByte, false);
            int aLevel = aPlayerInfo->mLevel;
            if (aPlayerInfo->GetFlag(PLAYER_FLAG_ADVENTURE_COMPLETE))
                aLevel = 50;
            int aMask = 1 << aBit;
            if (aLevel >= aMessage->mMinLevel && (aMask & aPlayerInfo->mMailMessageSeen[aByte]) == 0 &&
                aMessage->mIsValid)
            {
                ++aCount;
            }
        }
    }
    return aCount;
}

void Mailbox::ParseManifest(const Sexy::String &thePath)
{
    Sexy::XMLParser aParser;
    if (!aParser.OpenFile(thePath))
        return;

    bool aLocalFile = thePath == "properties/local_mailbox.xml";

    for (;;)
    {
        Sexy::XMLElement aElement;
        if (aParser.NextElement(&aElement) == 0)
            break;
        if (aElement.mType != 1 || aElement.mInstruction != "Message")
            continue;
        if (!aElement.HasAttribute(Sexy::String("id")))
            continue;

        int aId = -1;
        StringToInt(aElement.GetAttribute(Sexy::String("id")), &aId);

        Sexy::String aImage = aElement.GetAttribute(Sexy::String("image"));
        if (aId < 0 || aId >= 256 || aImage.empty())
            continue;

        // A manifest can replace an existing id; keep each id on just one page.
        mUnreadMessages.erase(std::remove(mUnreadMessages.begin(), mUnreadMessages.end(), aId), mUnreadMessages.end());
        mReadMessages.erase(std::remove(mReadMessages.begin(), mReadMessages.end(), aId), mReadMessages.end());

        MailMessage aMessage;
        aMessage.mImagePath = aImage;
        aMessage.mAlphaGridPath = aElement.GetAttribute(Sexy::String("alphagrid"));
        aMessage.mBackgroundPath = aElement.GetAttribute(Sexy::String("background"));
        aMessage.mBackgroundOffsetX = AttributeFloat(aElement, "backgroundOffsetX", 100.0f);
        aMessage.mBackgroundOffsetY = AttributeFloat(aElement, "backgroundOffsetY", 150.0f);
        aMessage.mMessageOffsetX = AttributeFloat(aElement, "messageOffsetX", 51.0f);
        aMessage.mMessageOffsetY = AttributeFloat(aElement, "messageOffsetY", 52.0f);
        aMessage.mMinLevel = AttributeInt(aElement, "deliveryLevel", 0);
        aMessage.mHasOffer = aElement.HasAttribute(Sexy::String("offerID"));
        aMessage.mOfferId = 0;
        if (aMessage.mHasOffer)
            aMessage.mOfferId = ParseOfferId(aElement.GetAttribute(Sexy::String("offerID")));
        aMessage.mFromLocalManifest = aLocalFile;
        aMessage.mIsRead = CheckMessageRead(aId);
        aMessage.mIsValid = true;

        if (aMessage.mIsRead)
            mReadMessages.push_back(aId);
        else
            mUnreadMessages.push_back(aId);

        mMessages[aId] = aMessage;
    }
    mLoadState = 3;
}

void Mailbox::RefreshMessages()
{
    Cleanup();
    ParseManifest("properties/local_mailbox.xml");
}
