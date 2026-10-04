// PILayer looks up an emitter by index or by a case-insensitive name.

#include "PILayer.h"

#include <cstring>

void Sexy::PILayer::SetVisible(bool visible)
{
    mVisible = visible;
}

PILayerEmitter * Sexy::PILayer::GetEmitter(int theIndex)
{
    if (theIndex < 0 || theIndex >= (int)mEmitters.size())
        return nullptr;
    return &mEmitters[theIndex];
}

PILayerEmitter * Sexy::PILayer::GetEmitter(Sexy::String const& theName)
{
    if (mEmitters.empty())
        return nullptr;
    if (theName.empty())
        return &mEmitters[0];

    for (PILayerEmitter &emitter : mEmitters)
    {
        if (strcasecmp(emitter.mName.c_str(), theName.c_str()) == 0)
            return &emitter;
    }
    return nullptr;
}

Sexy::PILayer::PILayer()
    : mVisible(true)
{
    mColor = Sexy::Color::White;
    mTransform.LoadIdentity();
}
