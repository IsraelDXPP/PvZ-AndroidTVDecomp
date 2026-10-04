#include "ReanimationHolder.h"

#include "Reanimation.h"
#include "DataArrayMethods.h"
#include <cstring>

namespace {

const int kReanimationCapacity = 1024;
const int kReanimationStride = 220;

} // namespace

ReanimationHolder::ReanimationHolder() = default;

int ReanimationHolder::InitializeHolder()
{
    DataArrayInitialize(kReanimationCapacity, "reanims");
    // First key handed out. Keys are the high half of a slot id.
    mNextKey = 0x10001u;
    return 1;
}

void ReanimationHolder::DisposeHolder()
{
    DataArrayDispose();
}

ReanimationHolder::~ReanimationHolder()
{
    DisposeHolder();
}

Reanimation* ReanimationHolder::AllocReanimation(float theX, float theY, int theRenderOrder, ReanimationType theReanimationType)
{
    Reanimation* aReanim = DataArrayAlloc();
    aReanim->mReanimationHolder = this;
    aReanim->mRenderOrder = theRenderOrder;
    aReanim->ReanimationInitializeType(theX, theY, theReanimationType);
    return aReanim;
}
