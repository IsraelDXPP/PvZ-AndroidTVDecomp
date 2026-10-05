#include "PIInterpolator.h"

#include "ReconFreeFunctions.h"

#include <cstring>

int Sexy::PIInterpolator::GetValueAt(float time)
{
    int count = (int)mKeyframes.size();
    if (count <= 0)
        return 0;
    if (count == 1)
        return mKeyframes[0].mField0;

    float mapped = mKeyframes[0].mField1
        + time * (mKeyframes[count - 1].mField1
                  - mKeyframes[0].mField1);

    int pair = count - 1;
    for (int key = 1; key < count; ++key)
    {
        if (mapped >= mKeyframes[key - 1].mField1
            && mapped <= mKeyframes[key].mField1)
        {
            pair = key;
            break;
        }
    }

    float prevTime = mKeyframes[pair - 1].mField1;
    float nextTime = mKeyframes[pair].mField1;
    float fraction = (mapped - prevTime) / (nextTime - prevTime);
    if (fraction > 1.0f)
        fraction = 1.0f;
    return InterpColor(mKeyframes[pair - 1].mField0, mKeyframes[pair].mField0, fraction);
}

unsigned int Sexy::PIInterpolator::GetKeyframeNum(int index)
{
    unsigned int count = (unsigned int)mKeyframes.size();
    if (count != 0)
        return mKeyframes[index % count].mField0;
    return 0;
}

int Sexy::PIInterpolator::GetKeyframeTime(int index)
{
    unsigned int count = (unsigned int)mKeyframes.size();
    if (count == 0)
        return 0;
    float keyTime = mKeyframes[index % count].mField1;
    int keyBits = 0;
    std::memcpy(&keyBits, &keyTime, sizeof(keyBits));
    return keyBits;
}
