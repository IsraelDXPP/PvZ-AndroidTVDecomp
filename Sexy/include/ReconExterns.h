// ReconExterns.h -- helpers recovered from libGameMain.so.
//
// Each name says what the implementation does. The address in the trailing
// comment is where that function lives in the .so. Call sites pass the types
// on the declaration. This file is hand-maintained. Never regenerate it.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_RECONEXTERNS_H
#define LAWNPROJECTTV_SEXY_INCLUDE_RECONEXTERNS_H

#include "SexyTypes.h"
#include <exception>

namespace Sexy
{
class FontLayer;
class String;
class Graphics;
}

// --- refcounted-object destructors and small accessors ---------------------
char *DisposeWStringRep(char *theRep); // @0x12EC94
char *DisposeMenuStringRep(char *theRep); // @0x12ED7C
char *DisposeSharedStringRep(char *theRep); // @0x12EE24
char *DisposeBufferStringRep(char *theRep); // @0x12EE9C
char *SetDescParserRepLength(char *theRep, unsigned int theLength); // @0x12EF0C
char *DisposeEditStringRep(char *theRep); // @0x12EF1C
char *DisposeGraphicsStringRep(char *theRep); // @0x12EF7C
char *SetFontRepLength(char *theRep, unsigned int theLength); // @0x12EFAC
Sexy::FontLayer *CloneFontLayerNode(const Sexy::FontLayer *theSource); // @0x12EFBC
char *DisposeAppStringRep(char *theRep); // @0x12EFF0
char *SetXmlRepLength(char *theRep, unsigned int theLength); // @0x12F020
char *DisposeXmlStringRep(char *theRep); // @0x12F030
char *DisposeLayoutStringRep(char *theRep); // @0x12F060
char *DisposeFontStringRep(char *theRep); // @0x12F0F0
Sexy::String *StringConcatCStr(Sexy::String *theResult, const char *theLeft, const Sexy::String *theRight); // @0x13A8B0
Sexy::String *StringConcat(
    Sexy::String *theResult, const Sexy::String *theLeft, const Sexy::String *theRight); // @0x13A914
char *StringConstructRange(const char *theBegin, const char *theEnd, const void *theAlloc, int theTag); // @0x162F48
char *IntStringAllocRep(unsigned int theCapacity, unsigned int theOldCapacity); // @0x1A3AF8
int CloseIntroVideo(); // @0x1A652C
int *OnIntroVideoEvent(int *theEvent); // @0x1A6C68
int DrawCreditsDiscoLights(Sexy::Graphics *theGraphics, float theCentreX, float theAngle); // @0x1B91D8
int RegisterAtExit(void *theArg, void (*theFunc)(void *), void *theDso); // @0x2C717C
void *FileBufFinish(void *theFileBuf); // @0x2C7534
int InitCtypeWiden(void *theFacet); // @0x2C78A8
void *StreamExtractString(void *theStream, Sexy::String *theString); // @0x2C799C
void *StreamGetLine(void *theStream, Sexy::String *theString, int theDelimiter); // @0x2C7D28

// --- the libstdc++ COW string block ---------------------------------------
// Runtime/SexyString.h names the ones whose identity is already established; the
// comment on those lines says which named helper the address corresponds to.
void *ListHook(void *theNode, void *thePos); // @0x2D63E8
void *ListUnhook(void *theNode); // @0x2D6404
void *DestroyLocale(void *theLocale); // @0x2D7014
void *IosSetState(void *theStream, int theState); // @0x2D74CC
void *IosInit(void *theStream, void *theStreambuf); // @0x2D7580
void *RbTreeNext(void *theNode); // @0x2D77D0
void *RbTreeNextConst(void *theNode); // @0x2D783C
void *RbTreePrev(void *theStream); // @0x2D78A8
void *RbTreePrevConst(void *theStream); // @0x2D78AC
int RbTreeInsert(int theInsertLeft, void *theNode, void *theParent, void *theHeader); // @0x2D78B0
void *RbTreeErase(void *theNode, void *theHeader); // @0x2D7A14
void *OStreamFlush(void *theStream); // @0x2D7F60
void *OStreamPut(void *theStream, char theFill); // @0x2D822C
void *OStreamWrite(void *theStream, const char *theValue, int theBase); // @0x2D83B4
int *OStreamInsert(int *theStream, const char *theValue, int theBase); // @0x2D8528
void *OStreamWriteNum(void *theStream, int theValue); // @0x2D8B28
void *OStreamWriteInt(void *theStream, int theValue); // @0x2D8B2C
void *IStreamReadInt(void *theStream, int *theValue); // @0x2E7078
void *IStreamGet(void *theStream, char *theText); // @0x2E7554
void *StringBufConstruct(void *theStream); // @0x2E766C
void *StringBufDestroy(void *theStream); // @0x2E778C
std::exception *DestroyInvalidArgument(std::exception *theException); // @0x2E78B4
std::exception *DestroyOutOfRange(std::exception *theException); // @0x2E7964
void *ConstructOutOfRange(void *theException, const Sexy::String *theMessage); // @0x2E7A98
void *ConstructOutOfRangeCopy(void *theException, const Sexy::String *theMessage); // @0x2E7AE8
void *InitStreamToken(void *theStream); // @0x2E923C
void *DestroyTextStream(void *theStream); // @0x2E94F0
char *StringBufSetPointers(void *theBuffer, char *theData, int theOffset, int theCount); // @0x2E9A68
void *ConstructIStringStream(void *theStream, const Sexy::String *theSource, int theMode); // @0x2E9FFC
void *FileBufConstruct(void *theFileBuf); // @0x2EAC58
void *FileBufClose(void *theFileBuf); // @0x2EB174
void *FileBufOpen(void *theFileBuf, const char *theFilename, int theMode); // @0x2EB2B8
unsigned int StringFind(
    Sexy::String *theString, const char *theText, unsigned int thePos, unsigned int theCount); // @0x2EC118
