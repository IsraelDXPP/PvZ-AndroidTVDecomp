#include "SDLAudioBackend.h"

#include "LogManager.h"
#include "SDLFile.h"

#include <libopenmpt/libopenmpt.h>
#include <libopenmpt/libopenmpt_ext.h>
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

    return 0;
}

long OggTell(void *theSource)
{
    return (long)((OggMemoryStream *)theSource)->mPos;
}

ov_callbacks OggCallbacks()
{
    ov_callbacks aCallbacks;
    aCallbacks.read_func = OggRead;
    aCallbacks.seek_func = OggSeek;
    aCallbacks.close_func = OggClose;
    aCallbacks.tell_func = OggTell;
    return aCallbacks;
}

SDL_AudioStream* CreateStream(SDL_AudioDeviceID device, int rate)
{
    SDL_AudioSpec input{SDL_AUDIO_F32, 2, rate};
    SDL_AudioSpec output{};
    if (!device || !SDL_GetAudioDeviceFormat(device, &output, nullptr)) return nullptr;
    SDL_AudioStream* stream = SDL_CreateAudioStream(&input, &output);
    if (stream && !SDL_BindAudioStream(device, stream))
    { SDL_DestroyAudioStream(stream); return nullptr; }
    return stream;
}

bool StreamEmpty(SDL_AudioStream* stream)
{
    return !stream || (SDL_GetAudioStreamQueued(stream) == 0 && SDL_GetAudioStreamAvailable(stream) == 0);
}

bool QueuePcm(SDL_AudioStream* stream, const SDLSoundBuffer& buffer, size_t& cursor,
              bool loop, double pan)
{
    size_t totalFrames = buffer.mSamples.size() / buffer.mChannels;
    if (!totalFrames || (!loop && cursor >= totalFrames)) return false;
    std::vector<float> samples;
    samples.reserve(8192);
    float left = float(1.0 - std::max(0.0, pan));
    float right = float(1.0 + std::min(0.0, pan));
    for (size_t frame = 0; frame < 4096; ++frame)
    {
        if (cursor == totalFrames) { if (!loop) break; cursor = 0; }
        size_t offset = cursor * buffer.mChannels;
        samples.push_back(buffer.mSamples[offset] / 32768.0f * left);
        samples.push_back(buffer.mSamples[offset + (buffer.mChannels == 2 ? 1 : 0)] / 32768.0f * right);
        ++cursor;
    }
    if (!SDL_PutAudioStreamData(stream, samples.data(), int(samples.size() * sizeof(float)))) return false;
    if (!loop && cursor == totalFrames) SDL_FlushAudioStream(stream);
    return true;
}

float ToGain(double theVolume)
{
    if (theVolume < 0.0)
        return 0.0f;
    if (theVolume > 1.0)
        return 1.0f;
    return float(theVolume);
}

bool ExtensionMatches(const std::string& theExt, const char* theName)
{
    if (theExt.empty())
        return true;
    return SDL_strcasecmp(theExt.c_str(), theName) == 0;
}

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

    const char* aExtensions[] = { ".ogg", ".wav", ".xm", ".it", ".s3m", ".mod", ".mo3" };
    const int aExtensionCount = (int)(sizeof(aExtensions) / sizeof(aExtensions[0]));
    for (int aKind = 0; aKind < aExtensionCount; ++aKind)
    {
        if (!ExtensionMatches(aGiven, aExtensions[aKind]))
            continue;
        std::string aTry = aBase + aExtensions[aKind];
        if (!SDLFile::FileExists(aTry))
            continue;
        if (!SDLFile::ReadFile(aTry, theBytes) || theBytes.empty())
            continue;
        theExt = aExtensions[aKind];
        return true;
    }
    return false;
}

namespace
{

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

SDLSoundBuffer* UploadPcm(int channels, const unsigned char* pcm,
                        unsigned int bytes, int rate, double length)
{
    if (!pcm || !bytes || rate <= 0 || (channels != 1 && channels != 2) || bytes % (2 * channels)) return nullptr;
    auto* result = new SDLSoundBuffer;
    result->mChannels = channels; result->mRate = rate;
    result->mLengthSeconds = length > 0 ? length : double(bytes / (2 * channels)) / rate;
    result->mSamples.resize(bytes / 2);
    for (size_t index = 0; index < result->mSamples.size(); ++index)
        result->mSamples[index] = int16_t(ReadLe16(pcm + index * 2));
    return result;
}

SDLSoundBuffer* DecodeWav(const void* theData, unsigned int theSize)
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

