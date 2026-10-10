// A sampled 1D curve. Keyframes are (time, value) pairs. When a bezier is
// attached and the time span is short, QuantizeCurve fills a 256-entry table
// so GetValueAt can interpolate. A long span is searched on the bezier itself.

#include "PIValue.h"

#include <cmath>

namespace
{

int TableIndex(float theTime, float theStart, float theScale)
{
    int index = (int)(theScale * (theTime - theStart) + 0.5f);
    if (index < 0)
        index = 0;
    if (index > 255)
        index = 255;
    return index;
}

} // namespace

Sexy::PIValue::PIValue() :
    mLastQueryTime(-1.0f),
    mLastValue(0.0f),
    mLastEvaluateTime(0.0f),
    mLastStepSize(0.01f)
{
}

Sexy::PIValue::PIValue(const PIValue &other) :
    mSamples(other.mSamples),
    mKeyframes(other.mKeyframes),
    mBezier(other.mBezier),
    mLastQueryTime(other.mLastQueryTime),
    mLastValue(other.mLastValue),
    mLastEvaluateTime(other.mLastEvaluateTime),
    mLastStepSize(other.mLastStepSize)
{
}

float Sexy::PIValue::GetLastKeyframe(float theTime)
{
    int index = (int)mKeyframes.size() - 1;
    if (index < 0)
        return 0.0f;
    if (theTime < mKeyframes[(size_t)index].mTime)
    {
        // The walk stops before keyframe 0 when the query starts on it, so a
        // time before the only keyframe is 0.
        while (true)
        {
            const bool more = index-- != 0;
            if (!more)
                break;
            if (mKeyframes[(size_t)index].mTime <= theTime)
                return mKeyframes[(size_t)index].mValue;
        }
        return 0.0f;
    }
    return mKeyframes[(size_t)index].mValue;
}

float Sexy::PIValue::GetLastKeyframeTime(float theTime)
{
    int index = (int)mKeyframes.size() - 1;
    if (index < 0)
        return 0.0f;
    float keyTime = mKeyframes[(size_t)index].mTime;
    if (theTime < keyTime)
    {
        while (true)
        {
            const bool more = index-- != 0;
            if (!more)
                break;
            keyTime = mKeyframes[(size_t)index].mTime;
            if (keyTime <= theTime)
                return keyTime;
        }
        return 0.0f;
    }
    return keyTime;
}

float Sexy::PIValue::GetNextKeyframeTime(float theTime) const
{
    const int count = (int)mKeyframes.size();
    if (count <= 0)
        return 0.0f;

    float keyTime = mKeyframes[0].mTime;
    if (theTime > keyTime)
    {
        for (int index = 0;;)
        {
            ++index;
            if (index == count)
                return 0.0f;
            keyTime = mKeyframes[(size_t)index].mTime;
            if (keyTime >= theTime)
                return keyTime;
        }
    }
    return keyTime;
}

int Sexy::PIValue::GetNextKeyframeIdx(float theTime)
{
    const int count = (int)mKeyframes.size();
    if (count <= 0)
        return -1;

    int index = 0;
    if (theTime > mKeyframes[0].mTime)
    {
        while (++index != count)
        {
            if (mKeyframes[(size_t)index].mTime >= theTime)
                return index;
        }
        return -1;
    }
    return index;
}

void Sexy::PIValue::QuantizeCurve()
{
    if (mKeyframes.size() < 2)
        return;

    const float rangeStart = mKeyframes.front().mTime;
    const float rangeEnd = mKeyframes.back().mTime;
    const float invRange = 1.0f / (rangeEnd - rangeStart);

    mSamples.assign(256, 0.0f);

    float previousValue = 0.0f;
    int bracketIndex = 0;
    int gapFill = 1;
    int scanIndex = 1;
    int lastWritten = 0;
    float sampleTime = rangeStart;
    const int lastIndex = (int)mKeyframes.size() - 1;

    // Walk the bezier at 1/512 of the key range. Each sample is written into
    // the 256-entry table at the bezier's own time, and a jump of more than
    // one slot is ramped from the previous value.
    while (scanIndex <= lastIndex)
    {
        const float *point = mBezier.Evaluate(sampleTime);
        const float bezierTime = point[0];
        const float bezierValue = point[1];
        bool advanced = false;

        while (scanIndex <= lastIndex)
        {
            const float keyTime = mKeyframes[(size_t)scanIndex].mTime;
            if (bezierTime < keyTime)
            {
                if (bezierTime >= mKeyframes[(size_t)bracketIndex].mTime)
                {
                    const int tableIndex = TableIndex(bezierTime, rangeStart, invRange * 255.0f);
                    if (gapFill != 0 || tableIndex <= lastWritten + 1)
                    {
                        mSamples[(size_t)tableIndex] = bezierValue;
                    }
                    else if (lastWritten <= tableIndex)
                    {
                        const int fillCount = tableIndex + 1 - lastWritten;
                        const float span = (float)(tableIndex - lastWritten);
                        const float invSpan = span != 0.0f ? 1.0f / span : 0.0f;
                        int step = gapFill;
                        for (int slot = lastWritten; step != fillCount && slot < 256; ++slot, ++step)
                        {
                            const float blend = (float)step * invSpan;
                            mSamples[(size_t)slot] = blend * bezierValue + (1.0f - blend) * previousValue;
                        }
                    }
                    previousValue = bezierValue;
                    lastWritten = tableIndex;
                }
                sampleTime += (rangeEnd - rangeStart) * 0.0019531f;
                gapFill = 0;
                advanced = true;
                break;
            }
            bracketIndex = scanIndex;
            ++scanIndex;
        }
        if (!advanced)
            break;
    }

    const float scale = invRange * 255.0f;
    for (const PIKeyframe &key : mKeyframes)
        mSamples[(size_t)TableIndex(key.mTime, rangeStart, scale)] = key.mValue;
}

