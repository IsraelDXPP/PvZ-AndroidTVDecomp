// PIEffect plays a particle definition. LoadEffect reads the file into the
// shared definition and the live emitters. Update advances the clock, rebuilds
// the deflector and force geometry, and spawns and steps every particle.
// Draw paints the live groups, then the blockers, deflectors and forces.

#include "PIEffect.h"

#include "DeviceImage.h"
#include "ImageLib.h"
#include "ReconFreeFunctions.h"
#include "ReconHelpers.h"
#include "SexyAppBase.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace
{

using Sexy::PIParticle;
using Sexy::PIParticleGroup;

const float kDegToRad = 0.017453f;
const float kHalfPi = 1.5708f;
const float kPi = 3.1416f;
const float kTwoPi = 6.2832f;
const float kInvHalfPi = 0.63662f;

int KeyCount(const Sexy::PIValue &theCurve)
{
    return (int)theCurve.mKeyframes.size();
}

Sexy::PIKeyframe *KeyAt(Sexy::PIValue &theCurve, int theIndex)
{
    return &theCurve.mKeyframes[(size_t)theIndex];
}

const Sexy::PIKeyframe *KeyAt(const Sexy::PIValue &theCurve, int theIndex)
{
    return &theCurve.mKeyframes[(size_t)theIndex];
}

int CurveCount(const Sexy::PIValue2D &theCurve)
{
    return (int)theCurve.mKeyframes.size();
}

const Sexy::PIKeyframe2D *CurveAt(const Sexy::PIValue2D &theCurve, int theIndex)
{
    return &theCurve.mKeyframes[(size_t)theIndex];
}

void PushCurveKey(Sexy::PIValue2D *theCurve, float theTime, float theX, float theY)
{
    Sexy::PIKeyframe2D key;
    key.mTime = theTime;
    key.mX = theX;
    key.mY = theY;
    theCurve->mKeyframes.push_back(key);
}

void ResizeKeys(Sexy::PIValue *theCurve, int theCount)
{
    if (theCount < 0)
        theCount = 0;
    theCurve->mKeyframes.resize((size_t)theCount);
}

void StoreKey(Sexy::PIValue *theCurve, int theIndex, float theTime, float theValue)
{
    Sexy::PIKeyframe &slot = theCurve->mKeyframes[(size_t)theIndex];
    slot.mTime = theTime;
    slot.mValue = theValue;
}

float KeyValue(const Sexy::PIValue &theCurve, int theIndex)
{
    return theCurve.mKeyframes[(size_t)theIndex].mValue;
}

int ColorWord(int theRed, int theGreen, int theBlue)
{
    return (int)(0xFF000000u | ((unsigned int)theRed << 16) | ((unsigned int)theGreen << 8) | (unsigned int)theBlue);
}

void EndianMatrix(Sexy::SexyTransform2D *theMatrix)
{
    for (int col = 0; col != 3; ++col)
    {
        for (int row = 0; row != 3; ++row)
            theMatrix->m[row * 3 + col] = EndianFloat(theMatrix->m[row * 3 + col]);
    }
}

void DeleteChain(PIParticle *theHead)
{
    PIParticle *node = theHead;
    while (node != nullptr)
    {
        PIParticle *next = node->mNext;
        PIParticle *child = node->mChildren;
        while (child != nullptr)
        {
            PIParticle *childNext = child->mNext;
            delete child;
            --PIParticleInstanceCounter::mCount;
            child = childNext;
        }
        delete node;
        --PIParticleInstanceCounter::mCount;
        node = next;
    }
}

void LinkParticle(PIParticleGroup *theGroup, PIParticle *theParticle, bool theReverse)
{
    if (theReverse)
    {
        if (theGroup->mHead != nullptr)
            theGroup->mHead->mPrev = theParticle;
        theParticle->mNext = theGroup->mHead;
        theParticle->mPrev = nullptr;
        if (theGroup->mTail == nullptr)
            theGroup->mTail = theParticle;
        theGroup->mHead = theParticle;
    }
    else
    {
        if (theGroup->mTail != nullptr)
            theGroup->mTail->mNext = theParticle;
        theParticle->mPrev = theGroup->mTail;
        theParticle->mNext = nullptr;
        if (theGroup->mHead == nullptr)
            theGroup->mHead = theParticle;
        theGroup->mTail = theParticle;
    }
    theGroup->mCount += 1;
}

void UnlinkParticle(PIParticleGroup *theGroup, PIParticle *theParticle)
{
    if (theParticle->mPrev != nullptr)
        theParticle->mPrev->mNext = theParticle->mNext;
    if (theParticle->mNext != nullptr)
        theParticle->mNext->mPrev = theParticle->mPrev;
    if (theGroup->mHead == theParticle)
        theGroup->mHead = theParticle->mNext;
    if (theGroup->mTail == theParticle)
        theGroup->mTail = theParticle->mPrev;
    theGroup->mCount -= 1;
}

Sexy::PIValue *EmitterCurve(Sexy::PIEmitter *theEmitter, int theIndex)
{
    if (theEmitter == nullptr || theIndex < 0 || theIndex >= 42)
        return nullptr;
    return &theEmitter->mValues[theIndex];
}

Sexy::PIValue *InstanceCurve(Sexy::PIEmitterInstanceDef *theDef, int theIndex)
{
    if (theDef == nullptr || theIndex < 0 || theIndex >= 19)
        return nullptr;
    return &theDef->mValues[theIndex];
}

float Sample(Sexy::PIValue *theCurve, float theTime, float theStep)
{
    if (theCurve == nullptr)
        return 0.0f;
    return theCurve->GetValueAt(theTime, theStep);
}

Sexy::DeviceImage *ImageOf(const Sexy::SharedImageRef &theRef)
{
    return static_cast<Sexy::DeviceImage *>(static_cast<Sexy::Image *>(theRef));
}

Sexy::SexyMatrix3 ParticleMatrix(PIParticle *theParticle)
{
    if (theParticle == nullptr)
    {
        Sexy::SexyMatrix3 identity;
        identity.LoadIdentity();
        return identity;
    }
    return theParticle->mTransform.mMatrix;
}

} // namespace

Sexy::PILiveEmitter *Sexy::PIEffect::FindLive(Sexy::PIEmitterInstance *theInstance)
{
    if (theInstance == nullptr)
        return nullptr;
    for (size_t layer = 0; layer < mLive.size(); ++layer)
    {
        for (size_t emitter = 0; emitter < mLive[layer].mEmitters.size(); ++emitter)
        {
            PILiveEmitter *live = &mLive[layer].mEmitters[emitter];
            if (live->mDef == theInstance->mDef)
                return live;
        }
    }
    return nullptr;
}

Sexy::PIParticleType *Sexy::PIEffect::FindType(Sexy::PIParticleDef *theDef)
{
    if (mDef == nullptr || theDef == nullptr)
        return nullptr;
    for (size_t i = 0; i < mDef->mTypes.size(); ++i)
    {
        if (mDef->mTypes[i] != nullptr && mDef->mTypes[i]->mPublished == theDef)
            return mDef->mTypes[i];
    }
    return nullptr;
}

void Sexy::PIEffect::SetImageOpts(Sexy::DeviceImage *theImage)
{
    (void)theImage;
}

Sexy::SharedImageRef Sexy::PIEffect::GetImage(Sexy::String const &unusedName, Sexy::String const &theImageName)
{
    (void)unusedName;
    Sexy::String dir;
    Sexy::String path;
    GetFileDir(&dir, &mFilePath, 1);
    Sexy::String name = theImageName;
    GetPathFrom(&path, &name, &dir);
    return gSexyAppBase->GetSharedImage(path, Sexy::String(), nullptr, true, false);
}

int Sexy::PIEffect::Fail(Sexy::String const &theMessage)
{
    if (mError.empty())
        mError = theMessage;
    return 0;
}

float Sexy::PIEffect::GetRandFloat()
{
    return (float)(mRand.Next() % 20000000) * 0.0000001f - 1.0f;
}

float Sexy::PIEffect::GetRandFloatU()
{
    return (float)(mRand.Next() % 10000000) * 0.0000001f;
}

float Sexy::PIEffect::GetRandSign()
{
    return (mRand.Next() & 1) != 0 ? 1.0f : -1.0f;
}

float Sexy::PIEffect::GetVariationScalar()
{
    float first = GetRandFloat();
    return GetRandFloat() * first;
}

float Sexy::PIEffect::GetVariationScalarU()
{
    float first = GetRandFloatU();
    return GetRandFloatU() * first;
}

Sexy::String Sexy::PIEffect::ReadString()
{
    if (mReadPos == 1024)
    {
        mReadPos = 0;
        unsigned int words = (unsigned int)p_fread(mReadBuf, 1, 1024, mReadFile) >> 2;
        unsigned int checksum = mChecksum;
        for (unsigned int i = 0; i != words; ++i)
            checksum += mReadWords[i] ^ i;
        mChecksum = checksum;
    }
    unsigned char length = mReadBuf[mReadPos];
    mReadPos += 1;
    mReadValue = length;

    Sexy::String result;
    if (length != 0)
    {
        char blank[256];
        std::memset(blank, 0, sizeof(blank));
        int filled = 0;
        while (filled < (int)length)
        {
            int chunk = (int)length - filled;
            if (chunk > 256)
                chunk = 256;
            result += Sexy::String(blank, chunk);
            filled += chunk;
        }
        ReadChecksummedBytes(this, result.data(), length);
    }
    return result;
}

size_t Sexy::PIEffect::WriteByte(char theByte)
{
    return p_fwrite(&theByte, 1u, 1u, mWriteFile);
}

size_t Sexy::PIEffect::WriteInt(int theValue)
{
    return p_fwrite(&theValue, 4u, 1u, mWriteFile);
}

size_t Sexy::PIEffect::WriteShort(short theValue)
{
    return p_fwrite(&theValue, 2u, 1u, mWriteFile);
}

size_t Sexy::PIEffect::WriteFloat(float theValue)
{
    return p_fwrite(&theValue, 4u, 1u, mWriteFile);
}

size_t Sexy::PIEffect::WriteBool(bool theValue)
{
    return WriteByte(theValue ? 1 : 0);
}

const void *Sexy::PIEffect::WriteString(const Sexy::String *theText)
{
    int length = (int)theText->size();
    WriteByte((char)length);
    if (length != 0)
        p_fwrite(theText->c_str(), 1u, (size_t)length, mWriteFile);
    return theText->c_str();
}

size_t Sexy::PIEffect::WriteValue2D(Sexy::PIValue2D *theCurve)
{
    int count = CurveCount(*theCurve);
    bool hasTangents = false;
    size_t result = 0;
    if (count <= 1)
    {
        result = WriteShort((short)count);
        if (count <= 0)
            return result;
    }
    else if (theCurve->mBezier.mDerivativePoints != nullptr)
    {
        hasTangents = true;
        WriteShort((short)count);
        WriteBool(true);
    }
    else
    {
        WriteShort((short)count);
        WriteBool(false);
    }

    int tangent = 0;
    for (int i = 0; i != count; ++i)
    {
        const Sexy::PIKeyframe2D *key = CurveAt(*theCurve, i);
        WriteInt((int)key->mTime);
        WriteFloat(key->mX);
        result = WriteFloat(key->mY);
        if (hasTangents)
        {
            if (i != 0)
            {
                WriteFloat(theCurve->mBezier.mDerivativePoints[2 * tangent] - key->mX);
                WriteFloat(theCurve->mBezier.mDerivativePoints[2 * tangent + 1] - key->mY);
                ++tangent;
            }
            else
            {
                WriteFloat(0.0f);
                WriteFloat(0.0f);
            }
            if (i == count - 1)
            {
                WriteFloat(0.0f);
                result = WriteFloat(0.0f);
            }
            else
            {
                WriteFloat(theCurve->mBezier.mDerivativePoints[2 * tangent] - key->mX);
                result = WriteFloat(theCurve->mBezier.mDerivativePoints[2 * tangent + 1] - key->mY);
            }
            ++tangent;
        }
    }
    return result;
}

size_t Sexy::PIEffect::WriteEPoint(Sexy::PIValue2D *theCurve)
{
    int count = CurveCount(*theCurve);
    size_t result = WriteShort((short)count);
    for (int i = 0; i != count; ++i)
    {
        const Sexy::PIKeyframe2D *key = CurveAt(*theCurve, i);
        WriteInt((int)key->mTime);
        WriteFloat(key->mX);
        result = WriteFloat(key->mY);
    }
    return result;
}

size_t Sexy::PIEffect::WriteValue(Sexy::PIValue *theCurve)
{
    int count = KeyCount(*theCurve);
    int flags = 0;
    int hasTangents = 0;
    if (count > 1 && theCurve->mBezier.mDerivativePoints != nullptr)
    {
        flags = (count >= 7 ? 7 : count) | 8;
        hasTangents = 1;
    }
    else
    {
        flags = count >= 7 ? 7 : count;
        if (count == 0)
            return WriteByte((char)flags);
    }

    float first = KeyValue(*theCurve, 0);
    if (KeyAt(*theCurve, 0)->mTime == 0.0f)
        flags |= 0x10;
    if (first == 0.0f)
        flags |= 0x20;
    else if (first == 1.0f)
        flags |= 0x40;
    else if (first == 2.0f)
        flags |= 0x60;

    size_t result = WriteByte((char)flags);
    if (count > 6)
        result = WriteShort((short)count);

    int tangent = 0;
    for (int i = 0; i < count; ++i)
    {
        Sexy::PIKeyframe *key = KeyAt(*theCurve, i);
        if (i != 0 || (flags & 0x10) == 0)
            result = WriteFloat(key->mTime);
        if (i != 0 || (flags & 0x60) == 0)
            result = WriteFloat(key->mValue);
        if (hasTangents != 0)
        {
            if (i != 0)
            {
                WriteFloat(theCurve->mBezier.mDerivativePoints[2 * tangent] - key->mTime);
                WriteFloat(theCurve->mBezier.mDerivativePoints[2 * tangent + 1] - key->mValue);
                ++tangent;
            }
            else
            {
                WriteFloat(0.0f);
                WriteFloat(0.0f);
            }
            if (i == count - 1)
            {
                WriteFloat(0.0f);
                result = WriteFloat(0.0f);
            }
            else
            {
                WriteFloat(theCurve->mBezier.mDerivativePoints[2 * tangent] - key->mTime);
                result = WriteFloat(theCurve->mBezier.mDerivativePoints[2 * tangent + 1] - key->mValue);
            }
            ++tangent;
        }
    }
    return result;
}