    double aLength = (double)(aUploadBytes / 2) / (double)(aRate * aChannels);
    return UploadPcm(aChannels, aUpload, aUploadBytes, aRate, aLength);
}

SDLSoundBuffer* OpenAudioFile(const String& theFileName)
{
    std::vector<unsigned char> aBytes;
    std::string aExt;
    if (!LoadAudioBytes(theFileName, aBytes, aExt))
    {
        TOD_WARN("SDLSoundManager: cannot read '%s'", theFileName.c_str());
        return nullptr;
    }

    SDLSoundBuffer* aDecoded = nullptr;
    if (SDL_strcasecmp(aExt.c_str(), ".wav") == 0)
        aDecoded = DecodeWav(aBytes.data(), (unsigned int)aBytes.size());
    else
        aDecoded = SDLSoundManager::DecodeOgg(aBytes.data(), (unsigned int)aBytes.size());
    if (aDecoded == nullptr)
        TOD_WARN("SDLSoundManager: cannot decode '%s'", theFileName.c_str());
    return aDecoded;
}

}

Sexy::SoundManager::~SoundManager()
{
}

SDLSoundBuffer::SDLSoundBuffer()
    : mLengthSeconds(0.0), mBaseVolume(1.0), mBasePan(0) {}

SDLSoundBuffer::~SDLSoundBuffer() = default;

SDLSoundInstance::SDLSoundInstance(SDLSoundManager* manager, const std::shared_ptr<SDLSoundBuffer>& buffer)
    : mManager(manager), mStream(nullptr), mBuffer(buffer), mPlaying(false), mReleased(false),
      mAutoRelease(false), mBaseVolume(1.0), mVolume(1.0), mPan(0), mBaseRate(1.0)
{
    if (manager && buffer) mStream = CreateStream(manager->mDevice, buffer->mRate);
}

SDLSoundInstance::~SDLSoundInstance() { if (mStream) SDL_DestroyAudioStream(mStream); }

void SDLSoundInstance::Release()
{
    mReleased = true;
    Stop();
}

void SDLSoundInstance::SetBaseVolume(double theVolume)
{
    mBaseVolume = theVolume;
    ApplyState();
}

void SDLSoundInstance::SetBasePan(int pan) { mPan = pan; ApplyState(); }

void SDLSoundInstance::SetBaseRate(double theRate)
{
    mBaseRate = theRate;
    ApplyState();
}

void SDLSoundInstance::AdjustPitch(double thePitch)
{
    // AdjustPitch uses semitones, as in AudiereSoundInstance @0x4416d0.
    mPitchRatio = std::pow(1.0594630943592953, thePitch);
    ApplyState();
}

void SDLSoundInstance::SetVolume(double theVolume)
{
    mVolume = theVolume;
    ApplyState();
}

void SDLSoundInstance::SetMasterVolumeIdx(int theIndex)
{

    (void)theIndex;
    ApplyState();
}

void SDLSoundInstance::SetPan(int thePan)
{
    SetBasePan(thePan);
}

void SDLSoundInstance::Play(bool loop, bool autoRelease)
{
    if (!mStream || mReleased) return;
    SDL_ClearAudioStream(mStream);
    mCursor = 0; mLoop = loop; mAutoRelease = autoRelease; mPlaying = true;
    ApplyState(); IsPlaying();
}

void SDLSoundInstance::Stop() { if (mStream) SDL_ClearAudioStream(mStream); mPlaying = false; }

bool SDLSoundInstance::IsPlaying()
{
    if (!mStream || !mPlaying) return false;
    double pan = mPan > 1 || mPan < -1 ? mPan / 10000.0 : double(mPan);
    pan = std::clamp(pan, -1.0, 1.0);
    while (SDL_GetAudioStreamQueued(mStream) < 8192 * int(sizeof(float)) &&
           QueuePcm(mStream, *mBuffer, mCursor, mLoop, pan)) {}
    if (!mLoop && mCursor >= mBuffer->mSamples.size() / mBuffer->mChannels && StreamEmpty(mStream)) mPlaying = false;
    return mPlaying;
}

