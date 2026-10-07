// IdaMacros.h -- the byte/word extraction macros IDA emits, as lvalues.
//
// WHY THIS IS ITS OWN HEADER
//
// The Windows SDK's <minwindef.h> (reached through <windows.h>) defines
// LOWORD, HIWORD, LOBYTE and HIBYTE as VALUE expressions with no `#ifndef`
// guard:
//
//     #define LOWORD(l) ((WORD)(((DWORD_PTR)(l)) & 0xffff))
//     #define LOBYTE(w) ((BYTE)(((DWORD_PTR)(w)) & 0xff))
//
// Because there is no guard, including <windows.h> at ANY point re-defines
// them, overwriting whatever was there before. Defining our versions earlier
// therefore does not survive: the SDK's win, and the transcribed bodies fail,
// because IDA uses these macros as ASSIGNMENT TARGETS as well as values:
//
//     LOBYTE(v6)  = 0;             // store into the low byte
//     LOWORD(v13) = 0x1234;        // store into the low halfword
//     HIBYTE(x)  |= mask;          // read-modify-write the second byte
//
// The SDK forms are rvalues, so `LOBYTE(v6) = 0;` is C2106.
//
// So this header is IDEMPOTENT -- it undefines whatever is present and then
// installs the lvalue forms -- and it must be included AFTER <windows.h>.
// AndroidShims.h (which pulls in windows.h) includes it last for that reason,
// and SexyTypes.h includes it too so a unit that never sees windows.h still
// gets the same spelling.
//
// The forms below cast the ADDRESS of the operand and dereference it, which is
// an lvalue naming the same storage. On a read they yield exactly what the SDK
// macros yielded on this little-endian 32-bit target: LOBYTE(x) is still
// `x & 0xFF`, HIWORD(x) is still `x >> 16`.
//
// The operand must be addressable -- a variable, a dereference or a cast, which
// is every use in the transcription. A bare temporary such as `LOBYTE(a + b)`
// would not be, and needs a local at that site.

// NOTE: there is deliberately NO include guard. The whole point of this header
// is to re-assert the macros after <windows.h> has clobbered them, so it must
// run every time it is included. That is safe: it undefines first, so
// re-including it is idempotent.

#ifdef LOBYTE
#undef LOBYTE
#endif
#ifdef HIBYTE
#undef HIBYTE
#endif
#ifdef BYTE1
#undef BYTE1
#endif
#ifdef BYTE2
#undef BYTE2
#endif
#ifdef LOWORD
#undef LOWORD
#endif
#ifdef HIWORD
#undef HIWORD
#endif
#ifdef WORD1
#undef WORD1
#endif
#ifdef LODWORD
#undef LODWORD
#endif
#ifdef HIDWORD
#undef HIDWORD
#endif
#ifdef BYTE3
#undef BYTE3
#endif
#ifdef BYTE4
#undef BYTE4
#endif
#ifdef BYTE5
#undef BYTE5
#endif
#ifdef BYTE6
#undef BYTE6
#endif
#ifdef BYTE7
#undef BYTE7
#endif

#define LOBYTE(x)  (*(unsigned char  *)&(x))
#define HIBYTE(x)  (*(unsigned char  *)((unsigned char *)&(x) + 1))
#define BYTE1(x)   (*(unsigned char  *)((unsigned char *)&(x) + 1))
#define BYTE2(x)   (*(unsigned char  *)((unsigned char *)&(x) + 2))
#define LOWORD(x)  (*(unsigned short *)&(x))
#define HIWORD(x)  (*(unsigned short *)((unsigned char *)&(x) + 2))
#define WORD1(x)   (*(unsigned short *)((unsigned char *)&(x) + 2))
#define LODWORD(x) (*reinterpret_cast<std::uint32_t *>(&(x)))
#define HIDWORD(x) (*reinterpret_cast<std::uint32_t *>(reinterpret_cast<unsigned char *>(&(x)) + 4))

// The signed spellings (SLODWORD / SHIDWORD / SLOBYTE) are deliberately NOT
// defined here. They are defined in Sexy/ReconFreeFunctions.h instead, next to
// the declarations they replace.
//
// Defining them in this file was tried and reverted: IdaMacros.h reaches every
// translation unit through SexyTypes.h, very early, so the macros came alive in
// headers that are parsed before ReconFreeFunctions.h -- including places where
// the identifier is not an lvalue reinterpret at all -- and the tree went from
// 0 to 104 compile errors. ReconFreeFunctions.h is the last header in the chain,
// which is where they were already working.

// IDA's "byte N of a 64-bit value" spellings. The decompiler emits them when it
// widens a value to 64 bits and then reads the upper halves -- `BYTE4(v221)`
// in Mailbox.cpp is the first one this tree hit. Without the macros the parser
// reads `BYTE4(x)` as a call to an unknown function, and `BYTE4(x) = y;` is
// then C2106 ("left operand must be an lvalue"). All are lvalues by the same
// address-cast construction the macros above use.
#define BYTE3(x)   (*(unsigned char  *)((unsigned char *)&(x) + 3))
#define BYTE4(x)   (*(unsigned char  *)((unsigned char *)&(x) + 4))
#define BYTE5(x)   (*(unsigned char  *)((unsigned char *)&(x) + 5))
#define BYTE6(x)   (*(unsigned char  *)((unsigned char *)&(x) + 6))
#define BYTE7(x)   (*(unsigned char  *)((unsigned char *)&(x) + 7))