size_t Sexy::PIEffect::WriteEmitterType(Sexy::PIEmitter *theEmitter)
{
    int defCount = (int)theEmitter->mParticleDefs.size();
    WriteInt(0);
    WriteString(&theEmitter->mName);
    WriteBool(theEmitter->mSerialize);
    WriteInt(0);
    WriteBool(theEmitter->mEmitInReverse);
    WriteShort((short)defCount);

    for (int i = 0; i < defCount; ++i)
    {
        int index = theEmitter->mEmitInReverse ? defCount - 1 - i : i;
        PIParticleType *def = FindType(&theEmitter->mParticleDefs[index]);
        if (def == nullptr)
            continue;
        WriteInt(0);
        WriteInt(0);
        WriteInt(0);
        WriteFloat(0.0f);
        WriteInt(0);
        WriteInt(0);
        WriteInt(0);
        WriteInt(1);
        WriteInt(0);
        WriteInt(0);
        WriteInt(0);
        WriteInt(2);
        WriteInt(0);
        WriteInt(0);
        WriteInt(0);
        WriteInt(3);
        WriteBool(def->mAdditive);
        WriteBool(def->mSuperEmitter);
        WriteBool(def->mAdditiveSecond);
        WriteBool(def->mAttachToParent);
        WriteFloat(def->mAttachWeight);
        WriteBool(def->mFlipX);
        WriteBool(def->mFlipY);
        WriteBool(def->mRandomFrame);
        WriteInt(def->mColorRepeat);
        WriteInt(def->mAlphaRepeat);
        WriteBool(def->mAlignToPath);
        WriteString(&def->mName);
        WriteBool(def->mAlignToVelocity);
        WriteBool(def->mUseOwnAngle);
        WriteBool(def->mUseOwnAngle && def->mAttachToParent);
        WriteInt(def->mFixedAngle);
        WriteInt(def->mAngleOffset);
        WriteInt(def->mAnimLength);
        WriteBool(def->mUseColorKeys);
        WriteInt(0);
        WriteInt(def->mImageIndex);
        WriteShort((short)def->mColorKeys.mKeyframes.size());
        for (size_t key = 0; key < def->mColorKeys.mKeyframes.size(); ++key)
        {
            unsigned int word = (unsigned int)def->mColorKeys.mKeyframes[key].mField0;
            WriteByte((char)((word >> 16) & 0xFF));
            WriteByte((char)((word >> 8) & 0xFF));
            WriteByte((char)(word & 0xFF));
            WriteFloat(FloatBits(def->mColorKeys.mKeyframes[key].mField1));
        }
        WriteShort((short)def->mAlphaKeys.mKeyframes.size());
        for (size_t key = 0; key < def->mAlphaKeys.mKeyframes.size(); ++key)
        {
            WriteByte((char)(def->mAlphaKeys.mKeyframes[key].mField0 & 0xFF));
            WriteFloat(FloatBits(def->mAlphaKeys.mKeyframes[key].mField1));
        }
        for (int curve = 0; curve != 23; ++curve)
            WriteValue(&def->mValues[curve]);
        WriteFloat(def->mPivotX);
        WriteFloat(def->mPivotY);
        WriteInt(0);
        WriteInt(0);
        WriteBool(def->mLockSpin);
        WriteValue(&def->mSpinY);
        WriteValue(&def->mSpinVarY);
        WriteValue(&def->mExtraCurve);
        WriteInt(def->mAngleSpread);
        WriteInt(def->mAngleSpreadRange);
        WriteBool(def->mSampleColor);
        WriteBool(def->mSampleAlpha);
        WriteBool(def->mUseOwnAngle);
        WriteValue(&def->mAngle);
        WriteValue(&def->mSpread);
        WriteInt(0);
        WriteValue(&def->mSpread);
        WriteBool(def->mKeepAlpha);
        WriteBool(def->mForceAlpha);
        WriteBool(def->mUseSpawnColor);
        WriteInt(def->mColorStride);
        WriteBool(def->mAlignToPath);
    }

    WriteInt(999);
    for (int i = 0; i != 42; ++i)
        WriteValue(&theEmitter->mValues[i]);
    WriteInt(0);
    return WriteInt(0);
}

int Sexy::PIEffect::SaveParticleDefInstance(Sexy::Buffer &theBuffer, Sexy::PIParticleDefInstance *theRecord)
{
    theBuffer.WriteFloat(theRecord->mField0);
    theBuffer.WriteFloat(theRecord->mField1);
    theBuffer.WriteLong((long)theRecord->mField2);
    theBuffer.WriteLong((long)theRecord->mField3);
    return 0;
}

int Sexy::PIEffect::SaveParticle(Sexy::Buffer &theBuffer, Sexy::PILayer *theLayer, PIParticle *theParticle)
{
    PIParticleType *def = theParticle->mType;
    theBuffer.WriteFloat(theParticle->mLife);
    theBuffer.WriteFloat(theParticle->mLifePct);
    theBuffer.WriteFloat(theParticle->mAge);
    theBuffer.WriteFloat(theParticle->mAngle);
    theBuffer.WriteFPoint(Sexy::TPoint<float>(theParticle->mPosX, theParticle->mPosY));
    theBuffer.WriteFPoint(Sexy::TPoint<float>(theParticle->mSpinX, theParticle->mSpinY));
    theBuffer.WriteFPoint(Sexy::TPoint<float>(theParticle->mScaleX, theParticle->mScaleY));
    if (def != nullptr && def->mAttachToParent)
    {
        theBuffer.WriteFPoint(Sexy::TPoint<float>(theParticle->mVelX, theParticle->mVelY));
        theBuffer.WriteFloat(theParticle->mLaunchAngle);
    }
    theBuffer.WriteFloat(theParticle->mSize);

    int mask = 0;
    const float *matrix = &theParticle->mTransform.mMatrix.m[0];
    for (int i = 0; i != 9; ++i)
    {
        if (fabsf(matrix[i]) >= 0.00001f)
            mask |= 1 << i;
    }
    theBuffer.WriteShort((short)mask);
    for (int i = 0; i != 9; ++i)
    {
        if ((mask & (1 << i)) != 0)
            theBuffer.WriteFloat(matrix[i]);
    }

    theBuffer.WriteFloat(theParticle->mSpinScaleX);
    theBuffer.WriteFloat(theParticle->mSpinScaleY);
    if (def != nullptr)
    {
        if (def->mUseColorKeys)
            theBuffer.WriteFloat(theParticle->mKeyParam);
        if (def->mRandomFrame)
            theBuffer.WriteShort((short)theParticle->mFrame);
    }
    if (theLayer != nullptr && !theLayer->mEmitters.empty())
        theBuffer.WriteFloat(theParticle->mEmitterTime);
    return 0;
}

int Sexy::PIEffect::LoadParticleDefInstance(Sexy::Buffer const &theBuffer, Sexy::PIParticleDefInstance *theRecord)
{
    theRecord->mField0 = theBuffer.ReadFloat();
    theRecord->mField1 = theBuffer.ReadFloat();
    theRecord->mField2 = theBuffer.ReadLong();
    theRecord->mField3 = theBuffer.ReadLong();
    return theRecord->mField3;
}

int Sexy::PIEffect::LoadParticle(Sexy::Buffer const &theBuffer, Sexy::PILayer *theLayer, PIParticle *theParticle)
{
    theParticle->mLife = theBuffer.ReadFloat();
    theParticle->mLifePct = theBuffer.ReadFloat();
    theParticle->mAge = theBuffer.ReadFloat();
    theParticle->mAngle = theBuffer.ReadFloat();

    Sexy::TPoint<float> point = theBuffer.ReadFPoint();
    theParticle->mPosX = point.mX;
    theParticle->mPosY = point.mY;
    point = theBuffer.ReadFPoint();
    theParticle->mSpinX = point.mX;
    theParticle->mSpinY = point.mY;
    point = theBuffer.ReadFPoint();
    theParticle->mScaleX = point.mX;
    theParticle->mScaleY = point.mY;

    PIParticleType *def = theParticle->mType;
    if (def != nullptr && def->mAttachToParent)
    {
        point = theBuffer.ReadFPoint();
        theParticle->mVelX = point.mX;
        theParticle->mVelY = point.mY;
        theParticle->mLaunchAngle = theBuffer.ReadFloat();
    }
    theParticle->mSize = theBuffer.ReadFloat();

    int mask = theBuffer.ReadShort();
    float *matrix = &theParticle->mTransform.mMatrix.m[0];
    for (int i = 0; i != 9; ++i)
    {
        if (((mask >> i) & 1) != 0)
            matrix[i] = theBuffer.ReadFloat();
        else
            matrix[i] = 0.0f;
    }

    theParticle->mSpinScaleX = theBuffer.ReadFloat();
    theParticle->mSpinScaleY = theBuffer.ReadFloat();
    if (def != nullptr && def->mUseColorKeys)
        theParticle->mKeyParam = theBuffer.ReadFloat();
    if (def != nullptr && def->mRandomFrame)
        theParticle->mFrame = theBuffer.ReadShort();

    if (theLayer != nullptr && !theLayer->mEmitters.empty())
        theParticle->mEmitterTime = theBuffer.ReadFloat();

    if (def != nullptr && def->mRandomFrame)
        theParticle->mFrame = (int)(mRand.Next() & 0x7FFF);
    else
        theParticle->mFrame = 0;
    return 0;
}

int Sexy::PIEffect::ResetAnim()
{
    mTime = 0.0f;
    if (mDef == nullptr)
        return 0;

    for (size_t layerIndex = 0; layerIndex < mLive.size(); ++layerIndex)
    {
        PILiveLayer &layer = mLive[layerIndex];
        for (size_t emitterIndex = 0; emitterIndex < layer.mEmitters.size(); ++emitterIndex)
        {
            PILiveEmitter &live = layer.mEmitters[emitterIndex];
            DeleteChain(live.mFree.mHead);
            live.mFree.mHead = nullptr;
            live.mFree.mTail = nullptr;
            live.mFree.mCount = 0;
            DeleteChain(live.mTrail.mHead);
            live.mTrail.mHead = nullptr;
            live.mTrail.mTail = nullptr;
            live.mTrail.mCount = 0;
            for (size_t record = 0; record < live.mFreeSpawns.size(); ++record)
            {
                live.mFreeSpawns[record].mField0 = 0.0f;
                live.mFreeSpawns[record].mField1 = 0.0f;
                live.mFreeSpawns[record].mField2 = 0;
                live.mFreeSpawns[record].mField3 = 0;
            }
            for (size_t record = 0; record < live.mDefSpawns.size(); ++record)
            {
                live.mDefSpawns[record].mField0 = 0.0f;
                live.mDefSpawns[record].mField1 = 0.0f;
                live.mDefSpawns[record].mField2 = 0;
                live.mDefSpawns[record].mField3 = 0;
            }
            live.mKeepSpawn = true;
            live.mEmitting = false;
        }
    }
    mUpdateCountB = 0;
    mUpdateCountA = 0;
    mPixelCount = 0;
    mNeedsSeed = true;
    return (int)mLive.size();
}

Sexy::PILayer *Sexy::PIEffect::GetLayer(int theIndex)
{
    if (theIndex < 0 || theIndex >= (int)mLayers.size())
        return nullptr;
    return &mLayers[theIndex];
}

Sexy::PILayer *Sexy::PIEffect::GetLayer(Sexy::String const &theName)
{
    if (mLayers.empty())
        return nullptr;
    if (theName.empty())
        return &mLayers[0];
    for (size_t i = 0; i < mLayers.size() && mDef != nullptr && i < mDef->mLayerDefs.size(); ++i)
    {
        if (mDef->mLayerDefs[i].mName == theName)
            return &mLayers[i];
    }
    return nullptr;
}

Sexy::TPoint<float> Sexy::PIEffect::GetEmitterPos(Sexy::PIEmitterInstance *theInstance, bool theTransform)
{
    Sexy::TPoint<float> pos;
    PILiveEmitter *live = FindLive(theInstance);
    Sexy::PIEmitterInstanceDef *def =
        theInstance != nullptr ? theInstance->mDef : (live != nullptr ? live->mDef : nullptr);
    if (def != nullptr)
    {
        def->mValue2D.GetValueAt(mTime);
        pos.mX = def->mValue2D.mLastValueX;
        pos.mY = def->mValue2D.mLastValueY;
    }
    if (theTransform && live != nullptr)
    {
        Sexy::TPoint<float> transformed;
        TransformFPoint(&transformed, live->mTransform, pos);
        pos = transformed;
        TransformFPoint(&transformed, mWorldTransform, pos);
        pos = transformed;
        pos.mX += live->mOffsetX;
        pos.mY += live->mOffsetY;
    }
    return pos;
}

int Sexy::PIEffect::CountParticles(PIParticle *theHead)
{
    int count = 0;
    for (PIParticle *node = theHead; node != nullptr; node = node->mNext)
        ++count;
    return count;
}

bool Sexy::PIEffect::HasTimelineExpired()
{
    return mTime >= (float)mTimelineEnd;
}

int Sexy::PIEffect::IsActive()
{
    for (size_t layerIndex = 0; layerIndex < mLive.size(); ++layerIndex)
    {
        PILiveLayer &layer = mLive[layerIndex];
        if (layer.mPublic == nullptr || !layer.mPublic->mVisible)
            continue;
        for (size_t emitterIndex = 0; emitterIndex < layer.mEmitters.size(); ++emitterIndex)
        {
            PILiveEmitter &live = layer.mEmitters[emitterIndex];
            if (!live.mActive || live.mDef == nullptr)
                continue;
            float next = live.mDef->mValues[13].GetNextKeyframeTime(mTime);
            if (next >= mTime)
                return 1;
            if (live.mKeepSpawn)
                return 1;
            if (live.mFree.mHead != nullptr || live.mTrail.mHead != nullptr)
                return 1;
        }
    }
    return 0;
}

Sexy::String Sexy::PIEffect::GetNotesParam(Sexy::String const &theKey, Sexy::String const &theDefault)
{
    Sexy::String key;
    Upper(&key, &theKey);
    std::map<Sexy::String, Sexy::String>::iterator found = mNotes.find(key);
    if (found == mNotes.end())
        return theDefault;
    return found->second;
}

int Sexy::PIEffect::CheckCache()
{
    return 0;
}

int Sexy::PIEffect::SetCacheUpToDate()
{
    return 0;
}

void Sexy::PIEffect::WriteToCache() {}

void Sexy::PIEffect::Deref()
{
    if (mDef == nullptr)
        return;
    int count = mDef->mRefCount - 1;
    bool wasLast = mDef->mRefCount == 1;
    mDef->mRefCount = count;
    if (count >= 0 && !wasLast)
    {
        mDef = nullptr;
        return;
    }

    for (size_t i = 0; i < mDef->mEmitters.size(); ++i)
        delete mDef->mEmitters[i];
    for (size_t i = 0; i < mDef->mTypes.size(); ++i)
        delete mDef->mTypes[i];
    for (size_t i = 0; i < mDef->mImages.size(); ++i)
        delete mDef->mImages[i];
    delete mDef;
    mDef = nullptr;
}

int Sexy::PIEffect::Clear()
{
    mError.clear();
    ResetAnim();
    mStrings.clear();
    mNotes.clear();
    mLive.clear();
    if (mDef != nullptr)
    {
        for (size_t i = 0; i < mDef->mEmitters.size(); ++i)
            delete mDef->mEmitters[i];
        mDef->mEmitters.clear();
        for (size_t i = 0; i < mDef->mTypes.size(); ++i)
            delete mDef->mTypes[i];
        mDef->mTypes.clear();
        for (size_t i = 0; i < mDef->mImages.size(); ++i)
            delete mDef->mImages[i];
        mDef->mImages.clear();
        mDef->mLayerDefs.clear();
        mDef->mEmitterIndex.clear();
    }
    mLayers.clear();
    mFileVersion = 0;
    mLoaded = false;
    mRandSeedCursor = (int)mRandSeeds.size() > 0 ? mRandSeeds[0] : 0;
    return 0;
}

Sexy::PIEffect::PIEffect() :
    mReadFile(nullptr),
    mWriteFile(nullptr),
    mChecksum(0),
    mWideBooleans(false),
    mLoadedFlag(true),
    mFileVersion(0),
    mFilePath(),
    mWritePath(),
    mRand(),
    mBuffer(),
    mReadValue(0),
    mReadPos(0),
    mNotesText(),
    mNotesCursor(0),
    mStrings(),
    mSettingA(0),
    mSettingB(0),
    mLoadColor(),
    mParticleScale(0),
    mTimelineStart(0),
    mTimelineEnd(0),
    mUnusedE40(0),
    mNotes(),
    mDef(nullptr),
    mLayers(),
    mLive(),
    mError(),
    mLoaded(false),
    mFrameTick(0),
    mTime(0.0f),
    mTimeCrossed(false),
    mFreeList(),
    mFreeStride(228),
    mDeadList(),
    mDeadStride(256),
    mEmitEnabled(false),
    mEmitAfter(false),
    mLoopFlag(false),
    mUpdateCountA(0),
    mUpdateCountB(0),
    mPixelCount(0),
    mTimeScale(1.0f),
    mColor(Sexy::Color::White),
    mDebugDraw(false),
    mVisibleFlag(true),
    mPausedFlag(false),
    mRandSeeds(),
    mRandSeedCursor(0),
    mUnusedED8(0),
    mNeedsSeed(true),
    mTransform(),
    mWorldTransform()
{
    mFreeList.mField0 = 0;
    mFreeList.mField1 = 0;
    mFreeList.mField2 = 0;
    mFreeList.mField3 = 0;
    mDeadList.mField0 = 0;
    mDeadList.mField1 = 0;
    mDeadList.mField2 = 0;
    mDeadList.mField3 = 0;
    mTransform.LoadIdentity();
    mWorldTransform.LoadIdentity();

    gSexyAppBase->mCritSect.Lock();
    gSexyAppBase->mPIEffects.insert(this);
    gSexyAppBase->mCritSect.Unlock();

    mDef = new PIEffectDef();
}