bool SDLSoundInstance::IsReleased()
{
    return mReleased || (mAutoRelease && !IsPlaying());
}

void SDLSoundInstance::ApplyState()
{
    if (!mStream) return;
    double gain = mVolume * mBaseVolume;
    if (mManager) gain *= mManager->mVolume * mManager->mMasterVolume;
    SDL_SetAudioStreamGain(mStream, ToGain(gain));
    SDL_SetAudioStreamFrequencyRatio(mStream, float(std::clamp(mBaseRate * mPitchRatio, 0.5, 2.0)));
}

SDLSoundManager::SDLSoundManager()
    : mDevice(0), mInitialized(false),
      mVolume(1.0), mMasterVolume(1.0), mMusicInterface(nullptr)
{
}

SDLSoundManager::~SDLSoundManager()
{

    delete mMusicInterface;
    mMusicInterface = nullptr;
    Shutdown();
}

bool SDLSoundManager::Init()
{
    if (mInitialized) return true;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return false;
    mDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!mDevice) { TOD_WARN("SDL audio unavailable: %s", SDL_GetError()); SDL_QuitSubSystem(SDL_INIT_AUDIO); return false; }
    SDL_ResumeAudioDevice(mDevice);
    mInitialized = true;
    mMusicInterface = new SDLMusicInterface(this);
    return true;
}

void SDLSoundManager::Shutdown()
{
    ReleaseSounds();
    if (mDevice) { SDL_CloseAudioDevice(mDevice); SDL_QuitSubSystem(SDL_INIT_AUDIO); }
    mDevice = 0; mInitialized = false;
}

SDLSoundBuffer *SDLSoundManager::DecodeOgg(const void *theData, unsigned int theSize)
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
        TOD_WARN("SDLSoundManager: not a valid OGG stream");
        return nullptr;
    }

    vorbis_info *anInfo = ov_info(&aFile, -1);
    if (anInfo == nullptr)
    {
        ov_clear(&aFile);
        return nullptr;
    }

    int aChannels = anInfo->channels;
    int aRate = (int)anInfo->rate;
    if ((aChannels != 1 && aChannels != 2) || aRate <= 0)
    {
        ov_clear(&aFile);
        return nullptr;
    }

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

            continue;
        }
        aPCM.insert(aPCM.end(), aChunk, aChunk + aRead);
    }

    double aLength = ov_time_total(&aFile, -1);
    ov_clear(&aFile);

    if (aPCM.empty())
        return nullptr;

    return UploadPcm(aChannels, (const unsigned char*)&aPCM[0], (unsigned int)aPCM.size(), aRate, aLength);
}

bool SDLSoundManager::Initialized()
{
    return mInitialized;
}

bool SDLSoundManager::LoadSound(unsigned int theSoundId, const String & theFileName)
{
    if (!mInitialized)
        return false;

    SDLSoundBuffer *aBuffer2 = OpenAudioFile(theFileName);
    if (aBuffer2 == nullptr)
        return false;

    if (theSoundId >= mBuffers.size())
        mBuffers.resize(theSoundId + 1, nullptr);
    mBuffers[theSoundId].reset(aBuffer2);
    return true;
}

int SDLSoundManager::LoadSound(const String & theFileName)
{
    int anId = GetFreeSoundId();
    if (anId < 0)
        return -1;
    if (!LoadSound((unsigned int)anId, theFileName))
        return -1;
    return anId;
}

void SDLSoundManager::ReleaseSound(unsigned int theSoundId)
{
    if (theSoundId >= mBuffers.size())
        return;
    mBuffers[theSoundId].reset();
}

void SDLSoundManager::SetVolume(double theVolume)
{
    mVolume = theVolume;
    for (size_t i = 0; i < mInstances.size(); ++i)
        mInstances[i]->SetMasterVolumeIdx(0);
}

void SDLSoundManager::SetVolume(int theSoundId, double theVolume)
{
    SetBaseVolume((unsigned int)theSoundId, theVolume);
}

void SDLSoundManager::SetBaseVolume(unsigned int theSoundId, double theVolume)
{
    if (theSoundId < mBuffers.size() && mBuffers[theSoundId] != nullptr)
        mBuffers[theSoundId]->mBaseVolume = theVolume;
}

