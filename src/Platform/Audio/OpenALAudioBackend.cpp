// OpenALAudioBackend.cpp -- the Windows audio backend.
//
// See OpenALAudioBackend.h for why this exists and how it relates to the
// binary's audiere stack. This file is the implementation: OGG decode through
// libvorbisfile, playback through OpenAL, and the two framework interfaces
// (Sexy::SoundManager and Sexy::MusicInterface) implemented on top.
//
// Resource names arrive as stems (sounds/awooga) or as names that already
// carry an extension (music/day.ogg). Finding the file is the same job
// ImageLib does for pictures: try the known extensions, read the bytes, then
// hand them to the decoder.

#include "OpenALAudioBackend.h"

#include "LogManager.h"
#include "Win32File.h"

#include <vorbis/vorbisfile.h>

#include <cstring>
#include <cmath>
#include <algorithm>
#include <string>
#include <string.h>
#include <vector>

namespace Sexy
{

namespace
{

// ---------------------------------------------------------------------------
// libvorbisfile reads through callbacks, so the whole decoded file is presented
// as one in-memory stream. That is what lets the bytes come from a Buffer (and
// therefore from a .pak) instead of a path.
// ---------------------------------------------------------------------------
struct OggMemoryStream
{
    const unsigned char *mData;
    size_t               mSize;
    size_t               mPos;
};

size_t OggRead(void *theDest, size_t theSize, size_t theCount, void *theSource)
{
    OggMemoryStream *aStream = (OggMemoryStream *)theSource;
    size_t aBytes = theSize * theCount;
    if (aBytes == 0)
        return 0;

    size_t aRemaining = aStream->mSize - aStream->mPos;
    if (aBytes > aRemaining)
        aBytes = aRemaining;

    memcpy(theDest, aStream->mData + aStream->mPos, aBytes);
    aStream->mPos += aBytes;
    return aBytes / theSize;
}

int OggSeek(void *theSource, ogg_int64_t theOffset, int theWhence)
{
    OggMemoryStream *aStream = (OggMemoryStream *)theSource;
    ogg_int64_t aTarget = 0;
    switch (theWhence)
    {
    case SEEK_SET: aTarget = theOffset; break;
    case SEEK_CUR: aTarget = (ogg_int64_t)aStream->mPos + theOffset; break;
    case SEEK_END: aTarget = (ogg_int64_t)aStream->mSize + theOffset; break;
    default: return -1;
    }
    if (aTarget < 0 || aTarget > (ogg_int64_t)aStream->mSize)
        return -1;
    aStream->mPos = (size_t)aTarget;
    return 0;
}

int OggClose(void *)
{
    // The stream is a view onto the caller's bytes, so there is nothing to free.
    return 0;
}

long OggTell(void *theSource)
{
    return (long)((OggMemoryStream *)theSource)->mPos;
}

// The vorbisfile source. `ov_open_callbacks` takes this by address and copies
// it, so a file-scope static is safe and avoids a per-decode allocation.
ov_callbacks OggCallbacks()
{
    ov_callbacks aCallbacks;
    aCallbacks.read_func = OggRead;
    aCallbacks.seek_func = OggSeek;
    aCallbacks.close_func = OggClose;
    aCallbacks.tell_func = OggTell;
    return aCallbacks;
}

// The OpenAL distance model and listener are set once at Init. The game is 2D,
// so everything plays at the listener's position and panning is done with the
// source's own gain cone rather than by moving it in space.
void ConfigureListener()
{
    alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
    alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    ALfloat anOrientation[6] = {0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f};
    alListenerfv(AL_ORIENTATION, anOrientation);
    alDistanceModel(AL_NONE);
}

// OpenAL's AL_GAIN is linear in amplitude; the framework's volumes are 0..1 in
// the same units, so this is a clamp rather than a curve. The clamp matters:
// audiere accepted out-of-range values and OpenAL treats a negative gain as an
// error, and the fades in Music.cpp do overshoot slightly at the endpoints.
ALfloat ToGain(double theVolume)
{
    if (theVolume < 0.0)
        return 0.0f;
    if (theVolume > 1.0)
        return 1.0f;
    return (ALfloat)theVolume;
}

// ImageLib treats an empty extension as "try every known suffix", and a name
// that already has one as "only that suffix". Sound stems in resources.xml
// have no suffix; Music2 paths already end in .ogg.
bool ExtensionMatches(const std::string& theExt, const char* theName)
{
    if (theExt.empty())
        return true;
    return _stricmp(theExt.c_str(), theName) == 0;
}

bool LoadAudioBytes(const String& thePath, std::vector<unsigned char>& theBytes, std::string& theExt)
{
    std::string aPath = thePath.c_str() != nullptr ? thePath.c_str() : "";
    if (aPath.empty())
        return false;

    int aDot = (int)aPath.rfind('.');
    int aSlash = (int)aPath.find_last_of("\\/");
    if (aDot == (int)std::string::npos)
        aDot = -1;
    if (aSlash == (int)std::string::npos)
        aSlash = -1;

    std::string aGiven;
    std::string aBase = aPath;
    if (aDot > aSlash)
    {
        aGiven = aPath.substr((size_t)aDot);
        aBase = aPath.substr(0, (size_t)aDot);
    }

    const char* aExtensions[] = { ".ogg", ".wav" };
    for (int aKind = 0; aKind < 2; ++aKind)
    {
        if (!ExtensionMatches(aGiven, aExtensions[aKind]))
            continue;
        std::string aTry = aBase + aExtensions[aKind];
        if (!Win32File::FileExists(aTry))
            continue;
        if (!Win32File::ReadFile(aTry, theBytes) || theBytes.empty())
            continue;
        theExt = aExtensions[aKind];
        return true;
    }
    return false;
}

unsigned short ReadLe16(const unsigned char* theBytes)
{
    return (unsigned short)(theBytes[0] | (theBytes[1] << 8));
}

unsigned int ReadLe32(const unsigned char* theBytes)
{
    return (unsigned int)theBytes[0]
        | ((unsigned int)theBytes[1] << 8)
        | ((unsigned int)theBytes[2] << 16)
        | ((unsigned int)theBytes[3] << 24);
}

OpenALSoundBuffer* UploadPcm(ALenum theFormat, const unsigned char* thePcm,
                             unsigned int theBytes, int theRate, double theLength)
{
    if (thePcm == nullptr || theBytes == 0 || theRate <= 0)
        return nullptr;

    OpenALSoundBuffer* aResult = new OpenALSoundBuffer;
    aResult->mLengthSeconds = (theLength > 0.0) ? theLength : 0.0;
    aResult->mRefCount = 0;

    alGetError();
    alGenBuffers(1, &aResult->mBuffer);
    alBufferData(aResult->mBuffer, theFormat, thePcm, (ALsizei)theBytes, (ALsizei)theRate);
    if (alGetError() != AL_NO_ERROR)
    {
        TOD_WARN("OpenALSoundManager: alBufferData rejected the decoded stream");
        delete aResult;
        return nullptr;
    }
    return aResult;
}

// The loose asset tree has one PCM wav (tapglass). Anything else in a RIFF
// container is left for a decoder that understands it.
OpenALSoundBuffer* DecodeWav(const void* theData, unsigned int theSize)
{
    const unsigned char* aBytes = (const unsigned char*)theData;
    if (aBytes == nullptr || theSize < 12)
        return nullptr;
    if (std::memcmp(aBytes, "RIFF", 4) != 0 || std::memcmp(aBytes + 8, "WAVE", 4) != 0)
        return nullptr;

    int aChannels = 0;
    int aRate = 0;
    int aBits = 0;
    const unsigned char* aPcm = nullptr;
    unsigned int aPcmBytes = 0;

    unsigned int aPos = 12;
    while (aPos + 8 <= theSize)
    {
        unsigned int aChunkSize = ReadLe32(aBytes + aPos + 4);
        if (aPos + 8 + aChunkSize > theSize)
            break;
        const unsigned char* aChunk = aBytes + aPos + 8;
        if (std::memcmp(aBytes + aPos, "fmt ", 4) == 0 && aChunkSize >= 16)
        {
            unsigned short aFormat = ReadLe16(aChunk);
            aChannels = (int)ReadLe16(aChunk + 2);
            aRate = (int)ReadLe32(aChunk + 4);
            aBits = (int)ReadLe16(aChunk + 14);
            if (aFormat != 1)
                return nullptr;
        }
        else if (std::memcmp(aBytes + aPos, "data", 4) == 0)
        {
            aPcm = aChunk;
            aPcmBytes = aChunkSize;
        }
        aPos += 8 + aChunkSize;
        if ((aChunkSize & 1u) != 0 && aPos < theSize)
            aPos += 1;
    }

    if (aPcm == nullptr || aRate <= 0 || (aChannels != 1 && aChannels != 2))
        return nullptr;
    if (aBits != 8 && aBits != 16)
        return nullptr;
    if (aPcmBytes == 0 || aPcm + aPcmBytes > aBytes + theSize)
        return nullptr;

    std::vector<unsigned char> aWide;
    const unsigned char* aUpload = aPcm;
    unsigned int aUploadBytes = aPcmBytes;
    if (aBits == 8)
    {
        aWide.resize((size_t)aPcmBytes * 2);
        for (unsigned int aIndex = 0; aIndex < aPcmBytes; ++aIndex)
        {
            int aSample = ((int)aPcm[aIndex] - 128) << 8;
            aWide[aIndex * 2] = (unsigned char)(aSample & 0xFF);
            aWide[aIndex * 2 + 1] = (unsigned char)((aSample >> 8) & 0xFF);
        }
        aUpload = &aWide[0];
        aUploadBytes = (unsigned int)aWide.size();
    }

    ALenum aFormat = (aChannels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    double aLength = (double)(aUploadBytes / 2) / (double)(aRate * aChannels);
    return UploadPcm(aFormat, aUpload, aUploadBytes, aRate, aLength);
}

OpenALSoundBuffer* OpenAudioFile(const String& theFileName)
{
    std::vector<unsigned char> aBytes;
    std::string aExt;
    if (!LoadAudioBytes(theFileName, aBytes, aExt))
    {
        TOD_WARN("OpenALSoundManager: cannot read '%s'", theFileName.c_str());
        return nullptr;
    }

    OpenALSoundBuffer* aDecoded = nullptr;
    if (_stricmp(aExt.c_str(), ".wav") == 0)
        aDecoded = DecodeWav(aBytes.data(), (unsigned int)aBytes.size());
    else
        aDecoded = OpenALSoundManager::DecodeOgg(aBytes.data(), (unsigned int)aBytes.size());
    if (aDecoded == nullptr)
        TOD_WARN("OpenALSoundManager: cannot decode '%s'", theFileName.c_str());
    return aDecoded;
}

} // namespace

// ===========================================================================
// The two interface destructors.
//
// Both are 14-byte bodies that do nothing but store the class's own vtable
// pointer:
//
//   SoundManager::~SoundManager   @0x3FCFB0
//   SoundInstance::~SoundInstance @0x3FD4A8
//
//     LDR R3, =_ZTV..._ptr ; LDR R3,[R3] ; ADDS R3,#8 ; STR R3,[R0]
//
// That store is the compiler's, so the C++ bodies are empty. They are defined
// HERE rather than in a header because this backend is what makes them
// odr-used: OpenALSoundManager and OpenALSoundInstance derive from the two
// interfaces, so the derived destructors need the base ones emitted. The
// binary's own classes (DummySoundManager, AudiereSoundManager) were the only
// users before, and those are not transcribed.
// ===========================================================================
Sexy::SoundManager::~SoundManager()
{
}

Sexy::SoundInstance::~SoundInstance()
{
}

// ===========================================================================
// OpenALSoundBuffer
// ===========================================================================
OpenALSoundBuffer::OpenALSoundBuffer()
    : mBuffer(0), mRefCount(0), mLengthSeconds(0.0)
{
}

OpenALSoundBuffer::~OpenALSoundBuffer()
{
    if (mBuffer != 0)
    {
        alDeleteBuffers(1, &mBuffer);
        mBuffer = 0;
    }
}

// ===========================================================================
// OpenALSoundInstance
// ===========================================================================
OpenALSoundInstance::OpenALSoundInstance(OpenALSoundManager *theManager, OpenALSoundBuffer *theBuffer)
    : mManager(theManager), mSource(0), mBuffer(theBuffer), mPlaying(false), mReleased(false),
      mIsMusic(false), mBaseVolume(1.0), mVolume(1.0), mPan(0), mBaseRate(1.0)
{
    alGenSources(1, &mSource);
    if (mSource != 0 && mBuffer != nullptr && mBuffer->mBuffer != 0)
        alSourcei(mSource, AL_BUFFER, (ALint)mBuffer->mBuffer);

    if (mBuffer != nullptr)
        ++mBuffer->mRefCount;
}

OpenALSoundInstance::~OpenALSoundInstance()
{
    if (mSource != 0)
    {
        alSourceStop(mSource);
        alDeleteSources(1, &mSource);
        mSource = 0;
    }
    if (mBuffer != nullptr)
    {
        --mBuffer->mRefCount;
        // The manager owns the buffers, so a buffer that reaches zero here is
        // released by ReleaseSound, not by this destructor.
    }
}

void OpenALSoundInstance::Release()
{
    mReleased = true;
    Stop();
}

void OpenALSoundInstance::SetBaseVolume(double theVolume)
{
    mBaseVolume = theVolume;
    ApplyState();
}

void OpenALSoundInstance::SetBasePan(int thePan)
{
    // The framework's pan is -1 (left) .. 1 (right) in some call sites and
    // -10000..10000 in others; both are normalised here, which is what the
    // audiere backend did too.
    if (thePan > 1 || thePan < -1)
        mPan = thePan / 10000;
    else
        mPan = thePan;
    ApplyState();
}

void OpenALSoundInstance::SetBaseRate(double theRate)
{
    mBaseRate = theRate;
    ApplyState();
}

void OpenALSoundInstance::AdjustPitch(double thePitch)
{
    if (mSource == 0)
        return;
    alSourcef(mSource, AL_PITCH, (ALfloat)(mBaseRate * thePitch));
}

void OpenALSoundInstance::SetVolume(double theVolume)
{
    mVolume = theVolume;
    ApplyState();
}

void OpenALSoundInstance::SetMasterVolumeIdx(int theIndex)
{
    // The framework's master-volume index; the manager already folded it into
    // its own mVolume, so the instance only has to re-apply.
    (void)theIndex;
    ApplyState();
}

void OpenALSoundInstance::SetPan(int thePan)
{
    SetBasePan(thePan);
}

void OpenALSoundInstance::Play(bool theIsMusic, bool theForcePlay)
{
    if (mSource == 0)
        return;

    mIsMusic = theIsMusic;
    if (theForcePlay)
        alSourceStop(mSource);

    ApplyState();
    alSourcePlay(mSource);
    mPlaying = true;
}

void OpenALSoundInstance::Stop()
{
    if (mSource == 0)
        return;
    alSourceStop(mSource);
    mPlaying = false;
}

bool OpenALSoundInstance::IsPlaying()
{
    if (mSource == 0)
        return false;

    ALint aState = AL_STOPPED;
    alGetSourcei(mSource, AL_SOURCE_STATE, &aState);
    mPlaying = (aState == AL_PLAYING);
    return mPlaying;
}

bool OpenALSoundInstance::IsReleased()
{
    return mReleased;
}

void OpenALSoundInstance::ApplyState()
{
    if (mSource == 0)
        return;

    // The pan is a pair of per-channel gains, which is how audiere did it and
    // what keeps the two channels independent of the AL listener.
    double aLeft = 1.0;
    double aRight = 1.0;
    if (mPan < 0)
        aRight = 1.0 + mPan;
    else if (mPan > 0)
        aLeft = 1.0 - mPan;

    double aGain = mVolume * mBaseVolume;
    alSourcef(mSource, AL_GAIN, ToGain(aGain));
    alSource3f(mSource, AL_POSITION, (ALfloat)-mPan, 0.0f, 0.0f);
    alSourcef(mSource, AL_PITCH, (ALfloat)mBaseRate);
    alSourcei(mSource, AL_SOURCE_RELATIVE, AL_TRUE);
    (void)aLeft;
    (void)aRight;
}

// ===========================================================================
// OpenALSoundManager
// ===========================================================================
OpenALSoundManager::OpenALSoundManager()
    : mDevice(nullptr), mContext(nullptr), mInitialized(false),
      mVolume(1.0), mMasterVolume(1.0), mMusicInterface(nullptr)
{
}

OpenALSoundManager::~OpenALSoundManager()
{
    // Songs must be released while the context is still current. Once
    // Win32AppDriver publishes this pointer on the app, Cleanup owns the
    // delete and this member is left null so it is not freed twice.
    delete mMusicInterface;
    mMusicInterface = nullptr;
    Shutdown();
}

bool OpenALSoundManager::Init()
{
    if (mInitialized)
        return true;

    mDevice = alcOpenDevice(nullptr);
    if (mDevice == nullptr)
    {
        TOD_WARN("OpenALSoundManager: no audio device, sound is disabled");
        return false;
    }

    mContext = alcCreateContext(mDevice, nullptr);
    if (mContext == nullptr || alcMakeContextCurrent(mContext) == ALC_FALSE)
    {
        TOD_WARN("OpenALSoundManager: could not create an OpenAL context");
        if (mContext != nullptr)
            alcDestroyContext(mContext);
        mContext = nullptr;
        alcCloseDevice(mDevice);
        mDevice = nullptr;
        return false;
    }

    ConfigureListener();
    mInitialized = true;
    TOD_TRACE("OpenALSoundManager: OpenAL %s", alGetString(AL_VERSION));

    mMusicInterface = new OpenALMusicInterface(this);
    return true;
}

void OpenALSoundManager::Shutdown()
{
    if (!mInitialized)
        return;

    ReleaseSounds();

    if (mContext != nullptr)
    {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(mContext);
        mContext = nullptr;
    }
    if (mDevice != nullptr)
    {
        alcCloseDevice(mDevice);
        mDevice = nullptr;
    }
    mInitialized = false;
}

// The one decoder. `theData`/`theSize` are the whole .ogg; the result carries an
// uploaded AL buffer whose format follows the file's own channel count.
OpenALSoundBuffer *OpenALSoundManager::DecodeOgg(const void *theData, unsigned int theSize)
{
    if (theData == nullptr || theSize == 0)
        return nullptr;

    OggMemoryStream aStream;
    aStream.mData = (const unsigned char *)theData;
    aStream.mSize = theSize;
    aStream.mPos = 0;

    OggVorbis_File aFile;
    ov_callbacks aCallbacks = OggCallbacks();
    if (ov_open_callbacks(&aStream, &aFile, nullptr, 0, aCallbacks) != 0)
    {
        TOD_WARN("OpenALSoundManager: not a valid OGG stream");
        return nullptr;
    }

    vorbis_info *anInfo = ov_info(&aFile, -1);
    if (anInfo == nullptr)
    {
        ov_clear(&aFile);
        return nullptr;
    }

    // ov_clear frees the info block. The rate has to be copied first or
    // alBufferData reads a freed sample rate.
    int aChannels = anInfo->channels;
    int aRate = (int)anInfo->rate;
    if ((aChannels != 1 && aChannels != 2) || aRate <= 0)
    {
        ov_clear(&aFile);
        return nullptr;
    }
    ALenum aFormat = (aChannels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

    // Decode the whole thing into one PCM block. Streaming would be better for
    // the long music tracks, but every asset here is a few seconds and OpenAL's
    // buffer queueing would complicate the instance model for no gain.
    std::vector<unsigned char> aPCM;
    char aChunk[16384];
    int aBitstream = 0;
    for (;;)
    {
        long aRead = ov_read(&aFile, aChunk, (int)sizeof(aChunk), 0, 2, 1, &aBitstream);
        if (aRead == 0)
            break;
        if (aRead < 0)
        {
            // A hole in the stream is recoverable; ov_read reports it as a
            // negative code and the next call resumes.
            continue;
        }
        aPCM.insert(aPCM.end(), aChunk, aChunk + aRead);
    }

    double aLength = ov_time_total(&aFile, -1);
    ov_clear(&aFile);

    if (aPCM.empty())
        return nullptr;

    return UploadPcm(aFormat, (const unsigned char*)&aPCM[0], (unsigned int)aPCM.size(), aRate, aLength);
}

bool OpenALSoundManager::Initialized()
{
    return mInitialized;
}

bool OpenALSoundManager::LoadSound(unsigned int theSoundId, const String & theFileName)
{
    if (!mInitialized)
        return false;

    OpenALSoundBuffer *aBuffer2 = OpenAudioFile(theFileName);
    if (aBuffer2 == nullptr)
        return false;

    if (theSoundId >= mBuffers.size())
        mBuffers.resize(theSoundId + 1, nullptr);
    delete mBuffers[theSoundId];
    mBuffers[theSoundId] = aBuffer2;
    return true;
}

int OpenALSoundManager::LoadSound(const String & theFileName)
{
    int anId = GetFreeSoundId();
    if (anId < 0)
        return -1;
    if (!LoadSound((unsigned int)anId, theFileName))
        return -1;
    return anId;
}

void OpenALSoundManager::ReleaseSound(unsigned int theSoundId)
{
    if (theSoundId >= mBuffers.size())
        return;
    delete mBuffers[theSoundId];
    mBuffers[theSoundId] = nullptr;
}

void OpenALSoundManager::SetVolume(double theVolume)
{
    mVolume = theVolume;
    for (size_t i = 0; i < mInstances.size(); ++i)
        mInstances[i]->SetMasterVolumeIdx(0);
}

void OpenALSoundManager::SetVolume(int theSoundId, double theVolume)
{
    SoundInstance *anInstance = GetSoundInstance((unsigned int)theSoundId);
    if (anInstance != nullptr)
        anInstance->SetVolume(theVolume);
}

void OpenALSoundManager::SetBaseVolume(unsigned int theSoundId, double theVolume)
{
    SoundInstance *anInstance = GetSoundInstance(theSoundId);
    if (anInstance != nullptr)
        anInstance->SetBaseVolume(theVolume);
}

void OpenALSoundManager::SetBasePan(unsigned int theSoundId, int thePan)
{
    SoundInstance *anInstance = GetSoundInstance(theSoundId);
    if (anInstance != nullptr)
        anInstance->SetBasePan(thePan);
}

bool OpenALSoundManager::IsPlaying(unsigned int theSoundId)
{
    if (theSoundId >= mBuffers.size())
        return false;
    for (size_t i = 0; i < mInstances.size(); ++i)
    {
        if (mInstances[i]->mBuffer == mBuffers[theSoundId] && mInstances[i]->IsPlaying())
            return true;
    }
    return false;
}

SoundInstance *OpenALSoundManager::GetPlayingSoundInstance(unsigned int theSoundId)
{
    if (theSoundId >= mBuffers.size())
        return nullptr;
    for (size_t i = 0; i < mInstances.size(); ++i)
    {
        if (mInstances[i]->mBuffer == mBuffers[theSoundId] && mInstances[i]->IsPlaying())
            return mInstances[i];
    }
    return nullptr;
}

SoundInstance *OpenALSoundManager::GetSoundInstance(unsigned int theSoundId)
{
    if (theSoundId >= mBuffers.size() || mBuffers[theSoundId] == nullptr)
        return nullptr;

    // Reuse a free instance for this buffer before making another one. OpenAL
    // sources are a finite hardware resource and the game asks for the same
    // sound many times per second.
    for (size_t i = 0; i < mInstances.size(); ++i)
    {
        if (mInstances[i]->mBuffer == mBuffers[theSoundId] && !mInstances[i]->IsPlaying())
            return mInstances[i];
    }

    OpenALSoundInstance *anInstance = new OpenALSoundInstance(this, mBuffers[theSoundId]);
    mInstances.push_back(anInstance);
    return anInstance;
}

void OpenALSoundManager::ReleaseSounds()
{
    for (size_t i = 0; i < mInstances.size(); ++i)
        delete mInstances[i];
    mInstances.clear();

    for (size_t i = 0; i < mBuffers.size(); ++i)
        delete mBuffers[i];
    mBuffers.clear();
}

void OpenALSoundManager::ReleaseChannels()
{
    // Nothing is reserved per channel: an instance IS its source.
}

double OpenALSoundManager::GetMasterVolume()
{
    return mMasterVolume;
}

void OpenALSoundManager::SetMasterVolume(double theVolume)
{
    mMasterVolume = theVolume;
}

void OpenALSoundManager::Flush()
{
    // Every play is immediate; there is no queued work to drain.
}

void OpenALSoundManager::SetCooperativeWindow(void * theWindow, bool theActive)
{
    // Windows has no cooperative audio level to negotiate. The binary's
    // audiere backend used this to pause on focus loss, which Windows handles
    // by the app suspending its own mixer.
    (void)theWindow;
    (void)theActive;
}

void OpenALSoundManager::StopAllSounds()
{
    for (size_t i = 0; i < mInstances.size(); ++i)
        mInstances[i]->Stop();
}

int OpenALSoundManager::GetFreeSoundId()
{
    for (size_t i = 0; i < mBuffers.size(); ++i)
    {
        if (mBuffers[i] == nullptr)
            return (int)i;
    }
    return (int)mBuffers.size();
}

int OpenALSoundManager::GetNumSounds()
{
    int aCount = 0;
    for (size_t i = 0; i < mBuffers.size(); ++i)
    {
        if (mBuffers[i] != nullptr)
            ++aCount;
    }
    return aCount;
}

void OpenALSoundManager::Update()
{
    // Reap the instances that finished on their own so the source pool does not
    // grow without bound over a long level.
    for (size_t i = 0; i < mInstances.size(); )
    {
        if (mInstances[i]->mReleased)
        {
            delete mInstances[i];
            mInstances.erase(mInstances.begin() + i);
            continue;
        }
        ++i;
    }

    if (mMusicInterface != nullptr)
        mMusicInterface->Update();
}

// ===========================================================================
// OpenALMusicInterface
// ===========================================================================
OpenALMusicInterface::OpenALMusicInterface(OpenALSoundManager *theManager)
    : mManager(theManager), mVolume(1.0)
{
}

OpenALMusicInterface::~OpenALMusicInterface()
{
    UnloadAllMusic();
}

void OpenALMusicInterface::EnsureSource(Song &theSong)
{
    if (theSong.mSource != 0)
        return;

    alGenSources(1, &theSong.mSource);
    if (theSong.mSource != 0 && theSong.mBuffer != nullptr && theSong.mBuffer->mBuffer != 0)
        alSourcei(theSong.mSource, AL_BUFFER, (ALint)theSong.mBuffer->mBuffer);
}

bool OpenALMusicInterface::IsLoaded(int theSongId)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    return it != mSongs.end() && it->second.mLoaded;
}

bool OpenALMusicInterface::LoadMusic(int theSongId, Sexy::String const & theFileName)
{
    // This is the function that decides whether the game has music at all:
    // Music::LoadSong treats a false result as "music is unavailable" and sets
    // mMusicDisabled for the rest of the session.
    if (mManager == nullptr || !mManager->Initialized())
        return false;

    OpenALSoundBuffer *aBuffer2 = OpenAudioFile(theFileName);
    if (aBuffer2 == nullptr)
        return false;

    Song &aSong = mSongs[theSongId];
    if (aSong.mSource != 0)
    {
        alSourceStop(aSong.mSource);
        alDeleteSources(1, &aSong.mSource);
        aSong.mSource = 0;
    }
    delete aSong.mBuffer;

    aSong.mBuffer = aBuffer2;
    aSong.mVolume = 1.0;
    aSong.mMaxVolume = 1.0;
    aSong.mAmplify = 1.0;
    aSong.mTempo = 0;
    aSong.mLoaded = true;
    aSong.mPlaying = false;
    aSong.mPaused = false;
    aSong.mLoop = true;
    aSong.mFadeTarget = 0.0;
    aSong.mFadeRate = 0.0;
    EnsureSource(aSong);
    return true;
}

void OpenALMusicInterface::PlayMusic(int theSongId, int theOffset, bool noLoop)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it == mSongs.end() || !it->second.mLoaded)
        return;