Sexy::PIEffect::~PIEffect()
{
    ResetAnim();
    Deref();
    gSexyAppBase->mCritSect.Lock();
    gSexyAppBase->mPIEffects.erase(this);
    gSexyAppBase->mCritSect.Unlock();
}

namespace
{

int ReadBlockByte(Sexy::PIEffect *theEffect)
{
    if (theEffect->mReadPos == 1024)
    {
        theEffect->mReadPos = 0;
        unsigned int words = (unsigned int)p_fread(theEffect->mReadBuf, 1, 1024, theEffect->mReadFile) >> 2;
        unsigned int checksum = theEffect->mChecksum;
        for (unsigned int i = 0; i != words; ++i)
            checksum += theEffect->mReadWords[i] ^ i;
        theEffect->mChecksum = checksum;
    }
    unsigned char byte = theEffect->mReadBuf[theEffect->mReadPos];
    theEffect->mReadPos += 1;
    return byte;
}

void ReadRaw(Sexy::PIEffect *theEffect, int theLen)
{
    unsigned char stage[4];
    std::memcpy(stage, &theEffect->mReadValue, 4);
    for (int i = 0; i < theLen; ++i)
        stage[i] = (unsigned char)ReadBlockByte(theEffect);
    std::memcpy(&theEffect->mReadValue, stage, 4);
}

float ReadRawFloat(Sexy::PIEffect *theEffect)
{
    ReadRaw(theEffect, 4);
    float stored = 0.0f;
    std::memcpy(&stored, &theEffect->mReadValue, sizeof(stored));
    return EndianFloat(stored);
}

bool ReadFileBool(Sexy::PIEffect *theEffect)
{
    if (theEffect->mWideBooleans)
        return ReadBlockByte(theEffect) != 0;
    ReadRaw(theEffect, 4);
    return EndianInt(theEffect->mReadValue) != 0;
}

int ReadFileInt(Sexy::PIEffect *theEffect)
{
    ReadRaw(theEffect, 4);
    return EndianInt(theEffect->mReadValue);
}

short ReadFileShort(Sexy::PIEffect *theEffect)
{
    ReadRaw(theEffect, 2);
    return EndianShort((short)theEffect->mReadValue);
}

void ClearSpawn(Sexy::PIParticleDefInstance *theRecord)
{
    theRecord->mField0 = 0.0f;
    theRecord->mField1 = 0.0f;
    theRecord->mField2 = 0;
    theRecord->mField3 = 0;
}

Sexy::PIParticleType *TypeOf(Sexy::PIEffect *theEffect, Sexy::PIParticleDef *theDef)
{
    return theEffect->FindType(theDef);
}

Sexy::PILiveEmitter *&CurrentLive()
{
    static Sexy::PILiveEmitter *current = nullptr;
    return current;
}

struct LiveScope
{
    Sexy::PILiveEmitter *mPrevious;

    explicit LiveScope(Sexy::PILiveEmitter *theLive) :
        mPrevious(CurrentLive())
    {
        CurrentLive() = theLive;
    }

    ~LiveScope()
    {
        CurrentLive() = mPrevious;
    }
};

Sexy::PILiveEmitter *LiveOf(Sexy::PIEffect *theEffect, Sexy::PIEmitterInstance *theInstance)
{
    if (CurrentLive() != nullptr)
        return CurrentLive();
    return theEffect->FindLive(theInstance);
}

float CurveAtTime(Sexy::PIValue *theCurve, float theTime, float theStep)
{
    if (theCurve == nullptr)
        return 0.0f;
    return theCurve->GetValueAt(theTime, theStep);
}

} // namespace

Sexy::String Sexy::PIEffect::ReadStringS()
{
    int length = EndianShort(ReadFileShort(this));
    if (length == -1)
    {
        ReadFileShort(this);
        length = EndianShort(ReadFileShort(this));
    }
    else if (length < 0)
    {
        int index = length & 0x7FFF;
        if (index < 0 || index >= (int)mStrings.size())
            return Sexy::String();
        Sexy::String result = mStrings[index];
        mStrings.push_back(result);
        return result;
    }

    Sexy::String result;
    if (length > 0)
    {
        char blank[256];
        std::memset(blank, 0, sizeof(blank));
        int filled = 0;
        while (filled < length)
        {
            int chunk = length - filled;
            if (chunk > 256)
                chunk = 256;
            result += Sexy::String(blank, chunk);
            filled += chunk;
        }
        char *data = result.data();
        for (int i = 0; i < length; ++i)
            data[i] = (char)ReadBlockByte(this);
    }
    mStrings.push_back(result);
    mStrings.push_back(result);
    return result;
}

int Sexy::PIEffect::ExpectCmd(Sexy::String const &theExpected)
{
    if (mWideBooleans)
        return 1;
    Sexy::String read = ReadStringS();
    if (read == theExpected)
        return 1;
    Sexy::String message = "Expected '";
    message += theExpected;
    message += "'";
    return Fail(message);
}

void Sexy::PIEffect::ReadValue2D(Sexy::PIValue2D *theCurve)
{
    int count = EndianShort(ReadFileShort(this));
    bool hasTangents = false;
    if (mWideBooleans && count > 1)
        hasTangents = ReadBlockByte(this) != 0;
    else if (count <= 0)
        return;

    std::vector<float> times;
    std::vector<Sexy::TPoint<float>> points;
    std::vector<Sexy::TPoint<float>> tangents;
    for (int i = 0; i != count; ++i)
    {
        ExpectCmd(Sexy::String("CKey"));
        float time = (float)ReadFileInt(this);
        times.push_back(time);
        float x = ReadRawFloat(this);
        float y = ReadRawFloat(this);
        points.push_back(Sexy::TPoint<float>(x, y));
        if (!mWideBooleans || hasTangents)
        {
            float inX = ReadRawFloat(this);
            float inY = ReadRawFloat(this);
            if (i != 0)
                tangents.push_back(Sexy::TPoint<float>(x + inX, y + inY));
            float outX = ReadRawFloat(this);
            float outY = ReadRawFloat(this);
            tangents.push_back(Sexy::TPoint<float>(x + outX, y + outY));
            if (!mWideBooleans)
            {
                ReadFileInt(this);
                int flags = ReadFileInt(this);
                hasTangents = hasTangents || (flags & 1) == 0;
            }
        }
        PushCurveKey(theCurve, time, x, y);
    }
    if (count > 1 && hasTangents && !points.empty())
        theCurve->mBezier.Init(points.data(), tangents.data(), times.data(), count);
}

int Sexy::PIEffect::ReadEPoint(Sexy::PIValue2D *theCurve)
{
    int count = EndianShort(ReadFileShort(this));
    for (int i = 0; i != count; ++i)
    {
        ExpectCmd(Sexy::String("CPointKey"));
        const float time = (float)ReadFileInt(this);
        const float x = ReadRawFloat(this);
        const float y = ReadRawFloat(this);
        PushCurveKey(theCurve, time, x, y);
    }
    return 0;
}

void Sexy::PIEffect::ReadValue(Sexy::PIValue *theCurve)
{
    int flags = ReadBlockByte(this);
    int count = flags & 7;
    if (count == 7)
        count = EndianShort(ReadFileShort(this));
    ResizeKeys(theCurve, count);
    bool hasTangents = (flags & 8) != 0;
    std::vector<float> times;
    std::vector<Sexy::TPoint<float>> points;
    std::vector<Sexy::TPoint<float>> tangents;
    int tangent = 0;
    for (int i = 0; i < count; ++i)
    {
        float time = 0.0f;
        float value = 0.0f;
        if (i == 0 && (flags & 0x10) != 0)
            time = 0.0f;
        else
            time = ReadRawFloat(this);
        if (i == 0 && (flags & 0x60) == 0x20)
            value = 0.0f;
        else if (i == 0 && (flags & 0x60) == 0x40)
            value = 1.0f;
        else if (i == 0 && (flags & 0x60) == 0x60)
            value = 2.0f;
        else
            value = ReadRawFloat(this);
        StoreKey(theCurve, i, time, value);
        times.push_back(time);
        points.push_back(Sexy::TPoint<float>(time, value));
        if (hasTangents)
        {
            float inX = ReadRawFloat(this);
            float inY = ReadRawFloat(this);
            if (i != 0)
            {
                tangents.push_back(Sexy::TPoint<float>(time + inX, value + inY));
                ++tangent;
            }
            float outX = ReadRawFloat(this);
            float outY = ReadRawFloat(this);
            tangents.push_back(Sexy::TPoint<float>(time + outX, value + outY));
            ++tangent;
            (void)tangent;
        }
    }
    if (count > 1 && hasTangents && !points.empty())
        theCurve->mBezier.Init(points.data(), tangents.data(), times.data(), count);
}

int Sexy::PIEffect::ReadEmitterType(Sexy::PIEmitter *theEmitter)
{
    ReadFileInt(this);
    theEmitter->mName = ReadString();
    theEmitter->mSerialize = ReadFileBool(this);
    ReadFileInt(this);
    theEmitter->mEmitInReverse = ReadFileBool(this);
    int defCount = EndianShort(ReadFileShort(this));
    for (int i = 0; i < defCount; ++i)
    {
        ExpectCmd(Sexy::String("CEmParticleType"));
        PIParticleType *type = new PIParticleType();
        for (int skipped = 0; skipped != 16; ++skipped)
            ReadFileInt(this);
        type->mAdditive = ReadFileBool(this);
        type->mSuperEmitter = ReadFileBool(this);
        type->mAdditiveSecond = ReadFileBool(this);
        type->mAttachToParent = ReadFileBool(this);
        type->mAttachWeight = ReadRawFloat(this);
        type->mFlipX = ReadFileBool(this);
        type->mFlipY = ReadFileBool(this);
        type->mRandomFrame = ReadFileBool(this);
        type->mColorRepeat = ReadFileInt(this);
        type->mAlphaRepeat = ReadFileInt(this);
        type->mAlignToPath = ReadFileBool(this);
        type->mName = ReadString();
        type->mAlignToVelocity = ReadFileBool(this);
        type->mUseOwnAngle = ReadFileBool(this);
        ReadFileBool(this);
        type->mFixedAngle = ReadFileInt(this);
        type->mAngleOffset = ReadFileInt(this);
        type->mAnimLength = ReadFileInt(this);
        type->mUseColorKeys = ReadFileBool(this);
        ReadFileInt(this);
        type->mImageIndex = ReadFileInt(this);
        int colorCount = EndianShort(ReadFileShort(this));
        for (int key = 0; key < colorCount; ++key)
        {
            ExpectCmd(Sexy::String("CColorPoint"));
            int red = ReadBlockByte(this);
            int green = ReadBlockByte(this);
            int blue = ReadBlockByte(this);
            PIInterpolatorPoint point;
            point.mField0 = ColorWord(red, green, blue);
            point.mField1 = FloatToBits(ReadRawFloat(this));
            type->mColorKeys.mKeyframes.push_back(point);
        }
        int alphaCount = EndianShort(ReadFileShort(this));
        for (int key = 0; key < alphaCount; ++key)
        {
            ExpectCmd(Sexy::String("CAlphaPoint"));
            PIInterpolatorPoint point;
            point.mField0 = ReadBlockByte(this);
            point.mField1 = FloatToBits(ReadRawFloat(this));
            type->mAlphaKeys.mKeyframes.push_back(point);
        }
        for (int curve = 0; curve != 23; ++curve)
            ReadValue(&type->mValues[curve]);
        type->mPivotX = ReadRawFloat(this);
        type->mPivotY = ReadRawFloat(this);
        if (!mWideBooleans && mDef != nullptr && type->mImageIndex >= 0 &&
            type->mImageIndex < (int)mDef->mImages.size())
        {
            Sexy::DeviceImage *image = ImageOf(mDef->mImages[type->mImageIndex]->mImage);
            if (image != nullptr && image->mWidth != 0 && image->mHeight != 0)
            {
                type->mPivotX = type->mPivotX / (float)image->mWidth;
                type->mPivotY = type->mPivotY / (float)image->mHeight;
            }
        }
        ReadFileInt(this);
        ReadFileInt(this);
        type->mLockSpin = ReadFileBool(this);
        ReadValue(&type->mSpinY);
        ReadValue(&type->mSpinVarY);
        ReadValue(&type->mExtraCurve);
        type->mAngleSpread = ReadFileInt(this);
        type->mAngleSpreadRange = ReadFileInt(this);
        type->mSampleColor = ReadFileBool(this);
        type->mSampleAlpha = ReadFileBool(this);
        ReadFileBool(this);
        ReadValue(&type->mAngle);
        ReadValue(&type->mSpread);
        ReadFileInt(this);
        ReadValue(&type->mSpread);
        type->mKeepAlpha = ReadFileBool(this);
        type->mForceAlpha = ReadFileBool(this);
        type->mUseSpawnColor = ReadFileBool(this);
        type->mColorStride = ReadFileInt(this);
        type->mAlignToPath = ReadFileBool(this);

        theEmitter->mParticleDefs.push_back(PIParticleDef());
        type->mPublished = &theEmitter->mParticleDefs.back();
        if (mDef != nullptr)
            mDef->mTypes.push_back(type);
        else
            delete type;
    }
    ReadFileInt(this);
    for (int i = 0; i != 42; ++i)
        ReadValue(&theEmitter->mValues[i]);
    theEmitter->mHasLifeKeys = KeyCount(theEmitter->mValues[0]) > 0;
    ReadFileInt(this);
    ReadFileInt(this);
    return 0;
}

