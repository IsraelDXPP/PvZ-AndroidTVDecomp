// MTRand is a Mersenne Twister. There is no vtable. The state is the 624-word
// vector, the index into it, and the flag SetRandAllowed stores after the
// index. Next draws the next value; the overloads scale it into a range.
//
// unsigned long and unsigned int are the same width on the original target
// and distinct on this port, so both range overloads are declared. A call
// that passes a plain integer then matches the unsigned int form.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_MTRAND_H
#define LAWNPROJECTTV_SEXY_INCLUDE_MTRAND_H

#include "ReconTypes.h"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace Sexy
{

constexpr int MTRAND_N = 624;

class MTRand
{
public:
    unsigned long mt[MTRAND_N];
    int mti;
    bool mRandAllowed;
    unsigned char mPad9C5[3];

    MTRand *SetRandAllowed(bool theAllowed);
    MTRand *SRand(unsigned long theSeed);
    MTRand(unsigned long theSeed);
    MTRand();
    MTRand *SRand(Sexy::String const &theBlob);
    MTRand(Sexy::String const &theBlob);
    unsigned int NextNoAssert();
    unsigned int Next();
    unsigned int NextNoAssert(unsigned long theRange);
    unsigned int NextNoAssert(unsigned int theRange);
    unsigned int Next(unsigned long theRange);
    unsigned int Next(unsigned int theRange);
    float NextNoAssert(float theRange);
    float Next(float theRange);
    MTRand *Serialize();
};

} // namespace Sexy

using Sexy::MTRand;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_MTRAND_H
