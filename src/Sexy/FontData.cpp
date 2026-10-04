// FontData turns a font descriptor into layers. Commands fill the char
// map, the layer list and the layer map. The define table belongs to
// DescParser; this destructor does not delete those values.

#include "FontData.h"
#include "CharData.h"
#include "DataElement.h"
#include "DescParser.h"
#include "Font.h"
#include "FontLayer.h"
#include "Image.h"
#include "ListDataElement.h"
#include "PixelUtil.h"
#include "ResourceManager.h"
#include "SexyAppBase.h"
#include "SharedImageRef.h"
#include "SingleDataElement.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace {

bool IsModLayerName(Sexy::String const& theName)
{
    return theName.length() > 5
        && theName.compare(theName.length() - 5, 5, "__MOD") == 0;
}

void StoreDefine(std::map<Sexy::String, Sexy::DataElement*>& theMap,
                 Sexy::String const& theKey, Sexy::DataElement* theValue)
{
    auto previous = theMap.find(theKey);
    if (previous != theMap.end())
    {
        delete previous->second;
        previous->second = theValue;
        return;
    }
    theMap.emplace(theKey, theValue);
}

Sexy::FontLayer* StoreIntField(Sexy::FontData* theFont, Sexy::ListDataElement const& theParams,
                               int& theField)
{
    if (theParams.mDataElements.size() != 3)
    {
        theFont->ReportError("Invalid Number of Parameters");
        return nullptr;
    }
    Sexy::FontLayer* aLayer = nullptr;
    if (theFont->DataToLayer(theParams.mDataElements[1], &aLayer) == 0)
    {
        theFont->ReportError("Invalid Paramater Type");
        return nullptr;
    }
    Sexy::SingleDataElement* aValueElement =
        static_cast<Sexy::SingleDataElement*>(theParams.mDataElements[2]);
    if (aValueElement->mElementKind != 0)
    {
        theFont->ReportError("Invalid Paramater Type");
        return nullptr;
    }
    Sexy::String aText(aValueElement->mData);
    int aValue = 0;
    if (StringToInt(aText, &aValue) == 0)
    {
        theFont->ReportError("Invalid Paramater Type");
        return nullptr;
    }
    theField = aValue;
    return aLayer;
}

} // namespace

Sexy::FontData::FontData()
    : mIsListFont(false)
    , mRefCount(0)
    , mApp(nullptr)
    , mDefaultPointSize(0)
{
}

int Sexy::FontData::Ref()
{
    ++mRefCount;
    return mRefCount;
}

Sexy::FontData* Sexy::FontData::DeRef()
{
    if (--mRefCount == 0)
        delete this;
    return this;
}

int Sexy::FontData::DataToLayer(Sexy::DataElement* theElement, Sexy::FontLayer** theLayer)
{
    *theLayer = nullptr;
    if (theElement->mElementKind != 0)
        return 0;

    SingleDataElement* aSingle = static_cast<SingleDataElement*>(theElement);
    Sexy::String aName;
    StringToUpper(&aName, aSingle->mData.c_str());

    auto found = mLayerMap.find(aName);
    if (found != mLayerMap.end())
    {
        *theLayer = found->second;
        return 1;
    }

    Sexy::String aMessage = "Undefined Layer";
    Error(aMessage);
    return 0;
}

int Sexy::FontData::GetColorFromDataElement(Sexy::DataElement* theElement, Color& theColor)
{
    if (theElement->mElementKind != 0)
    {
        std::vector<double> aValues;
        if (DataToDoubleVector(theElement, &aValues) == 0 || aValues.size() != 4)
            return 0;
        theColor = Color((int)(aValues[0] * 255.0), (int)(aValues[1] * 255.0),
                         (int)(aValues[2] * 255.0), (int)(aValues[3] * 255.0));
        return 1;
    }

    SingleDataElement* aSingle = static_cast<SingleDataElement*>(theElement);
    Sexy::String aText(aSingle->mData);
    int aValue = 0;
    int aResult = StringToInt(aText, &aValue);
    if (aResult == 0)
        return 0;
    theColor = Color(aValue);
    return aResult;
}