int Sexy::PIEffect::LoadEffect(Sexy::String const &thePath)
{
    Deref();
    Clear();
    mDef = new PIEffectDef();
    mFilePath = thePath;
    mError.clear();
    mChecksum = 0;
    mReadPos = 1024;
    mWideBooleans = false;

    std::vector<PILiveEmitter> loadedEmitters;
    Sexy::String path = thePath;
    mReadFile = p_fopen(path.c_str(), "rb");
    if (mReadFile == nullptr)
        return Fail(Sexy::String("Failed to open effect"));

    unsigned char magic[4];
    if (p_fread(magic, 1, 4, mReadFile) != 4)
    {
        p_fclose(mReadFile);
        mReadFile = nullptr;
        return Fail(Sexy::String("Failed to read effect"));
    }
    mFileVersion = (int)magic[0] | ((int)magic[1] << 8);
    mWideBooleans = magic[2] != 0;

    for (;;)
    {
        if (p_feof(mReadFile) != 0)
            break;
        Sexy::String command = ReadStringS();
        if (command.empty())
            break;
        if (command == "CEmitterType")
        {
            PIEmitter *emitter = new PIEmitter();
            ReadEmitterType(emitter);
            mDef->mEmitters.push_back(emitter);
            continue;
        }
        if (command == "CMultiTexture" || command == "CTexture")
        {
            PITextureData *texture = new PITextureData();
            texture->mName = ReadString();
            texture->mAtlas = command == "CTexture";
            int count = ReadFileInt(this);
            if (count < 0)
                count = 0;
            texture->mImageCount = count;
            for (int i = 0; i < count; ++i)
            {
                Sexy::String imageName = ReadString();
                texture->mRefs.push_back(GetImage(Sexy::String(), imageName));
            }
            if (!texture->mRefs.empty())
                texture->mImage = texture->mRefs[0];
            mDef->mImages.push_back(texture);
            continue;
        }
        if (command == "CLayer")
        {
            PILayerDef layer;
            layer.mName = ReadString();
            int emitterCount = ReadFileInt(this);
            for (int i = 0; i < emitterCount; ++i)
            {
                ExpectCmd(Sexy::String("CEmitter"));
                PIEmitterInstanceDef placed;
                PILiveEmitter loaded;
                loaded.mName = ReadString();
                loaded.mLifeBias = ReadFileInt(this);
                loaded.mEmitterType = ReadFileInt(this);
                loaded.mShape = ReadFileInt(this);
                if (loaded.mShape == 2)
                    loaded.mShape = 4;
                loaded.mHeadingLock = ReadFileBool(this);
                loaded.mHeadingRandom = ReadFileBool(this);
                loaded.mCount = ReadFileInt(this);
                loaded.mRows = ReadFileInt(this);
                loaded.mFreeEmitters = ReadFileBool(this);
                int curveCount = ReadFileInt(this);
                for (int curve = 0; curve < curveCount; ++curve)
                    placed.mCurve.push_back(ReadFileInt(this));
                loaded.mMaskCompare = ReadFileBool(this);
                ReadEPoint(&placed.mValue2D);
                for (int value = 0; value != 19; ++value)
                    ReadValue(&placed.mValues[value]);
                int extra = ReadFileInt(this);
                for (int curve = 0; curve < extra; ++curve)
                {
                    PIValue2D value;
                    ReadEPoint(&value);
                    placed.mCurves.push_back(value);
                }
                layer.mEmitters.push_back(placed);
                loadedEmitters.push_back(loaded);
            }
            ReadValue2D(&layer.mValue2D);
            layer.mValue2D.GetValueAt(0.0f);
            layer.mOffsetX = layer.mValue2D.mLastValueX;
            layer.mOffsetY = layer.mValue2D.mLastValueY;
            ReadValue(&layer.mSpin);

            int deflectorCount = ReadFileInt(this);
            for (int i = 0; i < deflectorCount; ++i)
            {
                ExpectCmd(Sexy::String("CDeflector"));
                PIDeflector deflector;
                deflector.mName = ReadString();
                deflector.mBounce = ReadRawFloat(this);
                deflector.mHitChance = ReadRawFloat(this);
                deflector.mThickness = ReadRawFloat(this);
                deflector.mVisible = ReadFileBool(this);
                ReadValue2D(&deflector.mOffset);
                ReadValue(&deflector.mActive);
                ReadValue(&deflector.mSpin);
                int points = ReadFileInt(this);
                for (int point = 0; point < points; ++point)
                {
                    PIValue2D curve;
                    ReadEPoint(&curve);
                    deflector.mPointCurves.push_back(curve);
                    deflector.mPoints.push_back(Sexy::TPoint<float>());
                }
                layer.mDeflectors.push_back(deflector);
            }
            int blockerCount = ReadFileInt(this);
            for (int i = 0; i < blockerCount; ++i)
            {
                ExpectCmd(Sexy::String("CBlocker"));
                PIBlocker blocker;
                blocker.mName = ReadString();
                blocker.mFlag = ReadFileBool(this);
                ReadValue2D(&blocker.mOffset);
                ReadValue(&blocker.mActive);
                ReadValue(&blocker.mSpin);
                int points = ReadFileInt(this);
                for (int point = 0; point < points; ++point)
                {
                    PIValue2D curve;
                    ReadEPoint(&curve);
                    blocker.mPoints.push_back(curve);
                }
                layer.mBlockers.push_back(blocker);
            }
            int forceCount = ReadFileInt(this);
            for (int i = 0; i < forceCount; ++i)
            {
                ExpectCmd(Sexy::String("CForce"));
                PIForce force;
                force.mName = ReadString();
                force.mEnabled = ReadFileBool(this);
                ReadValue2D(&force.mValue2D);
                ReadValue(&force.mStrength);
                ReadValue(&force.mDirection);
                ReadValue(&force.mActive);
                ReadValue(&force.mSpin);
                ReadValue(&force.mExtra);
                ReadValue(&force.mAngle);
                layer.mForces.push_back(force);
            }
            mDef->mLayerDefs.push_back(layer);
            continue;
        }
        if (command == "CDataKey" || command == "CDataOverLifeKey")
        {
            Sexy::String key = ReadString();
            Sexy::String value = ReadString();
            Sexy::String upper;
            Upper(&upper, &key);
            mNotes[upper] = value;
            mNotesText += key;
            mNotesText += "=";
            mNotesText += value;
            mNotesText += "\n";
            if (upper == "RAND")
            {
                int cursor = 0;
                while (cursor < (int)value.size())
                {
                    int comma = -1;
                    const char *text = value.c_str();
                    for (unsigned int index = (unsigned int)cursor; index < value.size(); ++index)
                    {
                        if (text[index] == ',')
                        {
                            comma = (int)index;
                            break;
                        }
                    }
                    int end = comma < 0 ? (int)value.size() : comma;
                    Sexy::String token = value.substr(cursor, end - cursor);
                    if (!token.empty())
                        mRandSeeds.push_back(std::atoi(token.c_str()));
                    if (comma < 0)
                        break;
                    cursor = comma + 1;
                }
            }
            if (upper == "EMITAFTER")
                mDebugDraw = value != "no";
            continue;
        }
        if (command == "Notes" || command == "CNotes")
        {
            mNotesText = ReadString();
            continue;
        }
        break;
    }
    p_fclose(mReadFile);
    mReadFile = nullptr;

    mLayers.clear();
    mLive.clear();
    mLayers.resize(mDef->mLayerDefs.size());
    mLive.resize(mDef->mLayerDefs.size());
    for (size_t layerIndex = 0; layerIndex < mDef->mLayerDefs.size(); ++layerIndex)
    {
        PILayerDef &layerDef = mDef->mLayerDefs[layerIndex];
        mLive[layerIndex].mDef = &layerDef;
        mLive[layerIndex].mPublic = &mLayers[layerIndex];
        mLayers[layerIndex].mVisible = true;
        mLayers[layerIndex].mColor = Sexy::Color::White;
        mLayers[layerIndex].mTransform.LoadIdentity();
        mLive[layerIndex].mEmitters.resize(layerDef.mEmitters.size());
        mLayers[layerIndex].mEmitters.resize(layerDef.mEmitters.size());
        size_t loadedBase = 0;
        for (size_t earlier = 0; earlier < layerIndex; ++earlier)
            loadedBase += mDef->mLayerDefs[earlier].mEmitters.size();
        for (size_t emitterIndex = 0; emitterIndex < layerDef.mEmitters.size(); ++emitterIndex)
        {
            PIEmitterInstanceDef &placed = layerDef.mEmitters[emitterIndex];
            PILiveEmitter &live = mLive[layerIndex].mEmitters[emitterIndex];
            live.mDef = &placed;
            if (loadedBase + emitterIndex < loadedEmitters.size())
            {
                PILiveEmitter &loaded = loadedEmitters[loadedBase + emitterIndex];
                live.mName = loaded.mName;
                live.mLifeBias = loaded.mLifeBias;
                live.mEmitterType = loaded.mEmitterType;
                live.mShape = loaded.mShape;
                live.mHeadingLock = loaded.mHeadingLock;
                live.mHeadingRandom = loaded.mHeadingRandom;
                live.mCount = loaded.mCount;
                live.mRows = loaded.mRows;
                live.mFreeEmitters = loaded.mFreeEmitters;
                live.mMaskCompare = loaded.mMaskCompare;
            }
            int typeIndex = live.mEmitterType;
            std::map<int, int>::iterator mapped = mDef->mEmitterIndex.find(typeIndex);
            if (mapped != mDef->mEmitterIndex.end())
                typeIndex = mapped->second;
            if (typeIndex >= 0 && typeIndex < (int)mDef->mEmitters.size())
                live.mEmitter = mDef->mEmitters[typeIndex];
            live.mActive = true;
            live.mEmitting = false;
            live.mKeepSpawn = true;
            live.mRateScale = 1.0f;
            live.mTint = mLoadColor;
            live.mTransform.LoadIdentity();
            if (live.mEmitter != nullptr)
                live.mDefSpawns.resize(live.mEmitter->mParticleDefs.size());
            live.mFreeSpawns.resize(placed.mCurve.size());
            live.mFree.mUseEmitter = live.mFreeEmitters ? 1 : 0;
            live.mTrail.mUseEmitter = 0;
            mLayers[layerIndex].mEmitters[emitterIndex].mName = live.mName;
        }
    }
    mLoaded = mError.empty();
    mLoadedFlag = mLoaded;
    return mLoaded ? 1 : 0;
}

Sexy::TPoint<float> Sexy::PIEffect::GetGeomPos(
    Sexy::PIEmitterInstance *theInstance, PIParticle *theParticle, float *theAngleOut, bool *theMask)
{
    Sexy::TPoint<float> pos;
    PILiveEmitter *live = LiveOf(this, theInstance);
    if (live == nullptr || live->mDef == nullptr)
        return pos;
    PIEmitterInstanceDef *def = live->mDef;
    int shape = live->mShape;
    float radiusX = CurveAtTime(&def->mValues[15], mTime, 0.0f);
    float radiusY = CurveAtTime(&def->mValues[16], mTime, 0.0f);
    int count = live->mCount;
    int rows = live->mRows;
    int spawn = theParticle != nullptr ? theParticle->mSpawnIndex : 0;
    float angle = 0.0f;

    if (shape == 1)
    {
        int curves = (int)def->mCurves.size();
        int length = 0;
        if (curves > 1)
        {
            for (int i = 0; i < curves - 1; ++i)
            {
                def->mCurves[i].GetValueAt(mTime);
                float x0 = def->mCurves[i].mLastValueX;
                float y0 = def->mCurves[i].mLastValueY;
                def->mCurves[i + 1].GetValueAt(mTime);
                float dx = def->mCurves[i + 1].mLastValueX - x0;
                float dy = def->mCurves[i + 1].mLastValueY - y0;
                length = (int)((dx * dx + dy * dy) + (float)length);
            }
        }
        float along = 0.0f;
        if (count != 0)
            along = (float)(length * (spawn % count)) / (float)(count - 1 > 0 ? count - 1 : 1);
        else
            along = GetRandFloatU() * (float)length;
        float walked = 0.0f;
        float blend = 0.0f;
        int segment = 0;
        if (curves > 1)
        {
            for (int i = 0; i < curves - 1; ++i)
            {
                def->mCurves[i].GetValueAt(mTime);
                float x0 = def->mCurves[i].mLastValueX;
                float y0 = def->mCurves[i].mLastValueY;
                def->mCurves[i + 1].GetValueAt(mTime);
                float dx = def->mCurves[i + 1].mLastValueX - x0;
                float dy = def->mCurves[i + 1].mLastValueY - y0;
                float span = dx * dx + dy * dy;
                if (along >= walked && along <= walked + span && span != 0.0f)
                {
                    blend = (along - walked) / span;
                    segment = i;
                    break;
                }
                walked += span;
            }
        }
        if (curves > 0)
        {
            int next = segment + 1 < curves ? segment + 1 : segment;
            def->mCurves[segment].GetValueAt(mTime);
            float x0 = def->mCurves[segment].mLastValueX;
            float y0 = def->mCurves[segment].mLastValueY;
            def->mCurves[next].GetValueAt(mTime);
            float x1 = def->mCurves[next].mLastValueX;
            float y1 = def->mCurves[next].mLastValueY;
            pos.mX = blend * x1 + (1.0f - blend) * x0;
            pos.mY = blend * y1 + (1.0f - blend) * y0;
            if (theAngleOut != nullptr)
            {
                float side = 1.0f;
                if (live->mHeadingLock)
                    side = live->mHeadingRandom ? GetRandSign() : -1.0f;
                *theAngleOut = (*theAngleOut + kHalfPi) + side * kHalfPi + atan2f(y1 - y0, x1 - x0);
            }
        }
    }
    else if (shape == 2 || shape == 4)
    {
        if (count != 0)
        {
            angle = (float)(spawn % count) * kTwoPi / (float)count;
            if (shape == 2 && angle > kPi)
                angle -= kTwoPi;
        }
        else
        {
            angle = GetRandFloat() * kPi;
        }
        if (shape == 2 && radiusX > radiusY)
        {
            float ratio = radiusX / (radiusY != 0.0f ? radiusY : 1.0f);
            float power = (ratio * 0.3f - 0.3f) + 1.0f;
            if (angle >= -kHalfPi)
            {
                if (angle < 0.0f)
                    angle = -(powf(-(angle * kInvHalfPi), power) * kHalfPi);
                else if (angle >= kHalfPi)
                    angle = kPi - powf((kPi - angle) * kInvHalfPi, power) * kHalfPi;
                else
                    angle = powf(angle * kInvHalfPi, power) * kHalfPi;
            }
            else
            {
                angle = powf((angle + kPi) * kInvHalfPi, power) * kHalfPi + kPi;
            }
        }
        else if (shape == 2 && radiusX < radiusY)
        {
            float ratio = radiusY / (radiusX != 0.0f ? radiusX : 1.0f);
            float power = (ratio * 0.3f - 0.3f) + 1.0f;
            if (angle < -kHalfPi)
                angle = -kHalfPi - powf((-kHalfPi - angle) * kInvHalfPi, power) * kHalfPi;
            else if (angle < 0.0f)
                angle = (powf((angle + kHalfPi) * kInvHalfPi, power) - 1.0f) * kHalfPi;
            else if (angle >= kHalfPi)
                angle = (powf((angle - kHalfPi) * kInvHalfPi, power) + 1.0f) * kHalfPi;
            else
                angle = (1.0f - powf((kHalfPi - angle) * kInvHalfPi, power)) * kHalfPi;
        }
        float used = shape == 4 ? radiusX : radiusX;
        pos.mX = cosf(angle) * used;
        pos.mY = sinf(angle) * used;
        if (theAngleOut != nullptr)
        {
            float side = kHalfPi;
            if (live->mHeadingLock)
                side = live->mHeadingRandom ? GetRandSign() * kHalfPi : -kHalfPi;
            *theAngleOut = angle + *theAngleOut + side;
        }
    }
    else if (shape == 3)
    {
        if (count != 0)
        {
            if (count > 1)
                pos.mX = radiusX * (((float)(spawn % count) / (float)(count - 1)) - 0.5f);
            if (rows > 1)
                pos.mY = radiusY * (((float)((spawn / (count != 0 ? count : 1)) % rows) / (float)(rows - 1)) - 0.5f);
        }
        else
        {
            pos.mX = (radiusX * 0.5f) * GetRandFloat();
            pos.mY = (radiusY * 0.5f) * GetRandFloat();
        }
        Sexy::DeviceImage *mask = ImageOf(live->mImage);
        if (mask != nullptr && theMask != nullptr && radiusX != 0.0f && radiusY != 0.0f)
        {
            float u = pos.mX / radiusX + 0.5f;
            float v = pos.mY / radiusY + 0.5f;
            int px = (int)(u * (float)mask->mWidth);
            int py = (int)(v * (float)mask->mHeight);
            if (px >= mask->mWidth - 1)
                px = mask->mWidth - 1;
            if (py >= mask->mHeight - 1)
                py = mask->mHeight - 1;
            if (px < 0)
                px = 0;
            if (py < 0)
                py = 0;
            unsigned int *bits = mask->GetBits();
            if (bits != nullptr)
            {
                int sample = (int)bits[px + py * mask->mWidth];
                bool negative = sample < 0;
                if (live->mMaskCompare != negative)
                    *theMask = true;
            }
        }
    }

    Sexy::TPoint<float> emitter = GetEmitterPos(theInstance, false);
    pos.mX += emitter.mX + live->mOffsetX;
    pos.mY += emitter.mY + live->mOffsetY;
    Sexy::TPoint<float> transformed;
    TransformFPoint(&transformed, live->mTransform, pos);
    TransformFPoint(&pos, mWorldTransform, transformed);
    return pos;
}

