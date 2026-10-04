// MenuParser reads a menu descriptor and builds the widgets it names.
//
// HandleCommand is one command per call. The first element is the command
// name; the rest are its parameters. CloseFile resolves the focus ids that
// SetGameLinks recorded and then closes the file.

#include "MenuParser.h"

#include "ButtonWidget.h"
#include "DataElement.h"
#include "Font.h"
#include "Image.h"
#include "ImageWidget.h"
#include "LabelWidget.h"
#include "MenuWidget.h"
#include "ReconData.h"
#include "SexyAppBase.h"
#include "SharedImageRef.h"
#include "SingleDataElement.h"
#include "Widget.h"
#include "WidgetAnimator.h"
#include <cstdio>
#include <cstring>
#include <new>

// Declared in the recon headers. Those headers are not included here: they
// still spell the decompiler names this file is not allowed to mention.
Sexy::String SexyWStringFromString(Sexy::String const& theText);
void StringToUpper(Sexy::String* theDest, const char* theSource);

namespace Sexy {
class Font;
class ResourceManager
{
public:
    Font* GetFont(Sexy::String const& theName);
};
}

namespace Sexy {

// The holder a command builds its widget in. CreateWidget fills the typed
// slots so later commands can tell a label from a button without a second lookup.
class MenuParser_ParseWidget : public RefCount
{
public:
    MenuParser_ParseWidget()
        : mWidget(nullptr), mLabelWidget(nullptr), mImageWidget(nullptr), mButtonWidget(nullptr)
    {
        mRefCount = 0;
    }
    ~MenuParser_ParseWidget() override = default;

    Widget*       mWidget;
    LabelWidget*  mLabelWidget;
    ImageWidget*  mImageWidget;
    ButtonWidget* mButtonWidget;
};

} // namespace Sexy

namespace {

const char* kBadCount = "Invalid Number of Parameters";
const char* kBadType = "Invalid Paramater Type";
const char* kMissingWidget = "Missing parse widget";
const char* kWrongWidget = "Incorrect widget type in parse widget";
const char* kMissingResource = "No resource with that name";

bool SameCommand(const Sexy::String& theName, const char* theSpelling)
{
    return strcasecmp(theName.c_str(), theSpelling) == 0;
}

bool IsList(const Sexy::DataElement* theElement)
{
    return theElement != nullptr && theElement->mElementKind != 0;
}

// The define table is keyed on upper-case names. A second definition of the
// same name drops the old element before the new one is stored.
void DropDefine(std::map<Sexy::String, Sexy::DataElement*>& theMap, const Sexy::String& theKey)
{
    auto found = theMap.find(theKey);
    if (found == theMap.end())
        return;
    delete found->second;
    theMap.erase(found);
}

void StoreDefine(std::map<Sexy::String, Sexy::DataElement*>& theMap, const Sexy::String& theKey, Sexy::DataElement* theValue)
{
    auto found = theMap.find(theKey);
    if (found != theMap.end())
    {
        delete found->second;
        found->second = theValue;
        return;
    }
    theMap.emplace(theKey, theValue);
}

Sexy::String CommandText(const Sexy::DataElement* theElement)
{
    Sexy::String aText;
    if (theElement != nullptr && theElement->mElementKind == 0)
        SexyWStringToString(&aText, &((const Sexy::SingleDataElement*)theElement)->mData);
    return aText;
}

} // namespace

Sexy::WidgetAnimator* Sexy::MenuParser::CreateWidgetAnimator(Sexy::String theType, Sexy::ListDataElement const& theParams,
                                           Sexy::MenuParser_ParseWidget* theParse)
{
    (void)theType;
    (void)theParams;
    (void)theParse;
    return nullptr;
}

Sexy::MenuParser_ParseWidget* Sexy::MenuParser::CreateParseWidget()
{
    return new MenuParser_ParseWidget();
}