bool Sexy::FontData::Error(const Sexy::String& theError)
{
    if (mApp == nullptr)
        return false;

    Sexy::String aMessage(mErrorHeader);
    aMessage += theError;
    if (mCurrentLine.length() != 0)
    {
        Sexy::String aLineNumber;
        StrFormat(&aLineNumber, "%d:\r\n\r\n", mCurrentLineNum);
        Sexy::String aOnLine = " on Line ";
        aOnLine += aLineNumber;
        Sexy::String aLine;
        WStringToString(&aLine, (const wchar_t*)mCurrentLine.c_str());
        aOnLine += aLine;
        aMessage += aOnLine;
    }
    mApp->Popup(aMessage);
    return false;
}

int Sexy::FontData::Load(Sexy::SexyAppBase* theApp, Sexy::String const& theFileName)
{
    if (mIsListFont)
        return 0;

    mApp = theApp;
    Sexy::String aHeader = "Font Descriptor Error in ";
    aHeader += "\r\n";
    mErrorHeader = aHeader;
    mFileName = theFileName;
    mIsListFont = LoadDescriptor(theFileName);
    return 1;
}

int Sexy::FontData::LoadLegacy(Sexy::Image* theImage, Sexy::String const& theFileName)
{
    if (mIsListFont)
        return 0;

    FontLayer aTemplate(this);
    FontLayer* aLayer = AddLayer(aTemplate);
    if (aLayer == nullptr)
        return 0;
    RegisterLayer(aLayer->mLayerName, aLayer);

    aLayer->mImage.Release();
    aLayer->mImage.mImage = theImage;
    int aImageHeight = aLayer->mImage.mImage->GetHeight();
    aLayer->mDefaultHeight = aImageHeight;
    aLayer->mAscent = aImageHeight;

    FILE* aFile = fopen(theFileName.c_str(), "r");
    if (aFile == nullptr)
        return 0;
    mFileName = theFileName;

    int aHeight = 0;
    fscanf(aFile, "%d%d", &aHeight, &aLayer->mHeight);
    CharData* aSpace = aLayer->GetCharData(32);
    aSpace->mAdvance = (unsigned short)aHeight;

    if (feof(aFile) == 0)
    {
        int aBaseline = 0;
        for (;;)
        {
            char aCharText[8] = { 0 };
            int aCharHeight = 0;
            fscanf(aFile, "%1s%d", aCharText, &aCharHeight);
            if (aCharText[0] == 0)
                break;
            int aChar = (unsigned char)aCharText[0];
            CharData* aGlyph = aLayer->GetCharData(aChar);
            aGlyph->mX = (unsigned short)aBaseline;
            aGlyph->mHeight = (unsigned short)aImageHeight;
            aGlyph->mY = 0;
            aGlyph->mWidth = (unsigned short)aCharHeight;
            aBaseline = (unsigned short)(aBaseline + aCharHeight);
            aGlyph->mAdvance = (unsigned short)aCharHeight;
            if (feof(aFile) != 0)
                break;
        }
    }

    for (int aUpper = 65; aUpper != 91; ++aUpper)
    {
        if (aLayer->GetCharData(aUpper)->mAdvance != 0)
            continue;
        if (aLayer->GetCharData(aUpper + 32)->mAdvance != 0)
            mCharMap[aUpper] = aUpper + 32;
    }
    for (int aLower = 97; aLower != 123; ++aLower)
    {
        if (aLayer->GetCharData(aLower)->mAdvance != 0)
            continue;
        if (aLayer->GetCharData(aLower - 32)->mAdvance != 0)
            mCharMap[aLower] = aLower - 32;
    }

    mIsListFont = true;
    fclose(aFile);
    return 1;
}

Sexy::FontData::~FontData()
{
}

bool Sexy::FontData::ReportError(const char* theMessage)
{
    Sexy::String aMessage = theMessage;
    Error(aMessage);
    return false;
}

Sexy::FontLayer* Sexy::FontData::AddLayer(Sexy::FontLayer const& theLayer)
{
    mLayerList.push_back(theLayer);
    return &mLayerList.back();
}