int Sexy::PIEffect::CalcParticleTransform(Sexy::PILayer *theLayer, Sexy::PIEmitterInstance *theInstance,
    Sexy::PIEmitter *theEmitter, Sexy::PIParticleDef *theDef, PIParticleGroup *theGroup, PIParticle *theParticle)
{
    (void)theLayer;
    (void)theEmitter;
    PIParticleType *type = theParticle != nullptr ? theParticle->mType : TypeOf(this, theDef);
    PILiveEmitter *live = LiveOf(this, theInstance);
    PIEmitterInstanceDef *placed = live != nullptr ? live->mDef : nullptr;
    float life = theParticle != nullptr ? theParticle->mLifePct : 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float area = 1.0f;
    int imageW = 0;
    int imageH = 0;
    bool scaled = false;

    if (type != nullptr && mDef != nullptr && type->mImageIndex >= 0 && type->mImageIndex < (int)mDef->mImages.size())
    {
        PITextureData *texture = mDef->mImages[type->mImageIndex];
        Sexy::DeviceImage *image = nullptr;
        if (!texture->mRefs.empty())
        {
            int frame = theParticle != nullptr ? theParticle->mFrameIndex : 0;
            if (frame < 0 || frame >= (int)texture->mRefs.size())
                frame = 0;
            image = ImageOf(texture->mRefs[frame]);
            if (image != nullptr)
            {
                imageW = image->mWidth;
                imageH = image->mHeight;
            }
        }
        else
        {
            image = ImageOf(texture->mImage);
            if (image != nullptr)
            {
                Sexy::TRect<int> cel = image->GetCelRect(0);
                imageW = cel.mWidth;
                imageH = cel.mHeight;
                if (texture->mAtlas)
                {
                    imageW -= 2;
                    imageH -= 2;
                }
            }
        }
        if (type->mSuperEmitter && theParticle != nullptr)
        {
            float emitterScale = theGroup != nullptr && theGroup->mUseEmitter != 0 && theEmitter != nullptr
                ? CurveAtTime(EmitterCurve(theEmitter, 10), mTime, 0.0f)
                : (placed != nullptr ? CurveAtTime(&placed->mValues[2], mTime, 0.0f) : 1.0f);
            theParticle->mSpinScaleX =
                (CurveAtTime(&type->mValues[2], mTime, 0.0f) + theParticle->mVarSpin) * emitterScale;
            float emitterScaleY = theGroup != nullptr && theGroup->mUseEmitter != 0 && theEmitter != nullptr
                ? CurveAtTime(EmitterCurve(theEmitter, 11), mTime, 0.0f)
                : (placed != nullptr ? CurveAtTime(&placed->mValues[17], mTime, 0.0f) : 1.0f);
            theParticle->mSpinScaleY =
                (CurveAtTime(&type->mValues[25], mTime, 0.0f) + theParticle->mVarSpin2) * emitterScaleY;
        }
        if (theParticle != nullptr)
        {
            scaleX = CurveAtTime(&type->mValues[16], life, 0.0f) * theParticle->mSpinScaleX;
            if (scaleX < 0.1f)
                scaleX = 0.1f;
            scaleY = CurveAtTime(&type->mValues[27], life, 0.0f) * theParticle->mSpinScaleY;
            if (scaleY < 0.1f)
                scaleY = 0.1f;
        }
        int longest = imageW > imageH ? imageW : imageH;
        float basis = longest != 0 ? (float)longest : 1.0f;
        float drawX = (scaleX / basis) * 2.0f;
        float drawY = (scaleY / basis) * 2.0f;
        area = drawX * drawY;
        scaled = drawX != 1.0f || drawY != 1.0f;
        scaleX = drawX;
        scaleY = drawY;
    }

    Sexy::SexyTransform2D basis;
    if (placed != nullptr)
    {
        float spin = CurveAtTime(&placed->mValues[14], mTime, 0.0f);
        if (spin != 0.0f)
            basis.LoadRotateDeg(spin);
    }
    if (theParticle != nullptr && theParticle->mParent != nullptr && theParticle->mParent->mAngle != 0.0f)
        basis.RotateRad(-theParticle->mParent->mAngle);

    float m00 = 1.0f;
    float m10 = 0.0f;
    float m01 = 0.0f;
    float m11 = 1.0f;
    float tx = 0.0f;
    float ty = 0.0f;
    if (type != nullptr)
    {
        float pivotX =
            -(type->mPivotX * (imageH != 0 ? (float)imageH / (float)(imageW != 0 ? imageW : 1) : 1.0f)) * (float)imageW;
        float pivotY =
            -(type->mPivotY * (imageW != 0 ? (float)imageW / (float)(imageH != 0 ? imageH : 1) : 1.0f)) * (float)imageH;
        tx = -(type->mPivotX) * (float)imageW;
        ty = -(type->mPivotY) * (float)imageH;
        (void)pivotX;
        (void)pivotY;
        if (type->mFlipX)
        {
            m00 = -1.0f;
            tx = -tx;
        }
        if (type->mFlipY)
        {
            m11 = -1.0f;
            ty = -ty;
        }
    }
    if (scaled)
    {
        m00 *= scaleX;
        m11 *= scaleY;
        tx *= scaleX;
        ty *= scaleY;
    }
    if (theParticle != nullptr && theParticle->mAngle != 0.0f)
    {
        float sine = sinf(theParticle->mAngle);
        float cosine = cosf(theParticle->mAngle);
        float old00 = m00;
        float old11 = m11;
        m00 = cosine * old00 - sine * m10;
        m10 = sine * old00 + cosine * m10;
        float oldTx = tx;
        tx = cosine * tx - sine * ty;
        ty = sine * oldTx + cosine * ty;
        m01 = -old11 * sine;
        m11 = cosine * old11;
    }
    if (theParticle != nullptr)
    {
        tx += theParticle->mPosX;
        ty += theParticle->mPosY;
        float size = theParticle->mSize;
        if (type != nullptr && type->mSuperEmitter && theEmitter != nullptr)
        {
            float emitterSize = theGroup != nullptr && theGroup->mUseEmitter != 0
                ? CurveAtTime(EmitterCurve(theEmitter, 17), mTime, 0.0f)
                : (placed != nullptr ? CurveAtTime(&placed->mValues[8], mTime, 0.0f) : 1.0f);
            size = CurveAtTime(EmitterCurve(theEmitter, 17), mTime, 1.0f) * emitterSize;
            theParticle->mSize = size;
        }
        area *= size * size;
        if (size != 1.0f)
        {
            m00 *= size;
            m10 *= size;
            m01 *= size;
            m11 *= size;
            tx *= size;
            ty *= size;
        }
        if (type != nullptr && type->mAttachToParent && theGroup != nullptr && theGroup->mSkipAttach == 0 &&
            theParticle->mParent != nullptr)
        {
            float weight = type->mAttachWeight;
            float parentX =
                theParticle->mParent->mLaunchPosX + theParticle->mParent->mScaleX + theParticle->mParent->mPosX;
            float parentY =
                theParticle->mParent->mLaunchPosY + theParticle->mParent->mScaleY + theParticle->mParent->mPosY;
            theParticle->mLaunchPosX = weight * parentX + (1.0f - weight) * theParticle->mLaunchPosX;
            theParticle->mLaunchPosY = weight * parentY + (1.0f - weight) * theParticle->mLaunchPosY;
        }
        tx += theParticle->mLaunchPosX;
        ty += theParticle->mLaunchPosY;
    }
    if (theLayer != nullptr && mDef != nullptr)
    {
        for (size_t i = 0; i < mLive.size(); ++i)
        {
            if (mLive[i].mPublic == theLayer && mLive[i].mDef != nullptr)
            {
                mLive[i].mDef->mValue2D.GetValueAt(mTime);
                tx += mLive[i].mDef->mValue2D.mLastValueX - mLive[i].mDef->mOffsetX;
                ty += mLive[i].mDef->mValue2D.mLastValueY - mLive[i].mDef->mOffsetY;
                float spin = CurveAtTime(&mLive[i].mDef->mSpin, mTime, 0.0f);
                if (spin != 0.0f)
                {
                    float sine = sinf(spin * kDegToRad);
                    float cosine = cosf(spin * kDegToRad);
                    float old00 = m00;
                    m00 = cosine * m00 - sine * m10;
                    m10 = sine * old00 + cosine * m10;
                    float oldTx = tx;
                    tx = cosine * tx - sine * ty;
                    ty = sine * oldTx + cosine * ty;
                }
                break;
            }
        }
    }
    float simple[6];
    simple[0] = m00;
    simple[1] = m10;
    simple[2] = m01;
    simple[3] = m11;
    simple[4] = tx;
    simple[5] = ty;
    if (theParticle != nullptr)
    {
        theParticle->mTransform.SetMatrix(simple);
        theParticle->mArea = area;
    }
    (void)basis;
    return 0;
}

void Sexy::PIEffect::UpdateParticleGroup(
    Sexy::PILayer *theLayer, Sexy::PIEmitterInstance *theInstance, PIParticleGroup *theGroup)
{
    if (theGroup == nullptr || theGroup->mHead == nullptr)
        return;
    PILiveEmitter *live = LiveOf(this, theInstance);
    PIEmitterInstanceDef *placed = live != nullptr ? live->mDef : nullptr;
    float frameScale = mTimeScale != 0.0f ? 100.0f / mTimeScale : 100.0f;
    float step = 1.0f / frameScale;
    PIParticle *node = theGroup->mHead;
    while (node != nullptr)
    {
        PIParticle *next = node->mNext;
        PIParticleType *type = node->mType;
        float age = node->mEmitterTime;
        node->mEmitterTime = age + step;
        float life = 0.0f;
        if (type != nullptr && type->mSuperEmitter && placed != nullptr)
        {
            float nextKey = placed->mValues[13].GetNextKeyframeTime(mTime);
            int nextIndex = placed->mValues[13].GetNextKeyframeIdx(mTime);
            if (nextKey >= mTime && nextIndex == 1)
            {
                if (nextKey < 1.0f)
                    nextKey = 1.0f;
                life = ((float)live->mLifeBias + mTime) / nextKey;
                if (life > 1.0f)
                    life = 1.0f;
            }
            else
            {
                life = 0.02f;
            }
        }
        else if (node->mLife > 0.00000001f)
        {
            life = node->mEmitterTime / node->mLife;
        }
        node->mLifePct = life;
        bool expired =
            life >= 1.0f || node->mLife <= 0.00000001f || (live != nullptr && !live->mEmitting && !live->mFreeEmitters);
        if (expired)
        {
            if (theGroup->mSkipAttach == 0 && type != nullptr && type->mSuperEmitter && live != nullptr &&
                live->mEmitting)
            {
                node = next;
                continue;
            }
            UnlinkParticle(theGroup, node);
            DeleteChain(node);
            node = next;
            continue;
        }
        if (theGroup->mSkipAttach == 0 && type != nullptr && type->mSuperEmitter && placed != nullptr)
        {
            int shape = live->mShape;
            bool grid = false;
            if (shape == 1 || shape == 4)
                grid = live->mCount != 0;
            else if (shape == 3)
                grid = live->mCount * live->mRows != 0;
            if (grid)
            {
                Sexy::TPoint<float> geom = GetGeomPos(theInstance, node, nullptr, nullptr);
                Sexy::TPoint<float> origin = GetEmitterPos(theInstance, false);
                node->mScaleX = origin.mX;
                node->mScaleY = origin.mY;
                node->mVelX = geom.mX - origin.mX;
                node->mVelY = geom.mY - origin.mY;
            }
        }
        if (mTimeCrossed && theGroup->mSkipAttach == 0)
        {
            float jitter = GetRandFloat() * GetRandFloat();
            float jitterY = GetRandFloat() * GetRandFloat();
            float rate = 0.0f;
            if (type != nullptr)
            {
                float base = theGroup->mUseEmitter != 0 && node->mEmitter != nullptr
                    ? CurveAtTime(EmitterCurve(node->mEmitter, 15), mTime, 0.0f)
                    : (placed != nullptr ? CurveAtTime(&placed->mValues[6], mTime, 0.0f) : 0.0f);
                rate = CurveAtTime(&type->mValues[20], life, 0.0f) * base *
                    (CurveAtTime(&type->mValues[6], mTime, 0.0f) + node->mVarJitter);
                if (rate < 0.0f)
                    rate = 0.0f;
            }
            node->mSpinX += jitter * rate;
            node->mSpinY += jitterY * rate;
        }
        float gravityScale = theGroup->mSkipAttach != 0 ? 50.0f : 100.0f;
        float gravity = 0.0f;
        if (node->mEmitter != nullptr)
        {
            float emitterG = theGroup->mUseEmitter != 0
                ? CurveAtTime(EmitterCurve(node->mEmitter, 13), mTime, 0.0f)
                : (placed != nullptr ? CurveAtTime(&placed->mValues[4], mTime, 0.0f) : 0.0f);
            float overLife = type != nullptr ? CurveAtTime(&type->mValues[18], life, 0.0f) : 1.0f;
            float local = type != nullptr ? CurveAtTime(&type->mValues[4], mTime, 0.0f) : 0.0f;
            gravity = (overLife * (emitterG * gravityScale) - emitterG * gravityScale) * (local + node->mVarGravity);
        }
        float timeBias = ((float)mParticleScale - 100.0f) * 0.0005f + 1.0f;
        node->mSpinY += timeBias * step * gravity;
        float drag = type != nullptr ? CurveAtTime(&type->mValues[17], life, 0.0f) : 1.0f;
        float moveX = drag * step * node->mSpinX;
        float moveY = drag * step * node->mSpinY;
        node->mPosX += moveX;
        node->mPosY += moveY;
        PILayerDef *layerDef = nullptr;
        for (size_t layerIndex = 0; layerIndex < mLive.size(); ++layerIndex)
        {
            if (mLive[layerIndex].mPublic == theLayer)
            {
                layerDef = mLive[layerIndex].mDef;
                break;
            }
        }
        if (layerDef != nullptr && theGroup->mSkipAttach == 0)
        {
            Sexy::TPoint<float> start(node->mPosX - moveX, node->mPosY - moveY);
            Sexy::TPoint<float> end(node->mPosX, node->mPosY);
            for (size_t i = 0; i < layerDef->mDeflectors.size(); ++i)
            {
                PIDeflector &deflector = layerDef->mDeflectors[i];
                float active = deflector.mActive.GetLastKeyframe(mTime);
                if (FloatBits(FloatToBits(active)) <= 0.99f)
                    continue;
                if (GetRandFloatU() > deflector.mHitChance)
                    continue;
                int count = (int)deflector.mPoints.size();
                for (int point = 0; point + 1 < count; ++point)
                {
                    float fraction = 0.0f;
                    Sexy::TPoint<float> hit;
                    if (!LineSegmentIntersects(
                            start, end, deflector.mPoints[point], deflector.mPoints[point + 1], &fraction, &hit))
                        continue;
                    float dx = deflector.mPoints[point + 1].mX - deflector.mPoints[point].mX;
                    float dy = deflector.mPoints[point + 1].mY - deflector.mPoints[point].mY;
                    float length = sqrtf(dx * dx + dy * dy);
                    float nx = length != 0.0f ? -dy / length : 0.0f;
                    float ny = length != 0.0f ? dx / length : 0.0f;
                    float dot = node->mSpinX * nx + node->mSpinY * ny;
                    float reflX = node->mSpinX - 2.0f * dot * nx;
                    float reflY = node->mSpinY - 2.0f * dot * ny;
                    float bounce = powf(0.5f, deflector.mBounce);
                    float ratio = node->mVarBounce;
                    node->mSpinX = reflX * bounce;
                    node->mSpinY = reflY * bounce;
                    node->mSpinX = reflX * 100.0f;
                    node->mSpinY = reflY * 100.0f * (1.0f + (powf(0.5f, deflector.mBounce) - 1.0f) * ratio);
                    node->mPosX = hit.mX;
                    node->mPosY = hit.mY;
                    break;
                }
            }
            for (size_t i = 0; i < layerDef->mForces.size(); ++i)
            {
                PIForce &force = layerDef->mForces[i];
                if (!force.mEnabled)
                    continue;
                float active = force.mActive.GetLastKeyframe(mTime);
                if (FloatBits(FloatToBits(active)) <= 0.99f)
                    continue;
                int inside = 0;
                for (int corner = 0; corner != 4; ++corner)
                {
                    int next = (corner + 1) & 3;
                    float edgeX = force.mCorners[next].mX - force.mCorners[corner].mX;
                    float edgeY = force.mCorners[next].mY - force.mCorners[corner].mY;
                    float side = edgeX * (node->mPosY - force.mCorners[corner].mY) -
                        edgeY * (node->mPosX - force.mCorners[corner].mX);
                    if (((corner & 1) == 0 && side > 0.0f) || ((corner & 1) != 0 && side < 0.0f))
                        inside += 1;
                }
                if (inside != 4)
                    continue;
                float variation = node->mVarUnused;
                float magnitude = variation * 0.00085f + (variation - 100.0f) * 0.004f * variation;
                float strength = CurveAtTime(&force.mStrength, mTime, 0.0f);
                float direction = CurveAtTime(&force.mDirection, mTime, 0.0f) * kDegToRad;
                magnitude *= strength;
                node->mSpinX += cosf(direction) * magnitude * 100.0f;
                node->mSpinY += sinf(direction) * magnitude * 100.0f;
            }
        }
        CalcParticleTransform(theLayer, theInstance, node->mEmitter, nullptr, theGroup, node);
        if (type != nullptr && type->mAlignToVelocity)
            node->mAngle = -(float)type->mAngleOffset * kDegToRad - atan2f(moveY, moveX);
        if (theGroup->mSkipAttach == 0 && type != nullptr && type->mAlignToPath == 0)
        {
            float spin = 0.0f;
            if (theGroup->mUseEmitter != 0 && node->mEmitter != nullptr)
                spin = CurveAtTime(EmitterCurve(node->mEmitter, 14), mTime, 0.0f);
            else if (placed != nullptr)
                spin = CurveAtTime(&placed->mValues[5], mTime, 0.0f);
            float over = type != nullptr ? CurveAtTime(&type->mValues[19], life, 0.0f) : 0.0f;
            float local = type != nullptr ? CurveAtTime(&type->mValues[5], mTime, 0.0f) : 0.0f;
            node->mAngle += (step * spin) * (local + node->mVarSpinRate) * (1.0f - over);
        }
        node = next;
    }
}