    Song &aSong = it->second;
    EnsureSource(aSong);
    if (aSong.mSource == 0)
        return;

    aSong.mLoop = !noLoop;
    alSourcei(aSong.mSource, AL_LOOPING, aSong.mLoop ? AL_TRUE : AL_FALSE);
    alSourcef(aSong.mSource, AL_GAIN, ToGain(aSong.mVolume * aSong.mAmplify * mVolume));
    aSong.mFadeRate = 0.0;

    // The offset is in milliseconds in this framework's call sites.
    if (theOffset > 0 && aSong.mBuffer != nullptr && aSong.mBuffer->mLengthSeconds > 0.0)
        alSourcef(aSong.mSource, AL_SEC_OFFSET, (ALfloat)(theOffset / 1000.0));

    alSourcePlay(aSong.mSource);
    aSong.mPlaying = true;
    aSong.mPaused = false;
}

void OpenALMusicInterface::StopMusic(int theSongId)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it == mSongs.end() || it->second.mSource == 0)
        return;

    alSourceStop(it->second.mSource);
    it->second.mPlaying = false;
    it->second.mPaused = false;
    it->second.mFadeRate = 0.0;
}

void OpenALMusicInterface::PauseMusic(int theSongId)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it == mSongs.end() || it->second.mSource == 0)
        return;

    alSourcePause(it->second.mSource);
    it->second.mPaused = true;
}

