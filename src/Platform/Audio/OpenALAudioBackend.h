// OpenALAudioBackend.h -- the Windows audio backend.
//
// The binary's audio stack is audiere (the ELF exports
// `_ZTVN4Sexy21AudiereMusicInterfaceE` and `_ZTV18AudiereSoundDriver`, and the
// audiere namespace is visible in `_ZTVN7audiere15NullAudioDeviceE`). audiere is
// dead and Windows-only-ish, so this project replaces it the same way the
// graphics side replaces the Android GLES2 driver with desktop GL: the game code
// keeps calling the framework interfaces, and the platform layer supplies a
// Windows implementation behind them.
//
//     Sexy::MusicInterface      (Sexy\MusicInterface.h)   <- the game calls this
//       +-- OpenALMusicInterface      (this file)
//     Sexy::SoundManager        (Sexy\SoundManager.h)     <- the game calls this
//       +-- OpenALSoundManager        (this file)
//     Sexy::SoundInstance       (Sexy\SoundInstance.h)
//       +-- OpenALSoundInstance       (this file)
//
// DECODING. OpenAL is a mixer, not a decoder: it wants PCM. Resource names are
// stems, the same way images are. This file probes .ogg then .wav through
// Win32File, then libvorbisfile decodes Ogg Vorbis and a PCM WAV reader covers
// the one loose wav. libvorbis, libogg and libvorbisfile are already linked.
//
// WHY THIS IS NOT THE SAME SHAPE AS THE GL BACKEND. Graphics has an
// `IGraphicsBackend` indirection because `Graphics` talks to a `RenderDevice`
// that the binary itself made abstract. Audio has no such seam: the binary's
// `MusicInterface` and `SoundManager` ARE the abstract interfaces, so the backend
// simply derives from them. Adding a second layer of indirection would be
// invention.
//
// THREADING. OpenAL's own mixer thread runs the sources; nothing here blocks on
// audio. `Update()` is called once a frame from the app loop and only does
// bookkeeping (reaping finished one-shot instances and advancing fades), so it
// must stay cheap.

#ifndef __OPENALAUDIOBACKEND_H__
#define __OPENALAUDIOBACKEND_H__

#include "ReconTypes.h"
#include "SexyString.h"
#include "MusicInterface.h"
#include "SoundManager.h"
#include "SoundInstance.h"

#include <map>
#include <vector>

struct openmpt_module_ext;
struct openmpt_module;

// The OpenAL 1.1 core, plus the two AL_SOFT/EFX-free extensions this backend
// uses. OpenALSoft ships its own `AL/al.h`; including it here keeps the handle
// types in one place.
#include <AL/al.h>
#include <AL/alc.h>

namespace Sexy
{

// Declared before the classes that hold pointers to them: the instance points
// back at its manager, and the manager owns the music interface, which is built
// on the same OpenAL context rather than opening a second device.
class OpenALSoundManager;
class OpenALMusicInterface;
class OpenALSoundBuffer;
class OpenALSoundInstance;

// ---------------------------------------------------------------------------
// One decoded PCM buffer. Shared by every instance that plays the same sound,
// which is why the reference count is here rather than in the instance.
// ---------------------------------------------------------------------------
class OpenALSoundBuffer
{
public:
    ALuint  mBuffer;        // the OpenAL buffer name, 0 when not uploaded
    int     mRefCount;      // instances holding it
    double  mLengthSeconds; // for the "is it still playing" fallback

    OpenALSoundBuffer();
    ~OpenALSoundBuffer();
};

// ---------------------------------------------------------------------------
// Sexy::SoundInstance for OpenAL. One AL source per instance, which is what the
// framework's SetVolume/SetPan/AdjustPitch map onto one for one.
// ---------------------------------------------------------------------------
class OpenALSoundInstance : public SoundInstance
{
public:
    OpenALSoundManager *mManager;
    ALuint              mSource;
    OpenALSoundBuffer  *mBuffer;
    bool                mPlaying;
    bool                mReleased;
    bool                mIsMusic;
    double              mBaseVolume;
    double              mVolume;
    int                 mPan;
    double              mBaseRate;

public:
    OpenALSoundInstance(OpenALSoundManager *theManager, OpenALSoundBuffer *theBuffer);
    virtual ~OpenALSoundInstance();

    // SoundInstance
    virtual void Release() override;
    virtual void SetBaseVolume(double theVolume) override;
    virtual void SetBasePan(int thePan) override;
    virtual void SetBaseRate(double theRate) override;
    virtual void AdjustPitch(double thePitch) override;
    virtual void SetVolume(double theVolume) override;
    virtual void SetMasterVolumeIdx(int theIndex) override;
    virtual void SetPan(int thePan) override;
    virtual void Play(bool theIsMusic, bool theForcePlay) override;
    virtual void Stop() override;
    virtual bool IsPlaying() override;
    virtual bool IsReleased() override;

