#ifndef __SDLAUDIOBACKEND_H__
#define __SDLAUDIOBACKEND_H__

#include "ReconTypes.h"
#include "SexyString.h"
#include "MusicInterface.h"
#include "SoundManager.h"
#include "SoundInstance.h"

#include <map>
#include <memory>
#include <vector>

struct openmpt_module_ext;
struct openmpt_module;

#include <SDL3/SDL.h>
#include <cstdint>

namespace Sexy
{

class SDLSoundManager;
class SDLMusicInterface;
class SDLSoundBuffer;
class SDLSoundInstance;

class SDLSoundBuffer
{
public:
    std::vector<int16_t> mSamples;
    int mChannels = 0;
    int mRate = 0;
    double  mLengthSeconds;
    double  mBaseVolume;
    int     mBasePan;

    SDLSoundBuffer();
    ~SDLSoundBuffer();
};

class SDLSoundInstance : public SoundInstance
{
public:
    SDLSoundManager *mManager;
    SDL_AudioStream* mStream;
    size_t mCursor = 0;
    bool mLoop = false;
    double mPitchRatio = 1.0;
    std::shared_ptr<SDLSoundBuffer> mBuffer;
    bool                mPlaying;
    bool                mReleased;
    bool                mAutoRelease;
    double              mBaseVolume;
    double              mVolume;
    int                 mPan;
    double              mBaseRate;

public:
    SDLSoundInstance(SDLSoundManager *theManager, const std::shared_ptr<SDLSoundBuffer>& theBuffer);
    virtual ~SDLSoundInstance();

    virtual void Release() override;
    virtual void SetBaseVolume(double theVolume) override;
    virtual void SetBasePan(int thePan) override;
    virtual void SetBaseRate(double theRate) override;
    virtual void AdjustPitch(double thePitch) override;
    virtual void SetVolume(double theVolume) override;
    virtual void SetMasterVolumeIdx(int theIndex) override;
    virtual void SetPan(int thePan) override;
    virtual void Play(bool theLoop, bool theAutoRelease) override;
    virtual void Stop() override;
    virtual bool IsPlaying() override;
    virtual bool IsReleased() override;

    void ApplyState();
};

class SDLSoundManager : public SoundManager
{
public:
    SDL_AudioDeviceID mDevice;
    bool        mInitialized;

    std::vector<std::shared_ptr<SDLSoundBuffer>> mBuffers;

    std::vector<SDLSoundInstance *> mInstances;

    double mVolume;
    double mMasterVolume;

    SDLMusicInterface *mMusicInterface;

public:
    SDLSoundManager();
    virtual ~SDLSoundManager();

    bool Init();
    void Shutdown();

    static SDLSoundBuffer *DecodeOgg(const void *theData, unsigned int theSize);

    SDLMusicInterface *GetMusicInterface() { return mMusicInterface; }

    virtual bool Initialized() override;
    virtual bool LoadSound(unsigned int theSoundId, const String & theFileName) override;
    virtual int LoadSound(const String & theFileName) override;
    virtual void ReleaseSound(unsigned int theSoundId) override;
    virtual void SetVolume(double theVolume) override;
    virtual void SetVolume(int theSoundId, double theVolume) override;
    virtual void SetBaseVolume(unsigned int theSoundId, double theVolume) override;
    virtual void SetBasePan(unsigned int theSoundId, int thePan) override;
    virtual bool IsPlaying(unsigned int theSoundId) override;
    virtual SoundInstance * GetPlayingSoundInstance(unsigned int theSoundId) override;
    virtual SoundInstance * GetSoundInstance(unsigned int theSoundId) override;
    virtual void ReleaseSounds() override;
    virtual void ReleaseChannels() override;
    virtual double GetMasterVolume() override;
    virtual void SetMasterVolume(double theVolume) override;
    virtual void Flush() override;
    virtual void SetCooperativeWindow(void * theWindow, bool theActive) override;
    virtual void StopAllSounds() override;
    virtual int GetFreeSoundId() override;
    virtual int GetNumSounds() override;
    virtual void Update() override;
};

class SDLMusicInterface : public MusicInterface
{
public:
    SDLSoundManager *mManager;

    struct Song
    {
        SDLSoundBuffer *mBuffer;
        SDL_AudioStream* mStream;
        size_t mCursor;
        bool mFinished;
        double             mVolume;
        double             mMaxVolume;
        double             mAmplify;
        int                mTempo;
        bool               mLoaded;
        bool               mPlaying;
        bool               mPaused;
        bool               mLoop;

        double             mFadeTarget;
        double             mFadeRate;

        openmpt_module_ext *mModule;
        openmpt_module *    mModuleBase;

        std::vector<float>  mChannelGain;
        int                 mStreamRate;
    };

    std::map<int, Song> mSongs;
    double              mVolume;

public:
    SDLMusicInterface(SDLSoundManager *theManager);
    virtual ~SDLMusicInterface();

    void EnsureStream(Song &theSong);
    void ApplySongGain(Song &theSong);
    void ReleaseModule(Song &theSong);
    bool QueueModuleSlice(Song &theSong);
    void RefillModule(Song &theSong);

    virtual bool IsLoaded(int theSongId) override;
    virtual bool LoadMusic(int theSongId, Sexy::String const & theFileName) override;
    virtual void PlayMusic(int theSongId, int theOffset, bool noLoop) override;
    virtual void StopMusic(int theSongId) override;
    virtual void PauseMusic(int theSongId) override;
    virtual void ResumeMusic(int theSongId) override;
    virtual void StopAllMusic() override;
    virtual void UnloadMusic(int theSongId) override;
    virtual void UnloadAllMusic() override;
    virtual void PauseAllMusic() override;
    virtual void ResumeAllMusic() override;
    virtual void FadeIn(int theSongId, int theOffset, double theVolume, bool noLoop) override;
    virtual void FadeOut(int theSongId, bool noLoop, double theVolume) override;
    virtual void FadeOutAll(bool noLoop, double theVolume) override;
    virtual void SetSongVolume(int theSongId, double theVolume) override;
    virtual void SetSongMaxVolume(int theSongId, double theVolume) override;
    virtual bool IsPlaying(int theSongId) override;
    virtual void SetChannelVolume(int theSongId, int theChannel, float theVolume) override;
    virtual int GetOrder(int theSongId) override;
    virtual int GetTempo(int theSongId) override;
    virtual void SetTempo(int theSongId, int theTempo) override;
    virtual void SetVolume(double theVolume) override;
    virtual void SetMusicAmplify(int theSongId, double theVolume) override;
    virtual void Update() override;
};

}

using Sexy::SDLSoundManager;
using Sexy::SDLSoundInstance;
using Sexy::SDLMusicInterface;

#endif
