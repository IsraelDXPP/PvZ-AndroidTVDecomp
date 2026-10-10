// JSONWorker -- reconstructed from libGameMain.so.
//
// Plain (non-polymorphic) class: no RTTI export and no vtable, so it declares no
// virtuals and needs no base. None of its methods is virtual; the parser is
// driven through ordinary member calls from JSONNode.
//
// The class has no constructor in the binary (the object is produced by the
// JSONNode entry points), so the member table cannot be recovered from an
// initialiser. Exactly one member is provably used: word 0, which parse()
// advances by three (`*(_DWORD *)this += 3;`). The remaining 13 words are the
// parser's own scratch state, which the transcription never pinned down, so they
// are left as explicit words rather than invented names.
//
// The 0x38 size is the transcriber's, from the largest `this`-relative offset in
// _recon/ida/JSONWorker.cpp.
//
// TODO(recon): recover the parser state fields (the offsets past +0x04 are only
// reached through the words above, never through a named member).

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_JSONWORKER_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_JSONWORKER_H

#include "ReconTypes.h"
// JSONNode.h is deliberately NOT included: nothing declared below spells
// JSONNode, and JSONNode is `internalJSONNode`'s business, not this class's.
//
// internalJSONNode.h and jsonChildren.h STAY. Nothing in this header's own
// declarations needs them complete -- internalJSONNode occurs only as
// `internalJSONNode const*`, which ReconTypes.h already forward-declares at line
// 1429 -- but Game\JSONWorker.cpp includes this header and Game\JSONNode.h and
// nothing else of the four, so its bodies reach jsonChildren and
// internalJSONNode through here. Dropping either one broke that file with C2027
// ("use of undefined type 'jsonChildren'"). The cycle is therefore cut on the
// jsonChildren.h side, not here.
#include "internalJSONNode.h"
#include "jsonChildren.h"
#include "JSONNode.h"
#include <string>
#include <vector>
#include <map>
#include <set>

class JSONWorker
{
public:
    // parse() stores the node it built in the receiver's first word. That word
    // is a JSONNode handle. The other words the dump reserved were never read.
    JSONNode mNode;

    JSONWorker *_parse_unformatted(char const *theFirst, char const *theLast);
    JSONWorker *parse_unformatted(Sexy::String const &theText);
    JSONWorker *FixString(Sexy::String const &theText, bool &theEscaped, Sexy::String &theDest);
    char *RemoveWhiteSpace(Sexy::String const &theText, unsigned int &theLength, bool theDecoded);
    JSONWorker *parse(Sexy::String const &theText);
    char *RemoveWhiteSpaceAndCommentsC(Sexy::String const &theText, bool theDecoded);
    Sexy::String RemoveWhiteSpaceAndComments(Sexy::String const &theText, bool theDecoded);
    void DoArray(internalJSONNode *theNode, Sexy::String const &theText);
    void DoNode(internalJSONNode *theNode, Sexy::String const &theText);
    int Hex(char const *&theCursor);
    int UTF8(char const *&theCursor, char const *theEnd);
    JSONWorker *SpecialChar(char const *&theCursor, char const *theEnd, Sexy::String &theDest);
    JSONWorker *toUTF8(unsigned char theByte, Sexy::String &theDest);
    unsigned char **UnfixString(Sexy::String const &theText, bool theEscape, Sexy::String &theDest);

    // JSONWorker::FindNextRelevant<char C>(std::string const&, unsigned int)
    //
    // The binary instantiates this twice, at (char)44 and (char)58:
    //   _ZN10JSONWorker16FindNextRelevantILc44EEEjRKSsj  @0x52b870  (220 bytes)
    //   _ZN10JSONWorker16FindNextRelevantILc58EEEjRKSsj  @0x52b94c  (220 bytes)
    // It scans the string for C while skipping over nested [] {} and quoted
    // runs, returning the index or -1. Declared here, defined in JSONWorker.cpp
    // with explicit instantiations for the two characters the binary uses.
    template <char C> static unsigned int FindNextRelevant(Sexy::String const &theText, unsigned int theStart);
};

// Layout check: 14 words.

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_JSONWORKER_H
