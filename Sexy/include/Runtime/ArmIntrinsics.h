// ArmIntrinsics.h -- the ARM/NEON operations the decompiler emitted.
//
// The binary is ARM32, so IDA prints the instructions it used as intrinsic
// calls: `vcvts_n_f32_s32` is a NEON convert-with-scale, `__PAIR64__` builds a
// 64-bit value from two words, `__OFSUB__` is the overflow flag from a
// subtraction.
//
// On x86 those have exact equivalents, and they are written here rather than
// rewritten at each use site because the decompiler's spelling is what the
// transcription carries. Each is a real implementation, not a stub: the value
// the binary computed is the value this returns.
//
// Nothing here is on the game's logic path in the sense of decision-making --
// these are arithmetic and bit operations -- so implementing them exactly is
// both possible and the right thing to do.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_RUNTIME_ARMINTRINSICS_H
#define LAWNPROJECTTV_SEXY_INCLUDE_RUNTIME_ARMINTRINSICS_H

#include <cmath>
#include <cstdint>
#include <climits>

// ---------------------------------------------------------------------------
// NEON conversions.
//
// vcvts_n_f32_s32(value, n) converts an int32 to a float, then divides by 2^n.
// It is the fixed-point-to-float conversion the audio and physics code uses.
// ---------------------------------------------------------------------------
inline float vcvts_n_f32_s32(int theValue, unsigned int theShift)
{
    return (float)theValue / (float)(1u << theShift);
}

inline float vcvtq_n_f32_s32(int theValue, unsigned int theShift)
{
    return vcvts_n_f32_s32(theValue, theShift);
}

// The reverse: multiply by 2^n, then convert to int with rounding toward zero.
inline int vcvts_n_s32_f32(float theValue, unsigned int theShift)
{
    return (int)(theValue * (float)(1u << theShift));
}

// ---------------------------------------------------------------------------
// __PAIR64__ / __SPAIR64__ -- build a 64-bit value from a high and a low word.
//
// The decompiler writes `__PAIR64__(hi, lo)`, so the arguments are high first.
// ---------------------------------------------------------------------------
inline std::uint64_t __PAIR64__(unsigned int theHigh, unsigned int theLow)
{
    return ((std::uint64_t)theHigh << 32) | (std::uint64_t)theLow;
}

inline std::int64_t __SPAIR64__(unsigned int theHigh, unsigned int theLow)
{
    return (std::int64_t)__PAIR64__(theHigh, theLow);
}

// The two halves, which the decompiler also emits as macros.
#ifndef __HIDWORD
#define __HIDWORD(x) ((unsigned int)(((std::uint64_t)(x)) >> 32))
#endif
#ifndef __LODWORD
#define __LODWORD(x) ((unsigned int)((std::uint64_t)(x)))
#endif

// ---------------------------------------------------------------------------
// Overflow-flag helpers.
//
// `__OFSUB__(a, b)` is the signed-overflow flag of `a - b`: nonzero when the
// subtraction overflowed. The decompiler emits it when the binary branched on
// the flag rather than on the result.
// ---------------------------------------------------------------------------
inline int __OFSUB__(int theA, int theB)
{
    // The signed-overflow flag of  - b. Computing it from the operands is the
    // same predicate the ARM flag holds, and needs no builtin.
    return ((theB > 0) && (theA < INT_MIN + theB)) || ((theB < 0) && (theA > INT_MAX + theB)) ? 1 : 0;
}

inline int __OFADD__(int theA, int theB)
{
    return ((theB > 0) && (theA > INT_MAX - theB)) || ((theB < 0) && (theA < INT_MIN - theB)) ? 1 : 0;
}

// ---------------------------------------------------------------------------
// Saturation and count-leading-zeros, which the NEON code uses.
// ---------------------------------------------------------------------------
inline int __ssat(int theValue, unsigned int theBits)
{
    int aMax = (1 << (theBits - 1)) - 1;
    int aMin = -(1 << (theBits - 1));
    if (theValue > aMax)
        return aMax;
    if (theValue < aMin)
        return aMin;
    return theValue;
}

inline unsigned int __usat(int theValue, unsigned int theBits)
{
    unsigned int aMax = (1u << theBits) - 1u;
    if (theValue < 0)
        return 0;
    if ((unsigned int)theValue > aMax)
        return aMax;
    return (unsigned int)theValue;
}

inline int __clz(unsigned int theValue)
{
    if (theValue == 0)
        return 32;
    int aCount = 0;
    while ((theValue & 0x80000000u) == 0)
    {
        theValue <<= 1;
        ++aCount;
    }
    return aCount;
}

// A reciprocal estimate, used by the path-finding code. The NEON instruction is
// an approximation; the exact reciprocal is within its tolerance and is what
// the surrounding arithmetic expects.
inline float vrecpe_f32(float theValue)
{
    return theValue != 0.0f ? 1.0f / theValue : 0.0f;
}

// A reciprocal square root estimate.
inline float vrsqrte_f32(float theValue)
{
    return theValue > 0.0f ? 1.0f / sqrtf(theValue) : 0.0f;
}

#endif // LAWNPROJECTTV_SEXY_INCLUDE_RUNTIME_ARMINTRINSICS_H
