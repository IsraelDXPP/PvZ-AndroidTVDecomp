// One particle type an emitter can spawn. The first word is unused by the
// writer, the string is the type name, and the next word is the image index.
// Twenty-eight curves follow. The tail holds the spawn flags and the two
// key lists the file writes as CColorPoint and CAlphaPoint records.

#pragma once

#include "ReconTypes.h"
#include "Bezier.h"
#include "PIValue.h"
#include "PIInterpolator.h"

namespace Sexy {

class PIParticleDef
{
public:
    int mUnused000;
    Sexy::String mUnused004;
    int mUnused008;
    Sexy::PIValue mValues[28];
    int mSpawnFlags[19];
    Sexy::PIInterpolator mColorKeys;
    Sexy::PIInterpolator mAlphaKeys;

    PIParticleDef();
    PIParticleDef(Sexy::PIParticleDef const& other);
};

} // namespace Sexy

using Sexy::PIParticleDef;