unsigned int StringFindText(Sexy::String *theString, const char *theText, unsigned int thePos); // @0x2EC1B4
int StringFindChar(Sexy::String *theString, int theChar, unsigned int thePos); // @0x2EC1E4
unsigned int StringRFind(
    Sexy::String *theString, const void *theData, unsigned int thePos, unsigned int theCount); // @0x2EC220
int StringRFindChar(const Sexy::String *theString, int theChar, unsigned int thePos); // @0x2EC284
unsigned int StringFindFirstNotOf(
    Sexy::String *theString, const void *theData, unsigned int thePos, unsigned int theCount); // @0x2EC320
char *CreateStringRep(unsigned int theCapacity, unsigned int theOldCapacity); // @0x2EC498
char *StringFillConstruct(unsigned int theCapacity, int theOldCapacity); // @0x2EC51C
char *StringFillSlot(char **theSlot, unsigned int theCapacity, int theOldCapacity); // @0x2EC5A0
Sexy::String *StringDestroy(Sexy::String *theString); // @0x2EC5E0
Sexy::String *StringMutate(
    Sexy::String *theString, unsigned int thePos, unsigned int theCount, unsigned int theReplacement); // @0x2EC620
char *StringLeakEnd(Sexy::String *theString); // @0x2EC7D8
char *StringEraseRange(Sexy::String *theString, char *theFirst, char *theLast); // @0x2EC848
Sexy::String *StringReplaceFill(Sexy::String *theString, unsigned int thePos, unsigned int theCount,
    unsigned int theFill, unsigned char theChar); // @0x2EC888
Sexy::String *StringReserve(Sexy::String *theString, unsigned int theCapacity); // @0x2ECAF8
Sexy::String *StringAppendText(Sexy::String *theString, const char *theText); // @0x2ECD28
Sexy::String *StringAppendFill(Sexy::String *theString, unsigned int theCount, int theChar); // @0x2ECD50
Sexy::String *StringResize(Sexy::String *theString, unsigned int theSize, int theChar); // @0x2ECE0C
Sexy::String *StringPushBack(Sexy::String *theString, char theChar); // @0x2ECE58
Sexy::String *StringCheckedSubstr(
    Sexy::String *theResult, const Sexy::String *theSource, unsigned int thePos, unsigned int theCount); // @0x2ED0D4
char *StringConstructIter(const char *theFirst, const char *theLast, const void *theAlloc, int theTag); // @0x2ED108
Sexy::String *StringAssignRange(
    Sexy::String *theString, const char *theFirst, const char *theLast, int theChar); // @0x2ED1CC
int StringReplace(Sexy::String *theString, unsigned int thePos, unsigned int theCount, const char *theReplacement,
    unsigned int theFlags); // @0x2ED1F4

// --- remaining game-side helpers ------------------------------------------
char *CloneBufferStringRep(const char *theRep, int theExtra); // @0x2F7548
void ReleaseClonedStringRep(unsigned int *theRep); // @0x2FE708
char *CloneWideStringRep(const char *theRep, int theExtra); // @0x2FE818
char *CloneImageStringRep(const char *theRep, int theExtra); // @0x30369C
bool DebugDrawStringEnabled(); // cached SEXY_DEBUG_DRAW_STRING byte @0x307C98
char *CloneWideStringChars(char *theRep, int theExtra); // @0x307D30
int ReadEndianString(void *theStream, Sexy::String *theText); // @0x30F8B4
void ReleaseWideStringRep(unsigned int *theRep); // @0x30F924
char *CreateWideStringRep(unsigned int theCapacity, unsigned int theOldCapacity); // @0x30FAEC
unsigned int *LeakWideStringData(unsigned int *theRep); // @0x30FB5C
int FillIntVectorFromCsv(Sexy::String *theText, void *theOut); // @0x330424
namespace Sexy
{
class SexyAppBase;
class MemoryImage;
}
Sexy::MemoryImage *MakeCursorImage(Sexy::SexyAppBase *theApp, const unsigned char *thePixels);
const char *SortWidgetRangeFinal(const char *theBegin, const char *theEnd); // @0x3ADDC4
const char *SortWidgetRangeIntro(const char *theBegin, const char *theEnd, int theFlags); // @0x3AE368
int *GrowIntString(int *theRep, int theExtra); // @0x3B4478
int *GrowIntStringFresh(int *theRange, int theCount); // @0x3BD0B8
int ReadDevicePixel(void *theImage, int theX, int theY); // @0x3CA788
unsigned char *ReadChecksummedBytes(void *theReader, void *theDest, int theCount); // @0x3CB348
Sexy::String *SpliceRemapPattern(const Sexy::String *theInput, unsigned int theBegin, unsigned int theEnd,
    const Sexy::String *theValue); // @0x3E6CD8
float MeasureCurvePiece(void *theCurve, int theIndex, float theStart, float theEnd); // @0x40EDFC
Sexy::String *AssignStringThunk(Sexy::String *theTarget, const Sexy::String *theSource); // @0x5229AC
void *HookListNode(void *theNode, void *thePos); // @0x5229CC
Sexy::String *MutateStringThunk(
    Sexy::String *theString, unsigned int thePos, unsigned int theCount, const char *theReplacement); // @0x522A1C

#endif // LAWNPROJECTTV_SEXY_INCLUDE_RECONEXTERNS_H