float Sexy::PIValue::GetValueAt(float theTime, float theFallback)
{
    if (mLastQueryTime == theTime)
        return mLastValue;

    const float previousQuery = mLastQueryTime;
    const int keyCount = (int)mKeyframes.size();
    mLastQueryTime = theTime;

    if (keyCount == 0)
    {
        mLastValue = theFallback;
        return theFallback;
    }
    if (keyCount == 1)
    {
        mLastValue = mKeyframes[0].mValue;
        return mLastValue;
    }

    if (mBezier.mPointCount > 0)
    {
        float rangeEnd = mKeyframes[(size_t)keyCount - 1].mTime;
        float rangeStart = mKeyframes[0].mTime;
        if (rangeEnd > 1.001f)
        {
            if (theTime > rangeStart)
            {
                if (theTime >= rangeEnd)
                {
                    mLastValue = mKeyframes[(size_t)keyCount - 1].mValue;
                    return mLastValue;
                }

                // Search the bezier for the time closest to the query. A small
                // step from the previous query reuses that evaluation; a coarse
                // step binary-searches the bracket.
                const float timeDelta = (theTime - previousQuery) / (rangeEnd - rangeStart);
                float step = (rangeEnd - rangeStart) * 0.001f;
                const float stepScale[4] = {0.1f, 0.1f, 0.1f, 0.5f};
                const float probeScale[3] = {1.0f, 0.75f, 1.25f};
                if (step > 0.1f)
                    step = 0.1f;

                float value = 0.0f;
                int iteration = 0;
                float probe = 0.0f;
                while (iteration < 1000)
                {
                    const bool fine = iteration <= 3 && timeDelta <= 0.05f;
                    float tolerance = step;
                    if (fine)
                        tolerance = step * stepScale[iteration];

                    const float savedStep = mLastStepSize;
                    if (iteration > 2 || savedStep == 0.0f || timeDelta > 0.05f)
                        probe = rangeStart + (rangeEnd - rangeStart) * 0.5f;
                    else
                        probe = mLastEvaluateTime + savedStep * probeScale[iteration];

                    if (probe >= rangeStart && probe <= rangeEnd)
                    {
                        const float *point = mBezier.Evaluate(probe);
                        const float pointTime = point[0];
                        value = point[1];
                        if (tolerance >= std::fabs(pointTime - theTime))
                            break;
                        if (pointTime - theTime < 0.0f)
                            rangeStart = probe;
                        else
                            rangeEnd = probe;
                    }
                    ++iteration;
                }

                const float savedStep = mLastStepSize;
                const float savedEval = mLastEvaluateTime;
                mLastEvaluateTime = probe;
                mLastValue = value;
                mLastStepSize = ((savedStep - savedEval) + probe) * 0.5f;
                return value;
            }
        }
        else
        {
            if (mSamples.empty())
                QuantizeCurve();
            const float indexF = ((theTime - rangeStart) / (rangeEnd - rangeStart)) * 255.0f + 0.5f;
            if (indexF > 0.0f)
            {
                float value;
                if (indexF < 255.0f && mSamples.size() >= 256)
                {
                    const int index = (int)indexF;
                    const float frac = indexF - (float)index;
                    value = (1.0f - frac) * mSamples[(size_t)index] + frac * mSamples[(size_t)index + 1];
                }
                else
                {
                    value = mKeyframes[(size_t)keyCount - 1].mValue;
                }
                mLastValue = value;
                return value;
            }
        }

        mLastValue = mKeyframes[0].mValue;
        return mLastValue;
    }

    if (keyCount <= 1)
    {
        mLastValue = theFallback;
        return theFallback;
    }

    int index = 1;
    float startTime = 0.0f;
    float endTime = 0.0f;
    while (true)
    {
        startTime = mKeyframes[(size_t)index - 1].mTime;
        if (theTime >= startTime)
        {
            endTime = mKeyframes[(size_t)index].mTime;
            if (theTime <= endTime)
                break;
        }
        if (index == keyCount - 1)
        {
            endTime = mKeyframes[(size_t)index].mTime;
            break;
        }
        ++index;
        if (index == keyCount)
        {
            mLastValue = theFallback;
            return theFallback;
        }
    }

    float mix = (theTime - startTime) / (endTime - startTime);
    if (mix > 1.0f)
        mix = 1.0f;
    const float result = mKeyframes[(size_t)index - 1].mValue +
        mix * (mKeyframes[(size_t)index].mValue - mKeyframes[(size_t)index - 1].mValue);
    mLastValue = result;
    return result;
}
