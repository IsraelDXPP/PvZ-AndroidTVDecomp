// Reanimation -- reconstructed from libGameMain.so.
//
// The class has no PvZ-src counterpart, so every body below is rewritten by hand
// from the decompilation, function by function, against the member table in
// Reanimation.h (itself taken from PvZ\TodLib\Effect\Reanimator.h:165).
//
// Two records the binary uses are not in any header and are modelled here:
// ReanimatorTrackInstance (the same layout Game\Zombie.cpp carries). ReanimAtlas
// lives in ReanimAtlas.h.

#include "Reanimation.h"
#include "DataArrayMethods.h"
#include "RawObject.h"
#include "ReanimAtlas.h"
#include "TodAllocator.h"
#include "Font.h"
#include "LawnApp.h"
#include "Graphics.h"
#include "Image.h"
#include "ReanimationHolder.h"
#include "ReanimatorDefinition.h"
#include "ReanimatorTransform.h"
#include "ReconFreeFunctions.h"
#include "ReconTypes.h"
#include "SexyAppBase.h"
#include "SexyTransform2D.h"
#include "TodParticleSystem.h"
#include <cstring>
#include <cmath>

// The process-wide pools. Each slot is 20 bytes; the next symbol after the
// table sits 0xA00 bytes later, so the table holds 128 allocators.
TodAllocator gGlobalAllocators[128];
int gGlobalAllocatorCount = 0;

TodAllocator *FindGlobalAllocator(int theItemSize)
{
  for (int i = 0; i < gGlobalAllocatorCount; ++i)
  {
    if (gGlobalAllocators[i].mItemSize == theItemSize)
      return &gGlobalAllocators[i];
  }

  TodAllocator *aSlot = &gGlobalAllocators[gGlobalAllocatorCount];
  aSlot->mFreeList = nullptr;
  aSlot->mBlockList = nullptr;
  aSlot->mGrowCount = 16;
  aSlot->mTotalItems = 0;
  aSlot->mItemSize = theItemSize;
  ++gGlobalAllocatorCount;
  TodAllocatorGrow(aSlot);
  return aSlot;
}

void FreeGlobalAllocators()
{
  for (int i = 0; i < gGlobalAllocatorCount; ++i)
  {
    char *aBlock = static_cast<char *>(gGlobalAllocators[i].mBlockList);
    while (aBlock != nullptr)
    {
      void *aNext = TodAllocatorReadLink(aBlock);
      delete[] aBlock;
      aBlock = static_cast<char *>(aNext);
    }
    gGlobalAllocators[i].mBlockList = nullptr;
    gGlobalAllocators[i].mFreeList = nullptr;
    gGlobalAllocators[i].mTotalItems = 0;
  }
  gGlobalAllocatorCount = 0;
}




// ReanimatorTrackInstance lives in Reanimation.h so other translation units
// can name mTrackColor. The constructor replicates the binary's at 0x1F9584.
ReanimatorTrackInstance::ReanimatorTrackInstance()
{
    mBlendCounter = 0;
    mBlendTime = 0;
    mShakeOverride = 0.0f;
    mShakeX = 0.0f;
    mShakeY = 0.0f;
    mAttachmentID = 0;
    mImageOverride = nullptr;
    mRenderGroup = 0;
    mTrackColor = Sexy::Color::White;
    mIgnoreClipRect = false;
    mTruncateDisappearingFrames = true;
    mIgnoreColorOverride = false;
    mIgnoreExtraAdditiveColor = false;
}

// ReanimatorTrack lives in ReanimatorDefinition.h. gReanimationParamArray
// points at ReanimationParamRecord; the member table is
// PvZ\TodLib\Effect\Reanimator.h:80.
void ReanimAtlas::ReanimAtlasDispose()
{
    if (mImage != nullptr)
    {
        delete mImage;
        mImage = nullptr;
    }
    mNumAtlasImages = 0;
}

// ARM rounds by adding half and truncating toward zero. A negative value
// subtracts the half so the result is the nearest int.
static int RoundToNearestInt(float theValue)
{
    if (theValue <= 0.0f)
        return (int)(theValue - 0.5f);
    return (int)(theValue + 0.5f);
}

// CountPixelInTrack @0x1F7210. A hidden or empty frame contributes nothing.
// Otherwise the count is the cel area after the overlay scale and the track scale.
static int CountPixelInTrack(Reanimation* theReanim, int theTrackIndex)
{
    ReanimatorTransform aTransform;
    theReanim->GetCurrentTransform(theTrackIndex, &aTransform);

    if (RoundToNearestInt(aTransform.mFrame) < 0)
        return 0;

    float aAlpha = (float)theReanim->mColorOverride.mAlpha * aTransform.mAlpha;
    if (RoundToNearestInt(aAlpha) <= 0)
        return 0;

    Sexy::Image* anImage = aTransform.mImage;
    ReanimAtlas* anAtlas = theReanim->mDefinition->mReanimAtlas;
    if (anAtlas != nullptr)
    {
        ReanimAtlasImage* anAtlasImage = anAtlas->GetEncodedReanimAtlas(anImage);
        if (anAtlasImage != nullptr)
            anImage = anAtlasImage->mOriginalImage;
    }
    if (anImage == nullptr)
        return 0;

    int aWidth = RoundToNearestInt(
        theReanim->mOverlayMatrix.m00 * aTransform.mScaleX * (float)anImage->GetCelWidth());
    int aHeight = RoundToNearestInt(
        theReanim->mOverlayMatrix.m11 * aTransform.mScaleY * (float)anImage->GetCelHeight());
    return aHeight * aWidth;
}

// CalcReanimPerfInfo @0x1F731C. theImageCount is how many tracks drew pixels.
void CalcReanimPerfInfo(Reanimation* theReanim, int* theImageCount, int* thePixelCount)
{
    *theImageCount = 0;
    *thePixelCount = 0;
    int aTrackCount = theReanim->mDefinition->mTrackCount;
    for (int aTrackIndex = 0; aTrackIndex < aTrackCount; ++aTrackIndex)
    {
        int aPixels = CountPixelInTrack(theReanim, aTrackIndex);
        *thePixelCount += aPixels;
        if (aPixels > 0)
            ++*theImageCount;
    }
}

ReanimAtlasImage *ReanimAtlas::GetEncodedReanimAtlas(Sexy::Image *theImage)
{
    // The argument is used as a small atlas-image id, never dereferenced. Every
    // real Image pointer is far above 1000, so the shipped build always answers
    // "no encoded atlas" here; the arithmetic is the binary's.
    unsigned int anId = (unsigned int)theImage;
    if (anId == 0 || anId > 1000)
        return nullptr;
    return &mAtlasImages[anId - 1];
}

// ==========================================================================
// Reanimation::~Reanimation()
// @001FAC10  46 bytes
// ==========================================================================
Reanimation::~Reanimation()
{
  // Die first -- that detaches the animation from whatever was holding it --
  // and then release the frames this instance owns. ReanimationDelete frees the
  // object's own buffers; the SyncObject base frees mSyncBlocks.
  ReanimationDie();
  ReanimationDelete();
}

// ==========================================================================
// Reanimation::SetAnimRate(float)
// @001F9610  32 bytes
// ==========================================================================
int Reanimation::SetAnimRate(float theRate)
{
  mAnimRate = theRate;
  mAnimTimeStep = (theRate * 0.01f) / (float)mFrameCount;
  return (int)this;
}

// ==========================================================================
// Reanimation::GetTransformAtTime(int,ReanimatorTransform *,ReanimatorFrameTime *)
// @001F9750  340 bytes
// ==========================================================================
int Reanimation::GetTransformAtTime(int theTrackIndex, ReanimatorTransform * theTransform, int theFrameTime)
{
  ReanimatorFrameTime *aFrameTime = (ReanimatorFrameTime *)theFrameTime;
  ReanimatorTrackInstance *aTrackInstance = mTrackInstances + theTrackIndex;
  ReanimatorTransform *aTransform =
      mDefinition->mTracks[theTrackIndex].mTransforms + aFrameTime->mAnimFrameBeforeInt;
  ReanimatorTransform *aNextTransform =
      mDefinition->mTracks[theTrackIndex].mTransforms + aFrameTime->mAnimFrameAfterInt;
  float aFraction = aFrameTime->mFraction;

  if ( aFraction == 0.0f )
  {
    theTransform->mTransX = aTransform->mTransX;
    theTransform->mTransY = aTransform->mTransY;
    theTransform->mSkewX = aTransform->mSkewX;
    theTransform->mSkewY = aTransform->mSkewY;
    theTransform->mScaleX = aTransform->mScaleX;
    theTransform->mScaleY = aTransform->mScaleY;
  }
  else
  {
    theTransform->mTransX = aTransform->mTransX + (float)((float)(aNextTransform->mTransX - aTransform->mTransX) * aFraction);
    theTransform->mTransY = aTransform->mTransY + (float)((float)(aNextTransform->mTransY - aTransform->mTransY) * aFraction);
    theTransform->mSkewX = aTransform->mSkewX + (float)((float)(aNextTransform->mSkewX - aTransform->mSkewX) * aFraction);
    theTransform->mSkewY = aTransform->mSkewY + (float)((float)(aNextTransform->mSkewY - aTransform->mSkewY) * aFraction);
    theTransform->mScaleX = aTransform->mScaleX + (float)((float)(aNextTransform->mScaleX - aTransform->mScaleX) * aFraction);
    theTransform->mScaleY = aTransform->mScaleY + (float)((float)(aNextTransform->mScaleY - aTransform->mScaleY) * aFraction);
  }
  theTransform->mFrame = aTransform->mFrame;
  theTransform->mAlpha = aTransform->mAlpha + (float)(aFraction * (float)(aNextTransform->mAlpha - aTransform->mAlpha));
  theTransform->mImage = aTransform->mImage;
  theTransform->mImage2 = aTransform->mImage2;
  theTransform->mFont = aTransform->mFont;
  theTransform->mText = aTransform->mText;
  if ( aTransform->mFrame != -1.0f && aNextTransform->mFrame == -1.0f
    && aFrameTime->mFraction > 0.0f && aTrackInstance->mTruncateDisappearingFrames )
    theTransform->mFrame = -1.0f;
  return (int)aTransform;
}