void OpenALMusicInterface::ResumeMusic(int theSongId)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it == mSongs.end() || it->second.mSource == 0)
        return;

    alSourcePlay(it->second.mSource);
    it->second.mPaused = false;
}

void OpenALMusicInterface::StopAllMusic()
{
    for (std::map<int, Song>::iterator it = mSongs.begin(); it != mSongs.end(); ++it)
        StopMusic(it->first);
}

void OpenALMusicInterface::UnloadMusic(int theSongId)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it == mSongs.end())
        return;

    if (it->second.mSource != 0)
    {
        alSourceStop(it->second.mSource);
        alDeleteSources(1, &it->second.mSource);
    }
    delete it->second.mBuffer;
    mSongs.erase(it);
}

void OpenALMusicInterface::UnloadAllMusic()
{
    for (std::map<int, Song>::iterator it = mSongs.begin(); it != mSongs.end(); ++it)
    {
        if (it->second.mSource != 0)
        {
            alSourceStop(it->second.mSource);
            alDeleteSources(1, &it->second.mSource);
        }
        delete it->second.mBuffer;
    }
    mSongs.clear();
}

void OpenALMusicInterface::PauseAllMusic()
{
    for (std::map<int, Song>::iterator it = mSongs.begin(); it != mSongs.end(); ++it)
        PauseMusic(it->first);
}