Sexy::Color *Sexy::PIEffect::DrawParticleGroup(Sexy::Graphics *theGraphics, Sexy::PILayer *theLayer,
    Sexy::PIEmitterInstance *theInstance, PIParticleGroup *theGroup, unsigned char theAdditivePass)
{
    (void)theLayer;
    if (theGraphics == nullptr || theGroup == nullptr)
        return nullptr;
    PILiveEmitter *live = LiveOf(this, theInstance);
    if (live == nullptr || !live->mEmitting && theGroup->mHead == nullptr)
        return &mColor;
    if (!live->mActive)
        return &mColor;
    Sexy::Color tint(theLayer != nullptr ? theLayer->mColor.mRed * mColor.mRed / 255 : mColor.mRed,
        theLayer != nullptr ? theLayer->mColor.mGreen * mColor.mGreen / 255 : mColor.mGreen,
        theLayer != nullptr ? theLayer->mColor.mBlue * mColor.mBlue / 255 : mColor.mBlue,
        theLayer != nullptr ? theLayer->mColor.mAlpha * mColor.mAlpha / 255 : mColor.mAlpha);
    bool tinted = tint != Sexy::Color::White;
    for (PIParticle *node = theGroup->mHead; node != nullptr; node = node->mNext)
    {
        PIParticleType *type = node->mType;
        if (type == nullptr)
            continue;
        bool additive = type->mAdditive && (type->mAdditiveSecond || theAdditivePass == 0);
        if (theAdditivePass != 0 && !type->mAdditive)
            continue;
        theGraphics->SetDrawMode(additive ? 1 : 0);
        Sexy::Image *image = nullptr;
        Sexy::TRect<int> cel;
        if (mDef != nullptr && type->mImageIndex >= 0 && type->mImageIndex < (int)mDef->mImages.size())
        {
            PITextureData *texture = mDef->mImages[type->mImageIndex];
            if (!texture->mRefs.empty())
            {
                int frame = node->mFrameIndex;
                if (frame < 0 || frame >= (int)texture->mRefs.size())
                    frame = 0;
                image = ImageOf(texture->mRefs[frame]);
                if (image != nullptr)
                    cel = Sexy::TRect<int>(0, 0, image->mWidth, image->mHeight);
            }
            else
            {
                image = ImageOf(texture->mImage);
                if (image != nullptr)
                    cel = image->GetCelRect(node->mFrameIndex);
            }
        }
        unsigned int color = 0xFFFFFFFFu;
        if (type->mUseColorKeys)
        {
            if (type->mAlignToVelocity)
            {
                int count = (int)type->mColorKeys.mKeyframes.size();
                int index = (int)((float)count * node->mKeyParam);
                if (count > 0 && index >= count - 1)
                    index = count - 1;
                color = type->mColorKeys.GetKeyframeNum(index);
            }
            else
            {
                color = (unsigned int)type->mColorKeys.GetValueAt(node->mKeyParam);
            }
        }
        else if (type->mUseSpawnColor)
        {
            int count = (int)type->mColorKeys.mKeyframes.size();
            int index =
                count != 0 ? (node->mSpawnIndex / (type->mColorStride != 0 ? type->mColorStride : 1)) % count : 0;
            color = type->mColorKeys.GetKeyframeNum(index);
        }
        else
        {
            float mapped = node->mLifePct;
            if (type->mColorRepeat != 0)
            {
                float scaled = (float)(type->mColorRepeat + 1) * node->mLifePct;
                mapped = scaled - (float)(int)scaled;
            }
            color = (unsigned int)type->mColorKeys.GetValueAt(mapped);
        }
        if (type->mSampleColor)
            color = (color & 0xFF000000u) | (node->mColor & 0x00FFFFFFu);
        if (type->mKeepAlpha)
            color = (color & 0x00FFFFFFu) | (node->mColor & 0xFF000000u);
        float alphaScale = 1.0f;
        if (live->mDef != nullptr)
            alphaScale = CurveAtTime(&live->mDef->mValues[10], mTime, 0.0f);
        float typeAlpha = node->mEmitter != nullptr ? CurveAtTime(EmitterCurve(node->mEmitter, 20), mTime, 1.0f) : 1.0f;
        int blended = InterpColor((int)color, (int)live->mTint.ToInt(), typeAlpha * alphaScale);
        float life = node->mLifePct;
        if (type->mAlphaRepeat != 0)
        {
            float scaled = (float)(type->mAlphaRepeat + 1) * life;
            life = scaled - (float)(int)scaled;
        }
        float alpha = (float)(unsigned int)type->mAlphaKeys.GetValueAt(life);
        float alphaB = live->mDef != nullptr ? CurveAtTime(&live->mDef->mValues[9], mTime, 0.0f) : 1.0f;
        float alphaC = CurveAtTime(&type->mValues[22], mTime, 0.0f);
        float alphaA = node->mEmitter != nullptr ? CurveAtTime(EmitterCurve(node->mEmitter, 18), mTime, 1.0f) : 1.0f;
        unsigned int alphaByte = (unsigned int)(alphaC * alphaB * alphaA * alpha);
        if (theAdditivePass != 0)
            blended &= (int)0xFF000000;
        Sexy::Color draw;
        Sexy::Color::FromInt(&draw, (blended & 0xFFFFFF) | (int)(alphaByte << 24));
        if (tinted)
        {
            draw = Sexy::Color(tint.mRed * draw.mRed / 255,
                tint.mGreen * draw.mGreen / 255,
                tint.mBlue * draw.mBlue / 255,
                tint.mAlpha * draw.mAlpha / 255);
        }
        if (draw.mAlpha == 0 || image == nullptr)
            continue;
        theGraphics->SetColor(draw);
        Sexy::Transform xform;
        Sexy::SexyMatrix3 combined = mTransform * ParticleMatrix(node);
        Sexy::SexyTransform2D wrapped(combined);
        xform.SetMatrix(wrapped);
        theGraphics->DrawImageTransform(image, xform, cel, 0.0f, 0.0f);
        mPixelCount = (int)((float)mPixelCount + (float)(image->mWidth * image->mHeight) * fabsf(node->mArea));
        node->mDead = 1;
    }
    return &mColor;
}

int Sexy::PIEffect::DrawLayer(Sexy::Graphics *theGraphics, Sexy::PILayer *theLayer)
{
    if (theGraphics == nullptr || theLayer == nullptr)
        return 0;
    theGraphics->PushState();
    theGraphics->SetColorizeImages(true);
    PILiveLayer *liveLayer = nullptr;
    for (size_t i = 0; i < mLive.size(); ++i)
    {
        if (mLive[i].mPublic == theLayer)
        {
            liveLayer = &mLive[i];
            break;
        }
    }
    if (liveLayer != nullptr)
    {
        for (int pass = 0; pass != 2; ++pass)
        {
            for (size_t emitter = 0; emitter < liveLayer->mEmitters.size(); ++emitter)
            {
                PILiveEmitter &live = liveLayer->mEmitters[emitter];
                if (!live.mActive || live.mDef == nullptr)
                    continue;
                LiveScope scope(&live);
                if (live.mFreeEmitters)
                {
                    for (PIParticle *node = live.mFree.mHead; node != nullptr; node = node->mNext)
                    {
                        PIParticleGroup children;
                        children.mHead = node->mChildren;
                        children.mTail = node->mChildTail;
                        children.mCount = node->mChildCount;
                        children.mSkipAttach = 0;
                        children.mUseEmitter = 1;
                        DrawParticleGroup(theGraphics, theLayer, nullptr, &children, (unsigned char)((pass ^ 1) & 1));
                    }
                }
                else
                {
                    DrawParticleGroup(theGraphics, theLayer, nullptr, &live.mTrail, (unsigned char)((pass ^ 1) & 1));
                }
            }
        }
    }
    theGraphics->SetDrawMode(0);
    if (liveLayer != nullptr && liveLayer->mDef != nullptr && mDebugDraw)
    {
        for (size_t i = 0; i < liveLayer->mDef->mBlockers.size(); ++i)
        {
            PIBlocker &blocker = liveLayer->mDef->mBlockers[i];
            float active = blocker.mActive.GetLastKeyframe(mTime);
            bool on = FloatBits(FloatToBits(active)) > 0.99f;
            if (!on)
                continue;
            int count = (int)blocker.mPoints.size();
            if (count > 512)
                count = 512;
            if (count <= 0)
                continue;
            Sexy::Color outline(0, on ? 255 : 64, on ? 255 : 64);
            theGraphics->SetColor(outline);
            for (int point = 0; point < count; ++point)
            {
                int next = (point + 1) % count;
                blocker.mPoints[point].GetValueAt(mTime);
                blocker.mPoints[next].GetValueAt(mTime);
                theGraphics->DrawLine((int)blocker.mPoints[point].mLastValueX,
                    (int)blocker.mPoints[point].mLastValueY,
                    (int)blocker.mPoints[next].mLastValueX,
                    (int)blocker.mPoints[next].mLastValueY);
            }
        }
        for (size_t i = 0; i < liveLayer->mDef->mDeflectors.size(); ++i)
        {
            PIDeflector &deflector = liveLayer->mDef->mDeflectors[i];
            float active = deflector.mActive.GetLastKeyframe(mTime);
            bool on = FloatBits(FloatToBits(active)) > 0.99f;
            theGraphics->SetColor(Sexy::Color(on ? 255 : 64, 0, 0));
            int count = (int)deflector.mPoints.size();
            for (int point = 0; point + 1 < count; ++point)
            {
                float x0 = deflector.mPoints[point].mX;
                float y0 = deflector.mPoints[point].mY;
                float x1 = deflector.mPoints[point + 1].mX;
                float y1 = deflector.mPoints[point + 1].mY;
                if (deflector.mThickness <= 1.5f)
                {
                    theGraphics->DrawLine((int)x0, (int)y0, (int)x1, (int)y1);
                    continue;
                }
                float dx = x1 - x0;
                float dy = y1 - y0;
                float length = sqrtf(dx * dx + dy * dy);
                if (length != 0.0f)
                {
                    dx /= length;
                    dy /= length;
                }
                float nx = -dy * deflector.mThickness;
                float ny = dx * deflector.mThickness;
                Sexy::TPoint<float> corners[4];
                corners[0] = Sexy::TPoint<float>(x0 + nx, y0 + ny);
                corners[1] = Sexy::TPoint<float>(x0 - nx, y0 - ny);
                corners[2] = Sexy::TPoint<float>(x1 + nx, y1 + ny);
                corners[3] = Sexy::TPoint<float>(x1 - nx, y1 - ny);
                for (int edge = 0; edge != 4; ++edge)
                {
                    int next = (edge + 1) & 3;
                    theGraphics->DrawLine(
                        (int)corners[edge].mX, (int)corners[edge].mY, (int)corners[next].mX, (int)corners[next].mY);
                }
            }
        }
        for (size_t i = 0; i < liveLayer->mDef->mForces.size(); ++i)
        {
            PIForce &force = liveLayer->mDef->mForces[i];
            float active = force.mActive.GetLastKeyframe(mTime);
            bool on = FloatBits(FloatToBits(active)) > 0.99f;
            if (!force.mEnabled && !on)
                continue;
            theGraphics->SetColor(Sexy::Color(on ? 255 : 64, 0, on ? 255 : 64));
            for (int edge = 0; edge != 4; ++edge)
            {
                int next = (edge + 1) & 3;
                theGraphics->DrawLine((int)force.mCorners[edge].mX,
                    (int)force.mCorners[edge].mY,
                    (int)force.mCorners[next].mX,
                    (int)force.mCorners[next].mY);
            }
            float direction = CurveAtTime(&force.mDirection, mTime, 0.0f);
            float spin = CurveAtTime(&force.mSpin, mTime, 0.0f);
            Sexy::Transform arrow;
            arrow.RotateRad((spin + direction) * kDegToRad);
            Sexy::TPoint<float> tips[3];
            tips[0] = Sexy::TPoint<float>(5.0f, 0.0f);
            tips[1] = Sexy::TPoint<float>(-5.0f, -10.0f);
            tips[2] = Sexy::TPoint<float>(-5.0f, 10.0f);
            for (int tip = 0; tip != 3; ++tip)
            {
                Sexy::TPoint<float> a;
                Sexy::TPoint<float> b;
                TransformFPoint(&a, arrow.mMatrix, tips[tip]);
                TransformFPoint(&b, arrow.mMatrix, tips[(tip + 1) % 3]);
                theGraphics->DrawLine((int)(force.mArrowOriginX + a.mX),
                    (int)(force.mArrowOriginY + a.mY),
                    (int)(force.mArrowOriginX + b.mX),
                    (int)(force.mArrowOriginY + b.mY));
            }
        }
    }
    theGraphics->PopState();
    return 0;
}

float Sexy::PIEffect::Draw(Sexy::Graphics *theGraphics)
{
    mPixelCount = 0;
    if (theGraphics == nullptr)
        return 0.0f;
    for (size_t i = 0; i < mLive.size(); ++i)
    {
        if (mLive[i].mPublic != nullptr && mLive[i].mPublic->mVisible)
            DrawLayer(theGraphics, mLive[i].mPublic);
    }
    if (mDebugDraw)
    {
        theGraphics->PushState();
        theGraphics->SetColor(Sexy::Color::Black);
        theGraphics->FillRect(-4, -4, 8, 8);
        theGraphics->SetColor(Sexy::Color::White);
        theGraphics->FillRect(-3, -3, 6, 6);
        theGraphics->PopState();
    }
    float scale = GetMatrixScale(mTransform);
    mPixelCount = (int)(scale * (float)mPixelCount);
    return scale;
}