// ==========================================================================
// Reanimation::GetCurrentTransform(int,ReanimatorFrameTime *,ReanimatorTransform *)
// @001F98A4  114 bytes
// ==========================================================================
int Reanimation::GetCurrentTransform(int theTrackIndex, int theFrameTime, ReanimatorTransform * theTransform)
{
  ReanimatorTrackInstance *aTrackInstance = mTrackInstances + theTrackIndex;
  int aFrame;

  GetTransformAtTime(theTrackIndex, theTransform, theFrameTime);
  if ( theTransform->mFrame <= 0.0f )
    aFrame = (int)(float)(theTransform->mFrame - 0.5f);
  else
    aFrame = (int)(float)(theTransform->mFrame + 0.5f);
  if ( aFrame >= 0 && aTrackInstance->mBlendCounter > 0 )
    BlendTransform(theTransform, theTransform, &aTrackInstance->mBlendTransform,
                   (float)aTrackInstance->mBlendCounter / (float)aTrackInstance->mBlendTime);
    return aFrame;
  return aFrame;
}

// ==========================================================================
// Reanimation::UpdateCurrentTransform(int,ReanimatorFrameTime *)
// @001F9918  22 bytes
// ==========================================================================
int Reanimation::UpdateCurrentTransform(int theTrackIndex, int theFrameTime)
{
  return GetCurrentTransform(theTrackIndex, theFrameTime, mReanimatorTransforms + theTrackIndex);
}

// ==========================================================================
// Reanimation::MatrixFromTransform(ReanimatorTransform &,Sexy::SexyMatrix3 &)
// @001F9930  144 bytes
// ==========================================================================
float Reanimation::MatrixFromTransform(ReanimatorTransform & theTransform, SexyMatrix3 & theMatrix)
{
  // Degrees to radians. The skew is applied as a negative rotation so the
  // track leans the way the reanim file records it.
  const float kDegreesToRadians = 0.017453f;
  float aSkewXRadians = -(kDegreesToRadians * theTransform.mSkewX);
  float aSkewYRadians = -(kDegreesToRadians * theTransform.mSkewY);
  float aCosX = cosf(aSkewXRadians);
  float aScaleX = theTransform.mScaleX;
  theMatrix.m00 = aCosX * aScaleX;
  float aSinX = sinf(aSkewXRadians);
  theMatrix.m20 = 0.0f;
  theMatrix.m10 = -(aSinX * aScaleX);
  float aSinY = sinf(aSkewYRadians);
  float aScaleY = theTransform.mScaleY;
  theMatrix.m01 = aSinY * aScaleY;
  float aCosY = cosf(aSkewYRadians);
  theMatrix.m21 = 0.0f;
  theMatrix.m11 = aCosY * aScaleY;
  theMatrix.m02 = theTransform.mTransX;
  theMatrix.m12 = theTransform.mTransY;
  theMatrix.m22 = 1.0f;
  return aCosY;
}

// ==========================================================================
// Reanimation::ReanimBltMatrix(Sexy::Graphics *,Sexy::Image *,Sexy::SexyMatrix3 const&,Sexy::TRect<int> const&,Sexy::Color const&,int,Sexy::TRect<int> const&)
// @001F99C4  530 bytes
// ==========================================================================
int Reanimation::ReanimBltMatrix(Sexy::Graphics * g, Sexy::Image * theImage, SexyMatrix3 const& theTransform, TRect<int> const& theClipRect, Color const& theColor, int theDrawMode, TRect<int> const& theSrcRect)
{
  ReanimationParamRecord *aParams = gReanimationParamArray;

  if ( ((SexyAppBase *)gSexyAppBase)->Is3DAccelerated() != 0
    || (aParams[mReanimationType].mFlags & 2) == 0
    || fabsf(theTransform.m01) >= 0.000001f
    || fabsf(theTransform.m10) >= 0.000001f
    || theTransform.m00 <= 0.0f
    || theTransform.m11 <= 0.0f
    || !(theColor == Sexy::Color::White) )
  {
    TodBltMatrix(g, theImage, theTransform, theClipRect, theColor, theDrawMode, theSrcRect);
    return 0;
  }
  float aScaleX = theTransform.m00;
  float aScaleY = theTransform.m11;
  // The transform's translation is the image center. The blit origin is half a
  // source rect back from that center, rounded to the nearest pixel.
  float aOriginX = theTransform.m02 - (aScaleX * 0.5f) * (float)theSrcRect.mWidth;
  int aDrawX = (int)(aOriginX <= 0.0f ? aOriginX - 0.5f : aOriginX + 0.5f);
  float aOriginY = theTransform.m12 - (aScaleY * 0.5f) * (float)theSrcRect.mHeight;
  float aDrawY = aOriginY <= 0.0f ? aOriginY - 0.5f : aOriginY + 0.5f;
  int aSavedDrawMode = g->GetDrawMode();
  g->SetDrawMode(theDrawMode);
  TRect<int> aSavedClip = g->mClipRect;
  g->SetClipRect(theClipRect);
  if ( fabsf(aScaleX - 1.0f) >= 0.000001f || fabsf(aScaleY - 1.0f) >= 0.000001f )
  {
    float aScaledWidth = (float)theSrcRect.mWidth * aScaleX;
    int aDrawWidth = (int)(aScaledWidth <= 0.0f ? aScaledWidth - 0.5f : aScaledWidth + 0.5f);
    float aScaledHeight = (float)theSrcRect.mHeight * aScaleY;
    float aDrawHeight = aScaledHeight <= 0.0f ? aScaledHeight - 0.5f : aScaledHeight + 0.5f;
    g->DrawImage(theImage, TRect<int>(aDrawX, (int)aDrawY, aDrawWidth, (int)aDrawHeight), theSrcRect);
  }
  else
  {
    g->DrawImage(theImage, aDrawX, (int)aDrawY, theSrcRect);
  }
  g->SetDrawMode(aSavedDrawMode);
  g->SetClipRect(aSavedClip);
  return (int)aDrawY;
}

// ==========================================================================
// Reanimation::SetFilterEffectShader(FilterEffect)
// @001F9BEC  2 bytes
// ==========================================================================
void Reanimation::SetFilterEffectShader()
{
}

