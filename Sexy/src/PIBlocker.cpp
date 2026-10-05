#include "PIBlocker.h"

#include "Bezier.h"
#include "PIValue.h"
#include "PIValue2D.h"

PIBlocker::PIBlocker(Sexy::PIBlocker const& other)
    : mName(other.mName),
      mFlag(other.mFlag),
      mOffset(other.mOffset),
      mActive(other.mActive),
      mSpin(other.mSpin),
      mPoints(other.mPoints)
{
}
