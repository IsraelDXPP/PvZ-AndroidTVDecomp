#include "TodParticle.h"
#include "DataArrayMethods.h"

// The allocation path at 0x209690 registers the 152-byte particle state.
TodParticle::TodParticle() : mParticleEmitter(nullptr), mSyncState{}
{
    SyncBlockInfo aBlock;
    aBlock.mData = &mSyncState;
    aBlock.mSize = sizeof(mSyncState);
    mSyncBlocks.push_back(aBlock);
}

TodParticle::~TodParticle()
{
}