// ==========================================================================
// Reanimation::DrawTrack(Sexy::Graphics *,int,int,TodTriangleGroup *)
// @001F9BF0  1742 bytes
// ==========================================================================
int Reanimation::DrawTrack(Sexy::Graphics * g, int theTrackIndex, int theRenderGroup, TodTriangleGroup *theTriangleGroup)
{
  ReanimatorTransform *aTransform = mReanimatorTransforms + theTrackIndex;
  ReanimatorTrackInstance *aTrackInstance = &mTrackInstances[theTrackIndex];
  Sexy::Color aColor = aTrackInstance->mTrackColor;
  Sexy::Color anAdditiveColor(0);
  Sexy::Color anOverlayColor;
  TRect<int> aClipRect;
  ReanimAtlasImage *anAtlasImage;
  Sexy::Image *aImage;
  Sexy::Image *aImage2;
  Sexy::SexyMatrix3 aMatrix;
  Sexy::SexyMatrix3 aTemp;
  bool aFullscreen = false;

  float aFrameValue = aTransform->mFrame;
  int aFrame = (int)(aFrameValue <= 0.0f ? aFrameValue - 0.5f : aFrameValue + 0.5f);
  if ( aFrame < 0 )
    return 0;

  if ( !aTrackInstance->mIgnoreColorOverride )
  {
    Sexy::Color aBlended;
    ColorsMultiply(&aBlended, &mColorOverride, &aColor);
    aColor = aBlended;
  }
  if ( g->mColorizeImages )
  {
    Sexy::Color aBlended;
    ColorsMultiply(&aBlended, &aColor, &g->mColor);
    aColor = aBlended;
  }
  float aScaledAlpha = (float)aColor.mAlpha * aTransform->mAlpha;
  int anAlpha = (int)(aScaledAlpha <= 0.0f ? aScaledAlpha - 0.5f : aScaledAlpha + 0.5f);
  if ( anAlpha <= 0 )
    return 0;
  if ( anAlpha >= 255 )
    anAlpha = 255;
  aColor.mAlpha = anAlpha;

  if ( mEnableExtraAdditiveDraw )
  {
    anAdditiveColor = mExtraAdditiveColor;
    anAdditiveColor.mAlpha = ColorComponentMultiply(mExtraAdditiveColor.mAlpha, anAlpha);
  }
  if ( mEnableExtraOverlayDraw )
  {
    anOverlayColor = mExtraOverlayColor;
    anOverlayColor.mAlpha = ColorComponentMultiply(mExtraOverlayColor.mAlpha, anAlpha);
  }

  aClipRect = g->mClipRect;
  if ( aTrackInstance->mIgnoreClipRect )
    aClipRect = TRect<int>(0, 0, 1280, 720);

  aImage = aTransform->mImage;
  ReanimAtlas *aAtlas = mDefinition->mReanimAtlas;
  aImage2 = aTransform->mImage2;
  if ( aAtlas != nullptr && aImage != nullptr )
  {
    anAtlasImage = aAtlas->GetEncodedReanimAtlas(aImage);
    if ( anAtlasImage != nullptr )
      aImage = anAtlasImage->mOriginalImage;
    if ( aTrackInstance->mImageOverride != nullptr )
    {
      anAtlasImage = nullptr;
      aImage2 = nullptr;
    }
  }
  else
  {
    // Outside the atlas path the track's own image override replaces both the
    // encoded atlas and the secondary image.
    anAtlasImage = nullptr;
    if ( aTrackInstance->mImageOverride != nullptr )
      aImage2 = nullptr;
  }

  if ( aImage != nullptr )
  {
    aMatrix.LoadIdentity();
    SexyMatrix3Translation(&aMatrix, (float)aImage->GetCelWidth() * 0.5f,
                           (float)aImage->GetCelHeight() * 0.5f);
  }
  else if ( aTransform->mFont != nullptr && aTransform->mText != nullptr
    && *aTransform->mText != 0 )
  {
    aMatrix.LoadIdentity();
    Sexy::Font *aFont = (Sexy::Font *)aTransform->mFont;
    Sexy::String aText;
    TodStringTranslate(&aText, aTransform->mText);
    SexyMatrix3Translation(&aMatrix, (float)-aFont->StringWidth(aText), (float)aFont->mAscent);
  }
  else if ( mDefinition->mTracks[theTrackIndex].mTrackId == ReanimTrackId_fullscreen )
  {
    aFullscreen = true;
  }
  else
  {
    return 0;
  }

  // A track that resolves to an encoded atlas rect draws that atlas instead of
  // the ordinary cel path; the group is flushed first when it does not.
  bool aDrawEncodedAtlas = (anAtlasImage != nullptr);
  if ( mDefinition->mReanimAtlas != nullptr && !aDrawEncodedAtlas )
    theTriangleGroup->DrawGroup(g);

  MatrixFromTransform(*aTransform, aTemp);
  SexyMatrix3Multiply(&aMatrix, aTemp, aMatrix);
  SexyMatrix3Multiply(&aMatrix, mOverlayMatrix, aMatrix);
  SexyMatrix3Translation(&aMatrix,
    (float)(aTrackInstance->mShakeX + g->mTransX) + -0.5f,
    (float)(g->mTransY + aTrackInstance->mShakeY) + -0.5f);

  if ( aDrawEncodedAtlas )
  {
    TRect<int> aAtlasRect(anAtlasImage->mX, anAtlasImage->mY,
                          anAtlasImage->mWidth, anAtlasImage->mHeight);
    Sexy::Image *anAtlasTexture = mDefinition->mReanimAtlas->mImage;
    if ( (int)mFilterEffect != -1 )
      SetFilterEffectShader();
    theTriangleGroup->AddTriangle(g, anAtlasTexture, aMatrix, aClipRect, aColor, (int)g->mDrawMode, aAtlasRect);
    if ( mEnableExtraAdditiveDraw && !aTrackInstance->mIgnoreExtraAdditiveColor )
      theTriangleGroup->AddTriangle(g, anAtlasTexture, aMatrix, aClipRect, anAdditiveColor, 1, aAtlasRect);
    if ( mEnableExtraOverlayDraw )
    {
      Sexy::Image *aFilteredImage = FilterEffectGetImage(anAtlasTexture, FilterEffect(2));
      theTriangleGroup->AddTriangle(g, aFilteredImage, aMatrix, aClipRect, anOverlayColor, 0, aAtlasRect);
    }
    SetFilterEffectShader();
    return 1;
  }

  if ( aImage != nullptr )
  {
    Sexy::Image *aDrawImage = aTrackInstance->mImageOverride;
    SetFilterEffectShader();
    if ( aDrawImage == nullptr )
      aDrawImage = aImage;
    int aCelCount = aDrawImage->mNumCols;
    while ( aFrame >= aCelCount )
      aFrame -= aCelCount;
    int aCelWidth = aDrawImage->GetCelWidth();
    TRect<int> aCelRect(aCelWidth * aFrame, 0, aCelWidth, aDrawImage->GetCelHeight());
    int aDrawMode = (int)g->mDrawMode;
    if ( aImage2 != 0 )
    {
      if ( aDrawMode != 0 || mEnableExtraAdditiveDraw )
        ReanimBltMatrix(g, aImage2, aMatrix, aClipRect, aColor, 0, aCelRect);
      else
        ReanimBltMatrix(g, aDrawImage, aMatrix, aClipRect, aColor, mEnableExtraAdditiveDraw, aCelRect);
    }
    else
    {
      ReanimBltMatrix(g, aDrawImage, aMatrix, aClipRect, aColor, aDrawMode, aCelRect);
      if ( mEnableExtraAdditiveDraw )
        ReanimBltMatrix(g, aDrawImage, aMatrix, aClipRect, anAdditiveColor, 1, aCelRect);
    }
  }
  else
  {
    Sexy::Font *aFont = (Sexy::Font *)aTransform->mFont;
    if ( aFont != nullptr && aTransform->mText != nullptr && *aTransform->mText != 0 )
    {
      Sexy::String aText(aTransform->mText);
      TodDrawStringMatrix(g, aFont, aMatrix, aText, aColor);
      if ( mEnableExtraAdditiveDraw )
      {
        int aDrawMode = (int)g->mDrawMode;
        g->SetDrawMode(1);
        Sexy::String anAdditiveText(aTransform->mText);
        TodDrawStringMatrix(g, aFont, aMatrix, anAdditiveText, anAdditiveColor);
        g->SetDrawMode(aDrawMode);
      }
    }
    else if ( aFullscreen )
    {
      Sexy::Color aSavedColor = g->GetColor();
      g->SetColor(aColor);
      g->FillRect((int)-g->mTransX, (int)-g->mTransY, LawnApp::FULLSCREEN_RECT.mWidth, LawnApp::FULLSCREEN_RECT.mHeight);
      g->SetColor(aSavedColor);
    }
  }
  SetFilterEffectShader();
  return 1;
}

// ==========================================================================
// Reanimation::GetFrameTime(ReanimatorFrameTime *)
// @001FA2DC  128 bytes
// ==========================================================================
float Reanimation::GetFrameTime(int theFrameTime)
{
  ReanimatorFrameTime *aFrameTime = (ReanimatorFrameTime *)theFrameTime;
  // The two "full last frame" loop types hold one frame back at the end of the
  // clip, so the pair the fraction interpolates between shifts by one.
  int aFrameCount;
  int anEndFrame;
  if ( mLoopType == 1 || mLoopType == 4 || mLoopType == 5 )
  {
    aFrameCount = mFrameCount;
    anEndFrame = mFrameCount;
  }
  else
  {
    aFrameCount = mFrameCount - 1;
    anEndFrame = mFrameCount;
  }

  int aStartFrame = mFrameStart;
  float aTime = (float)aStartFrame + (float)aFrameCount * mAnimTime;
  float aFloored = floorf(aTime);
  float aRounded = aFloored <= 0.0f ? aFloored - 0.5f : aFloored + 0.5f;
  aFrameTime->mFraction = aTime - aFloored;

  int aLastFrame = aStartFrame + anEndFrame - 1;
  int aFrame = (int)aRounded;
  if ( aLastFrame <= aFrame )
  {
    aFrameTime->mAnimFrameBeforeInt = aLastFrame;
    aFrameTime->mAnimFrameAfterInt = aLastFrame;
  }
  else
  {
    aFrameTime->mAnimFrameBeforeInt = aFrame;
    aFrameTime->mAnimFrameAfterInt = aFrame + 1;
  }
  return aFloored;
}

// ==========================================================================
// Reanimation::GetCurrentTransform(int,ReanimatorTransform *)
// @001FA35C  124 bytes
// ==========================================================================
int Reanimation::GetCurrentTransform(int theTrackIndex, ReanimatorTransform * theTransform)
{
  GetFrameTime((int)&mFrameTime);
  GetTransformAtTime(theTrackIndex, theTransform, (int)&mFrameTime);

  ReanimatorTrackInstance *aTrackInstance = &mTrackInstances[theTrackIndex];
  int aFrame = (int)(theTransform->mFrame <= 0.0f ? theTransform->mFrame - 0.5f : theTransform->mFrame + 0.5f);
  if ( aFrame >= 0 && aTrackInstance->mBlendCounter > 0 )
    BlendTransform(theTransform, theTransform, &aTrackInstance->mBlendTransform,
                   (float)aTrackInstance->mBlendCounter / (float)aTrackInstance->mBlendTime);
    return aFrame;
  return aFrame;
}

// ==========================================================================
// Reanimation::GetTrackMatrix(int,Sexy::SexyTransform2D &)
// @001FA3D8  304 bytes
// ==========================================================================
void Reanimation::GetTrackMatrix(int theTrackIndex, Sexy::SexyTransform2D & theMatrix)
{
  ReanimatorTrackInstance *aTrackInstance = &mTrackInstances[theTrackIndex];
  ReanimatorTransform aTransform;
  ReanimatorTransform aCurrentTransform;
  SexyTransform2D aTemp;

  GetCurrentTransform(theTrackIndex, &aTransform);
  int aFrame = (int)(aTransform.mFrame <= 0.0f ? aTransform.mFrame - 0.5f : aTransform.mFrame + 0.5f);
  Sexy::Image *aImage = aTransform.mImage;
  ReanimAtlas *aAtlas = mDefinition->mReanimAtlas;
  if ( aAtlas != nullptr && aImage != nullptr )
  {
    ReanimAtlasImage *anAtlasImage = aAtlas->GetEncodedReanimAtlas(aImage);
    if ( anAtlasImage != nullptr )
      aImage = anAtlasImage->mOriginalImage;
  }

  theMatrix.LoadIdentity();
  if ( aImage != nullptr && aFrame >= 0 )
  {
    SexyMatrix3Translation(&theMatrix, (float)aImage->GetCelWidth() * 0.5f,
                           (float)aImage->GetCelHeight() * 0.5f);
  }
  else if ( aTransform.mFont != nullptr && *aTransform.mText != 0 )
  {
    // A text track is positioned from the font's ascent, measured from its top.
    Sexy::Font *aFont = (Sexy::Font *)aTransform.mFont;
    SexyMatrix3Translation(&theMatrix, 0.0f, (float)aFont->mAscent);
  }

  MatrixFromTransform(aTransform, aTemp);
  SexyMatrix3Multiply(&theMatrix, aTemp, theMatrix);
  SexyMatrix3Multiply(&theMatrix, mOverlayMatrix, theMatrix);
  SexyMatrix3Translation(&theMatrix, aTrackInstance->mShakeX - 0.5f,
                         aTrackInstance->mShakeY - 0.5f);
}

