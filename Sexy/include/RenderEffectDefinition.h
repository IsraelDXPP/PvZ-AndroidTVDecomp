// A compiled render effect: the program bytes LoadFromMem fills, then the
// effect name and the file it was loaded from.

#pragma once

#include "ReconTypes.h"
#include "Buffer.h"
#include "SexyAppBase.h"

#include <vector>

namespace Sexy {

class RenderEffectDefinition
{
public:
    std::vector<unsigned char> mData;
    Sexy::String mName;
    Sexy::String mSourceFile;

    int LoadFromMem(unsigned long byteCount, void const* bytes, char const* name, char const* sourceFile);
    int LoadFromFile(char const* path, char const* name);
};

} // namespace Sexy

using Sexy::RenderEffectDefinition;
