// One emitter type: the 42 curves that drive spawn rate, life and motion, plus
// the particle definitions it can spawn. mSerialize is the first flag the file
// writes after the name. mEmitInReverse walks the definitions backwards.
// mHasLifeKeys is set when the first curve has at least one key.

#pragma once

#include "ReconTypes.h"
#include "Bezier.h"
#include "PIParticleDef.h"
#include "PIValue.h"
#include <vector>

namespace Sexy {

class PIEmitter
{
public:
    Sexy::String mName;
    Sexy::PIValue mValues[42];
    std::vector<Sexy::PIParticleDef> mParticleDefs;
    bool mSerialize;
    bool mEmitInReverse;
    bool mHasLifeKeys;

    PIEmitter()
        : mName(), mSerialize(false), mEmitInReverse(false), mHasLifeKeys(false)
    {
    }
};

} // namespace Sexy

using Sexy::PIEmitter;
