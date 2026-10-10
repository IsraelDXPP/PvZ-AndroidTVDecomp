// RenderEffectDefinition loads a compiled effect blob from memory or from disk.
// The disk path tries the shader-language subdirectory first, then the file
// beside the source path. The extension, without the dot, is the source type.

#include "RenderEffectDefinition.h"

#include "IGraphicsDriver.h"
#include "ReconHelpers.h"
#include "SexyAppBase.h"

#include <cstring>

namespace
{

Sexy::String FileDirectory(const char *thePath)
{
    Sexy::String aPath(thePath != nullptr ? thePath : "");
    Sexy::String aDir;
    GetFileDir(&aDir, &aPath, 1);
    return aDir;
}

Sexy::String FileNameOnly(const char *thePath)
{
    Sexy::String aPath(thePath);
    Sexy::String aName;
    GetFileName(&aName, &aPath, 1);
    return aName;
}

Sexy::String ExtensionWithoutDot(const Sexy::String &thePath)
{
    const char *aText = thePath.c_str();
    const char *aDot = nullptr;
    for (const char *aScan = aText; *aScan != '\0'; ++aScan)
    {
        if (*aScan == '.')
            aDot = aScan;
    }
    if (aDot == nullptr || aDot[1] == '\0')
        return Sexy::String();

    Sexy::String aExt(aDot + 1);
    Sexy::String aLower;
    StringToLower(&aLower, aExt.c_str());
    return aLower;
}

} // namespace

int Sexy::RenderEffectDefinition::LoadFromMem(
    unsigned long theByteCount, void const *theBytes, char const *theName, char const *theSourceFile)
{
    mData.resize(theByteCount);
    if (theByteCount != 0 && theBytes != nullptr)
        memcpy(mData.data(), theBytes, theByteCount);
    mName.assign(theName != nullptr ? theName : "");
    mSourceFile.assign(theSourceFile != nullptr ? theSourceFile : "");
    return 1;
}

int Sexy::RenderEffectDefinition::LoadFromFile(char const *thePath, char const *theName)
{
    if (thePath == nullptr)
        return 0;

    Sexy::String aFormat(".popfx");
    Sexy::String aLanguage;
    if (gSexyAppBase != nullptr && gSexyAppBase->mDDInterface != nullptr)
    {
        aFormat = gSexyAppBase->mDDInterface->GetRenderEffectFormat();
        aLanguage = gSexyAppBase->mDDInterface->GetShaderLanguage();
    }

    const Sexy::String aDir = FileDirectory(thePath);
    const Sexy::String aFile = FileNameOnly(thePath);

    Sexy::String aLanguagePath = aDir;
    if (!aLanguage.empty())
    {
        aLanguagePath += aLanguage;
        aLanguagePath += "\\";
    }
    aLanguagePath += aFile;
    aLanguagePath += aFormat;

    Sexy::String aPlainPath = aDir;
    aPlainPath += aFile;
    aPlainPath += aFormat;

    Sexy::Buffer aBuffer;
    const int aReadLanguage = gSexyAppBase->ReadBufferFromFile(aLanguagePath, &aBuffer, true);
    const int aReadPlain =
        (aReadLanguage != 0) ? aReadLanguage : gSexyAppBase->ReadBufferFromFile(aPlainPath, &aBuffer, true);
    if (aReadPlain == 0)
        return 0;

    const unsigned long aByteCount = (unsigned long)((aBuffer.mDataBitSize + 7) / 8);
    const void *aBytes = aBuffer.mData.empty() ? nullptr : aBuffer.mData.data();
    return LoadFromMem(aByteCount, aBytes, theName, ExtensionWithoutDot(aPlainPath).c_str());
}