    // Push the accumulated volume/pan/pitch onto the AL source. Every setter
    // above ends here, because OpenAL wants the final values, not the deltas.
    void ApplyState();
};

// ---------------------------------------------------------------------------
// Sexy::SoundManager for OpenAL.
//
// The ids the game uses are `SoundType` enumerators (SEEDLIFT, PLANT, GROAN,
// ...), which is why LoadSound takes an unsigned id: the game allocates ids
// densely from 0 and this table is indexed by them.
// ---------------------------------------------------------------------------
class OpenALSoundManager : public SoundManager
{
public:
    ALCdevice  *mDevice;
    ALCcontext *mContext;
    bool        mInitialized;

    // Indexed by sound id. A null entry means "that id is not loaded".
    std::vector<OpenALSoundBuffer *> mBuffers;
    // The live instances, so Update can reap the finished ones.
    std::vector<OpenALSoundInstance *> mInstances;

    double mVolume;       // the master volume, 0..1
    double mMasterVolume;

    // The music interface shares the device, so it is constructed with this
    // manager and does not open a second one.
    OpenALMusicInterface *mMusicInterface;

public:
    OpenALSoundManager();
    virtual ~OpenALSoundManager();

    bool Init();
    void Shutdown();

    // The decode entry point: bytes in, an uploaded AL buffer out. Shared with
    // OpenALMusicInterface so both paths decode .ogg the same way.
    static OpenALSoundBuffer *DecodeOgg(const void *theData, unsigned int theSize);

    // OpenAL is global state, so the manager that owns the context also owns
    // the music interface built on it.
    OpenALMusicInterface *GetMusicInterface() { return mMusicInterface; }

    // SoundManager
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

// ---------------------------------------------------------------------------
// Sexy::MusicInterface for OpenAL.
//
// This is the class that makes music audible: the tree's own MusicInterface is
// the binary's, whose 24 bodies are all `BX LR` / `MOVS R0,#0` because the TV
// build shipped without a backend. `LoadMusic` returning false is what makes
// Music::LoadSong set mMusicDisabled, so with only the base class the whole game
// is silent. Deriving here and handing the instance to
// SexyAppBase::CreateMusicInterface is what turns it back on.
//
// One AL source per song id, so the framework's per-song volume and fade calls
// map onto sources directly.
// ---------------------------------------------------------------------------
class OpenALMusicInterface : public MusicInterface
{
public:
    OpenALSoundManager *mManager;

    struct Song
    {
        OpenALSoundBuffer *mBuffer;
        ALuint             mSource;
        double             mVolume;      // the current (possibly fading) volume
        double             mMaxVolume;   // the ceiling the fades ramp toward
        double             mAmplify;     // the per-song multiplier
        int                mTempo;       // informational: OpenAL has no tempo
        bool               mLoaded;
        bool               mPlaying;
        bool               mPaused;
        bool               mLoop;
        // Fades are stepped in Update rather than by an AL filter, because
        // OpenAL's gain is the only portable control.
        double             mFadeTarget;
        double             mFadeRate;    // volume units per second, 0 = no fade

        // An XM (or any tracker module) is rendered by libopenmpt as it plays.
        // The three queued buffers are the streaming ring. mModule is null for
        // a decoded ogg or wav, which still uses mBuffer.
        openmpt_module_ext *mModule;
        openmpt_module *    mModuleBase;
        std::vector<ALuint> mStreamBuffers;
        std::vector<float>  mChannelGain;
        int                 mStreamRate;
        bool                mStreamPrimed;
    };

    std::map<int, Song> mSongs;
    double              mVolume;     // the master music volume

public:
    OpenALMusicInterface(OpenALSoundManager *theManager);
    virtual ~OpenALMusicInterface();

    // The music path reads its .ogg through the same decoder the sound path
    // uses, but keeps its own source so it can loop and fade independently.
    void EnsureSource(Song &theSong);
    void ApplySongGain(Song &theSong);
    void ReleaseModule(Song &theSong);
    bool QueueModuleSlice(Song &theSong, ALuint theBuffer);
    void RefillModule(Song &theSong);

    // MusicInterface
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

} // namespace Sexy

// The global spelling the transcribed bodies use.
using Sexy::OpenALSoundManager;
using Sexy::OpenALSoundInstance;
using Sexy::OpenALMusicInterface;

#endif // __OPENALAUDIOBACKEND_H__
