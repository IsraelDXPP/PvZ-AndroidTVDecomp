// A loaded particle texture. The effect keeps one per image name. Nothing in
// this tree reads a field of it yet, so the eight words stay unnamed until a
// body does. The destructor is implicit: the out-of-line body is not linked.

#pragma once

#include "ReconTypes.h"
#include "SharedImageRef.h"

namespace Sexy {

class PITexture
{
public:
    int mWords[8];
};

} // namespace Sexy

using Sexy::PITexture;