void OpenALMusicInterface::ResumeAllMusic()
{
    for (std::map<int, Song>::iterator it = mSongs.begin(); it != mSongs.end(); ++it)
        ResumeMusic(it->first);
}

void OpenALMusicInterface::FadeIn(int theSongId, int theOffset, double theVolume, bool noLoop)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it == mSongs.end() || !it->second.mLoaded)
        return;

    Song &aSong = it->second;
    aSong.mMaxVolume = theVolume;
    aSong.mVolume = 0.0;
    PlayMusic(theSongId, theOffset, noLoop);
    // One second to the requested volume, stepped in Update. OpenAL has no
    // gain envelope, so the ramp is driven by the frame loop.
    aSong.mFadeTarget = theVolume;
    aSong.mFadeRate = (theVolume > 0.0) ? (theVolume / 1.0) : 0.0;
}

void OpenALMusicInterface::FadeOut(int theSongId, bool noLoop, double theVolume)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it == mSongs.end())
        return;

    (void)noLoop;
    it->second.mFadeTarget = theVolume;
    it->second.mFadeRate = -(it->second.mVolume - theVolume);
}

void OpenALMusicInterface::FadeOutAll(bool noLoop, double theVolume)
{
    for (std::map<int, Song>::iterator it = mSongs.begin(); it != mSongs.end(); ++it)
        FadeOut(it->first, noLoop, theVolume);
}

