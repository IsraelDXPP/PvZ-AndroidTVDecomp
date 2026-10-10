// A ribbon: a short history of points that Draw turns into a triangle strip
// whose width and alpha come from the TrailDefinition's float tracks.

#include "Trail.h"
#include "DataArrayMethods.h"
#include "Graphics.h"
#include "TrailDefinition.h"
#include <cstring>
#include <cmath>

// The binary rounds a float alpha to the nearest integer and clamps it into the
// 0..255 byte range before it becomes a vertex colour.
static int AlphaToByte(float theAlpha)
{
    float aRounded = (theAlpha <= 0.0f) ? theAlpha - 0.5f : theAlpha + 0.5f;
    int anAlpha = (int)aRounded;
    if (anAlpha <= 0)
        return 0;
    if (anAlpha >= 255)
        return 255;
    return anAlpha;
}

Trail::~Trail()
{
    // Trail owns no heap object. SyncObject's destructor frees mSyncBlocks.
}

// Appends a point unless it is closer to the previous one than the definition's
// minimum distance. Once the ring is full the oldest point is dropped.
void Trail::AddPoint(float theX, float theY)
{
    int aMaxPoints = mDefinition->mMaxPoints;
    if (aMaxPoints < 2)
        aMaxPoints = 2;
    else if (aMaxPoints > 20)
        aMaxPoints = 20;

    int aNumPoints = mNumPoints;
    if (aNumPoints > 0)
    {
        const float *aLast = mPoints[aNumPoints - 1];
        float aDistX = aLast[0] - theX;
        float aDistY = aLast[1] - theY;
        float aDistance = sqrtf(aDistY * aDistY + aDistX * aDistX);
        if (mDefinition->mMinPointDistance > aDistance)
            return;

        if (aNumPoints == aMaxPoints)
        {
            memmove(&mPoints[0][0], &mPoints[1][0], 8 * (aNumPoints - 1));
            aNumPoints = mNumPoints - 1;
        }
    }

    mPoints[aNumPoints][0] = theX;
    mPoints[aNumPoints][1] = theY;
    mNumPoints = aNumPoints + 1;
}

void Trail::Update()
{
    if (++mTrailAge >= mTrailDuration)
    {
        // TrailFlags bit 0 makes the trail loop instead of expiring.
        if ((mDefinition->mTrailFlags & 1) != 0)
            mTrailAge = 0;
        else
            mDead = true;
    }
}

// The normal is the direction from the previous point to the next one, averaged
// from the two half-edges and normalised. The first and last points only have
// one neighbour. Returns false for a zero-length normal so the caller keeps
// the previous one.
bool Trail::GetNormalAtPoint(int theIndex, SexyVector2 &theNormal)
{
    float aDirX;
    float aDirY;

    if (theIndex == 0)
    {
        aDirX = mPoints[0][1] - mPoints[1][1];
        aDirY = mPoints[1][0] - mPoints[0][0];
    }
    else if (mNumPoints - 1 == theIndex)
    {
        aDirX = mPoints[mNumPoints - 2][1] - mPoints[mNumPoints - 1][1];
        aDirY = mPoints[mNumPoints - 1][0] - mPoints[mNumPoints - 2][0];
    }
    else
    {
        float aNextX = mPoints[theIndex + 1][0] - mPoints[theIndex][0];
        float aNextY = mPoints[theIndex + 1][1] - mPoints[theIndex][1];
        float aPrevX = mPoints[theIndex - 1][0] - mPoints[theIndex][0];
        float aPrevY = mPoints[theIndex - 1][1] - mPoints[theIndex][1];

        float aNextLength = sqrtf(aNextX * aNextX + aNextY * aNextY);
        if (aNextLength != 0.0f)
        {
            aNextX = aNextX / aNextLength;
            aNextY = aNextY / aNextLength;
        }

        float aPrevLength = sqrtf(aPrevX * aPrevX + aPrevY * aPrevY);
        if (aPrevLength != 0.0f)
        {
            aPrevX = aPrevX / aPrevLength;
            aPrevY = aPrevY / aPrevLength;
        }

        aDirX = aPrevX + aNextX;
        aDirY = aPrevY + aNextY;
    }

    float aLength = sqrtf(aDirX * aDirX + aDirY * aDirY);
    if (fabsf(aLength) < 0.000001f)
        return false;

    theNormal.mX = aDirX / aLength;
    theNormal.mY = aDirY / aLength;
    return true;
}