// ==========================================================================
// Reanimation::FindTrackIndexById(char *)
// @001FA508  46 bytes
// ==========================================================================
int Reanimation::FindTrackIndexById(const char* theTrackId)
{
  // The id is the interned track-name pointer, compared by identity.
  for ( int i = 0; i < mDefinition->mTrackCount; ++i )
  {
    if ( strcmp(mDefinition->mTracks[i].mTrackId, theTrackId) ==  0)
      return i;
  }
  return -1;
}

// ==========================================================================
// Reanimation::GetCurrentTrackImage(char *)
// @001FA538  54 bytes
// ==========================================================================
Sexy::Image * Reanimation::GetCurrentTrackImage(char *theTrackName)
{
  int aTrackIndex = FindTrackIndexById(theTrackName);
  ReanimatorTransform aTransform;
  GetCurrentTransform(aTrackIndex, &aTransform);

  Sexy::Image *aImage = aTransform.mImage;
  ReanimAtlas *aAtlas = mDefinition->mReanimAtlas;
  if ( aAtlas != nullptr && aImage != nullptr )
  {
    ReanimAtlasImage *anAtlasImage = aAtlas->GetEncodedReanimAtlas(aImage);
    if ( anAtlasImage != nullptr )
      return anAtlasImage->mOriginalImage;
  }
  return aImage;
}

// ==========================================================================
// Reanimation::FindTrackIndex(char const*)
// @001FA570  62 bytes
// ==========================================================================
int Reanimation::FindTrackIndex(char const* s2)
{
  ReanimatorTrack *aTracks = mDefinition->mTracks;
  int aTrackCount = mDefinition->mTrackCount;
  if ( aTrackCount <= 0 )
    return -1;

  for ( int i = 0; i < aTrackCount; ++i )
  {
    if ( strcasecmp(aTracks[i].mName, s2) == 0 )
      return i;
  }
  return -1;
}

// ==========================================================================
// Reanimation::GetCurrentTrackImage(char const*)
// @001FA5B0  54 bytes
// ==========================================================================
Sexy::Image * Reanimation::GetCurrentTrackImage(char const* theTrackName)
{
  int aTrackIndex = FindTrackIndex(theTrackName);
  ReanimatorTransform aTransform;
  GetCurrentTransform(aTrackIndex, &aTransform);

  Sexy::Image *aImage = aTransform.mImage;
  ReanimAtlas *aAtlas = mDefinition->mReanimAtlas;
  if ( aAtlas != nullptr && aImage != nullptr )
  {
    ReanimAtlasImage *anAtlasImage = aAtlas->GetEncodedReanimAtlas(aImage);
    if ( anAtlasImage != nullptr )
      return anAtlasImage->mOriginalImage;
  }
  return aImage;
}

// ==========================================================================
// Reanimation::AttachToAnotherReanimation(Reanimation*,char const*)
// @001FA5E8  56 bytes
// ==========================================================================
void Reanimation::AttachToAnotherReanimation(Reanimation *theAttachReanim, const char *theTrackName)
{
  if ( theAttachReanim->mDefinition->mTrackCount == 0 )
    return;

  // The base pose of a reanimation that is attached to something is the frame
  // it was on, so it has to be pinned before the attachment reads it.
  if ( theAttachReanim->mFrameBasePose == -1 )
    theAttachReanim->mFrameBasePose = theAttachReanim->mFrameStart;

  int aTrackIndex = theAttachReanim->FindTrackIndex(theTrackName);
  AttachReanim(theAttachReanim->mTrackInstances[aTrackIndex].mAttachmentID, this, 0.0f, 0.0f);
}

// ==========================================================================
// Reanimation::GetTrackBasePoseMatrix(int,Sexy::SexyTransform2D &)
// @001FA620  72 bytes
// ==========================================================================
float Reanimation::GetTrackBasePoseMatrix(int theTrackIndex, Sexy::SexyTransform2D & theBaseMatrix)
{
  // A base pose of -2 means the track has no pose of its own and the caller gets
  // the identity; -1 defers to the animation's own start frame.
  int aBasePose = mFrameBasePose;
  if ( aBasePose == -2 )
  {
    theBaseMatrix.LoadIdentity();
    return 1.0f;
  }
  if ( aBasePose == -1 )
    aBasePose = mFrameStart;

  ReanimatorFrameTime aFrameTime;
  aFrameTime.mFraction = 0.0f;
  aFrameTime.mAnimFrameBeforeInt = aBasePose;
  aFrameTime.mAnimFrameAfterInt = aBasePose + 1;

  ReanimatorTransform aTransform;
  GetTransformAtTime(theTrackIndex, &aTransform, (int)&aFrameTime);
  return MatrixFromTransform(aTransform, *(SexyMatrix3 *)&theBaseMatrix);
}

// ==========================================================================
// Reanimation::AttachParticleToTrack(char const*,TodParticleSystem *,float,float)
// @001FA668  86 bytes
// ==========================================================================
AttachEffect * Reanimation::AttachParticleToTrack(char const* theTrackName, TodParticleSystem * theParticleSystem, float thePosX, float thePosY)
{
  int aTrackIndex = FindTrackIndex(theTrackName);
  ReanimatorTrackInstance *aTrackInstance = &mTrackInstances[aTrackIndex];

  SexyTransform2D aBasePose;
  GetTrackBasePoseMatrix(aTrackIndex, aBasePose);

  // The offset is applied in the track's own space, so it is a matrix multiply
  // rather than a sum of coordinates.
  SexyMatrix3 anOffset;
  anOffset.LoadIdentity();
  anOffset.m02 = thePosX;
  anOffset.m12 = thePosY;
  SexyMatrix3 aResult;
  SexyMatrix3Multiply(&aResult, aBasePose, anOffset);
  return AttachParticle(aTrackInstance->mAttachmentID, theParticleSystem, aResult.m02, aResult.m12);
}

// ==========================================================================
// Reanimation::GetAttachmentOverlayMatrix(int,ReanimatorTransform &,Sexy::SexyTransform2D &)
// @001FA6C0  100 bytes
// ==========================================================================
int Reanimation::GetAttachmentOverlayMatrix(int theTrackIndex, ReanimatorTransform & theTransform, Sexy::SexyTransform2D & theOverlayMatrix)
{
  SexyTransform2D aOverlay;
  MatrixFromTransform(theTransform, *(SexyMatrix3 *)&aOverlay);
  SexyMatrix3Multiply(&aOverlay, mOverlayMatrix, aOverlay);

  SexyTransform2D aBasePose;
  GetTrackBasePoseMatrix(theTrackIndex, aBasePose);

  SexyTransform2D aInverseBasePose;
  SexyMatrix3Inverse(aBasePose, aInverseBasePose);

  SexyMatrix3Multiply(&theOverlayMatrix, aOverlay, aInverseBasePose);
  return (int)&theOverlayMatrix;
}

// ==========================================================================
// Reanimation::GetAttachmentOverlayMatrix(int,Sexy::SexyTransform2D &)
// @001FA860  116 bytes
// ==========================================================================
// The two-argument form. It builds the same overlay the three-argument one
// takes as input, by pulling the track's current transform out of the
// reanimation itself first: the ARM allocates a ReanimatorTransform, calls
// GetCurrentTransform(theTrackIndex, &transform), and then runs the identical
// MatrixFromTransform / mOverlayMatrix multiply / base-pose inverse /
// multiply sequence. Written as that delegation plus the shared tail rather
// than duplicating the three-argument body, which is what the ARM does.
int Reanimation::GetAttachmentOverlayMatrix(int theTrackIndex, Sexy::SexyTransform2D & theOverlayMatrix)
{
  ReanimatorTransform aTransform;
  GetCurrentTransform(theTrackIndex, &aTransform);
  return GetAttachmentOverlayMatrix(theTrackIndex, aTransform, theOverlayMatrix);
}

// ==========================================================================
// Reanimation::Draw(Sexy::Graphics *)
// @001FA858  6 bytes
// ==========================================================================
// A one-instruction tail call: `MOVS R2,#0; B DrawRenderGroup`. Group 0 is the
// whole reanimation, which is why every caller that wants a specific group
// calls DrawRenderGroup directly.
float Reanimation::Draw(Sexy::Graphics * g)
{
  return DrawRenderGroup(g, 0);
}

// ==========================================================================
// Reanimation::IgnoreClipRectForPrefix(char const*,bool)
// @001FB0BC  94 bytes
// ==========================================================================
// Sets mIgnoreClipRect on every track whose name begins with `s`. The ARM is
// strlen(s) once, then a walk over the track-instance array in strides of 20
// ints (0x14 * count, which is the 100-byte ReanimatorTrackInstance the
// authoritative Reanimator.h describes). Per track it reads the name at +4 and
// only calls strncasecmp when the track name is at least as long as the prefix,
// so a shorter name cannot match; on a match it writes the bool to +96, the
// record's own mIgnoreClipRect. The returned pointer is whatever the last
// strncasecmp left in r0, which no caller reads.
const char * Reanimation::IgnoreClipRectForPrefix(char const* theTrackName, bool theIgnore)
{
  size_t aPrefixLength = strlen(theTrackName);
  int aTrackCount = mDefinition->mTrackCount;
  if ( aTrackCount > 0 )
  {
    for ( int i = 0; i < aTrackCount; ++i )
    {
      const char *aTrackName = mDefinition->mTracks[i].mName;
      if ( strlen(aTrackName) >= aPrefixLength
           && strncasecmp(aTrackName, theTrackName, aPrefixLength) == 0 )
        mTrackInstances[i].mIgnoreClipRect = theIgnore;
    }
  }
  return theTrackName;
}

