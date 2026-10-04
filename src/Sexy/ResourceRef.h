// ResourceRef holds one BaseRes and yields the loaded object through conversion.
//
// No base class and no virtual function. Release and the conversion operators
// read the BaseRes at +0x00. A live resource (its kind word at +0x1C is 1)
// stores the loaded object at +100; otherwise the conversion yields null.
// The object is two words. Only the first is read by a body in this tree.

#pragma once

#include "ReconTypes.h"
#include "ResourceRecords.h"
#include "SharedImageRef.h"

namespace Sexy {

class ResourceRef
{
public:
    ResourceManager_BaseRes * mBaseRes; // +0x00
    int mRefPad;                        // +0x04

    ResourceRef();
    ResourceRef(const Sexy::ResourceRef & theOther);
    // Release, copy the BaseRes word, and raise that record's mRefCount.
    ResourceRef & operator=(const Sexy::ResourceRef & theOther);
    ~ResourceRef();

    bool HasResource();
    Sexy::String GetId();
    void Release();

    // +100 is the loaded object. Image kinds hand back the SharedImageRef stored
    // there; every other kind returns the word itself when mResType matches.
    const ResourceManager_ImageRes * AsImage() const
    {
        if (mBaseRes == nullptr || mBaseRes->mResType != ResType_Image)
            return nullptr;
        return static_cast<const ResourceManager_ImageRes *>(mBaseRes);
    }

    operator int() const
    {
        if (mBaseRes == nullptr || mBaseRes->mResType != ResType_Sound)
            return 0;
        return static_cast<const ResourceManager_SoundRes *>(mBaseRes)->mSoundId;
    }

    operator Sexy::SharedImageRef() const
    {
        const ResourceManager_ImageRes * anImage = AsImage();
        if (anImage == nullptr)
            return Sexy::SharedImageRef();
        return *(const Sexy::SharedImageRef *)(const void *)&anImage->mImage;
    }
    operator Sexy::Image*() const
    {
        const ResourceManager_ImageRes * anImage = AsImage();
        if (anImage == nullptr)
            return nullptr;
        return ((const Sexy::SharedImageRef *)(const void *)&anImage->mImage)->operator Sexy::Image*();
    }
    operator Sexy::MemoryImage*() const
    {
        const ResourceManager_ImageRes * anImage = AsImage();
        if (anImage == nullptr)
            return nullptr;
        return ((const Sexy::SharedImageRef *)(const void *)&anImage->mImage)->operator Sexy::MemoryImage*();
    }
    operator Sexy::DeviceImage*() const
    {
        const ResourceManager_ImageRes * anImage = AsImage();
        if (anImage == nullptr)
            return nullptr;
        return ((const Sexy::SharedImageRef *)(const void *)&anImage->mImage)->operator Sexy::DeviceImage*();
    }
    operator Sexy::Font*() const
    {
        if (mBaseRes == nullptr || mBaseRes->mResType != ResType_Font)
            return nullptr;
        return (Sexy::Font *)static_cast<const ResourceManager_FontRes *>(mBaseRes)->mFont;
    }
    operator Sexy::ImageFont*() const
    {
        if (mBaseRes == nullptr || mBaseRes->mResType != ResType_Font)
            return nullptr;
        return (Sexy::ImageFont *)static_cast<const ResourceManager_FontRes *>(mBaseRes)->mFont;
    }
    operator Sexy::PopAnim*() const
    {
        if (mBaseRes == nullptr || mBaseRes->mResType != ResType_Reanim)
            return nullptr;
        return (Sexy::PopAnim *)static_cast<const ResourceManager_PopAnimRes *>(mBaseRes)->mPopAnim;
    }
    operator Sexy::PIEffect*() const
    {
        if (mBaseRes == nullptr || mBaseRes->mResType != ResType_Particle)
            return nullptr;
        return (Sexy::PIEffect *)static_cast<const ResourceManager_PIEffectRes *>(mBaseRes)->mPIEffect;
    }
    operator Sexy::RenderEffectDefinition*() const
    {
        if (mBaseRes == nullptr || (int)mBaseRes->mResType != 5)
            return nullptr;
        return (Sexy::RenderEffectDefinition *)
            static_cast<const ResourceManager_RenderEffectRes *>(mBaseRes)->mRenderEffect;
    }
    operator Sexy::GenericResFile*() const
    {
        if (mBaseRes == nullptr || (int)mBaseRes->mResType != 6)
            return nullptr;
        return (Sexy::GenericResFile *)
            static_cast<const ResourceManager_GenericResFileRes *>(mBaseRes)->mGenericResFile;
    }
};

} // namespace Sexy

using Sexy::ResourceRef;