void SDLSoundManager::SetBasePan(unsigned int theSoundId, int thePan)
{
    if (theSoundId < mBuffers.size() && mBuffers[theSoundId] != nullptr)
        mBuffers[theSoundId]->mBasePan = thePan;
}

bool SDLSoundManager::IsPlaying(unsigned int theSoundId)
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

SoundInstance *SDLSoundManager::GetPlayingSoundInstance(unsigned int theSoundId)
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

SoundInstance *SDLSoundManager::GetSoundInstance(unsigned int theSoundId)
{
    if (theSoundId >= mBuffers.size() || mBuffers[theSoundId] == nullptr)
        return nullptr;

    SDLSoundInstance *anInstance = new SDLSoundInstance(this, mBuffers[theSoundId]);
    if (anInstance->mStream == nullptr)
    {
        delete anInstance;
        return nullptr;
    }
    anInstance->SetBaseVolume(mBuffers[theSoundId]->mBaseVolume);
    anInstance->SetBasePan(mBuffers[theSoundId]->mBasePan);
    mInstances.push_back(anInstance);
    return anInstance;
}

void SDLSoundManager::ReleaseSounds()
{
    for (size_t i = 0; i < mInstances.size(); ++i)
        delete mInstances[i];
    mInstances.clear();

    mBuffers.clear();
}

void SDLSoundManager::ReleaseChannels()
{

}

double SDLSoundManager::GetMasterVolume()
{
    return mMasterVolume;
}

void SDLSoundManager::SetMasterVolume(double theVolume)
{
    mMasterVolume = theVolume;
    for (SDLSoundInstance* anInstance : mInstances)
        anInstance->ApplyState();
}

void SDLSoundManager::Flush()
{

}

void SDLSoundManager::SetCooperativeWindow(void * theWindow, bool theActive)
{

    (void)theWindow;
    (void)theActive;
}

void SDLSoundManager::StopAllSounds()
{
    for (size_t i = 0; i < mInstances.size(); ++i)
        mInstances[i]->Stop();
}

int SDLSoundManager::GetFreeSoundId()
{
    for (size_t i = 0; i < mBuffers.size(); ++i)
    {
        if (mBuffers[i] == nullptr)
            return (int)i;
    }
    return (int)mBuffers.size();
}

int SDLSoundManager::GetNumSounds()
{
    int aCount = 0;
    for (size_t i = 0; i < mBuffers.size(); ++i)
    {
        if (mBuffers[i] != nullptr)
            ++aCount;
    }
    return aCount;
}