Sexy::Widget* Sexy::MenuParser::CreateWidget(Sexy::String theType, int theId, Sexy::MenuParser_ParseWidget* theParse)
{
    if (theType == "ButtonWidget")
    {
        // MenuWidget listens for its own buttons.
        ButtonWidget* aButton = new ButtonWidget(theId, mMenuWidget);
        theParse->mButtonWidget = aButton;
        aButton->SetColor(1, Color(255, 255, 255, 255));
        theParse->mWidget = aButton;
        return aButton;
    }

    if (theType == "LabelWidget")
    {
        LabelWidget* aLabel = new LabelWidget();
        theParse->mLabelWidget = aLabel;
        aLabel->mWidgetId = theId;
        theParse->mWidget = aLabel;
        return aLabel;
    }

    if (theType == "ImageWidget")
    {
        ImageWidget* aImage = new ImageWidget();
        theParse->mImageWidget = aImage;
        aImage->mWidgetId = theId;
        theParse->mWidget = aImage;
        return aImage;
    }

    Widget* aWidget = new Widget();
    theParse->mWidget = aWidget;
    aWidget->mWidgetId = theId;
    return aWidget;
}

Sexy::MenuParser::MenuParser(Sexy::MenuWidget* theMenu)
    : mMenuWidget(theMenu),
      mParseWidget(nullptr)
{
    mRefCount = 0;
}

int Sexy::MenuParser::Error(Sexy::String const& theError, Sexy::String theDetail)
{
    (void)theDetail;
    Sexy::String aWide = SexyWStringFromString(theError);
    return DescParser::Error(aWide);
}

Sexy::Widget* Sexy::MenuParser::FindWidget(int theId)
{
    // Focus ids are indexes into the menu's own children. -1 means no link.
    if (theId == -1 || mMenuWidget == nullptr)
        return nullptr;
    return mMenuWidget->FindWidget(theId);
}

int Sexy::MenuParser::DataToColor(Sexy::DataElement* theElement, Color& theColor)
{
    if (theElement == nullptr || theElement->mElementKind == 0)
        return 0;

    ListDataElement* aList = (ListDataElement*)theElement;
    const int aCount = (int)aList->mDataElements.size();
    if (aCount != 3 && aCount != 4)
        return 0;

    int aRed = 0;
    int aGreen = 0;
    int aBlue = 0;
    int aAlpha = 255;
    if (DataToInt(aList->mDataElements[0], &aRed) == 0)
        return 0;
    if (DataToInt(aList->mDataElements[1], &aGreen) == 0)
        return 0;
    if (DataToInt(aList->mDataElements[2], &aBlue) == 0)
        return 0;
    if (aCount == 4 && DataToInt(aList->mDataElements[3], &aAlpha) == 0)
        return 0;

    theColor = Color(aRed, aGreen, aBlue, aAlpha);
    return 1;
}

namespace {

bool Fail(Sexy::MenuParser* theParser, const char* theMessage)
{
    Sexy::String aMessage;
    aMessage = theMessage;
    theParser->Error(aMessage, Sexy::String());
    return false;
}

bool NeedCount(Sexy::MenuParser* theParser, const Sexy::ListDataElement& theParams, int theCount)
{
    if ((int)theParams.mDataElements.size() == theCount)
        return true;
    return Fail(theParser, kBadCount);
}

bool NeedCurrentWidget(Sexy::MenuParser* theParser)
{
    if (theParser->mParseWidget != nullptr && theParser->mParseWidget->mWidget != nullptr)
        return true;
    return Fail(theParser, kMissingWidget);
}

bool ReadInt(Sexy::MenuParser* theParser, Sexy::DataElement* theElement, int* theValue)
{
    if (IsList(theElement) || theParser->DataToInt(theElement, theValue) == 0)
        return Fail(theParser, kBadType);
    return true;
}

bool ReadBool(Sexy::MenuParser* theParser, Sexy::DataElement* theElement, bool* theValue)
{
    if (IsList(theElement) || theParser->DataToBoolean(theElement, theValue) == 0)
        return Fail(theParser, kBadType);
    return true;
}

bool ReadText(Sexy::MenuParser* theParser, Sexy::DataElement* theElement, Sexy::String* theText)
{
    if (IsList(theElement) || theParser->DataToString(theElement, theText) == 0)
        return Fail(theParser, kBadType);
    return true;
}

Sexy::SharedImageRef LoadSharedImage(const Sexy::String& theName)
{
    Sexy::String aEmpty;
    if (gSexyAppBase == nullptr)
        return Sexy::SharedImageRef();
    return gSexyAppBase->GetSharedImage(theName, aEmpty, nullptr, true, false);
}

} // namespace