Sexy::PIParticleDefInstance *Sexy::PIEffect::UpdateParticleDef(Sexy::PILayer *theLayer, Sexy::PIEmitter *theEmitter,
    Sexy::PIEmitterInstance *theInstance, Sexy::PIParticleDef *theDef, PIParticleDefInstance *theAccum,
    PIParticleGroup *theGroup, PIParticle *theParent)
{
    if (theAccum == nullptr || theGroup == nullptr)
        return theAccum;
    PILiveEmitter *live = LiveOf(this, theInstance);
    PIParticleType *type = theParent != nullptr && theDef == nullptr ? theParent->mType : TypeOf(this, theDef);
    PIEmitterInstanceDef *placed = live != nullptr ? live->mDef : nullptr;
    float frameScale = mTimeScale != 0.0f ? 100.0f / mTimeScale : 100.0f;
    float parentLife = theParent != nullptr ? theParent->mLifePct : 0.0f;
    theAccum->mField3 += 1;
    if ((theAccum->mField3 % 25) == 0 && theGroup->mSkipAttach == 0)
    {
        float jitter = GetRandFloat() * (theAccum->mField3 == 0 ? 0.25f : 0.375f);
        if (type != nullptr)
            theAccum->mField1 = jitter * CurveAtTime(&type->mValues[9], mTime, 0.0f);
    }
    float rate = 0.0f;
    if (theGroup->mSkipAttach != 0)
    {
        float base = theEmitter != nullptr ? CurveAtTime(EmitterCurve(theEmitter, 1), mTime, 0.0f) : 0.0f;
        float scale = theGroup->mUseEmitter != 0 && theEmitter != nullptr
            ? CurveAtTime(EmitterCurve(theEmitter, 9), mTime, 0.0f)
            : (placed != nullptr ? CurveAtTime(&placed->mValues[1], mTime, 0.0f) : 1.0f);
        rate = scale * base;
    }
    else if (type != nullptr && theEmitter != nullptr)
    {
        float scale = theGroup->mUseEmitter != 0
            ? CurveAtTime(EmitterCurve(theEmitter, 9), mTime, 0.0f)
            : (placed != nullptr ? CurveAtTime(&placed->mValues[1], mTime, 0.0f) : 1.0f);
        float local = CurveAtTime(&type->mValues[1], mTime, 0.0f) + theAccum->mField1;
        rate = CurveAtTime(EmitterCurve(theEmitter, 33), parentLife, 1.0f) * scale * local;
        if (rate < 0.0f)
            rate = 0.0f;
        if (theGroup->mUseEmitter != 0 && parentLife >= 1.0f)
            rate = 0.0f;
    }
    if (live != nullptr)
        rate *= live->mRateScale;
    if (theGroup->mSkipAttach != 0)
        rate *= 30.0f;
    else if (placed != nullptr && theGroup->mUseEmitter == 0)
    {
        if (live->mShape == 1 && live->mCount != 0)
            rate *= (float)live->mCount;
        else if (live->mShape == 3 && live->mCount != 0)
            rate *= (float)(live->mRows * live->mCount);
        else if (live->mShape == 4 && live->mCount != 0)
            rate *= (float)live->mCount;
        else if (live->mShape == 2 || live->mShape == 4)
        {
            float rx = placed != nullptr ? CurveAtTime(&placed->mValues[15], mTime, 0.0f) : 0.0f;
            float ry = placed != nullptr ? CurveAtTime(&placed->mValues[16], mTime, 0.0f) : 0.0f;
            rate = (rate * 0.17952f) * sqrtf(live->mShape == 4 ? rx * rx : ((rx * rx + ry * ry) * 0.5f));
        }
        else if (live->mShape == 3)
        {
            float rx = CurveAtTime(&placed->mValues[15], mTime, 0.0f);
            float ry = CurveAtTime(&placed->mValues[16], mTime, 0.0f);
            rate = rate + (rx * ry * 0.00027778f) * rate;
        }
    }
    theAccum->mField0 += (rate / frameScale) * 0.5f;
    bool emitting = live != nullptr && (live->mEmitting || live->mFreeEmitters);
    if (!emitting || (live != nullptr && !live->mKeepSpawn))
        theAccum->mField0 = 0.0f;

    while (theAccum->mField0 >= 1.0f)
    {
        theAccum->mField0 -= 1.0f;
        PIParticle *born = new PIParticle();
        ++PIParticleInstanceCounter::mCount;
        born->mType = type;
        born->mEmitter = theEmitter;
        born->mParent = theParent;
        born->mSpawnIndex = theAccum->mField2;
        theAccum->mField2 += 1;
        born->mSpinScaleX = 1.0f;
        born->mSpinScaleY = 1.0f;
        born->mArea = 1.0f;
        born->mColor = 0xFFFFFFFFu;
        float angle = 0.0f;
        if (type != nullptr && type->mUseOwnAngle)
        {
            float base = theGroup->mUseEmitter != 0 && theEmitter != nullptr
                ? CurveAtTime(EmitterCurve(theEmitter, 21), mTime, 0.0f)
                : (placed != nullptr ? CurveAtTime(&placed->mValues[11], mTime, 0.0f) : 0.0f);
            float spread = theGroup->mUseEmitter != 0 && theEmitter != nullptr
                ? CurveAtTime(EmitterCurve(theEmitter, 22), mTime, 0.0f)
                : (placed != nullptr ? CurveAtTime(&placed->mValues[12], mTime, 0.0f) : 0.0f);
            angle = -((base + spread * GetRandFloat() * 0.5f) * kDegToRad);
        }
        else if (type != nullptr)
        {
            float base = CurveAtTime(&type->mAngle, mTime, 0.0f);
            float spread = CurveAtTime(&type->mSpread, mTime, 0.0f);
            angle = -((base + spread * GetRandFloat() * 0.5f) * kDegToRad);
        }
        if (theParent != nullptr)
            angle += theParent->mAngle;
        else if (placed != nullptr)
            angle -= CurveAtTime(&placed->mValues[14], mTime, 0.0f) * kDegToRad;
        born->mLaunchAngle = theParent != nullptr ? theParent->mAngle : angle;
        born->mAngle = angle;
        if (type != nullptr && type->mRandomFrame)
            born->mFrame = (int)(mRand.Next() & 0x7FFF);
        born->mSize = 1.0f;
        if (theEmitter != nullptr)
        {
            float sizeA = theGroup->mUseEmitter != 0
                ? CurveAtTime(EmitterCurve(theEmitter, 17), mTime, 0.0f)
                : (placed != nullptr ? CurveAtTime(&placed->mValues[8], mTime, 0.0f) : 1.0f);
            born->mSize = CurveAtTime(EmitterCurve(theEmitter, 17), mTime, 1.0f) * sizeA;
        }
        if (theGroup->mSkipAttach != 0 && theEmitter != nullptr)
        {
            born->mVarLife = CurveAtTime(EmitterCurve(theEmitter, 23), mTime, 0.0f) * GetVariationScalar();
            born->mVarSpin = CurveAtTime(EmitterCurve(theEmitter, 25), mTime, 0.0f) * GetRandFloat();
            born->mVarSpin2 = born->mVarSpin;
            born->mVarLaunch = CurveAtTime(EmitterCurve(theEmitter, 27), mTime, 0.0f) * GetVariationScalar();
            born->mVarGravity = CurveAtTime(EmitterCurve(theEmitter, 28), mTime, 0.0f) * GetVariationScalar();
            born->mVarSpinRate = CurveAtTime(EmitterCurve(theEmitter, 29), mTime, 0.0f) * GetVariationScalar();
            born->mVarJitter = CurveAtTime(EmitterCurve(theEmitter, 30), mTime, 0.0f) * GetVariationScalar();
            born->mVarBounce = CurveAtTime(EmitterCurve(theEmitter, 31), mTime, 0.0f) * GetVariationScalar();
            born->mVarUnused = CurveAtTime(EmitterCurve(theEmitter, 32), mTime, 0.0f) * GetVariationScalar();
        }
        else if (type != nullptr)
        {
            born->mVarLife = CurveAtTime(&type->mValues[8], mTime, 0.0f) * GetVariationScalar();
            born->mVarSpin = CurveAtTime(&type->mValues[10], mTime, 0.0f) * GetVariationScalar();
            born->mVarSpin2 =
                type->mLockSpin ? born->mVarSpin : CurveAtTime(&type->mSpinVarY, mTime, 0.0f) * GetVariationScalar();
            born->mVarLaunch = CurveAtTime(&type->mValues[11], mTime, 0.0f) * GetVariationScalar();
            born->mVarGravity = CurveAtTime(&type->mValues[12], mTime, 0.0f) * GetVariationScalar();
            born->mVarSpinRate = CurveAtTime(&type->mValues[13], mTime, 0.0f) * GetVariationScalar();
            born->mVarJitter = CurveAtTime(&type->mValues[14], mTime, 0.0f) * GetVariationScalar();
            born->mVarBounce = CurveAtTime(&type->mValues[15], mTime, 0.0f) * GetVariationScalar();
        }
        born->mKeyParam = GetRandFloatU();
        born->mEmitterTime = 0.0f;
        if (theGroup->mSkipAttach != 0 && theEmitter != nullptr)
        {
            float life = CurveAtTime(EmitterCurve(theEmitter, 0), mTime, 0.0f) + born->mVarLife;
            float scale = theGroup->mUseEmitter != 0
                ? CurveAtTime(EmitterCurve(theEmitter, 8), mTime, 0.0f)
                : (placed != nullptr ? CurveAtTime(&placed->mValues[0], mTime, 0.0f) : 1.0f);
            born->mLife = life * 5.0f * scale;
        }
        else if (type != nullptr)
        {
            float scale = theGroup->mUseEmitter != 0 && theEmitter != nullptr
                ? CurveAtTime(EmitterCurve(theEmitter, 8), mTime, 0.0f)
                : (placed != nullptr ? CurveAtTime(&placed->mValues[0], mTime, 0.0f) : 1.0f);
            born->mLife = (CurveAtTime(&type->mValues[0], mTime, 0.0f) + born->mVarLife) * scale;
        }
        if (theParent != nullptr)
        {
            born->mScaleX = theParent->mPosX + theParent->mVelX;
            born->mScaleY = theParent->mPosY + theParent->mVelY;
        }
        else
        {
            Sexy::TPoint<float> origin = GetEmitterPos(theInstance, true);
            born->mScaleX = origin.mX;
            born->mScaleY = origin.mY;
        }
        born->mLaunchPosX = born->mScaleX;
        born->mLaunchPosY = born->mScaleY;
        bool masked = false;
        Sexy::TPoint<float> geom = GetGeomPos(theInstance, born, &born->mAngle, &masked);
        if (!masked)
        {
            born->mVelX = geom.mX - born->mScaleX;
            born->mVelY = geom.mY - born->mScaleY;
        }
        float launch = 0.0f;
        if (theGroup->mSkipAttach != 0 && theEmitter != nullptr)
            launch = CurveAtTime(EmitterCurve(theEmitter, 12), mTime, 0.0f);
        else if (placed != nullptr)
            launch = CurveAtTime(&placed->mValues[3], mTime, 0.0f);
        float speed = 0.0f;
        if (type != nullptr)
            speed = (CurveAtTime(&type->mValues[3], mTime, 0.0f) + born->mVarLaunch) * launch;
        if (theGroup->mSkipAttach != 0)
            speed *= 160.0f;
        born->mSpinX = cosf(born->mAngle) * speed;
        born->mSpinY = sinf(born->mAngle) * speed;
        if (type != nullptr && type->mAlignToVelocity)
        {
            float mag = sqrtf(born->mSpinX * born->mSpinX + born->mSpinY * born->mSpinY);
            born->mAngle = mag == 0.0f ? 0.0f : -born->mAngle;
            born->mAngle += (float)(-type->mAngleOffset) * kDegToRad;
        }
        else if (type != nullptr && type->mUseOwnAngle)
        {
            born->mAngle =
                -((float)type->mAngleSpread + (float)type->mAngleSpreadRange * 0.5f * GetRandFloat()) * kDegToRad;
        }
        else if (type != nullptr)
        {
            born->mAngle = (float)(-type->mFixedAngle) * kDegToRad;
        }
        Sexy::SexyTransform2D spin;
        if (placed != nullptr)
            spin.RotateDeg(CurveAtTime(&placed->mValues[14], mTime, 0.0f));
        Sexy::TPoint<float> local(born->mVelX, born->mVelY);
        Sexy::TPoint<float> rotated;
        TransformFPoint(&rotated, spin, local);
        born->mPosX = born->mScaleX + rotated.mX;
        born->mPosY = born->mScaleY + rotated.mY;
        if (theEmitter != nullptr && theEmitter->mEmitInReverse)
            LinkParticle(theGroup, born, true);
        else
            LinkParticle(theGroup, born, false);
        if (type != nullptr)
            born->mSpawnRecords.resize(1);
        (void)theLayer;
    }
    return theAccum;
}

