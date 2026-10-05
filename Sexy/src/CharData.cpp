#include "CharData.h"

// Clears the first twenty bytes. The three words after that are part of the
// object but this constructor does not touch them.
Sexy::CharData::CharData()
{
    mPackedOrigin = 0;
    mPackedSize = 0;
    mPackedOffset = 0;
    mPackedKern = 0;
    mPackedAdvance = 0;
}