// ==========================================================================
// Reanimation::CheckDirtyFlags()
// @001FA724  198 bytes
// ==========================================================================
// The dirty flags: bit 0 means the frame-time record is stale, bit 1 that the
// transforms are, bit 2 that the attachments are, and +0xD0 records that the
// dirty pass has already run for this object.
float Reanimation::CheckDirtyFlags()
{
  float aResult = 0.0f;
  if ( mFrameCount != 0 && !mDead )
  {
    int aDirty = mDirtyFlags;
    if ( aDirty != 0 )
    {
      if ( (aDirty & 1) != 0 )
      {
        aResult = GetFrameTime((int)&mFrameTime);
        aDirty = mDirtyFlags & ~1;
        mDirtyFlags = aDirty;
      }
      if ( (aDirty & 6) != 0 )
      {
        for ( int i = 0; i < mDefinition->mTrackCount; ++i )
        {
          ReanimatorTrackInstance *aTrackInstance = &mTrackInstances[i];
          if ( aTrackInstance->mRenderGroup != -1 || aTrackInstance->mAttachmentID != 0 )
          {
            if ( (aDirty & 4) != 0 )
              aResult = (float)UpdateCurrentTransform(i, (int)&mFrameTime);
            if ( aTrackInstance->mAttachmentID != 0 )
            {
              SexyTransform2D anOverlay;
              GetAttachmentOverlayMatrix(i, mReanimatorTransforms[i], anOverlay);
              AttachmentSetMatrix(aTrackInstance->mAttachmentID, anOverlay);
            }
            aDirty = mDirtyFlags;
          }
        }
        mDirtyFlags = aDirty & ~6;
      }
      mFrameTimeReady = true;
    }
  }
  return aResult;
}

// ==========================================================================
// Reanimation::DrawRenderGroup(Sexy::Graphics *,int)
// @001FA7EC  106 bytes
// ==========================================================================
float Reanimation::DrawRenderGroup(Sexy::Graphics * g, int theRenderGroup)
{
  float aResult = 0.0f;
  if ( !mDead )
  {
    aResult = CheckDirtyFlags();
    for ( int i = 0; i < mDefinition->mTrackCount; ++i )
    {
      ReanimatorTrackInstance *aTrackInstance = &mTrackInstances[i];
      if ( aTrackInstance->mRenderGroup == theRenderGroup )
      {
        aResult = (float)DrawTrack(g, i, theRenderGroup, nullptr);
        if ( aTrackInstance->mAttachmentID != 0 )
          AttachmentDraw(aTrackInstance->mAttachmentID, g, ((int)aResult & 1) == 0);
      }
    }
  }
  return aResult;
}




// ==========================================================================
// Reanimation::GetFramesForLayer(char *,int &,int &)
// @001FA8D4  172 bytes
// ==========================================================================
int Reanimation::GetFramesForLayer(char *theTrackName, int & theFrameStart, int & theFrameCount)
{
  if ( mDefinition->mTrackCount == 0 )
  {
    theFrameStart = 0;
    theFrameCount = 0;
    return 0;
  }

  theFrameStart = 0;
  theFrameCount = 1;
  int aTrackIndex = FindTrackIndexById(theTrackName);
  if ( aTrackIndex == -1 )
    return 0;

  ReanimatorTrack *aTrack = &mDefinition->mTracks[aTrackIndex];
  int aTransformCount = aTrack->mTransformCount;
  if ( aTransformCount <= 0 )
    return 0;

  // A layer is showing from its first frame whose frame value is non-negative
  // through the last such frame. The compiled record stores that value at +0x18.
  ReanimatorTransform *aTransforms = aTrack->mTransforms;
  int aFrame = 0;
  if ( aTransforms[0].mFrame < 0.0f )
  {
    aFrame = 0;
    while ( ++aFrame != aTransformCount )
    {
      if ( aTransforms[aFrame].mFrame >= 0.0f )
        break;
    }
    if ( aFrame == aTransformCount )
      return 0;
  }

  theFrameStart = aFrame;
  int aCount = aTrack->mTransformCount;
  if ( aFrame < aCount )
  {
    do
    {
      float aValue = aTransforms[aFrame].mFrame;
      if ( aValue >= 0.0f )
        aCount = aFrame - theFrameStart;
      ++aFrame;
      if ( aValue >= 0.0f )
      {
        theFrameCount = aCount + 1;
        aCount = aTrack->mTransformCount;
      }
    }
    while ( aCount > aFrame );
  }
  return 0;
}

// ==========================================================================
// Reanimation::GetFramesForLayer(char const*,int &,int &)
// @001FA980  172 bytes
// ==========================================================================
int Reanimation::GetFramesForLayer(char const* theTrackName, int & theFrameStart, int & theFrameCount)
{
  if ( mDefinition->mTrackCount == 0 )
  {
    theFrameStart = 0;
    theFrameCount = 0;
    return 0;
  }

  theFrameStart = 0;
  theFrameCount = 1;
  int aTrackIndex = FindTrackIndex(theTrackName);
  if ( aTrackIndex == -1 )
    return 0;

  ReanimatorTrack *aTrack = &mDefinition->mTracks[aTrackIndex];
  int aTransformCount = aTrack->mTransformCount;
  if ( aTransformCount <= 0 )
    return 0;

  // A layer is showing from its first frame whose frame value is non-negative
  // through the last such frame. PlayReanim uses this count as the clip length.
  ReanimatorTransform *aTransforms = aTrack->mTransforms;
  int aFrame = 0;
  if ( aTransforms[0].mFrame < 0.0f )
  {
    aFrame = 0;
    while ( ++aFrame != aTransformCount )
    {
      if ( aTransforms[aFrame].mFrame >= 0.0f )
        break;
    }
    if ( aFrame == aTransformCount )
      return 0;
  }

  theFrameStart = aFrame;
  int aCount = aTrack->mTransformCount;
  if ( aFrame < aCount )
  {
    do
    {
      float aValue = aTransforms[aFrame].mFrame;
      if ( aValue >= 0.0f )
        aCount = aFrame - theFrameStart;
      ++aFrame;
      if ( aValue >= 0.0f )
      {
        theFrameCount = aCount + 1;
        aCount = aTrack->mTransformCount;
      }
    }
    while ( aCount > aFrame );
  }
  return 0;
}

// ==========================================================================
// Reanimation::SetFramesForLayer(char const*)
// @001FAA2C  74 bytes
// ==========================================================================
int Reanimation::SetFramesForLayer(char const* theTrackName)
{
  // A negative rate means the clip plays backwards. Switching layers then
  // starts one step before the end (the float just under 1) instead of at 0.
  float aStartTime = 0.0f;
  if ( mAnimRate < 0.0f )
    aStartTime = 0.99999994f;

  mFrameTimeReady = false;
  mLastFrameTime = -1.0f;
  mAnimTime = aStartTime;
  GetFramesForLayer((char *)theTrackName, mFrameStart, mFrameCount);
  return SetAnimRate(mAnimRate);
}

// ==========================================================================
// Reanimation::TrackExists(char const*)
// @001FAA80  62 bytes
// ==========================================================================
int Reanimation::TrackExists(char const* s2)
{
  return FindTrackIndex(s2) != -1;
}

// ==========================================================================
// Reanimation::TrackExistsById(char *)
// @001FAAC0  52 bytes
// ==========================================================================
int Reanimation::TrackExistsById(const char* theTrackId)
{
  for ( int i = 0; i < mDefinition->mTrackCount; ++i )
  {
    if ( mDefinition->mTracks[i].mTrackId == theTrackId )
      return 1;
  }
  return 0;
}

// ==========================================================================
// Reanimation::StartBlend(int)
// @001FAAF4  180 bytes
// ==========================================================================
int Reanimation::StartBlend(int theBlendTime)
{
  int aResult = 0;
  for ( int i = 0; i < mDefinition->mTrackCount; ++i )
  {
    ReanimatorTransform aTransform;
    aResult = GetCurrentTransform(i, &aTransform);
    int aFrame = (int)(aTransform.mFrame <= 0.0f ? aTransform.mFrame - 0.5f
                                                  : aTransform.mFrame + 0.5f);
    if ( aFrame >= 0 )
    {
      // The blend target is the track's current pose, and both the blend
      // counter and the blend time are seeded with the same length.
      ReanimatorTransform *aBlend = &mReanimatorTransforms[i];
      *aBlend = aTransform;
      aBlend->mFrame = 0.0f;
      aBlend->mAlpha = 0.0f;
      aBlend->mFont = nullptr;
      aBlend->mText = gEmptyCString;
    }
  }
  return aResult;
}

// ==========================================================================
// Reanimation::ReanimationDelete()
// @001FABAC  46 bytes
// ==========================================================================
TodAllocator * Reanimation::ReanimationDelete()
{
  if ( mTrackInstances == nullptr )
    return nullptr;

  // The two arrays were handed out by one pool whose element size is the track
  // instance, so they are returned together through that pool's free list.
  TodAllocator *aPool = FindGlobalAllocator(148 * mDefinition->mTrackCount);
  void *aBlock = mTrackInstances;
  void *aNext = aPool->mFreeList;
  --aPool->mTotalItems;
  TodAllocatorWriteLink(aBlock, aNext);
  aPool->mFreeList = aBlock;
  mTrackInstances = nullptr;
  mReanimatorTransforms = nullptr;
  return aPool;
}

// ==========================================================================
// Reanimation::ReanimationDie()
// @001FABDC  50 bytes
// ==========================================================================
int Reanimation::ReanimationDie()
{
  if ( mDead )
    return 0;

  mDead = true;
  for ( int i = 0; i < mDefinition->mTrackCount; ++i )
    AttachmentDie(mTrackInstances[i].mAttachmentID);
  return 0;
}

// ==========================================================================
// Reanimation::SetShakeOverride(char const*,float)
// @001FAC74  28 bytes
// ==========================================================================
int Reanimation::SetShakeOverride(char const* theTrackName, float theShakeAmount)
{
  int aTrackIndex = FindTrackIndex(theTrackName);
  mTrackInstances[aTrackIndex].mShakeOverride = theShakeAmount;
  mFrameTimeReady = false;
  return aTrackIndex;
}

