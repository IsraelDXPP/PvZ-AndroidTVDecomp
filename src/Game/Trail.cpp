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
        const float* aLast = mPoints[aNumPoints - 1];
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
bool Trail::GetNormalAtPoint(int theIndex, SexyVector2& theNormal)
{
    float aDirX;
    float aDirY;

    if (theIndex == 0)
    {
        aDirX = mPoints[1][0] - mPoints[0][0];
        aDirY = mPoints[0][1] - mPoints[1][1];
    }
    else if (mNumPoints - 1 == theIndex)
    {
        aDirX = mPoints[mNumPoints - 1][0] - mPoints[mNumPoints - 2][0];
        aDirY = mPoints[mNumPoints - 2][1] - mPoints[mNumPoints - 1][1];
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
void Trail::Draw(Sexy::Graphics* theGraphics)
{
    if (mDead)
        return;
    if (mNumPoints <= 1)
        return;

    const int aMaxVertices = 2 * 19 * 3;
    SexyVertex2D aVertices[aMaxVertices];
    for (int i = 0; i < aMaxVertices; ++i)
    {
        aVertices[i].mX = 0.0f;
        aVertices[i].mY = 0.0f;
        aVertices[i].mU = 0.0f;
        aVertices[i].mV = 0.0f;
        aVertices[i].mColor = 0;
    }

    int aNumTriangles = 2 * (mNumPoints - 1);
    float anAgeFrac = (float)mTrailAge / (float)(mTrailDuration - 1);

    const float* aPoint = &mPoints[0][0];
    SexyVertex2D* aVertex = aVertices;

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

            float aWidthOverLength = FloatTrackEvaluate(
                mDefinition->mWidthOverLength, aPointFrac, mTrailInterp[0]);
            float aNextWidthOverLength = FloatTrackEvaluate(
                mDefinition->mWidthOverLength, aNextFrac, mTrailInterp[0]);
            float aWidthOverTime = FloatTrackEvaluate(
                mDefinition->mWidthOverTime, anAgeFrac, mTrailInterp[1]);
            float anAlphaOverLength = FloatTrackEvaluate(
                mDefinition->mAlphaOverLength, aPointFrac, mTrailInterp[2]);
            float aNextAlphaOverLength = FloatTrackEvaluate(
                mDefinition->mAlphaOverLength, aNextFrac, mTrailInterp[2]);
            float anAlphaOverTime = FloatTrackEvaluate(
                mDefinition->mAlphaOverTime, anAgeFrac, mTrailInterp[3]);

            float aColorAlpha = (float)mColor.mAlpha;
            Color aPointColor(mColor.mRed, mColor.mGreen, mColor.mBlue,
                              AlphaToByte(anAlphaOverLength * anAlphaOverTime * aColorAlpha));
            Color aNextColor(mColor.mRed, mColor.mGreen, mColor.mBlue,
                             AlphaToByte(aNextAlphaOverLength * anAlphaOverTime * aColorAlpha));

            aHaveQuad = true;

            float aWidth = aWidthOverTime * aWidthOverLength;
            float aNextWidth = aWidthOverTime * aNextWidthOverLength;

            float aPointX = mOffsetX + aPoint[0];
            float aPointY = mOffsetY + aPoint[1];
            float aNextX = mOffsetX + aPoint[2];
            float aNextY = mOffsetY + aPoint[3];

            float aLeftX = aPointX - aPrevNormalX * aWidth;
            float aLeftY = aPointY - aPrevNormalY * aWidth;
            float aRightX = aPointX + aWidth * aPrevNormalX;
            float aRightY = aPointY + aWidth * aPrevNormalY;
            float aNextRightX = aNextX + aNextWidth * aNextNormal.mX;
            float aNextRightY = aNextY + aNextWidth * aNextNormal.mY;
            float aNextLeftX = aNextX - aNextNormal.mX * aNextWidth;
            float aNextLeftY = aNextY - aNextNormal.mY * aNextWidth;

            aVertex[0].mX = aRightX;
            aVertex[0].mY = aRightY;
            aVertex[0].mU = aPointFrac;
            aVertex[0].mV = 1.0f;
            aVertex[0].mColor = aPointColor.ToInt();

            aVertex[1].mX = aLeftX;
            aVertex[1].mY = aLeftY;
            aVertex[1].mU = aPointFrac;
            aVertex[1].mV = 0.0f;
            aVertex[1].mColor = aPointColor.ToInt();

            aVertex[2].mX = aNextRightX;
            aVertex[2].mY = aNextRightY;
            aVertex[2].mU = aNextFrac;
            aVertex[2].mV = 1.0f;
            aVertex[2].mColor = aNextColor.ToInt();

            aVertex[3].mX = aNextRightX;
            aVertex[3].mY = aNextRightY;
            aVertex[3].mU = aNextFrac;
            aVertex[3].mV = 1.0f;
            aVertex[3].mColor = aNextColor.ToInt();

            aVertex[4].mX = aLeftX;
            aVertex[4].mY = aLeftY;
            aVertex[4].mU = aPointFrac;
            aVertex[4].mV = 0.0f;
            aVertex[4].mColor = aPointColor.ToInt();

            aVertex[5].mX = aNextLeftX;
            aVertex[5].mY = aNextLeftY;
            aVertex[5].mU = aNextFrac;
            aVertex[5].mV = 0.0f;
            aVertex[5].mColor = aNextColor.ToInt();
        }
        else
        {
            aLastIndex = anIndex;
        }

        aPoint += 2;
        aVertex += 6;
        ++aPrevIndex;
        ++anIndex;
    }
    while (aLastIndex < mNumPoints - 1);

    theGraphics->DrawTrianglesTex(mDefinition->mImage, (const SexyVertex2D(*)[3])aVertices, aNumTriangles);
}

Trail::Trail()
{
    // Bytes +0x10..+0xF7 (232 bytes) are one synchronised block, then cleared.
    const int aSyncBlockSize = 232;
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
