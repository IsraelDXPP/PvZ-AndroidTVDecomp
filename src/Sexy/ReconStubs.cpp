// Small helpers the rest of the tree calls.
//
// Endian* is an identity on this little-endian build: the cache is already
// in host order. SMemR copies bytes from a moving definition-cache cursor.
// The AG* functions are the Android video and store entry points. This port
// has no video backend and no store library, so they report failure or an
// empty account and do not simulate a purchase.

#include "ReconFreeFunctions.h"
#include "ReconHelpers.h"

#include <cstring>

int EndianInt(int theValue)
{
    return theValue;
}

double EndianDouble(double theValue)
{
    return theValue;
}

int EndianShort(short theValue)
{
    return theValue;
}

int EndianUINT16(unsigned short theValue)
{
    return theValue;
}

unsigned int EndianUINT32(unsigned int theValue)
{
    return theValue;
}

int SMemR(unsigned char** theCursor, void* theDest, int theCount, int theLimit)
{
    (void)theLimit;
    if (theCursor == nullptr || *theCursor == nullptr || theDest == nullptr || theCount < 0)
        return 0;

    std::memcpy(theDest, *theCursor, (size_t)theCount);
    *theCursor += theCount;
    return theCount;
}

const char* AGGetExternalFilesDir()
{
    return nullptr;
}

const char* AGGetModel(int theView)
{
    (void)theView;
    return nullptr;
}

int AGGetSdkVersion()
{
    return 19;
}

int AGSetFullVersionListener(int (*theCallback)(bool, void*), void* theApp)
{
    (void)theCallback;
    (void)theApp;
    return 1;
}

int AGSetMessageBoxListener(int (*theCallback)(int, void*), void* theApp)
{
    (void)theCallback;
    (void)theApp;
    return 1;
}

int AGVideoClose()
{
    return 0;
}

int AGVideoEnable(int theEnable)
{
    (void)theEnable;
    return 0;
}

int AGVideoIsPlaying(int theUnused)
{
    (void)theUnused;
    return 0;
}

int AGVideoOpen(const char* thePath)
{
    (void)thePath;
    return 1;
}

int AGVideoPlay(int theUnused)
{
    (void)theUnused;
    return 1;
}

int AGVideoShow(int theShow)
{
    (void)theShow;
    return 0;
}

int AGViewAddEventListener(int* (*theListener)(int*), int theFlags)
{
    (void)theListener;
    (void)theFlags;
    return 1;
}

int AGViewRemoveEventListener(int* (*theListener)(int*))
{
    (void)theListener;
    return 1;
}

int AGViewShow(int theShow)
{
    (void)theShow;
    return 1;
}

void AGHideAD()
{
}

void AGShowAD(int theGravity, int theAdType)
{
    (void)theGravity;
    (void)theAdType;
}

void AGShowOfferAD(const char* theOffer, int theX, int theY, int theWidth,
                   int theAdType, int theHeight)
{
    (void)theOffer;
    (void)theX;
    (void)theY;
    (void)theWidth;
    (void)theAdType;
    (void)theHeight;
}

void AGShowItems(const char* theItem, int theCount, const char* theName,
                 const char* theDescription, float thePrice, int theFlags,
                 int theAdType)
{
    (void)theItem;
    (void)theCount;
    (void)theName;
    (void)theDescription;
    (void)thePrice;
    (void)theFlags;
    (void)theAdType;
}

void AGQueryBalance(const char* theAccount, int theAdType)
{
    (void)theAccount;
    (void)theAdType;
}

void AGGoToLogin(bool theForce, int theAdType)
{
    (void)theForce;
    (void)theAdType;
}

const char* AGGetUserName(int theAdType)
{
    (void)theAdType;
    return "";
}