// ==========================================================================
// Reanimation::GetImageOverride(char *)
// @001FAC90  20 bytes
// ==========================================================================
Sexy::Image * Reanimation::GetImageOverride(char *theTrackName)
{
  return mTrackInstances[FindTrackIndexById(theTrackName)].mImageOverride;
}

// ==========================================================================
// Reanimation::GetImageOverride(char const*)
// @001FACA4  20 bytes
// ==========================================================================
Sexy::Image * Reanimation::GetImageOverride(char const* theTrackName)
{
  return mTrackInstances[FindTrackIndex(theTrackName)].mImageOverride;
}

// ==========================================================================
// Reanimation::SetImageOverride(char *,Sexy::Image *)
// @001FACB8  22 bytes
// ==========================================================================
int Reanimation::SetImageOverride(char *theTrackName, Sexy::Image * theImage)
{
  int aTrackIndex = FindTrackIndexById(theTrackName);
  mTrackInstances[aTrackIndex].mImageOverride = theImage;
  return aTrackIndex;
}

// ==========================================================================
// Reanimation::SetImageOverride(char const*,Sexy::Image *)
// @001FACD0  22 bytes
// ==========================================================================
int Reanimation::SetImageOverride(char const* theTrackName, Sexy::Image * theImage)
{
  int aTrackIndex = FindTrackIndex(theTrackName);
  mTrackInstances[aTrackIndex].mImageOverride = theImage;
  return aTrackIndex;
}

// ==========================================================================
// Reanimation::SetTruncateDisappearingFrames(char const*,bool)
// @001FACE8  54 bytes
// ==========================================================================
int Reanimation::SetTruncateDisappearingFrames(char const* theTrackName, bool theTruncateDisappearingFrames)
{
  if ( theTrackName != nullptr )
  {
    mTrackInstances[FindTrackIndex(theTrackName)].mTruncateDisappearingFrames = theTruncateDisappearingFrames;
  }
  else
  {
    for ( int i = 0; i < mDefinition->mTrackCount; ++i )
      mTrackInstances[i].mTruncateDisappearingFrames = theTruncateDisappearingFrames;
  }
  return 0;
}

// ==========================================================================
// Reanimation::SetPosition(float,float)
// @001FADBC  30 bytes
// ==========================================================================
int Reanimation::SetPosition(float theX, float theY)
{
  mOverlayMatrix.m02 = theX;
  mOverlayMatrix.m12 = theY;
  mFrameTimeReady = false;
  mDirtyFlags |= 2;
  return 0;
}

// ReanimationCreateAtlas @0x1F95D8 is a return. The TV build does not pack an atlas here.
void ReanimationCreateAtlas(ReanimatorDefinition*, ReanimationType)
{
}

// ReanimationPreload @0x1F95DC. Pack the atlas, then sand its image when one exists.
void ReanimationPreload(ReanimationType theType)
{
  ReanimatorDefinition* aDefinition = &gReanimatorDefArray[theType];
  ReanimationCreateAtlas(aDefinition, theType);
  if (aDefinition->mReanimAtlas != nullptr)
    TodSandImageIfNeeded(aDefinition->mReanimAtlas->mImage);
}

// ==========================================================================
// Reanimation::ReanimationInitialize(float,float,ReanimatorDefinition *)
// @001FADDC  268 bytes
// ==========================================================================
ReanimatorTransform * Reanimation::ReanimationInitialize(float theX, float theY, ReanimatorDefinition * theDefinition)
{
  ReanimationCreateAtlas(theDefinition, mReanimationType);
  mDead = false;
  mDefinition = theDefinition;
  SetPosition(theX, theY);

  mAnimRate = theDefinition->mFPS;
  mLastFrameTime = -1.0f;
  if ( theDefinition->mTrackCount != 0 )
  {
    mFrameCount = theDefinition->mTracks[0].mTransformCount;
    SetAnimRate(mAnimRate);

    // One pool serves both arrays: the track instances first, then the
    // transforms, and the pool's item size is the track instance's 148 bytes.
    int aTrackCount = mDefinition->mTrackCount;
    TodAllocator *aPool = FindGlobalAllocator(148 * aTrackCount);
    ReanimatorTrackInstance *aTrackInstances = static_cast<ReanimatorTrackInstance *>(TodAllocatorAlloc(aPool));
    memset(aTrackInstances, 0, 148 * aTrackCount);
    mTrackInstances = aTrackInstances;
    mReanimatorTransforms = (ReanimatorTransform *)&aTrackInstances[aTrackCount];

    for ( int i = 0; i < aTrackCount; ++i )
    {
      ConstructRaw<ReanimatorTrackInstance>(&mTrackInstances[i]);
      ConstructRaw<ReanimatorTransform>(&mReanimatorTransforms[i]);
    }
  }
  else
  {
    mFrameCount = 0;
  }
  mFrameTimeReady = false;
  return mReanimatorTransforms;
}

// ==========================================================================
// Reanimation::GetPosition(float &,float &)
// @001FAEE8  10 bytes
// ==========================================================================
int Reanimation::GetPosition(float & theX, float & theY)
{
  theX = mOverlayMatrix.m02;
  theY = mOverlayMatrix.m12;
  return 0;
}

// ==========================================================================
// Reanimation::OverrideScale(float,float)
// @001FAEF4  30 bytes
// ==========================================================================
int Reanimation::OverrideScale(float theScaleX, float theScaleY)
{
  mOverlayMatrix.m00 = theScaleX;
  mOverlayMatrix.m11 = theScaleY;
  mFrameTimeReady = false;
  mDirtyFlags |= 2;
  return 0;
}

// ==========================================================================
// Reanimation::GetTrackVelocity(int)
// @001FAF14  82 bytes
// ==========================================================================
float Reanimation::GetTrackVelocity(int theTrackIndex)
{
  ReanimatorTransform *aTransforms = mDefinition->mTracks[theTrackIndex].mTransforms;
  ReanimatorFrameTime aFrameTime;
  GetFrameTime((int)&aFrameTime);
  return (mAnimTime * 0.01f)
       * (aTransforms[aFrameTime.mAnimFrameAfterInt].mTransX
        - aTransforms[aFrameTime.mAnimFrameBeforeInt].mTransX);
}

// ==========================================================================
// Reanimation::GetTrackVelocity(char const*)
// @001FAF6C  20 bytes
// ==========================================================================
float Reanimation::GetTrackVelocity(char const* theTrackName)
{
  return GetTrackVelocity(FindTrackIndex(theTrackName));
}

// ==========================================================================
// Reanimation::IsTrackShowing(char const*)
// @001FAF80  68 bytes
// ==========================================================================
bool Reanimation::IsTrackShowing(char const* theTrackName)
{
  ReanimatorFrameTime aFrameTime;
  GetFrameTime((int)&aFrameTime);
  int aTrackIndex = FindTrackIndex(theTrackName);
  ReanimatorTransform *aTransforms = mDefinition->mTracks[aTrackIndex].mTransforms;
  return aTransforms[aFrameTime.mAnimFrameAfterInt].mFrame >= 0.0f;
}

// ==========================================================================
// Reanimation::ShowOnlyTrack(char const*)
// @001FAFC4  72 bytes
// ==========================================================================
int Reanimation::ShowOnlyTrack(char const* s2)
{
  // Every track is given a render group: the one whose name matches gets group
  // 0, the rest get -1, which takes them out of every DrawRenderGroup pass.
  for ( int i = 0; i < mDefinition->mTrackCount; ++i )
  {
    mTrackInstances[i].mRenderGroup =
        (strcasecmp(mDefinition->mTracks[i].mName, s2) == 0) ? 0 : -1;
  }
  mDirtyFlags = 7;
  return 0;
}

// ==========================================================================
// Reanimation::AssignRenderGroupToTrack(char const*,int)
// @001FB00C  80 bytes
// ==========================================================================
Reanimation * Reanimation::AssignRenderGroupToTrack(char const* theTrackName, int theRenderGroup)
{
  for ( int i = 0; i < mDefinition->mTrackCount; ++i )
  {
    if ( strcasecmp(mDefinition->mTracks[i].mName, theTrackName) == 0 )
    {
      mTrackInstances[i].mRenderGroup = theRenderGroup;
      mDirtyFlags = 7;
      break;
    }
  }
  return this;
}

// ==========================================================================
// Reanimation::AssignRenderGroupToPrefix(char const*,int)
// @001FB05C  94 bytes
// ==========================================================================
const char * Reanimation::AssignRenderGroupToPrefix(char const* theTrackName, int theRenderGroup)
{
  // Only the tracks whose name is at least as long as the prefix can match, and
  // the comparison is case-insensitive over exactly the prefix'theTrackName length.
  size_t aPrefixLength = strlen(theTrackName);
  for ( int i = 0; i < mDefinition->mTrackCount; ++i )
  {
    const char *aName = mDefinition->mTracks[i].mName;
    if ( aPrefixLength <= strlen(aName) && strncasecmp(aName, theTrackName, aPrefixLength) == 0 )
      mTrackInstances[i].mRenderGroup = theRenderGroup;
  }
  return theTrackName;
}

// ==========================================================================
// Reanimation::PropogateColorToAttachments()
// @001FB11C  78 bytes
// ==========================================================================
int Reanimation::PropogateColorToAttachments()
{
  for ( int i = 0; i < mDefinition->mTrackCount; ++i )
  {
    AttachmentPropogateColor(mTrackInstances[i].mAttachmentID, mColorOverride,
                             mEnableExtraAdditiveDraw, mExtraAdditiveColor,
                             mEnableExtraOverlayDraw, mExtraOverlayColor);
  }
  return 0;
}

// Reanimation::ShouldTriggerTimedEvent(float)
// @001FB16C  84 bytes
// ==========================================================================
bool Reanimation::ShouldTriggerTimedEvent(float theEventTime)
{
  if ( mFrameCount == 0 )
    return false;

  float aPrevious = mLastFrameTime;
  if ( aPrevious < 0.0f || mAnimTime <= 0.0f )
    return false;

  // The event fires when the animation time passes the requested one. Which way
  // "passes" goes depends on whether the animation time wrapped around since
  // the previous call.
  float aCurrent = mAnimTime;
  if ( aPrevious > aCurrent )
    return aPrevious <= theEventTime ? true : aCurrent > theEventTime;
  if ( aPrevious > theEventTime )
    return false;
  return aCurrent > theEventTime;
}

