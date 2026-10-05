#include "CritSect.h"

// The binary wraps a one-word mutex. The Windows stand-in is a
// CRITICAL_SECTION, which is twenty-four bytes and does not fit: ResourceManager
// places a CritSect twenty bytes from the end of its block, and two CritSects
// sit four bytes apart in SexyAppBase, InputManager and LogManager. The section
// is therefore heap allocated and the CritSect keeps the pointer in its first
// word, which CritSect.h already declares as mSection.
//
// The pointer is filled on first use, so a CritSect that exists only as raw
// storage still locks. ImageFont's gRenderCritSec is a bare word that the
// transcribed code treats as a CritSect; it has no constructor to run.

namespace
{
    // CritSect.h stores the section as void* so the header does not pull in
    // the Windows type. The word is the CRITICAL_SECTION*.
    struct SectionSlot
    {
        CRITICAL_SECTION* mSection;
    };

    SectionSlot* AsSlot(void* theSect)
    {
        return static_cast<SectionSlot*>(theSect);
    }

    CRITICAL_SECTION* SectionOf(void* theSect)
    {
        SectionSlot* theSlot = AsSlot(theSect);
        CRITICAL_SECTION* theSection = theSlot->mSection;
        if (theSection != nullptr)
            return theSection;

        CRITICAL_SECTION* theNew = new CRITICAL_SECTION;
        ::InitializeCriticalSection(theNew);
        if (::InterlockedCompareExchangePointer(
                static_cast<void* volatile*>(static_cast<void*>(&theSlot->mSection)), theNew,
                nullptr) != nullptr)
        {
            ::DeleteCriticalSection(theNew);
            delete theNew;
            return theSlot->mSection;
        }
        return theNew;
    }

    CRITICAL_SECTION* ExistingSectionOf(void* theSect)
    {
        return AsSlot(theSect)->mSection;
    }

    void DestroySection(void* theSect)
    {
        SectionSlot* theSlot = AsSlot(theSect);
        CRITICAL_SECTION* theSection = theSlot->mSection;
        if (theSection != nullptr)
        {
            ::DeleteCriticalSection(theSection);
            delete theSection;
            theSlot->mSection = nullptr;
        }
    }
}

Sexy::CritSect::CritSect()
{
    mSection = nullptr;
    (void)SectionOf(this);
}

Sexy::CritSect::~CritSect()
{
    DestroySection(this);
}

bool Sexy::CritSect::TryLock()
{
    return ::TryEnterCriticalSection(SectionOf(this)) != 0;
}

int Sexy::CritSect::Lock()
{
    ::EnterCriticalSection(SectionOf(this));
    return 0;
}

int Sexy::CritSect::Unlock()
{
    CRITICAL_SECTION* theSection = ExistingSectionOf(this);
    if (theSection == nullptr)
        return 0;
    ::LeaveCriticalSection(theSection);
    return 0;
}

int PakLib::CritSect::Enter()
{
    ::EnterCriticalSection(SectionOf(this));
    return 0;
}

int PakLib::CritSect::Leave()
{
    CRITICAL_SECTION* theSection = ExistingSectionOf(this);
    if (theSection == nullptr)
        return 0;
    ::LeaveCriticalSection(theSection);
    return 0;
}

// PakLib's class shares Sexy's thunks, so the remaining members forward.
PakLib::CritSect::CritSect()
{
    mSection = nullptr;
    (void)SectionOf(this);
}

PakLib::CritSect::~CritSect()
{
    DestroySection(this);
}

bool PakLib::CritSect::TryLock()
{
    return ::TryEnterCriticalSection(SectionOf(this)) != 0;
}

int PakLib::CritSect::Lock()
{
    ::EnterCriticalSection(SectionOf(this));
    return 0;
}

int PakLib::CritSect::Unlock()
{
    CRITICAL_SECTION* theSection = ExistingSectionOf(this);
    if (theSection == nullptr)
        return 0;
    ::LeaveCriticalSection(theSection);
    return 0;
}