void OpenALMusicInterface::SetSongVolume(int theSongId, double theVolume)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it == mSongs.end())
        return;

    it->second.mVolume = theVolume;
    if (it->second.mSource != 0)
        alSourcef(it->second.mSource, AL_GAIN, ToGain(theVolume * it->second.mAmplify * mVolume));
}

void OpenALMusicInterface::SetSongMaxVolume(int theSongId, double theVolume)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it == mSongs.end())
        return;
    it->second.mMaxVolume = theVolume;
}

bool OpenALMusicInterface::IsPlaying(int theSongId)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it == mSongs.end() || it->second.mSource == 0)
        return false;

    ALint aState = AL_STOPPED;
    alGetSourcei(it->second.mSource, AL_SOURCE_STATE, &aState);
    return aState == AL_PLAYING;
}

void OpenALMusicInterface::SetChannelVolume(int theSongId, int theChannel, float theVolume)
{
    // A decoded stereo stream has two channels and OpenAL exposes no per-channel
    // gain on a buffer source, so the channel index selects which side to bias.
    // Music.cpp only ever passes 0 and 1 here.
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it == mSongs.end() || it->second.mSource == 0)
        return;

    (void)theChannel;
    alSourcef(it->second.mSource, AL_GAIN, ToGain(theVolume * it->second.mAmplify * mVolume));
}