// ==========================================================================
// Reanimation::PlayReanim(char const*,ReanimLoopType,int,float)
// @001FB1C0  66 bytes
// ==========================================================================
int Reanimation::PlayReanim(char const* theTrackName, ReanimLoopType theLoopType, int theBlendTime, float theAnimRate)
{
  if ( theBlendTime > 0 )
    StartBlend(theBlendTime);
  if ( theAnimRate != 0.0f )
    SetAnimRate(theAnimRate);
  mLoopType = theLoopType;
  mLoopCount = 0;
  return SetFramesForLayer(theTrackName);
}

// ==========================================================================
// Reanimation::GetTrackInstanceByName(char const*)
// @001FB204  18 bytes
// ==========================================================================
ReanimatorTrackInstance * Reanimation::GetTrackInstanceByName(char const* theTrackName)
{
  return &mTrackInstances[FindTrackIndex(theTrackName)];
}

// ==========================================================================
// Reanimation::ParseAttacherTrack(ReanimatorTransform const&,AttacherInfo &)
// @001FB218  430 bytes
// ==========================================================================
void Reanimation::ParseAttacherTrack(ReanimatorTransform const& theTransform, AttacherInfo & theAttacherInfo)
{
  theAttacherInfo.mReanimName = Sexy::String();
  theAttacherInfo.mTrackName = Sexy::String();
  theAttacherInfo.mLowFPS = 12.0f;
  theAttacherInfo.mLoopType = 0;

  if ( theTransform.mFrame == -1.0f )
    return;

  const char *aText = theTransform.mText;
  if ( aText == nullptr )
    return;

  // A track name of the form "prefix__name[anim]" carries the reanimation and
  // the animation to play, separated by a double underscore; the bracketed part
  // is either a number or one of the words "hold" / "once".
  char *aDoubleUnderscore = strstr((char *)aText, "__");
  if ( aDoubleUnderscore == nullptr )
    return;

  const char *aName = aDoubleUnderscore + 2;
  char *aOpenBracket = strchr((char *)aName, '[');
  char *aSecondUnderscore = strstr((char *)aName, "__");

  bool aHasBracket = (aOpenBracket != nullptr);
  bool aHasName = (aSecondUnderscore != nullptr);
  if ( aHasBracket && aHasName )
  {
    if ( aOpenBracket < aSecondUnderscore )
      return;
    theAttacherInfo.mReanimName.assign(aName, (unsigned int)(aSecondUnderscore - aDoubleUnderscore - 2));
  }
  else if ( aHasName )
  {
    theAttacherInfo.mReanimName.assign(aName, (unsigned int)(aSecondUnderscore - aDoubleUnderscore - 2));
    theAttacherInfo.mTrackName.assign(aSecondUnderscore + 2);
    return;
  }
  else if ( aHasBracket )
  {
    theAttacherInfo.mReanimName.assign(aName, (unsigned int)(aOpenBracket - aDoubleUnderscore - 2));
  }
  else
  {
    theAttacherInfo.mReanimName.assign(aName);
    return;
  }

  if ( aHasBracket && aHasName )
    theAttacherInfo.mTrackName.assign(aSecondUnderscore + 2, (unsigned int)(aOpenBracket - aSecondUnderscore - 2));

  while ( aOpenBracket != nullptr )
  {
    char *aCloseBracket = strchr(aOpenBracket + 1, ']');
    if ( aCloseBracket == nullptr )
      break;

    Sexy::String aToken(aOpenBracket + 1, (unsigned int)(aCloseBracket - aOpenBracket - 1));
    if ( sscanf(aToken.c_str(), "%f", &theAttacherInfo.mLowFPS) != 1 )
    {
      if ( aToken.compare("hold") == 0 )
        theAttacherInfo.mLoopType = 3;
      else if ( aToken.compare("once") == 0 )
        theAttacherInfo.mLoopType = 2;
    }

    aOpenBracket = strchr(aCloseBracket + 1, '[');
  }
}

// ==========================================================================
// Reanimation::AttacherSynchWalkSpeed(int,Reanimation*,AttacherInfo const&)
// @001FB3F0  504 bytes
// ==========================================================================
// The mangled name is AttacherSynchWalkSpeed(int,Reanimation*,AttacherInfo const&),
// so the receiver is the host reanimation, the first argument is the host's track
// index and the second is the attached reanimation whose walk speed is matched.
// The binary never reads the AttacherInfo.
int Reanimation::AttacherSynchWalkSpeed(int theTrackIndex, Reanimation* theAttachReanim, AttacherInfo const& theAttacherInfo)
{
  (void)theAttacherInfo;
  ReanimatorTransform *aTransforms = mDefinition->mTracks[theTrackIndex].mTransforms;

  ReanimatorFrameTime aFrameTime;
  GetFrameTime((int)&aFrameTime);
  int aFrame = aFrameTime.mAnimFrameBeforeInt;
  int aLastFrame = mFrameStart + mFrameCount - 1;

  // The frames of one pose repeat, and they all share a text pointer, so the
  // run of frames the attacher walks over is found by comparing that pointer
  // forwards and backwards from the current frame.
  int aFirstIndex = aFrame;
  while ( aFirstIndex > mFrameStart
       && aTransforms[aFirstIndex - 1].mText == aTransforms[aFirstIndex].mText )
  {
    --aFirstIndex;
  }

  int aCounter = aFrame;
  int aLastIndex = aFrame;
  if ( aFrame < aLastFrame && aTransforms[aFrame + 1].mText == aTransforms[aFrame].mText )
  {
    for ( ;; )
    {
      ++aCounter;
      if ( aCounter == aLastFrame )
      {
        aLastIndex = aLastFrame - 1;
        break;
      }
      if ( aTransforms[aCounter + 1].mText != aTransforms[aCounter].mText )
      {
        aLastIndex = aCounter;
        break;
      }
    }
  }

  if ( fabsf(mAnimTime) < 0.000001f )
    return theAttachReanim->SetAnimRate(0.0f);

  float aTimeSpan = (float)(aCounter - aFirstIndex) / mAnimTime;
  if ( fabsf(aTimeSpan) < 0.000001f )
    return theAttachReanim->SetAnimRate(0.0f);

  float aTrackDistance = aTransforms[aFirstIndex].mTransX - aTransforms[aLastIndex].mTransX;

  int aGroundTrack = theAttachReanim->FindTrackIndex("_ground");
  ReanimatorTransform *aGroundTransforms = theAttachReanim->mDefinition->mTracks[aGroundTrack].mTransforms;
  float aGroundStartX = aGroundTransforms[theAttachReanim->mFrameStart].mTransX;
  float aGroundDistance =
      aGroundTransforms[theAttachReanim->mFrameStart + theAttachReanim->mFrameCount - 1].mTransX - aGroundStartX;
  if ( aGroundDistance < 0.000001f || aTrackDistance < 0.000001f )
    return theAttachReanim->SetAnimRate(0.0f);

  float aDistanceRatio = aTrackDistance / aGroundDistance;

  ReanimatorTransform aGroundTransform;
  theAttachReanim->GetCurrentTransform(aGroundTrack, &aGroundTransform);
  float aGroundOffset = aGroundTransform.mTransX - aGroundStartX;
  float aSlide = aGroundDistance * mAnimTime;
  AttachEffect *aFirstAttachment = FindFirstAttachment(mTrackInstances[theTrackIndex].mAttachmentID);
  if ( aFirstAttachment != nullptr )
    aFirstAttachment->mOffset.m02 = aSlide - aGroundOffset;

  return theAttachReanim->SetAnimRate((aDistanceRatio * (float)theAttachReanim->mFrameCount) / aTimeSpan);
}

// ==========================================================================
// Reanimation::IsAnimPlaying(char *)
// @001FB5F0  56 bytes
// ==========================================================================
bool Reanimation::IsAnimPlaying(char *theTrackName)
{
  int aStartFrame;
  int aFrameCount;
  GetFramesForLayer(theTrackName, aStartFrame, aFrameCount);
  return mFrameStart == aStartFrame && mFrameCount == aFrameCount && mAnimTime > 0.0f;
}

// ==========================================================================
// Reanimation::IsAnimPlaying(char const*)
// @001FB628  56 bytes
// ==========================================================================
bool Reanimation::IsAnimPlaying(char const* theTrackName)
{
  int aStartFrame;
  int aFrameCount;
  GetFramesForLayer((char *)theTrackName, aStartFrame, aFrameCount);
  return mFrameStart == aStartFrame && mFrameCount == aFrameCount && mAnimTime > 0.0f;
}

// ==========================================================================
// Reanimation::SetBasePoseFromAnim(char const*)
// @001FB660  28 bytes
// ==========================================================================
int Reanimation::SetBasePoseFromAnim(char const* theTrackName)
{
  int aStartFrame;
  int aFrameCount;
  int aResult = GetFramesForLayer((char *)theTrackName, aStartFrame, aFrameCount);
  mFrameBasePose = aStartFrame;
  mFrameTimeReady = false;
  return aResult;
}

// ==========================================================================
// Reanimation::FindSubReanim(ReanimationType)
// @001FB67C  66 bytes
// ==========================================================================
int Reanimation::FindSubReanim(int theTrackIndex)
{
  if ( mReanimationType == theTrackIndex )
    return (int)this;

  for ( int i = 0; i < mDefinition->mTrackCount; ++i )
  {
    Reanimation *aSub = FindReanimAttachment(mTrackInstances[i].mAttachmentID);
    if ( aSub != nullptr )
    {
      int aResult = aSub->FindSubReanim(theTrackIndex);
      if ( aResult != 0 )
        return aResult;
    }
  }
  return 0;
}