int Sexy::PIEffect::Update()
{
    if (!mError.empty())
        return 0;
    ++mFrameTick;
    if (mNeedsSeed)
    {
        if (!mRandSeeds.empty())
            mRand.SRand((unsigned int)mRandSeeds[(unsigned int)Rand() % mRandSeeds.size()]);
        else
            mRand.SRand((unsigned int)Rand());
        mNeedsSeed = false;
    }
    if (mTime == 0.0f && (mBuffer.mDataBitSize >> 3) != 0)
    {
        LoadState(mBuffer, true);
        mNeedsSeed = false;
        return 1;
    }
    float frameScale = mTimeScale != 0.0f ? 100.0f / mTimeScale : 100.0f;
    bool first = true;
    while ((float)mTimelineEnd > mTime || first)
    {
        int crossedFrom = (int)mTime;
        mUpdateCountA = 0;
        mUpdateCountB = 0;
        if (first && mTime == 0.0f)
            mTime += 0.0001f;
        else
            mTime += (float)mParticleScale / frameScale;
        mTimeCrossed = (int)mTime != crossedFrom;
        for (size_t layerIndex = 0; layerIndex < mLive.size(); ++layerIndex)
        {
            PILiveLayer &layer = mLive[layerIndex];
            if (layer.mPublic == nullptr || !layer.mPublic->mVisible || layer.mDef == nullptr)
                continue;
            for (size_t i = 0; i < layer.mDef->mDeflectors.size(); ++i)
            {
                PIDeflector &deflector = layer.mDef->mDeflectors[i];
                Sexy::SexyTransform2D xform;
                float spin = CurveAtTime(&deflector.mSpin, mTime, 0.0f);
                if (spin != 0.0f)
                    xform.RotateDeg(spin);
                deflector.mOffset.GetValueAt(mTime);
                xform.Translate(deflector.mOffset.mLastValueX, deflector.mOffset.mLastValueY);
                layer.mDef->mValue2D.GetValueAt(mTime);
                xform.Translate(layer.mDef->mValue2D.mLastValueX, layer.mDef->mValue2D.mLastValueY);
                float layerSpin = CurveAtTime(&layer.mDef->mSpin, mTime, 0.0f);
                if (layerSpin != 0.0f)
                    xform.RotateDeg(layerSpin);
                xform = mTransform * xform;
                deflector.mPoints.clear();
                for (size_t point = 0; point < deflector.mPointCurves.size(); ++point)
                {
                    deflector.mPointCurves[point].GetValueAt(mTime);
                    Sexy::TPoint<float> src(
                        deflector.mPointCurves[point].mLastValueX, deflector.mPointCurves[point].mLastValueY);
                    Sexy::TPoint<float> dst;
                    TransformFPoint(&dst, xform, src);
                    deflector.mPoints.push_back(dst);
                }
            }
            for (size_t i = 0; i < layer.mDef->mForces.size(); ++i)
            {
                PIForce &force = layer.mDef->mForces[i];
                Sexy::SexyTransform2D xform;
                float scaleX = CurveAtTime(&force.mExtra, mTime, 0.0f);
                float scaleY = CurveAtTime(&force.mAngle, mTime, 0.0f);
                xform.Scale(scaleX * 0.5f, scaleY * 0.5f);
                float spin = CurveAtTime(&force.mSpin, mTime, 0.0f);
                if (spin != 0.0f)
                    xform.RotateDeg(spin);
                force.mValue2D.GetValueAt(mTime);
                xform.Translate(force.mValue2D.mLastValueX, force.mValue2D.mLastValueY);
                layer.mDef->mValue2D.GetValueAt(mTime);
                xform.Translate(layer.mDef->mValue2D.mLastValueX, layer.mDef->mValue2D.mLastValueY);
                float layerSpin = CurveAtTime(&layer.mDef->mSpin, mTime, 0.0f);
                if (layerSpin != 0.0f)
                    xform.RotateDeg(layerSpin);
                xform = mTransform * xform;
                Sexy::TPoint<float> corners[5];
                corners[0] = Sexy::TPoint<float>(-1.0f, -1.0f);
                corners[1] = Sexy::TPoint<float>(1.0f, -1.0f);
                corners[2] = Sexy::TPoint<float>(1.0f, 1.0f);
                corners[3] = Sexy::TPoint<float>(-1.0f, 1.0f);
                corners[4] = Sexy::TPoint<float>(0.0f, 0.0f);
                for (int corner = 0; corner != 4; ++corner)
                    TransformFPoint(&force.mCorners[corner], xform, corners[corner]);
                Sexy::TPoint<float> origin;
                TransformFPoint(&origin, xform, corners[4]);
                force.mArrowOriginX = (int)origin.mX;
                force.mArrowOriginY = (int)origin.mY;
            }
            for (size_t emitterIndex = 0; emitterIndex < layer.mEmitters.size(); ++emitterIndex)
            {
                PILiveEmitter &live = layer.mEmitters[emitterIndex];
                if (!live.mActive || live.mDef == nullptr)
                    continue;
                int loops = 1;
                float last = live.mDef->mValues[13].GetLastKeyframe(mTime);
                bool emitting = FloatBits(FloatToBits(last)) > 0.99f;
                if (!emitting)
                    loops = 0;
                else if (!live.mEmitting)
                    loops = (int)((float)loops +
                        ((float)live.mLifeBias * frameScale) / (float)(mParticleScale != 0 ? mParticleScale : 1));
                live.mEmitting = emitting;
                float nextTime = live.mDef->mValues[13].GetNextKeyframeTime(0.0f);
                float lastTime = live.mDef->mValues[13].GetLastKeyframeTime((float)mTimelineEnd + 1.0f);
                float lastValue = live.mDef->mValues[13].GetLastKeyframe((float)mTimelineEnd + 1.0f);
                if (nextTime > mTime || (lastTime <= mTime && FloatBits(FloatToBits(lastValue)) <= 0.99f))
                    live.mKeepSpawn = false;
                else
                    live.mKeepSpawn = mPausedFlag || mTime < (float)mTimelineEnd;
                if (!live.mKeepSpawn && !emitting)
                    continue;
                LiveScope scope(&live);
                if (live.mFreeEmitters)
                {
                    for (size_t curve = 0; curve < live.mDef->mCurve.size() && curve < live.mFreeSpawns.size(); ++curve)
                    {
                        int typeIndex = live.mDef->mCurve[curve];
                        Sexy::PIEmitter *emitter = nullptr;
                        if (typeIndex >= 0 && typeIndex < (int)mDef->mEmitters.size())
                            emitter = mDef->mEmitters[typeIndex];
                        UpdateParticleDef(
                            layer.mPublic, emitter, nullptr, nullptr, &live.mFreeSpawns[curve], &live.mFree, nullptr);
                    }
                    UpdateParticleGroup(layer.mPublic, nullptr, &live.mFree);
                    for (PIParticle *node = live.mFree.mHead; node != nullptr; node = node->mNext)
                    {
                        if (node->mEmitter == nullptr)
                            continue;
                        if (node->mSpawnRecords.size() < node->mEmitter->mParticleDefs.size())
                            node->mSpawnRecords.resize(node->mEmitter->mParticleDefs.size());
                        PIParticleGroup children;
                        children.mHead = node->mChildren;
                        children.mTail = node->mChildTail;
                        children.mCount = node->mChildCount;
                        children.mSkipAttach = 0;
                        children.mUseEmitter = 1;
                        for (size_t defIndex = 0; defIndex < node->mEmitter->mParticleDefs.size(); ++defIndex)
                        {
                            UpdateParticleDef(layer.mPublic,
                                node->mEmitter,
                                nullptr,
                                &node->mEmitter->mParticleDefs[defIndex],
                                &node->mSpawnRecords[defIndex],
                                &children,
                                node);
                        }
                        UpdateParticleGroup(layer.mPublic, nullptr, &children);
                        node->mChildren = children.mHead;
                        node->mChildTail = children.mTail;
                        node->mChildCount = children.mCount;
                    }
                }
                else if (live.mEmitter != nullptr)
                {
                    if (live.mDefSpawns.size() < live.mEmitter->mParticleDefs.size())
                        live.mDefSpawns.resize(live.mEmitter->mParticleDefs.size());
                    for (size_t defIndex = 0; defIndex < live.mEmitter->mParticleDefs.size(); ++defIndex)
                    {
                        UpdateParticleDef(layer.mPublic,
                            live.mEmitter,
                            nullptr,
                            &live.mEmitter->mParticleDefs[defIndex],
                            &live.mDefSpawns[defIndex],
                            &live.mTrail,
                            nullptr);
                    }
                    UpdateParticleGroup(layer.mPublic, nullptr, &live.mTrail);
                }
                mUpdateCountA += live.mFree.mCount;
                mUpdateCountB += live.mTrail.mCount;
            }
        }
        first = false;
        if (mTime >= (float)mTimelineEnd && !mLoopFlag)
            break;
        if (!first && mTime >= (float)mTimelineEnd)
            break;
    }
    return 1;
}

int Sexy::PIEffect::SaveState(Sexy::Buffer &theBuffer, bool theCompact)
{
    theBuffer.WriteLong(0);
    theBuffer.WriteShort(1);
    if (!theCompact)
    {
        theBuffer.WriteString(mFilePath);
        theBuffer.WriteLong((long)mChecksum);
        theBuffer.WriteFloat(mTime);
        theBuffer.WriteBoolean(mPausedFlag);
        theBuffer.WriteTransform2D(mTransform);
        theBuffer.WriteTransform2D(mWorldTransform);
    }
    else
    {
        theBuffer.WriteFloat(mTime);
    }
    for (size_t layer = 0; layer < mLive.size(); ++layer)
    {
        for (size_t emitter = 0; emitter < mLive[layer].mEmitters.size(); ++emitter)
        {
            PILiveEmitter &live = mLive[layer].mEmitters[emitter];
            theBuffer.WriteBoolean(live.mActive);
            theBuffer.WriteBoolean(live.mEmitting);
            theBuffer.WriteBoolean(live.mKeepSpawn);
            for (size_t i = 0; i < live.mDefSpawns.size(); ++i)
                SaveParticleDefInstance(theBuffer, &live.mDefSpawns[i]);
            for (size_t i = 0; i < live.mFreeSpawns.size(); ++i)
                SaveParticleDefInstance(theBuffer, &live.mFreeSpawns[i]);
            theBuffer.WriteLong(live.mTrail.mCount);
            for (PIParticle *node = live.mTrail.mHead; node != nullptr; node = node->mNext)
                SaveParticle(theBuffer, mLive[layer].mPublic, node);
            theBuffer.WriteLong(live.mFree.mCount);
            for (PIParticle *node = live.mFree.mHead; node != nullptr; node = node->mNext)
                SaveParticle(theBuffer, mLive[layer].mPublic, node);
        }
    }
    return 1;
}

int Sexy::PIEffect::LoadState(Sexy::Buffer const &theBuffer, bool theCompact)
{
    if (!mError.empty())
        return 0;
    ResetAnim();
    theBuffer.mReadBitPos = (theBuffer.mReadBitPos + 7) & ~7;
    int length = theBuffer.ReadLong();
    (void)length;
    int version = theBuffer.ReadShort();
    if (!theCompact)
    {
        Sexy::String name = theBuffer.ReadString();
        if (!mLoaded)
            LoadEffect(name);
        int checksum = theBuffer.ReadLong();
        if ((unsigned int)checksum != mChecksum)
            return 0;
        mTime = theBuffer.ReadFloat();
        mPausedFlag = theBuffer.ReadBoolean();
        mTransform = theBuffer.ReadTransform2D();
        mWorldTransform = theBuffer.ReadTransform2D();
    }
    else
    {
        mTime = theBuffer.ReadFloat();
        if (version == 0)
            theBuffer.ReadBoolean();
    }
    mNeedsSeed = false;
    if (mTime <= 0.0f || mDef == nullptr)
        return 1;
    for (size_t layer = 0; layer < mLive.size(); ++layer)
    {
        for (size_t emitter = 0; emitter < mLive[layer].mEmitters.size(); ++emitter)
        {
            PILiveEmitter &live = mLive[layer].mEmitters[emitter];
            live.mActive = theBuffer.ReadBoolean();
            live.mEmitting = theBuffer.ReadBoolean();
            live.mKeepSpawn = theBuffer.ReadBoolean();
            for (size_t i = 0; i < live.mDefSpawns.size(); ++i)
                LoadParticleDefInstance(theBuffer, &live.mDefSpawns[i]);
            for (size_t i = 0; i < live.mFreeSpawns.size(); ++i)
                LoadParticleDefInstance(theBuffer, &live.mFreeSpawns[i]);
            int trailCount = theBuffer.ReadLong();
            for (int i = 0; i < trailCount; ++i)
            {
                PIParticle *node = new PIParticle();
                ++PIParticleInstanceCounter::mCount;
                LoadParticle(theBuffer, mLive[layer].mPublic, node);
                LinkParticle(&live.mTrail, node, false);
            }
            int freeCount = theBuffer.ReadLong();
            for (int i = 0; i < freeCount; ++i)
            {
                PIParticle *node = new PIParticle();
                ++PIParticleInstanceCounter::mCount;
                LoadParticle(theBuffer, mLive[layer].mPublic, node);
                LinkParticle(&live.mFree, node, false);
            }
        }
    }
    return 1;
}

Sexy::PIEffect::PIEffect(Sexy::PIEffect const &theOther) :
    mReadFile(nullptr),
    mWriteFile(nullptr),
    mChecksum(theOther.mChecksum),
    mWideBooleans(theOther.mWideBooleans),
    mLoadedFlag(theOther.mLoadedFlag),
    mFileVersion(theOther.mFileVersion),
    mFilePath(theOther.mFilePath),
    mWritePath(theOther.mWritePath),
    mRand(theOther.mRand),
    mBuffer(),
    mReadValue(0),
    mReadPos(0),
    mNotesText(theOther.mNotesText),
    mNotesCursor(theOther.mNotesCursor),
    mStrings(theOther.mStrings),
    mSettingA(theOther.mSettingA),
    mSettingB(theOther.mSettingB),
    mLoadColor(theOther.mLoadColor),
    mParticleScale(theOther.mParticleScale),
    mTimelineStart(theOther.mTimelineStart),
    mTimelineEnd(theOther.mTimelineEnd),
    mUnusedE40(theOther.mUnusedE40),
    mNotes(theOther.mNotes),
    mDef(theOther.mDef),
    mLayers(),
    mLive(),
    mError(),
    mLoaded(theOther.mLoaded),
    mFrameTick(0),
    mTime(0.0f),
    mTimeCrossed(false),
    mFreeList(),
    mFreeStride(theOther.mFreeStride),
    mDeadList(),
    mDeadStride(theOther.mDeadStride),
    mEmitEnabled(theOther.mEmitEnabled),
    mEmitAfter(theOther.mEmitAfter),
    mLoopFlag(theOther.mLoopFlag),
    mUpdateCountA(0),
    mUpdateCountB(0),
    mPixelCount(0),
    mTimeScale(theOther.mTimeScale),
    mColor(theOther.mColor),
    mDebugDraw(theOther.mDebugDraw),
    mVisibleFlag(theOther.mVisibleFlag),
    mPausedFlag(theOther.mPausedFlag),
    mRandSeeds(theOther.mRandSeeds),
    mRandSeedCursor(theOther.mRandSeedCursor),
    mUnusedED8(theOther.mUnusedED8),
    mNeedsSeed(true),
    mTransform(theOther.mTransform),
    mWorldTransform(theOther.mWorldTransform)
{
    if (mDef != nullptr)
        mDef->mRefCount += 1;
    gSexyAppBase->mCritSect.Lock();
    gSexyAppBase->mPIEffects.insert(this);
    gSexyAppBase->mCritSect.Unlock();
    if (mDef != nullptr)
    {
        mLayers.resize(mDef->mLayerDefs.size());
        mLive.resize(mDef->mLayerDefs.size());
        for (size_t layer = 0; layer < mDef->mLayerDefs.size(); ++layer)
        {
            mLive[layer].mDef = &mDef->mLayerDefs[layer];
            mLive[layer].mPublic = &mLayers[layer];
            mLive[layer].mEmitters.resize(mDef->mLayerDefs[layer].mEmitters.size());
            mLayers[layer].mEmitters.resize(mDef->mLayerDefs[layer].mEmitters.size());
            mLayers[layer].mVisible = true;
            for (size_t emitter = 0; emitter < mLive[layer].mEmitters.size(); ++emitter)
            {
                PILiveEmitter &live = mLive[layer].mEmitters[emitter];
                live.mDef = &mDef->mLayerDefs[layer].mEmitters[emitter];
                if (layer < theOther.mLive.size() && emitter < theOther.mLive[layer].mEmitters.size())
                {
                    live.mEmitter = theOther.mLive[layer].mEmitters[emitter].mEmitter;
                    live.mTint = theOther.mLive[layer].mEmitters[emitter].mTint;
                    live.mRateScale = theOther.mLive[layer].mEmitters[emitter].mRateScale;
                    live.mDefSpawns.resize(theOther.mLive[layer].mEmitters[emitter].mDefSpawns.size());
                    live.mFreeSpawns.resize(theOther.mLive[layer].mEmitters[emitter].mFreeSpawns.size());
                    live.mName = theOther.mLive[layer].mEmitters[emitter].mName;
                    live.mLifeBias = theOther.mLive[layer].mEmitters[emitter].mLifeBias;
                    live.mEmitterType = theOther.mLive[layer].mEmitters[emitter].mEmitterType;
                    live.mShape = theOther.mLive[layer].mEmitters[emitter].mShape;
                    live.mHeadingLock = theOther.mLive[layer].mEmitters[emitter].mHeadingLock;
                    live.mHeadingRandom = theOther.mLive[layer].mEmitters[emitter].mHeadingRandom;
                    live.mCount = theOther.mLive[layer].mEmitters[emitter].mCount;
                    live.mRows = theOther.mLive[layer].mEmitters[emitter].mRows;
                    live.mFreeEmitters = theOther.mLive[layer].mEmitters[emitter].mFreeEmitters;
                    live.mMaskCompare = theOther.mLive[layer].mEmitters[emitter].mMaskCompare;
                    live.mFree.mUseEmitter = live.mFreeEmitters ? 1 : 0;
                }
            }
        }
    }
    ResetAnim();
}

Sexy::PIEffect *Sexy::PIEffect::Duplicate()
{
    return new PIEffect(*this);
}

int Sexy::PIEffect::WriteImage(
    Sexy::String const &unusedResult, int theFrame, Sexy::DeviceImage *theImage, bool *theResult)
{
    (void)unusedResult;
    (void)theFrame;
    if (theImage == nullptr)
    {
        if (theResult != nullptr)
            *theResult = false;
        return 0;
    }
    Sexy::String path = mWritePath.empty() ? mFilePath : mWritePath;
    path += ".png";
    ImageLib::Image pixels;
    pixels.mWidth = theImage->mWidth;
    pixels.mHeight = theImage->mHeight;
    pixels.mBits = theImage->GetBits();
    ImageLib::WritePNGImage(path, &pixels);
    if (theResult != nullptr)
        *theResult = true;
    return 1;
}