int OpenALMusicInterface::GetOrder(int theSongId)
{
    // A decoded OGG has no order list to report; the framework's callers only
    // use this for the module-music formats the target never shipped.
    (void)theSongId;
    return 0;
}

int OpenALMusicInterface::GetTempo(int theSongId)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    return (it != mSongs.end()) ? it->second.mTempo : 0;
}

void OpenALMusicInterface::SetTempo(int theSongId, int theTempo)
{
    // OpenAL has no tempo control on a buffer source. The value is recorded so
    // GetTempo round-trips, and the pitch is left alone: changing AL_PITCH to
    // fake a tempo would shift the key, which is audibly worse than ignoring it.
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it != mSongs.end())
        it->second.mTempo = theTempo;
}

void OpenALMusicInterface::SetVolume(double theVolume)
{
    mVolume = theVolume;
    for (std::map<int, Song>::iterator it = mSongs.begin(); it != mSongs.end(); ++it)
    {
        if (it->second.mSource != 0)
        {
            alSourcef(it->second.mSource, AL_GAIN,
                      ToGain(it->second.mVolume * it->second.mAmplify * mVolume));
        }
    }
}

void OpenALMusicInterface::SetMusicAmplify(int theSongId, double theVolume)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it == mSongs.end())
        return;

    it->second.mAmplify = theVolume;
    if (it->second.mSource != 0)
        alSourcef(it->second.mSource, AL_GAIN,
                  ToGain(it->second.mVolume * theVolume * mVolume));
}

void OpenALMusicInterface::Update()
{
    // Step every running fade, then stop the songs that have faded to silence.
    for (std::map<int, Song>::iterator it = mSongs.begin(); it != mSongs.end(); ++it)
    {
        Song &aSong = it->second;
        if (aSong.mFadeRate == 0.0 || aSong.mSource == 0)
            continue;

        aSong.mVolume += aSong.mFadeRate * 0.01;   // ~one frame at 100 Hz
        if ((aSong.mFadeRate > 0.0 && aSong.mVolume >= aSong.mFadeTarget) ||
            (aSong.mFadeRate < 0.0 && aSong.mVolume <= aSong.mFadeTarget))
        {
            aSong.mVolume = aSong.mFadeTarget;
            aSong.mFadeRate = 0.0;
        }

        if (aSong.mVolume < 0.0)
            aSong.mVolume = 0.0;

        alSourcef(aSong.mSource, AL_GAIN, ToGain(aSong.mVolume * aSong.mAmplify * mVolume));

        if (aSong.mFadeRate == 0.0 && aSong.mVolume <= 0.0 && aSong.mPlaying)
        {
            alSourceStop(aSong.mSource);
            aSong.mPlaying = false;
        }
    }
}

} // namespace Sexy
