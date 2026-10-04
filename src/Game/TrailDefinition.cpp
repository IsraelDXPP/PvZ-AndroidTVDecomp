#include "TrailDefinition.h"
#include <cstring>

TrailDefinition::TrailDefinition()
{
    mImage = nullptr;
    mMaxPoints = 2;
    mMinPointDistance = 1.0f;
    mTrailFlags = 0;
    mTrailDuration.mNodes = nullptr;
    mTrailDuration.mCountNodes = 0;
    mWidthOverLength.mNodes = nullptr;
    mWidthOverLength.mCountNodes = 0;
    mWidthOverTime.mNodes = nullptr;
    mWidthOverTime.mCountNodes = 0;
    mAlphaOverLength.mNodes = nullptr;
    mAlphaOverLength.mCountNodes = 0;
    mAlphaOverTime.mNodes = nullptr;
    mAlphaOverTime.mCountNodes = 0;
}

TrailDefinition::~TrailDefinition()
{
}