// One segment produces two triangles. Width and alpha are the product of the
// definition's length track at that segment and its time track at the trail's
// current age. mOffsetX/mOffsetY shift the ribbon in screen space.
void Trail::Draw(Sexy::Graphics *theGraphics)
{
    if (mDead)
        return;
    if (mNumPoints <= 1)
        return;

    const int aMaxTriangles = 2 * 19;
    SexyVertex2D aTriangles[aMaxTriangles][3]{};

    int aNumTriangles = 2 * (mNumPoints - 1);
    float anAgeFrac = (float)mTrailAge / (float)(mTrailDuration - 1);

    SexyVector2 aPrevNormal;
    aPrevNormal.mX = 0.0f;
    aPrevNormal.mY = 0.0f;
    SexyVector2 aNextNormal;

    int aPrevIndex = 0;
    int anIndex = 1;
    int aLastIndex = 1;
    bool aHaveQuad = false;

    do
    {
        if (aHaveQuad || GetNormalAtPoint(aPrevIndex, aPrevNormal))
        {
            SexyVertex2D *aFirstTriangle = aTriangles[2 * aPrevIndex];
            SexyVertex2D *aSecondTriangle = aTriangles[2 * aPrevIndex + 1];
            aNextNormal.mX = 0.0f;
            aNextNormal.mY = 0.0f;
            aLastIndex = anIndex;

            float aPrevNormalX = aPrevNormal.mX;
            float aPrevNormalY = aPrevNormal.mY;
            if (GetNormalAtPoint(anIndex, aNextNormal))
            {
                aPrevNormal = aNextNormal;
            }
            else
            {
                aNextNormal = aPrevNormal;
            }

            float aPointFrac = 1.0f - (float)aPrevIndex / (float)(mNumPoints - 1);
            float aNextFrac = 1.0f - (float)anIndex / (float)(mNumPoints - 1);

            float aWidthOverLength = FloatTrackEvaluate(mDefinition->mWidthOverLength, aPointFrac, mTrailInterp[0]);
            float aNextWidthOverLength = FloatTrackEvaluate(mDefinition->mWidthOverLength, aNextFrac, mTrailInterp[0]);
            float aWidthOverTime = FloatTrackEvaluate(mDefinition->mWidthOverTime, anAgeFrac, mTrailInterp[1]);
            float anAlphaOverLength = FloatTrackEvaluate(mDefinition->mAlphaOverLength, aPointFrac, mTrailInterp[2]);
            float aNextAlphaOverLength = FloatTrackEvaluate(mDefinition->mAlphaOverLength, aNextFrac, mTrailInterp[2]);
            float anAlphaOverTime = FloatTrackEvaluate(mDefinition->mAlphaOverTime, anAgeFrac, mTrailInterp[3]);

            float aColorAlpha = (float)mColor.mAlpha;
            Color aPointColor(mColor.mRed,
                mColor.mGreen,
                mColor.mBlue,
                AlphaToByte(anAlphaOverLength * anAlphaOverTime * aColorAlpha));
            Color aNextColor(mColor.mRed,
                mColor.mGreen,
                mColor.mBlue,
                AlphaToByte(aNextAlphaOverLength * anAlphaOverTime * aColorAlpha));

            aHaveQuad = true;

            float aWidth = aWidthOverTime * aWidthOverLength;
            float aNextWidth = aWidthOverTime * aNextWidthOverLength;

            float aPointX = mOffsetX + mPoints[aPrevIndex][0];
            float aPointY = mOffsetY + mPoints[aPrevIndex][1];
            float aNextX = mOffsetX + mPoints[anIndex][0];
            float aNextY = mOffsetY + mPoints[anIndex][1];

            float aLeftX = aPointX - aPrevNormalX * aWidth;
            float aLeftY = aPointY - aPrevNormalY * aWidth;
            float aRightX = aPointX + aWidth * aPrevNormalX;
            float aRightY = aPointY + aWidth * aPrevNormalY;
            float aNextRightX = aNextX + aNextWidth * aNextNormal.mX;
            float aNextRightY = aNextY + aNextWidth * aNextNormal.mY;
            float aNextLeftX = aNextX - aNextNormal.mX * aNextWidth;
            float aNextLeftY = aNextY - aNextNormal.mY * aNextWidth;

            aFirstTriangle[0].mX = aRightX;
            aFirstTriangle[0].mY = aRightY;
            aFirstTriangle[0].mU = aPointFrac;
            aFirstTriangle[0].mV = 1.0f;
            aFirstTriangle[0].mColor = aPointColor.ToInt();

            aFirstTriangle[1].mX = aLeftX;
            aFirstTriangle[1].mY = aLeftY;
            aFirstTriangle[1].mU = aPointFrac;
            aFirstTriangle[1].mV = 0.0f;
            aFirstTriangle[1].mColor = aPointColor.ToInt();

            aFirstTriangle[2].mX = aNextRightX;
            aFirstTriangle[2].mY = aNextRightY;
            aFirstTriangle[2].mU = aNextFrac;
            aFirstTriangle[2].mV = 1.0f;
            aFirstTriangle[2].mColor = aNextColor.ToInt();

            aSecondTriangle[0] = aFirstTriangle[2];

            aSecondTriangle[1] = aFirstTriangle[1];

            aSecondTriangle[2].mX = aNextLeftX;
            aSecondTriangle[2].mY = aNextLeftY;
            aSecondTriangle[2].mU = aNextFrac;
            aSecondTriangle[2].mV = 0.0f;
            aSecondTriangle[2].mColor = aNextColor.ToInt();
        }
        else
        {
            aLastIndex = anIndex;
        }

        ++aPrevIndex;
        ++anIndex;
    } while (aLastIndex < mNumPoints - 1);

    theGraphics->DrawTrianglesTex(mDefinition->mImage, aTriangles, aNumTriangles);
}

Trail::Trail()
{
    // Bytes +0x10..+0xF7 (232 bytes) are one synchronised block, then cleared.
    const size_t aSyncBlockSize = MakeSyncBlock(mPoints[0][0], mColor).mSize;
    SyncBlockInfo aBlock;
    aBlock.mData = &mPoints[0][0];
    aBlock.mSize = aSyncBlockSize;
    mSyncBlocks.push_back(aBlock);
    memset(&mPoints[0][0], 0, aSyncBlockSize);

    mNumPoints = 0;
    mDead = false;
    mRenderOrder = 0;
    mTrailAge = 0;
    mTrailDuration = 0;
    mDefinition = nullptr;
    mTrailHolder = nullptr;
    mOffsetX = 0.0f;
    mOffsetY = 0.0f;
    mIsAttachment = false;
    mColor = Color::White;

    for (int i = 0; i < 4; ++i)
        mTrailInterp[i] = Rand(1.0f);
}