void SDLSoundManager::Update()
{

    for (size_t i = 0; i < mInstances.size(); )
    {
        mInstances[i]->IsPlaying();
        if (mInstances[i]->IsReleased())
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

namespace
{

const int kModuleStreamRate = 44100;
const int kModuleSliceFrames = 4096;
const int kModuleQueueDepth = 3;

void ClearSong(SDLMusicInterface::Song &theSong)
{
    theSong.mBuffer = nullptr;
    theSong.mStream = nullptr;
    theSong.mCursor = 0;
    theSong.mFinished = false;
    theSong.mVolume = 1.0;
    theSong.mMaxVolume = 1.0;
    theSong.mAmplify = 1.0;
    theSong.mTempo = 0;
    theSong.mLoaded = false;
    theSong.mPlaying = false;
    theSong.mPaused = false;
    theSong.mLoop = true;
    theSong.mFadeTarget = 0.0;
    theSong.mFadeRate = 0.0;
    theSong.mModule = nullptr;
    theSong.mModuleBase = nullptr;
    theSong.mStreamRate = kModuleStreamRate;
}

}

SDLMusicInterface::SDLMusicInterface(SDLSoundManager *theManager)
    : mManager(theManager), mVolume(1.0)
{
}

SDLMusicInterface::~SDLMusicInterface()
{
    UnloadAllMusic();
}

void SDLMusicInterface::ReleaseModule(Song& song)
{
    if (song.mModule) openmpt_module_ext_destroy(song.mModule);
    song.mModule = nullptr; song.mModuleBase = nullptr;
    song.mChannelGain.clear();
}

bool SDLMusicInterface::QueueModuleSlice(Song& song)
{
    std::vector<float> frames(size_t(kModuleSliceFrames) * 2);
    size_t rendered = openmpt_module_read_interleaved_float_stereo(
        song.mModuleBase, song.mStreamRate, kModuleSliceFrames, frames.data());
    if (!rendered) { SDL_FlushAudioStream(song.mStream); return false; }
    return SDL_PutAudioStreamData(song.mStream, frames.data(), int(rendered * 2 * sizeof(float)));
}

void SDLMusicInterface::RefillModule(Song& song)
{
    if (!song.mStream || !song.mPlaying || song.mPaused) return;
    while (!song.mFinished && SDL_GetAudioStreamQueued(song.mStream) < kModuleSliceFrames * kModuleQueueDepth * 2 * int(sizeof(float)))
    {
        bool queued = song.mModuleBase ? QueueModuleSlice(song) : QueuePcm(song.mStream, *song.mBuffer, song.mCursor, song.mLoop, 0);
        if (!queued) song.mFinished = true;
    }
    if (song.mFinished && StreamEmpty(song.mStream)) song.mPlaying = false;
}

void SDLMusicInterface::EnsureStream(Song& song)
{
    if (!song.mStream) song.mStream = CreateStream(mManager->mDevice, song.mBuffer ? song.mBuffer->mRate : song.mStreamRate);
}

void SDLMusicInterface::ApplySongGain(Song& song)
{ if (song.mStream) SDL_SetAudioStreamGain(song.mStream, ToGain(song.mVolume * song.mAmplify * mVolume)); }

bool SDLMusicInterface::IsLoaded(int theSongId)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    return it != mSongs.end() && it->second.mLoaded;
}

bool SDLMusicInterface::LoadMusic(int theSongId, Sexy::String const & theFileName)
{

    if (mManager == nullptr || !mManager->Initialized())
        return false;

    std::vector<unsigned char> aBytes;
    std::string aExt;
    if (!LoadAudioBytes(theFileName, aBytes, aExt))
    {
        TOD_WARN("SDLMusicInterface: cannot read '%s'", theFileName.c_str());
        return false;
    }

    Song &aSong = mSongs[theSongId];
    if (aSong.mStream) SDL_DestroyAudioStream(aSong.mStream);
    ReleaseModule(aSong);
    delete aSong.mBuffer;
    aSong.mBuffer = nullptr;
    ClearSong(aSong);

    bool aIsModule = SDL_strcasecmp(aExt.c_str(), ".xm") == 0
        || SDL_strcasecmp(aExt.c_str(), ".it") == 0
        || SDL_strcasecmp(aExt.c_str(), ".s3m") == 0
        || SDL_strcasecmp(aExt.c_str(), ".mod") == 0
        || SDL_strcasecmp(aExt.c_str(), ".mo3") == 0;
    if (aIsModule)
    {
        int aError = OPENMPT_ERROR_OK;
        aSong.mModule = openmpt_module_ext_create_from_memory(
            aBytes.data(), aBytes.size(), nullptr, nullptr, nullptr, nullptr,
            &aError, nullptr, nullptr);
        if (aSong.mModule == nullptr)
        {
            TOD_WARN("SDLMusicInterface: cannot load module '%s' (%d)",
                     theFileName.c_str(), aError);
            return false;
        }
        aSong.mModuleBase = openmpt_module_ext_get_module(aSong.mModule);
        openmpt_module_set_repeat_count(aSong.mModuleBase, -1);
        int aChannels = openmpt_module_get_num_channels(aSong.mModuleBase);
        if (aChannels < 0)
            aChannels = 0;
        aSong.mChannelGain.assign((size_t)aChannels, 1.0f);
        aSong.mStreamRate = kModuleStreamRate;
        aSong.mLoaded = true;
        EnsureStream(aSong);
        return aSong.mStream != nullptr;
    }

    SDLSoundBuffer *aBuffer2 = nullptr;
    if (SDL_strcasecmp(aExt.c_str(), ".wav") == 0)
        aBuffer2 = DecodeWav(aBytes.data(), (unsigned int)aBytes.size());
    else
        aBuffer2 = SDLSoundManager::DecodeOgg(aBytes.data(), (unsigned int)aBytes.size());
    if (aBuffer2 == nullptr)
    {
        TOD_WARN("SDLMusicInterface: cannot load '%s'", theFileName.c_str());
        return false;
    }

    aSong.mBuffer = aBuffer2;
    aSong.mLoaded = true;
    EnsureStream(aSong);
    return aSong.mStream != nullptr;
}

void SDLMusicInterface::PlayMusic(int id, int offset, bool noLoop)
{
    auto found = mSongs.find(id);
    if (found == mSongs.end() || !found->second.mLoaded) return;
    Song& song = found->second;
    EnsureStream(song); if (!song.mStream) return;
    SDL_ClearAudioStream(song.mStream);
    // SDL streams are linked into their device's binding list. Play can be
    // called again on a running song; bind only after Pause has unbound it.
    if (SDL_GetAudioStreamDevice(song.mStream) == 0 && !SDL_BindAudioStream(mManager->mDevice, song.mStream))
        return;
    song.mLoop = !noLoop; song.mFadeRate = 0; song.mFinished = false;
    song.mPlaying = true; song.mPaused = false;
    if (song.mModuleBase)
    {
        openmpt_module_set_repeat_count(song.mModuleBase, song.mLoop ? -1 : 0);
        if (offset >= 0) openmpt_module_set_position_order_row(song.mModuleBase, offset, 0);
    }
    else if (song.mBuffer)
    {
        if (offset >= 0) song.mCursor = std::min(song.mBuffer->mSamples.size() / song.mBuffer->mChannels,
            size_t(double(offset) * song.mBuffer->mRate / 1000));
    }
    ApplySongGain(song); RefillModule(song);
}

void SDLMusicInterface::StopMusic(int id)
{
    auto found = mSongs.find(id); if (found == mSongs.end()) return;
    Song& song = found->second;
    if (song.mStream) SDL_ClearAudioStream(song.mStream);
    song.mPlaying = false; song.mPaused = false; song.mFadeRate = 0;
}

void SDLMusicInterface::PauseMusic(int id)
{
    auto found = mSongs.find(id);
    if (found == mSongs.end() || !found->second.mStream || !found->second.mPlaying) return;
    SDL_UnbindAudioStream(found->second.mStream); found->second.mPaused = true;
}

void SDLMusicInterface::ResumeMusic(int id)
{
    auto found = mSongs.find(id);
    if (found == mSongs.end() || !found->second.mStream || !found->second.mPaused) return;
    if (SDL_GetAudioStreamDevice(found->second.mStream) == 0 &&
        !SDL_BindAudioStream(mManager->mDevice, found->second.mStream)) return;
    found->second.mPaused = false;
}

void SDLMusicInterface::StopAllMusic()
{
    for (std::map<int, Song>::iterator it = mSongs.begin(); it != mSongs.end(); ++it)
        StopMusic(it->first);
}

void SDLMusicInterface::UnloadMusic(int id)
{
    auto found = mSongs.find(id); if (found == mSongs.end()) return;
    if (found->second.mStream) SDL_DestroyAudioStream(found->second.mStream);
    ReleaseModule(found->second); delete found->second.mBuffer; mSongs.erase(found);
}

void SDLMusicInterface::UnloadAllMusic() { while (!mSongs.empty()) UnloadMusic(mSongs.begin()->first); }

void SDLMusicInterface::PauseAllMusic()
{
    for (std::map<int, Song>::iterator it = mSongs.begin(); it != mSongs.end(); ++it)
        PauseMusic(it->first);
}

void SDLMusicInterface::ResumeAllMusic()
{
    for (std::map<int, Song>::iterator it = mSongs.begin(); it != mSongs.end(); ++it)
        ResumeMusic(it->first);
}

void SDLMusicInterface::FadeIn(int theSongId, int theOffset, double theVolume, bool noLoop)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it == mSongs.end() || !it->second.mLoaded)
        return;

    Song &aSong = it->second;
    aSong.mMaxVolume = theVolume;
    aSong.mVolume = 0.0;
    PlayMusic(theSongId, theOffset, noLoop);

    aSong.mFadeTarget = theVolume;
    aSong.mFadeRate = (theVolume > 0.0) ? (theVolume / 1.0) : 0.0;
}