// ==========================================================================
// Reanimation::Reanimation()
// @001FB75C  294 bytes
// ==========================================================================
Reanimation::Reanimation()
{
  // SyncObject's constructor has already zeroed the sync-block vector and the
  // compiler wrote the vtable pointer, so the transcribed vptr store and the
  // explicit base construction are both gone. The overlay matrix is the identity
  // until a later assignment replaces it.

  // The first sync block covers the whole object: the binary pushes a
  // SyncBlockInfo whose data pointer is the object itself and whose size is 172.
  SyncBlockInfo aBlock;
  aBlock.mData = this;
  aBlock.mSize = 172;
  mSyncBlocks.push_back(aBlock);

  mAnimTime = 0.0f;
  mAnimRate = 12.0f;
  mLastFrameTime = -1.0f;
  mDefinition = nullptr;
  mDead = false;
  mFrameStart = 0;
  mFrameCount = 0;
  mLoopType = REANIM_PLAY_ONCE;
  mFrameBasePose = -1;
  mOverlayMatrix.LoadIdentity();

  // All four colours start as White; the binary copies the same 16 bytes into
  // each of them.
  mColorOverride = Sexy::Color::White;
  mExtraAdditiveColor = Sexy::Color::White;
  mEnableExtraAdditiveDraw = false;
  mExtraOverlayColor = Sexy::Color::White;
  mEnableExtraOverlayDraw = false;

  mLoopCount = 0;
  mReanimationHolder = nullptr;
  mIsAttachment = false;
  mRenderOrder = 0;
  mTrackInstances = nullptr;
  mFilterEffect = (FilterEffect)-1;
  mCustomFilterEffectColor = Sexy::Color::White;
  mReanimationType = REANIM_NONE;
  mDirtyFlags = 0;
  mReanimatorTransforms = nullptr;
  mFrameTimeReady = false;
  SetAnimRate(mAnimRate);
}

// ==========================================================================
// Reanimation::ReanimationInitializeType(float,float,ReanimationType)
// @001FBC58  50 bytes
// ==========================================================================
ReanimatorTransform * Reanimation::ReanimationInitializeType(float theX, float theY, ReanimationType theReanimType)
{
  ReanimatorEnsureDefinitionLoaded(theReanimType, true);
  mReanimationType = theReanimType;
  return ReanimationInitialize(theX, theY, gReanimatorDefArray + (int)theReanimType);
}

// ==========================================================================
// Reanimation::UpdateAttacherTrack(int)
// @001FBD08  868 bytes
// ==========================================================================
void Reanimation::UpdateAttacherTrack(int theTrackIndex)
{
  ReanimatorTrackInstance *aTrackInstance = &mTrackInstances[theTrackIndex];

  ReanimatorTransform aTransform;
  GetCurrentTransform(theTrackIndex, (int)&mFrameTime, &aTransform);

  AttacherInfo anInfo;
  ParseAttacherTrack(aTransform, anInfo);
  if ( anInfo.mReanimName.empty() )
  {
    AttachmentDie(aTrackInstance->mAttachmentID);
    return;
  }

  // The reanimation file the track names is looked up in the parameter array by
  // its "reanim/<name>.reanim" path, and the entry's type is the one to play.
  Sexy::String aFileName;
  StrFormat(&aFileName, "reanim/%s.reanim", anInfo.mReanimName.c_str());
  ReanimationType aReanimationType = REANIM_NONE;
  ReanimationParamRecord *aParams = gReanimationParamArray;
  for ( int i = 0; i < (int)gReanimationParamArraySize; ++i )
  {
    if ( strcasecmp(aFileName.c_str(), aParams[i].mFileName) == 0 )
    {
      aReanimationType = aParams[i].mReanimationType;
      break;
    }
  }

  if ( aReanimationType == REANIM_NONE )
  {
    AttachmentDie(aTrackInstance->mAttachmentID);
    return;
  }

  Reanimation *aSub = FindReanimAttachment(aTrackInstance->mAttachmentID);
  if ( aSub == nullptr || aSub->mReanimationType != aReanimationType )
  {
    AttachmentDie(aTrackInstance->mAttachmentID);
    aSub = gEffectSystem->mReanimationHolder->AllocReanimation(0.0f, 0.0f, 0, aReanimationType);
    aSub->SetAnimRate(anInfo.mLowFPS);
    aSub->mLoopType = (ReanimLoopType)anInfo.mLoopType;
    AttachReanim(aTrackInstance->mAttachmentID, aSub, 0.0f, 0.0f);
    mFrameBasePose = -2;
  }

  if ( !anInfo.mTrackName.empty() )
  {
    int aStartFrame;
    int aFrameCount;
    aSub->GetFramesForLayer(anInfo.mTrackName.c_str(), aStartFrame, aFrameCount);
    if ( aSub->mFrameStart != aStartFrame || aSub->mFrameCount != aFrameCount )
    {
      aSub->StartBlend(20);
      aSub->SetFramesForLayer(anInfo.mTrackName.c_str());
    }

    if ( anInfo.mLowFPS == 12.0f
      && anInfo.mTrackName.compare("anim_walk") == 0
      && aSub->TrackExists("_ground") )
    {
      // The walk speed is synchronised against this track, so the nominal
      // low-FPS rate is replaced by the measured one.
      AttacherSynchWalkSpeed(theTrackIndex, aSub, anInfo);
    }
    else
    {
      aSub->SetAnimRate(anInfo.mLowFPS);
    }
    aSub->mLoopType = (ReanimLoopType)anInfo.mLoopType;
  }

  // The attachment takes the host's colour, with the alpha scaled by the track
  // transform's own alpha.
  Sexy::Color aTrackColor;
  ColorsMultiply(&aTrackColor, &mColorOverride, &aTrackInstance->mTrackColor);
  float aScaledAlpha = (float)aTrackColor.mAlpha * aTransform.mAlpha;
  int anAlpha = (int)(aScaledAlpha <= 0.0f ? aScaledAlpha - 0.5f : aScaledAlpha + 0.5f);
  if ( anAlpha <= 0 )
    anAlpha = 0;
  else if ( anAlpha >= 255 )
    anAlpha = 255;
  aTrackColor.mAlpha = anAlpha;

  AttachmentPropogateColor(aTrackInstance->mAttachmentID, aTrackColor,
                           mEnableExtraAdditiveDraw, mExtraAdditiveColor,
                           mEnableExtraOverlayDraw, mExtraOverlayColor);
}

// ==========================================================================
// Reanimation::Update()
// @001FC08C  484 bytes
// ==========================================================================
void Reanimation::Update()
{
  if ( mFrameCount == 0 || mDead )
    return;

  float aPreviousTime = mAnimTime;
  mLastFrameTime = aPreviousTime;
  float aNewTime = aPreviousTime + mAnimTimeStep;
  mAnimTime = aNewTime;

  if ( mAnimRate <= 0.0f )
  {
    // A negative rate plays the clip backwards: it stops at the start, or wraps
    // back past the end, depending on the loop type.
    if ( mLoopType > 1 )
    {
      if ( mLoopType == REANIM_PLAY_ONCE || mLoopType == REANIM_PLAY_ONCE_FULL_LAST_FRAME )
      {
        if ( aNewTime < 0.0f )
        {
          mAnimTime = 0.0f;
          mLoopCount = 1;
          mDead = true;
        }
      }
      else if ( (mLoopType == REANIM_PLAY_ONCE_AND_HOLD
              || mLoopType == REANIM_PLAY_ONCE_FULL_LAST_FRAME_AND_HOLD)
             && aNewTime < 0.0f )
      {
        mAnimTime = 0.0f;
        mLoopCount = 1;
      }
    }
    else if ( aNewTime < 0.0f )
    {
      int aLoops = mLoopCount + 1;
      int aCount;
      do
      {
        aNewTime = aNewTime + 1.0f;
        aCount = aLoops++;
      }
      while ( aNewTime < 0.0f );
      mLoopCount = aCount;
      mAnimTime = aNewTime;
    }
  }
  else
  {
    if ( mLoopType > 1 )
    {
      if ( mLoopType == REANIM_PLAY_ONCE || mLoopType == REANIM_PLAY_ONCE_FULL_LAST_FRAME )
      {
        if ( aNewTime >= 1.0f )
        {
          mAnimTime = 1.0f;
          mLoopCount = 1;
          mDead = true;
        }
      }
      else if ( (mLoopType == REANIM_PLAY_ONCE_AND_HOLD
              || mLoopType == REANIM_PLAY_ONCE_FULL_LAST_FRAME_AND_HOLD)
             && aNewTime >= 1.0f )
      {
        mAnimTime = 1.0f;
        mLoopCount = 1;
      }
    }
    else if ( aNewTime >= 1.0f )
    {
      int aLoops = mLoopCount + 1;
      int aCount;
      do
      {
        aNewTime = aNewTime - 1.0f;
        aCount = aLoops++;
      }
      while ( aNewTime >= 1.0f );
      mLoopCount = aCount;
      mAnimTime = aNewTime;
    }
  }

  for ( int i = 0; i < mDefinition->mTrackCount; ++i )
  {
    ReanimatorTrackInstance *aTrackInstance = &mTrackInstances[i];
    if ( aTrackInstance->mBlendCounter > 0 )
      --aTrackInstance->mBlendCounter;

    // A non-zero shake override re-rolls the shake offsets on every frame.
    if ( aTrackInstance->mShakeOverride != 0.0f )
    {
      aTrackInstance->mShakeX = (float)RandRangeFloat(-aTrackInstance->mShakeOverride,
                                                       aTrackInstance->mShakeOverride);
      aTrackInstance->mShakeY = (float)RandRangeFloat(-aTrackInstance->mShakeOverride,
                                                       aTrackInstance->mShakeOverride);
    }

    if ( mDefinition->mTracks[i].mIsAttacher )
      UpdateAttacherTrack(i);

    if ( aTrackInstance->mAttachmentID != 0 )
    {
      AttachmentUpdate(aTrackInstance->mAttachmentID);
      mDirtyFlags |= 2;
    }
  }
  mDirtyFlags |= 5;
}