void Sexy::FontData::RegisterLayer(Sexy::String const& theName, Sexy::FontLayer* theLayer)
{
    if (mLayerMap.find(theName) != mLayerMap.end())
        ReportError("Layer Already Exists");
    mLayerMap[theName] = theLayer;
}

bool Sexy::FontData::HandleCommand(Sexy::ListDataElement const& theParams)
{
    SingleDataElement* aNameElement = static_cast<SingleDataElement*>(theParams.mDataElements[0]);
    Sexy::String aCommandName;
    ToString(&aCommandName, &aNameElement->mData);
    Sexy::String aCommand;
    StringToLower(&aCommand, aCommandName.c_str());

    if (aCommand == "createhorzspanrectlist")
    {
        if (theParams.mDataElements.size() != 4)
            return ReportError("Invalid Number of Parameters");

        std::vector<int> aRects;
        std::vector<int> aCounts;
        DataElement* aFirst = theParams.mDataElements[1];
        if (aFirst->mElementKind != 0)
            return ReportError("Invalid Paramater Type");
        if (DataToIntVector(theParams.mDataElements[2], &aRects) == 0 || aRects.size() != 4
            || DataToIntVector(theParams.mDataElements[3], &aCounts) == 0)
            return ReportError("Invalid Paramater Type");
        if (aCounts.empty())
            return false;

        SingleDataElement* aName = static_cast<SingleDataElement*>(theParams.mDataElements[1]);
        Sexy::String aKey;
        StringToUpper(&aKey, aName->mData.c_str());

        ListDataElement* aOuter = new ListDataElement();
        int aOffset = 0;
        for (unsigned int i = 0; i < aCounts.size(); ++i)
        {
            ListDataElement* aRow = new ListDataElement();
            aOuter->mDataElements.push_back(aRow);

            int aFields[4];
            aFields[0] = aRects[0] + aOffset;
            aFields[1] = aRects[1];
            aFields[2] = aCounts[i];
            aFields[3] = aRects[3];
            for (int aField = 0; aField < 4; ++aField)
            {
                char aBuffer[16];
                sprintf(aBuffer, "%d", aFields[aField]);
                Sexy::String aWide;
                StringToWString(&aWide, aBuffer);
                SingleDataElement* aElement = new SingleDataElement();
                aElement->mData = aWide;
                aRow->mDataElements.push_back(aElement);
            }
            aOffset += aCounts[i];
        }

        StoreDefine(mDefineMap, aKey, aOuter);
        return true;
    }

    if (aCommand == "setdefaultpointsize")
    {
        if (theParams.mDataElements.size() != 2)
            return ReportError("Invalid Number of Parameters");
        DataElement* aParam = theParams.mDataElements[1];
        if (aParam->mElementKind != 0)
            return ReportError("Invalid Paramater Type");
        SingleDataElement* aSingle = static_cast<SingleDataElement*>(aParam);
        Sexy::String aText(aSingle->mData);
        int aValue = 0;
        if (StringToInt(aText, &aValue) == 0)
            return ReportError("Invalid Paramater Type");
        mDefaultPointSize = aValue;
        return true;
    }

    if (aCommand == "setcharmap")
    {
        if (theParams.mDataElements.size() != 3)
            return ReportError("Invalid Number of Parameters");

        std::vector<Sexy::String> aKeys;
        std::vector<Sexy::String> aValues;
        if (DataToStringVector(theParams.mDataElements[1], &aKeys) == 0
            || DataToStringVector(theParams.mDataElements[2], &aValues) == 0)
            return ReportError("Invalid Paramater Type");
        if (aKeys.size() != aValues.size())
            return ReportError("List Size Mismatch");

        for (unsigned int i = 0; i < aKeys.size(); ++i)
        {
            int aFrom = 0;
            int aTo = 0;
            StringToInt(aKeys[i], &aFrom);
            StringToInt(aValues[i], &aTo);
            mCharMap[aFrom] = aTo;
        }
        return true;
    }

    if (aCommand == "createlayer")
    {
        if (theParams.mDataElements.size() != 2)
            return ReportError("Invalid Number of Parameters");
        SingleDataElement* aName = static_cast<SingleDataElement*>(theParams.mDataElements[1]);
        if (aName->mElementKind != 0)
            return ReportError("Invalid Paramater Type");
        Sexy::String aKey;
        StringToUpper(&aKey, aName->mData.c_str());

        FontLayer aLayer(this);
        FontLayer* aStored = AddLayer(aLayer);
        SexyWStringToString(&aStored->mLayerName, &aKey);
        RegisterLayer(aKey, aStored);
        return true;
    }

    if (aCommand == "createlayerfrom")
    {
        if (theParams.mDataElements.size() != 3)
            return ReportError("Invalid Number of Parameters");
        if (theParams.mDataElements[1]->mElementKind != 0)
            return ReportError("Invalid Paramater Type");
        FontLayer* aSource = nullptr;
        if (DataToLayer(theParams.mDataElements[2], &aSource) == 0)
            return ReportError("Invalid Paramater Type");

        SingleDataElement* aName = static_cast<SingleDataElement*>(theParams.mDataElements[1]);
        Sexy::String aKey;
        StringToUpper(&aKey, aName->mData.c_str());
        FontLayer aCopy(*aSource);
        FontLayer* aStored = AddLayer(aCopy);
        SexyWStringToString(&aStored->mLayerName, &aKey);
        RegisterLayer(aKey, aStored);
        return true;
    }

    if (aCommand == "layerrequiretags" || aCommand == "layerexcludetags")
    {
        if (theParams.mDataElements.size() != 3)
            return ReportError("Invalid Number of Parameters");
        FontLayer* aLayer = nullptr;
        if (DataToLayer(theParams.mDataElements[1], &aLayer) == 0)
            return ReportError("Invalid Paramater Type");
        std::vector<Sexy::String> aTags;
        if (DataToStringVector(theParams.mDataElements[2], &aTags) == 0)
            return ReportError("Invalid Paramater Type");

        std::vector<Sexy::String>& aDest = aCommand == "layerrequiretags"
            ? aLayer->mRequiredTags
            : aLayer->mExcludedTags;
        for (unsigned int i = 0; i < aTags.size(); ++i)
        {
            Sexy::String aTag;
            StringToUpper(&aTag, aTags[i].c_str());
            aDest.push_back(aTag);
        }
        return true;
    }

    if (aCommand == "layerpointrange")
    {
        if (theParams.mDataElements.size() != 4)
            return ReportError("Invalid Number of Parameters");
        FontLayer* aLayer = nullptr;
        if (DataToLayer(theParams.mDataElements[1], &aLayer) == 0)
            return ReportError("Invalid Paramater Type");
        SingleDataElement* aMin = static_cast<SingleDataElement*>(theParams.mDataElements[2]);
        SingleDataElement* aMax = static_cast<SingleDataElement*>(theParams.mDataElements[3]);
        if (aMin->mElementKind != 0 || aMax->mElementKind != 0)
            return ReportError("Invalid Paramater Type");
        Sexy::String aMinText(aMin->mData);
        Sexy::String aMaxText(aMax->mData);
        int aMinSize = 0;
        int aMaxSize = 0;
        if (StringToInt(aMinText, &aMinSize) == 0 || StringToInt(aMaxText, &aMaxSize) == 0)
            return ReportError("Invalid Paramater Type");
        aLayer->mMinPointSize = aMinSize;
        aLayer->mMaxPointSize = aMaxSize;
        return true;
    }

    if (aCommand == "layersetpointsize" || aCommand == "layersetheight"
        || aCommand == "layersetspacing" || aCommand == "layersetascent"
        || aCommand == "layersetascentpadding" || aCommand == "layersetlinespacingoffset"
        || aCommand == "layersetbaseorder" || aCommand == "layersetdrawmode")
    {
        if (theParams.mDataElements.size() != 3)
            return ReportError("Invalid Number of Parameters");
        FontLayer* aLayer = nullptr;
        if (DataToLayer(theParams.mDataElements[1], &aLayer) == 0)
            return ReportError("Invalid Paramater Type");
        SingleDataElement* aValueElement =
            static_cast<SingleDataElement*>(theParams.mDataElements[2]);
        if (aValueElement->mElementKind != 0)
            return ReportError("Invalid Paramater Type");
        Sexy::String aText(aValueElement->mData);
        int aValue = 0;
        if (StringToInt(aText, &aValue) == 0)
            return ReportError("Invalid Paramater Type");

        if (aCommand == "layersetdrawmode")
        {
            if ((unsigned int)aValue > 1)
                return ReportError("Invalid Paramater Type");
            aLayer->mDrawMode = aValue;
        }
        else if (aCommand == "layersetpointsize")
            aLayer->mPointSize = aValue;
        else if (aCommand == "layersetheight")
            aLayer->mHeight = aValue;
        else if (aCommand == "layersetspacing")
            aLayer->mSpacing = aValue;
        else if (aCommand == "layersetascent")
            aLayer->mAscent = aValue;
        else if (aCommand == "layersetascentpadding")
            aLayer->mAscentPadding = aValue;
        else if (aCommand == "layersetlinespacingoffset")
            aLayer->mLineSpacingOffset = aValue;
        else
            aLayer->mBaseOrder = aValue;
        return true;
    }

    if (aCommand == "layersetoffset")
    {
        if (theParams.mDataElements.size() != 3)
            return ReportError("Invalid Number of Parameters");
        FontLayer* aLayer = nullptr;
        if (DataToLayer(theParams.mDataElements[1], &aLayer) == 0)
            return ReportError("Invalid Paramater Type");
        std::vector<int> aValues;
        if (DataToIntVector(theParams.mDataElements[2], &aValues) == 0 || aValues.size() != 2)
            return ReportError("Invalid Paramater Type");
        aLayer->mOffset.mX = aValues[0];
        aLayer->mOffset.mY = aValues[1];
        return true;
    }

    if (aCommand == "layersetcharwidths")
    {
        if (theParams.mDataElements.size() != 4)
            return ReportError("Invalid Number of Parameters");
        FontLayer* aLayer = nullptr;
        if (DataToLayer(theParams.mDataElements[1], &aLayer) == 0)
            return ReportError("Invalid Paramater Type");
        std::vector<Sexy::String> aChars;
        std::vector<int> aWidths;
        if (DataToStringVector(theParams.mDataElements[2], &aChars) == 0
            || DataToIntVector(theParams.mDataElements[3], &aWidths) == 0)
            return ReportError("Invalid Paramater Type");
        if (aChars.size() != aWidths.size())
            return ReportError("List Size Mismatch");
        for (unsigned int i = 0; i < aChars.size(); ++i)
        {
            if (aChars[i].length() != 1)
                return ReportError("Invalid Paramater Type");
            CharData* aGlyph = aLayer->SetCharData((unsigned char)aChars[i][0]);
            aGlyph->mAdvance = (unsigned short)aWidths[i];
        }
        return true;
    }

    if (aCommand == "layersetimage")
    {
        if (theParams.mDataElements.size() < 3)
            return ReportError("Invalid Number of Parameters");
        FontLayer* aLayer = nullptr;
        if (DataToLayer(theParams.mDataElements[1], &aLayer) == 0)
            return ReportError("Invalid Paramater Type");
        Sexy::String aImageName;
        if (DataToString(theParams.mDataElements[2], &aImageName) == 0)
            return ReportError("Invalid Paramater Type");
        // The name is already narrow. Join it with the font file's directory
        // so data/BrianneTod16.txt plus BrianneTod16 is data/BrianneTod16.
        Sexy::String aDir;
        GetFileDir(&aDir, &mFileName, 1, 0);
        Sexy::String aPath = aDir + aImageName;

        SharedImageRef aImage;
        SexyAppBase* anApp = gSexyAppBase;
        ResourceManager* aRes = anApp != nullptr ? anApp->mResourceManager : nullptr;
        if (aRes != nullptr && mImageGroup.empty())
        {
            Sexy::String aId = aRes->GetIdByPath(aPath);
            if (!aId.empty())
            {
                aImage = aRes->GetImage(aId);
                if (aImage.mImage == nullptr)
                    aImage = aRes->LoadImage(aId);
                if (aImage.mImage == nullptr)
                    return ReportError("Failed to Load Image");
            }
        }
        if (aImage.mImage == nullptr && mApp != nullptr)
        {
            Sexy::String aVariant;
            aImage = mApp->GetSharedImage(aPath, aVariant, nullptr, true, false);
        }
        if (aImage.mImage == nullptr)
            return ReportError("Failed to Load Image");
        aLayer->mImage = aImage;
        aLayer->mImageFileName = aPath;
        return true;
    }

    if (aCommand == "layersetimagepixelformat")
    {
        if (theParams.mDataElements.size() != 3)
            return ReportError("Invalid Number of Parameters");
        FontLayer* aLayer = nullptr;
        if (DataToLayer(theParams.mDataElements[1], &aLayer) == 0)
            return ReportError("Invalid Paramater Type");
        Sexy::String aFormat;
        if (DataToString(theParams.mDataElements[2], &aFormat) == 0)
            return ReportError("Invalid Paramater Type");
        int aValue = PixelUtil::GetPixelFormat(aFormat);
        if (aValue != 0)
            aLayer->mUseAlphaCorrection = aValue;
        return true;
    }

    if (aCommand == "layersetcolormult" || aCommand == "layersetcoloradd")
    {
        if (theParams.mDataElements.size() != 3)
            return ReportError("Invalid Number of Parameters");
        FontLayer* aLayer = nullptr;
        if (DataToLayer(theParams.mDataElements[1], &aLayer) == 0)
            return ReportError("Invalid Paramater Type");
        Color& aTarget = aCommand == "layersetcolormult" ? aLayer->mColorMult : aLayer->mColorAdd;
        if (GetColorFromDataElement(theParams.mDataElements[2], aTarget) == 0)
            return ReportError("Invalid Paramater Type");
        return true;
    }

    if (aCommand == "layersetcharoffsets")
    {
        if (theParams.mDataElements.size() != 4)
            return ReportError("Invalid Number of Parameters");
        std::vector<Sexy::String> aChars;
        ListDataElement aOffsets;
        FontLayer* aLayer = nullptr;
        if (DataToLayer(theParams.mDataElements[1], &aLayer) == 0
            || DataToStringVector(theParams.mDataElements[2], &aChars) == 0
            || DataToList(theParams.mDataElements[3], &aOffsets) == 0)
            return ReportError("Invalid Paramater Type");
        if (aChars.size() != aOffsets.mDataElements.size())
            return ReportError("List Size Mismatch");
        for (unsigned int i = 0; i < aChars.size(); ++i)
        {
            std::vector<int> aPair;
            if (aChars[i].length() != 1
                || DataToIntVector(aOffsets.mDataElements[i], &aPair) == 0
                || aPair.size() != 2)
                return ReportError("Invalid Paramater Type");
            CharData* aGlyph = aLayer->GetCharData((unsigned char)aChars[i][0]);
            aGlyph->mOffsetX = (unsigned short)aPair[0];
            aGlyph->mOffsetY = (unsigned short)aPair[1];
        }
        return true;
    }

    if (aCommand == "layersetkerningpairs")
    {
        if (theParams.mDataElements.size() != 4)
            return ReportError("Invalid Number of Parameters");
        std::vector<Sexy::String> aPairs;
        std::vector<int> aAmounts;
        FontLayer* aLayer = nullptr;
        if (DataToLayer(theParams.mDataElements[1], &aLayer) == 0
            || DataToStringVector(theParams.mDataElements[2], &aPairs) == 0
            || DataToIntVector(theParams.mDataElements[3], &aAmounts) == 0)
            return ReportError("Invalid Paramater Type");
        if (aPairs.size() != aAmounts.size())
            return ReportError("List Size Mismatch");

        std::vector<SortedKern> aKerns;
        for (unsigned int i = 0; i < aPairs.size(); ++i)
        {
            if (aPairs[i].length() != 2)
                return ReportError("Invalid Paramater Type");
            SortedKern aKern;
            aKern.mChar = (unsigned char)aPairs[i][0];
            aKern.mSecond = (unsigned char)aPairs[i][1];
            aKern.mAmount = aAmounts[i];
            aKerns.push_back(aKern);
        }
        if (!aKerns.empty())
            qsort(aKerns.data(), aKerns.size(), sizeof(SortedKern), SortedKern::Compare);

        aLayer->mKerningValues.resize(aKerns.size());
        for (unsigned int j = 0; j < aKerns.size(); ++j)
        {
            aLayer->mKerningValues[j].mSecondChar = aKerns[j].mSecond;
            aLayer->mKerningValues[j].mAmount = aKerns[j].mAmount;
            CharData* aGlyph = aLayer->GetCharData(aKerns[j].mChar);
            if (aGlyph->mKernCount == 0)
                aGlyph->mKernIndex = (unsigned short)j;
            aGlyph->mKernCount = (unsigned short)(aGlyph->mKernCount + 1);
        }
        return true;
    }

    if (aCommand == "layersetcharorders")
    {
        if (theParams.mDataElements.size() != 4)
            return ReportError("Invalid Number of Parameters");
        std::vector<Sexy::String> aChars;
        std::vector<int> aOrders;
        FontLayer* aLayer = nullptr;
        if (DataToLayer(theParams.mDataElements[1], &aLayer) == 0
            || DataToStringVector(theParams.mDataElements[2], &aChars) == 0
            || DataToIntVector(theParams.mDataElements[3], &aOrders) == 0)
            return ReportError("Invalid Paramater Type");
        if (aChars.size() != aOrders.size())
            return ReportError("List Size Mismatch");
        for (unsigned int i = 0; i < aChars.size(); ++i)
        {
            if (aChars[i].length() != 1)
                return ReportError("Invalid Paramater Type");
            CharData* aGlyph = aLayer->GetCharData((unsigned char)aChars[i][0]);
            aGlyph->mCharOrder = (unsigned short)aOrders[i];
        }
        return true;
    }

    if (aCommand == "layersetexinfo")
    {
        if (theParams.mDataElements.size() != 4)
            return ReportError("Invalid Number of Parameters");
        std::vector<Sexy::String> aKeys;
        std::vector<Sexy::String> aValues;
        FontLayer* aLayer = nullptr;
        if (DataToLayer(theParams.mDataElements[1], &aLayer) == 0
            || DataToStringVector(theParams.mDataElements[2], &aKeys) == 0
            || DataToStringVector(theParams.mDataElements[3], &aValues) == 0)
            return ReportError("Invalid Paramater Type");
        if (aKeys.size() != aValues.size())
            return ReportError("List Size Mismatch");
        for (unsigned int i = 0; i < aKeys.size(); ++i)
            aLayer->mExtendedInfo[aKeys[i]] = aValues[i];
        return true;
    }

    if (aCommand == "layersetalphacorrection")
    {
        if (theParams.mDataElements.size() != 3)
            return ReportError("Invalid Number of Parameters");
        FontLayer* aLayer = nullptr;
        if (DataToLayer(theParams.mDataElements[1], &aLayer) == 0)
            return ReportError("Invalid Paramater Type");
        int aValue = 0;
        if (DataToInt(theParams.mDataElements[2], &aValue) == 0)
            return ReportError("Invalid Paramater Type");
        aLayer->mUseAlphaCorrection = aValue;
        return true;
    }

    if (aCommand == "layersetimagemap")
    {
        if (theParams.mDataElements.size() != 4)
            return ReportError("Invalid Number of Parameters");
        std::vector<Sexy::String> aChars;
        ListDataElement aRects;
        FontLayer* aLayer = nullptr;
        if (DataToLayer(theParams.mDataElements[1], &aLayer) == 0
            || DataToStringVector(theParams.mDataElements[2], &aChars) == 0
            || DataToList(theParams.mDataElements[3], &aRects) == 0)
            return ReportError("Invalid Paramater Type");
        if (aChars.size() != aRects.mDataElements.size())
            return ReportError("List Size Mismatch");
        if (aLayer->mImage.mImage == nullptr)
            return ReportError("Layer image not set");

        int aImageWidth = aLayer->mImage.mImage->GetWidth();
        int aImageHeight = aLayer->mImage.mImage->GetHeight();
        for (unsigned int i = 0; i < aChars.size(); ++i)
        {
            std::vector<int> aRect;
            if (aChars[i].length() != 1
                || DataToIntVector(aRects.mDataElements[i], &aRect) == 0
                || aRect.size() != 4)
                return ReportError("Invalid Paramater Type");
            short aX = (short)aRect[0];
            short aY = (short)aRect[1];
            short aW = (short)aRect[2];
            short aH = (short)aRect[3];
            if (aX < 0 || aY < 0)
            {
                aX = 0;
                aY = 0;
            }
            int aRight = aX + aW;
            if (aImageWidth < aRight)
                aRight = aImageWidth;
            int aBottom = aY + aH;
            if (aImageHeight < aBottom)
                aBottom = aImageHeight;
            if (aRight - aX < 0 || aBottom - aY < 0)
            {
                aX = 0;
                aY = 0;
                aW = 0;
                aH = 0;
            }
            else
            {
                aW = (short)(aRight - aX);
                aH = (short)(aBottom - aY);
            }
            CharData* aGlyph = aLayer->GetCharData((unsigned char)aChars[i][0]);
            aGlyph->mX = (unsigned short)aX;
            aGlyph->mY = (unsigned short)aY;
            aGlyph->mWidth = (unsigned short)aW;
            aGlyph->mHeight = (unsigned short)aH;
        }
        return true;
    }

    // Define Name Value. The name is stored upper-case. A list value is the
    // list itself; a single value is that token, or the define it names.
    if (aCommand == "define")
    {
        if (theParams.mDataElements.size() != 3)
            return ReportError("Invalid Number of Parameters");
        if (theParams.mDataElements[1]->mElementKind != 0)
            return ReportError("Invalid Paramater Type");

        SingleDataElement* aName = static_cast<SingleDataElement*>(theParams.mDataElements[1]);
        Sexy::String aKey;
        StringToUpper(&aKey, aName->mData.c_str());
        if (IsImmediate(aKey) != 0)
            return ReportError("Invalid Paramater Type");

        DataElement* aValueElement = theParams.mDataElements[2];
        DataElement* aStored = nullptr;
        if (aValueElement->mElementKind != 0)
        {
            ListDataElement* aValues = new ListDataElement();
            if (GetValues(static_cast<ListDataElement*>(aValueElement), aValues) == 0)
            {
                delete aValues;
                return false;
            }
            aStored = aValues;
        }
        else
        {
            SingleDataElement* aSingle = static_cast<SingleDataElement*>(aValueElement);
            DataElement* aResolved = Dereference(aSingle->mData);
            aStored = (aResolved != nullptr) ? aResolved->Duplicate() : aValueElement->Duplicate();
        }
        StoreDefine(mDefineMap, aKey, aStored);
        return true;
    }

    if (theParams.mDataElements.size() < 2)
        return false;

    SingleDataElement* aParam = static_cast<SingleDataElement*>(theParams.mDataElements[1]);
    if (aParam->mElementKind != 0)
    {
        ListDataElement* aValues = new ListDataElement();
        if (GetValues(static_cast<ListDataElement*>(theParams.mDataElements[1]), aValues) == 0)
        {
            delete aValues;
            return false;
        }
        StoreDefine(mDefineMap, aCommand, aValues);
        return true;
    }

    Sexy::String aKey;
    StringToUpper(&aKey, aParam->mData.c_str());
    if (IsImmediate(aKey) != 0)
        return false;

    DataElement* aValue = Dereference(aParam->mData);
    if (aValue == nullptr)
        aValue = aParam->Duplicate();
    else
        aValue = aValue->Duplicate();
    StoreDefine(mDefineMap, aKey, aValue);
    return true;
}