void SDLMusicInterface::FadeOut(int theSongId, bool noLoop, double theVolume)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it == mSongs.end())
        return;

    (void)noLoop;
    it->second.mFadeTarget = theVolume;
    it->second.mFadeRate = -(it->second.mVolume - theVolume);
}

void SDLMusicInterface::FadeOutAll(bool noLoop, double theVolume)
{
    for (std::map<int, Song>::iterator it = mSongs.begin(); it != mSongs.end(); ++it)
        FadeOut(it->first, noLoop, theVolume);
}

void SDLMusicInterface::SetSongVolume(int id, double volume)
{ auto found = mSongs.find(id); if (found != mSongs.end()) { found->second.mVolume = volume; ApplySongGain(found->second); } }

void SDLMusicInterface::SetSongMaxVolume(int theSongId, double theVolume)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it == mSongs.end())
        return;
    it->second.mMaxVolume = theVolume;
}

bool SDLMusicInterface::IsPlaying(int id)
{ auto found = mSongs.find(id); return found != mSongs.end() && found->second.mPlaying && !found->second.mPaused; }

void SDLMusicInterface::SetChannelVolume(int theSongId, int theChannel, float theVolume)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it == mSongs.end())
        return;

    Song &aSong = it->second;
    if (aSong.mModule == nullptr)
        return;
    if (theChannel < 0 || theChannel >= (int)aSong.mChannelGain.size())
        return;

    aSong.mChannelGain[(size_t)theChannel] = theVolume;
    openmpt_module_ext_interface_interactive aInteractive;
    std::memset(&aInteractive, 0, sizeof(aInteractive));
    if (openmpt_module_ext_get_interface(aSong.mModule, LIBOPENMPT_EXT_C_INTERFACE_INTERACTIVE,
                                         &aInteractive, sizeof(aInteractive)) == 0
        || aInteractive.set_channel_mute_status == nullptr)
        return;
    aInteractive.set_channel_mute_status(aSong.mModule, theChannel, theVolume <= 0.0f ? 1 : 0);
}