bool Sexy::MenuParser::HandleCommand(Sexy::ListDataElement const& theParams)
{
    if (theParams.mDataElements.size() == 0)
        return Fail(this, kBadCount);

    Sexy::String aCommand = CommandText(theParams.mDataElements[0]);

    if (SameCommand(aCommand, "Define"))
    {
        if (!NeedCount(this, theParams, 3))
            return false;
        if (IsList(theParams.mDataElements[1]))
            return Fail(this, kBadType);

        Sexy::String aKey;
        StringToUpper(&aKey, CommandText(theParams.mDataElements[1]).c_str());
        if (IsImmediate(aKey) != 0)
            return Fail(this, kBadType);

        DropDefine(mDefineMap, aKey);

        DataElement* aValueElement = theParams.mDataElements[2];
        DataElement* aStored = nullptr;
        if (IsList(aValueElement))
        {
            ListDataElement* aValues = new ListDataElement();
            if (GetValues((ListDataElement*)aValueElement, aValues) == 0)
            {
                delete aValues;
                return false;
            }
            aStored = aValues;
        }
        else
        {
            SingleDataElement* aSingle = (SingleDataElement*)aValueElement;
            DataElement* aResolved = Dereference(aSingle->mData);
            aStored = (aResolved != nullptr) ? aResolved->Duplicate() : aValueElement->Duplicate();
        }
        StoreDefine(mDefineMap, aKey, aStored);
        return true;
    }

    if (SameCommand(aCommand, "DefineWidgetIds") || SameCommand(aCommand, "Enum"))
    {
        if (!NeedCount(this, theParams, 2))
            return false;
        if (!IsList(theParams.mDataElements[1]))
            return Fail(this, kBadType);

        // Each name in the list becomes a define whose value is that name's
        // index, written as a decimal. Later commands read the name as an id.
        ListDataElement* aNames = (ListDataElement*)theParams.mDataElements[1];
        int aIndex = 0;
        for (unsigned int i = 0; i < aNames->mDataElements.size(); ++i)
        {
            Sexy::String aName;
            if (DataToString(aNames->mDataElements[i], &aName) == 0)
                continue;

            char aDigits[16];
            std::sprintf(aDigits, "%d", aIndex);
            Sexy::String aIndexText;
            aIndexText = aDigits;

            SingleDataElement* aValue = new SingleDataElement();
            aValue->mData = aIndexText;

            Sexy::String aKey;
            StringToUpper(&aKey, aName.c_str());
            DropDefine(mDefineMap, aKey);
            StoreDefine(mDefineMap, aKey, aValue);
            ++aIndex;
        }
        return true;
    }

    if (SameCommand(aCommand, "SetBackground"))
    {
        if (!NeedCount(this, theParams, 2))
            return false;
        Sexy::String aName;
        if (!ReadText(this, theParams.mDataElements[1], &aName))
            return false;
        SharedImageRef aImage = LoadSharedImage(aName);
        if ((Image*)aImage == nullptr)
            return Fail(this, kMissingResource);
        if (mMenuWidget != nullptr)
            mMenuWidget->mImage = aImage;
        return true;
    }

    if (SameCommand(aCommand, "SetInitialFocus"))
    {
        if (!NeedCount(this, theParams, 2))
            return false;
        int aId = -1;
        if (!ReadInt(this, theParams.mDataElements[1], &aId))
            return false;
        if (mMenuWidget != nullptr)
            mMenuWidget->mInitialFocusId = aId;
        return true;
    }

    if (SameCommand(aCommand, "AddWidget"))
    {
        if (!NeedCount(this, theParams, 3))
            return false;
        if (IsList(theParams.mDataElements[1]) || IsList(theParams.mDataElements[2]))
            return Fail(this, kBadType);

        Sexy::String aKind = CommandText(theParams.mDataElements[1]);
        int aId = 0;
        DataToInt(theParams.mDataElements[2], &aId);

        MenuParser_ParseWidget* aParse = CreateParseWidget();
        if (aParse != mParseWidget)
        {
            if (mParseWidget != nullptr)
                mParseWidget->Release();
            if (aParse != nullptr)
                ++aParse->mRefCount;
            mParseWidget = aParse;
        }
        if (mMenuWidget != nullptr)
            mMenuWidget->AddWidget(CreateWidget(aKind, aId, mParseWidget));
        return true;
    }

    if (SameCommand(aCommand, "SetVisible"))
    {
        if (!NeedCount(this, theParams, 2) || !NeedCurrentWidget(this))
            return false;
        bool aVisible = false;
        if (!ReadBool(this, theParams.mDataElements[1], &aVisible))
            return false;
        mParseWidget->mWidget->SetVisible(aVisible);
        return true;
    }

    if (SameCommand(aCommand, "SetDisabled"))
    {
        if (!NeedCount(this, theParams, 2) || !NeedCurrentWidget(this))
            return false;
        bool aDisabled = false;
        if (!ReadBool(this, theParams.mDataElements[1], &aDisabled))
            return false;
        mParseWidget->mWidget->SetDisabled(aDisabled);
        return true;
    }

    if (SameCommand(aCommand, "SetPos"))
    {
        if (!NeedCount(this, theParams, 3) || !NeedCurrentWidget(this))
            return false;
        int aX = 0;
        int aY = 0;
        DataToInt(theParams.mDataElements[1], &aX);
        DataToInt(theParams.mDataElements[2], &aY);
        mParseWidget->mWidget->Move(aX, aY);
        return true;
    }

    if (SameCommand(aCommand, "Resize"))
    {
        if (!NeedCount(this, theParams, 5) || !NeedCurrentWidget(this))
            return false;
        int aX = 0;
        int aY = 0;
        int aWidth = 0;
        int aHeight = 0;
        DataToInt(theParams.mDataElements[1], &aX);
        DataToInt(theParams.mDataElements[2], &aY);
        DataToInt(theParams.mDataElements[3], &aWidth);
        DataToInt(theParams.mDataElements[4], &aHeight);
        mParseWidget->mWidget->Resize(aX, aY, aWidth, aHeight);
        return true;
    }

    if (SameCommand(aCommand, "SetGameLinks"))
    {
        if (!NeedCount(this, theParams, 5) || !NeedCurrentWidget(this))
            return false;

        // The four ids are resolved in CloseFile, once every widget exists.
        WidgetLinks aLinks;
        aLinks.mWidget = mParseWidget->mWidget;
        aLinks.mUpId = 0;
        aLinks.mDownId = 0;
        aLinks.mLeftId = 0;
        aLinks.mRightId = 0;
        DataToInt(theParams.mDataElements[1], &aLinks.mUpId);
        DataToInt(theParams.mDataElements[2], &aLinks.mDownId);
        DataToInt(theParams.mDataElements[3], &aLinks.mLeftId);
        DataToInt(theParams.mDataElements[4], &aLinks.mRightId);
        mWidgetLinks.push_back(aLinks);
        return true;
    }

    if (SameCommand(aCommand, "SetColor"))
    {
        if (!NeedCount(this, theParams, 3) || !NeedCurrentWidget(this))
            return false;
        if (IsList(theParams.mDataElements[1]) || !IsList(theParams.mDataElements[2]))
            return Fail(this, kBadType);

        int aIndex = 0;
        if (DataToInt(theParams.mDataElements[1], &aIndex) == 0)
            return Fail(this, kBadType);

        Color aColor;
        if (DataToColor(theParams.mDataElements[2], aColor) == 0)
            return Fail(this, kBadType);
        mParseWidget->mWidget->SetColor(aIndex, aColor);
        return true;
    }

    if (SameCommand(aCommand, "SetLabelJustify"))
    {
        if (!NeedCount(this, theParams, 2) || !NeedCurrentWidget(this))
            return false;
        if (mParseWidget->mButtonWidget == nullptr)
            return Fail(this, kWrongWidget);
        int aJustify = 0;
        if (!ReadInt(this, theParams.mDataElements[1], &aJustify))
            return false;
        mParseWidget->mButtonWidget->mLabelJustify = aJustify;
        return true;
    }

    if (SameCommand(aCommand, "AddAnimator"))
    {
        const int aCount = (int)theParams.mDataElements.size();
        if (aCount != 4 && aCount != 5)
            return Fail(this, kBadCount);
        if (IsList(theParams.mDataElements[1]) || IsList(theParams.mDataElements[2])
            || !IsList(theParams.mDataElements[3]))
            return Fail(this, kBadType);
        if (!NeedCurrentWidget(this))
            return false;

        int aState = 0;
        if (DataToInt(theParams.mDataElements[1], &aState) == 0 || (unsigned int)aState > 2u)
            return Fail(this, kBadType);

        Sexy::String aKind;
        if (!ReadText(this, theParams.mDataElements[2], &aKind))
            return false;

        WidgetAnimator* aAnimator = CreateWidgetAnimator(
            aKind, *(ListDataElement*)theParams.mDataElements[3], mParseWidget);
        if (aAnimator == nullptr)
            return Fail(this, kBadType);

        if (aCount == 5)
        {
            int aTicks = 0;
            if (DataToInt(theParams.mDataElements[4], &aTicks) != 0)
                aAnimator->mUpdateCounter = aTicks;
        }
        mParseWidget->mWidget->SetAnimatorForState(WidgetAnimationState(aState), aAnimator);
        return true;
    }

    if (SameCommand(aCommand, "Layout"))
        return true;

    if (SameCommand(aCommand, "SetAlign"))
    {
        if (!NeedCount(this, theParams, 2) || !NeedCurrentWidget(this))
            return false;
        if (mParseWidget->mLabelWidget == nullptr)
            return Fail(this, kWrongWidget);

        int aAlign = 0;
        if (!ReadInt(this, theParams.mDataElements[1], &aAlign))
            return false;
        if ((unsigned int)aAlign <= 2u)
            mParseWidget->mLabelWidget->mJustification = aAlign;
        return true;
    }

    if (SameCommand(aCommand, "SetFont"))
    {
        if (!NeedCount(this, theParams, 2) || !NeedCurrentWidget(this))
            return false;
        if (mParseWidget->mButtonWidget == nullptr && mParseWidget->mLabelWidget == nullptr)
            return Fail(this, kWrongWidget);

        Sexy::String aName;
        if (!ReadText(this, theParams.mDataElements[1], &aName))
            return false;
        if (gSexyAppBase == nullptr || gSexyAppBase->mResourceManager == nullptr)
            return Fail(this, kMissingResource);

        Font* aFont = gSexyAppBase->mResourceManager->GetFont(aName);
        if (aFont == nullptr)
            return Fail(this, kMissingResource);
        if (mParseWidget->mLabelWidget != nullptr)
            mParseWidget->mLabelWidget->mFont = aFont;
        else
            mParseWidget->mButtonWidget->SetFont(aFont);
        return true;
    }

    if (SameCommand(aCommand, "SetLabel"))
    {
        if (!NeedCount(this, theParams, 2) || !NeedCurrentWidget(this))
            return false;
        if (mParseWidget->mButtonWidget == nullptr && mParseWidget->mLabelWidget == nullptr)
            return Fail(this, kWrongWidget);

        Sexy::String aText;
        if (!ReadText(this, theParams.mDataElements[1], &aText))
            return false;
        if (mParseWidget->mLabelWidget != nullptr)
        {
            if (gSexyAppBase != nullptr)
                mParseWidget->mLabelWidget->mString = gSexyAppBase->GetString(aText, aText);
            else
                mParseWidget->mLabelWidget->mString = aText;
        }
        else
        {
            mParseWidget->mButtonWidget->SetLabel(aText);
        }
        return true;
    }

    if (SameCommand(aCommand, "SetImage") || SameCommand(aCommand, "SetOverImage")
        || SameCommand(aCommand, "SetDownImage") || SameCommand(aCommand, "SetDisabledImage"))
    {
        if (!NeedCount(this, theParams, 2) || !NeedCurrentWidget(this))
            return false;

        const bool aButtonOnly = !SameCommand(aCommand, "SetImage");
        if (aButtonOnly)
        {
            if (mParseWidget->mButtonWidget == nullptr)
                return Fail(this, kWrongWidget);
        }
        else if (mParseWidget->mButtonWidget == nullptr && mParseWidget->mImageWidget == nullptr)
        {
            return Fail(this, kWrongWidget);
        }

        Sexy::String aName;
        if (!ReadText(this, theParams.mDataElements[1], &aName))
            return false;

        SharedImageRef aImage = LoadSharedImage(aName);
        if ((Image*)aImage == nullptr)
            return true;

        if (SameCommand(aCommand, "SetImage"))
        {
            if (mParseWidget->mImageWidget != nullptr)
            {
                mParseWidget->mImageWidget->mImage = aImage;
                mParseWidget->mImageWidget->AttachSharedImage(aImage);
            }
            else if (mParseWidget->mButtonWidget != nullptr)
            {
                ButtonWidget* aButton = mParseWidget->mButtonWidget;
                aButton->mButtonImage = aImage;
                aButton->mNormalRect.mWidth = ((Image*)aImage)->mWidth;
                aButton->mNormalRect.mHeight = ((Image*)aImage)->mHeight;
                aButton->AttachSharedImage(aImage);
            }
        }
        else if (SameCommand(aCommand, "SetOverImage"))
        {
            mParseWidget->mButtonWidget->mOverImage = aImage;
            mParseWidget->mButtonWidget->AttachSharedImage(aImage);
        }
        else if (SameCommand(aCommand, "SetDownImage"))
        {
            mParseWidget->mButtonWidget->mDownImage = aImage;
            mParseWidget->mButtonWidget->AttachSharedImage(aImage);
        }
        else
        {
            mParseWidget->mButtonWidget->mDisabledImage = aImage;
            mParseWidget->mButtonWidget->AttachSharedImage(aImage);
        }
        return true;
    }

    return Fail(this, "Unknown Menu Command");
}

int Sexy::MenuParser::CloseFile()
{
    // Every widget the file named now exists, so the stored ids can be resolved.
    for (unsigned int i = 0; i < mWidgetLinks.size(); ++i)
    {
        WidgetLinks& aLink = mWidgetLinks[i];
        if (aLink.mWidget == nullptr)
            continue;
        aLink.mWidget->SetFocusLinks(
            FindWidget(aLink.mUpId),
            FindWidget(aLink.mDownId),
            FindWidget(aLink.mLeftId),
            FindWidget(aLink.mRightId));
    }
    mWidgetLinks.clear();
    return EncodingParser::CloseFile();
}

Sexy::MenuParser::~MenuParser()
{
    if (mParseWidget != nullptr)
        mParseWidget->Release();
}