int SDLMusicInterface::GetOrder(int theSongId)
{

    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it == mSongs.end() || it->second.mModuleBase == nullptr)
        return 0;

    int aOrder = openmpt_module_get_current_order(it->second.mModuleBase);
    int aRow = openmpt_module_get_current_row(it->second.mModuleBase);
    if (aOrder < 0)
        aOrder = 0;
    if (aRow < 0)
        aRow = 0;
    return (aOrder & 0xFFFF) | ((aRow & 0xFFFF) << 16);
}

int SDLMusicInterface::GetTempo(int theSongId)
{
    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    return (it != mSongs.end()) ? it->second.mTempo : 0;
}

void SDLMusicInterface::SetTempo(int theSongId, int theTempo)
{

    std::map<int, Song>::iterator it = mSongs.find(theSongId);
    if (it != mSongs.end())
        it->second.mTempo = theTempo;
}

void SDLMusicInterface::SetVolume(double volume)
{ mVolume = volume; for (auto& entry : mSongs) ApplySongGain(entry.second); }

void SDLMusicInterface::SetMusicAmplify(int id, double volume)
{ auto found = mSongs.find(id); if (found != mSongs.end()) { found->second.mAmplify = volume; ApplySongGain(found->second); } }

void SDLMusicInterface::Update()
{

    for (std::map<int, Song>::iterator it = mSongs.begin(); it != mSongs.end(); ++it)
    {
        Song &aSong = it->second;
        if (aSong.mPlaying)
            RefillModule(aSong);
        if (aSong.mFadeRate == 0.0 || aSong.mStream == nullptr)
            continue;

        aSong.mVolume += aSong.mFadeRate * 0.01;
        if ((aSong.mFadeRate > 0.0 && aSong.mVolume >= aSong.mFadeTarget) ||
            (aSong.mFadeRate < 0.0 && aSong.mVolume <= aSong.mFadeTarget))
        {
            aSong.mVolume = aSong.mFadeTarget;
            aSong.mFadeRate = 0.0;
        }

        if (aSong.mVolume < 0.0)
            aSong.mVolume = 0.0;

        ApplySongGain(aSong);

        if (aSong.mFadeRate == 0.0 && aSong.mVolume <= 0.0 && aSong.mPlaying)
        {
            SDL_ClearAudioStream(aSong.mStream);
            aSong.mPlaying = false;
        }
    }
}

}
