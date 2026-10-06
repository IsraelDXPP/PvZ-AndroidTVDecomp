// MpegVideo.cpp -- PL_MPEG picture decode and OpenAL soundtrack for the intro.
// pl_mpeg.h names FILE in its prototypes and only includes stdio.h inside the
// implementation, so the header has to come first.

#include <stdio.h>

#define PL_MPEG_IMPLEMENTATION
#include "third_party/pl_mpeg.h"

#include "MpegVideo.h"

#include "Graphics.h"
#include "Win32File.h"
#include "GL/GLRenderDevice.h"
#include "GL/GLHeaders.h"

#include <AL/al.h>
#include <windows.h>

#include <cstring>
#include <algorithm>
#include <string>
#include <vector>

namespace
{

const int kQueuedBuffers = 8;

struct VideoPlayer
{
    plm_t* mMovie;
    bool mOpened;
    bool mPlaying;
    bool mEnded;
    DWORD mLastTick;
    std::vector<unsigned char> mChroma;
    unsigned int mYTexture;
    unsigned int mCbTexture;
    unsigned int mCrTexture;
    int mWidth;
    int mHeight;
    int mChromaWidth;
    int mChromaHeight;
    ALuint mSource;
    ALuint mBuffers[kQueuedBuffers];
    int mBufferCount;
    int mNextBuffer;
    int mSampleRate;
    std::vector<MpegVideoEventListener> mEventListeners;

    VideoPlayer()
        : mMovie(nullptr)
        , mOpened(false)
        , mPlaying(false)
        , mEnded(false)
        , mLastTick(0)
        , mYTexture(0)
        , mCbTexture(0)
        , mCrTexture(0)
        , mWidth(0)
        , mHeight(0)
        , mChromaWidth(0)
        , mChromaHeight(0)
        , mSource(0)
        , mBufferCount(0)
        , mNextBuffer(0)
        , mSampleRate(44100)
    {
        for (int aIndex = 0; aIndex < kQueuedBuffers; ++aIndex)
            mBuffers[aIndex] = 0;
    }
};

VideoPlayer& Player()
{
    static VideoPlayer aPlayer;
    return aPlayer;
}

void ReleaseAudio(VideoPlayer& thePlayer)
{
    if (thePlayer.mSource != 0)
    {
        alSourceStop(thePlayer.mSource);
        alSourcei(thePlayer.mSource, AL_BUFFER, 0);
        alDeleteSources(1, &thePlayer.mSource);
        thePlayer.mSource = 0;
    }
    if (thePlayer.mBufferCount > 0)
    {
        alDeleteBuffers(thePlayer.mBufferCount, thePlayer.mBuffers);
        for (int aIndex = 0; aIndex < kQueuedBuffers; ++aIndex)
            thePlayer.mBuffers[aIndex] = 0;
        thePlayer.mBufferCount = 0;
    }
    thePlayer.mNextBuffer = 0;
}

void DeletePlane(unsigned int& theTexture)
{
    if (theTexture == 0)
        return;
    glDeleteTextures(1, &theTexture);
    theTexture = 0;
}

unsigned int MakePlane(int theWidth, int theHeight, const unsigned char* thePixels)
{
    unsigned int aTexture = 0;
    glGenTextures(1, &aTexture);
    glBindTexture(GL_TEXTURE_2D, aTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, theWidth, theHeight, 0,
                 GL_RED, GL_UNSIGNED_BYTE, thePixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    return aTexture;
}

void UploadPlane(unsigned int theTexture, int theWidth, int theHeight,
                 const unsigned char* thePixels)
{
    glBindTexture(GL_TEXTURE_2D, theTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, theWidth, theHeight,
                    GL_RED, GL_UNSIGNED_BYTE, thePixels);
}

void ReleasePlayer(VideoPlayer& thePlayer)
{
    ReleaseAudio(thePlayer);
    thePlayer.mChroma.clear();
    DeletePlane(thePlayer.mYTexture);
    DeletePlane(thePlayer.mCbTexture);
    DeletePlane(thePlayer.mCrTexture);
    if (thePlayer.mMovie != nullptr)
    {
        plm_destroy(thePlayer.mMovie);
        thePlayer.mMovie = nullptr;
    }
    thePlayer.mOpened = false;
    thePlayer.mPlaying = false;
    thePlayer.mEnded = false;
    thePlayer.mWidth = 0;
    thePlayer.mHeight = 0;
}

std::string ResolveMovie(const char* thePath)
{
    if (thePath == nullptr || thePath[0] == '\0')
        return {};

    std::string aPath = thePath;
    const char* aScheme = std::strstr(aPath.c_str(), "://");
    if (aScheme != nullptr)
        aPath = aScheme + 3;

    if (Sexy::Win32File::FileExists(aPath))
        return Sexy::Win32File::ResolvePath(aPath);

    // TitleScreen builds "<external dir>/movies/intro.mpg" and then replaces
    // the whole string with the relative name. Either spelling should open.
    const char* aMovies = std::strstr(aPath.c_str(), "movies/");
    if (aMovies != nullptr && Sexy::Win32File::FileExists(aMovies))
        return Sexy::Win32File::ResolvePath(aMovies);
    return {};
}

void RecycleAudio(VideoPlayer& thePlayer)
{
    if (thePlayer.mSource == 0)
        return;
    ALint aProcessed = 0;
    alGetSourcei(thePlayer.mSource, AL_BUFFERS_PROCESSED, &aProcessed);
    while (aProcessed > 0)
    {
        ALuint aDone = 0;
        alSourceUnqueueBuffers(thePlayer.mSource, 1, &aDone);
        --aProcessed;
    }
}

void OnAudio(plm_t* theMovie, plm_samples_t* theSamples, void* theUser)
{
    (void)theMovie;
    VideoPlayer* aPlayer = static_cast<VideoPlayer*>(theUser);
    if (aPlayer == nullptr || theSamples == nullptr || aPlayer->mSource == 0)
        return;
    if (theSamples->count == 0)
        return;

    RecycleAudio(*aPlayer);
    ALint aQueued = 0;
    alGetSourcei(aPlayer->mSource, AL_BUFFERS_QUEUED, &aQueued);
    if (aQueued >= aPlayer->mBufferCount)
        return;

    const int aCount = (int)theSamples->count * 2;
    std::vector<short> aPcm((size_t)aCount);
    for (int aIndex = 0; aIndex < aCount; ++aIndex)
    {
        float aSample = theSamples->interleaved[aIndex];
        if (aSample > 1.0f)
            aSample = 1.0f;
        if (aSample < -1.0f)
            aSample = -1.0f;
        aPcm[(size_t)aIndex] = (short)(aSample * 32767.0f);
    }

    ALuint aBuffer = aPlayer->mBuffers[aPlayer->mNextBuffer];
    aPlayer->mNextBuffer = (aPlayer->mNextBuffer + 1) % aPlayer->mBufferCount;
    alBufferData(aBuffer, AL_FORMAT_STEREO16, aPcm.data(),
                 aCount * (int)sizeof(short), aPlayer->mSampleRate);
    alSourceQueueBuffers(aPlayer->mSource, 1, &aBuffer);

    ALint aState = 0;
    alGetSourcei(aPlayer->mSource, AL_SOURCE_STATE, &aState);
    if (aState != AL_PLAYING)
        alSourcePlay(aPlayer->mSource);
}

void OnVideo(plm_t* theMovie, plm_frame_t* theFrame, void* theUser)
{
    (void)theMovie;
    VideoPlayer* aPlayer = static_cast<VideoPlayer*>(theUser);
    if (aPlayer == nullptr || theFrame == nullptr)
        return;
    if (theFrame->width == 0 || theFrame->height == 0)
        return;

    int aWidth = (int)theFrame->width;
    int aHeight = (int)theFrame->height;
    int aChromaWidth = (int)theFrame->cb.width;
    int aChromaHeight = (int)theFrame->cb.height;
    if (aWidth <= 0 || aHeight <= 0 || aChromaWidth <= 0 || aChromaHeight <= 0)
        return;
    if (theFrame->y.data == nullptr || theFrame->cb.data == nullptr || theFrame->cr.data == nullptr)
        return;

    if (Sexy::GLRenderDevice::sCurrent == nullptr)
        return;

    // MPEG-1 stores chroma at half resolution. The planes are not packed, so
    // copy each one tight before the upload. The shader turns them into RGB.
    if (aPlayer->mWidth != aWidth || aPlayer->mHeight != aHeight
        || aPlayer->mChromaWidth != aChromaWidth || aPlayer->mChromaHeight != aChromaHeight
        || aPlayer->mYTexture == 0)
    {
        DeletePlane(aPlayer->mYTexture);
        DeletePlane(aPlayer->mCbTexture);
        DeletePlane(aPlayer->mCrTexture);
        aPlayer->mWidth = aWidth;
        aPlayer->mHeight = aHeight;
        aPlayer->mChromaWidth = aChromaWidth;
        aPlayer->mChromaHeight = aChromaHeight;
        aPlayer->mChroma.assign((size_t)aChromaWidth * (size_t)aChromaHeight, 0);
        aPlayer->mYTexture = MakePlane(aWidth, aHeight, nullptr);
        aPlayer->mCbTexture = MakePlane(aChromaWidth, aChromaHeight, nullptr);
        aPlayer->mCrTexture = MakePlane(aChromaWidth, aChromaHeight, nullptr);
    }

    UploadPlane(aPlayer->mYTexture, aWidth, aHeight, theFrame->y.data);
    for (int aRow = 0; aRow < aChromaHeight; ++aRow)
    {
        std::memcpy(aPlayer->mChroma.data() + (size_t)aRow * (size_t)aChromaWidth,
                    theFrame->cb.data + aRow * (int)theFrame->cb.width,
                    (size_t)aChromaWidth);
    }
    UploadPlane(aPlayer->mCbTexture, aChromaWidth, aChromaHeight, aPlayer->mChroma.data());
    for (int aRow = 0; aRow < aChromaHeight; ++aRow)
    {
        std::memcpy(aPlayer->mChroma.data() + (size_t)aRow * (size_t)aChromaWidth,
                    theFrame->cr.data + aRow * (int)theFrame->cr.width,
                    (size_t)aChromaWidth);
    }
    UploadPlane(aPlayer->mCrTexture, aChromaWidth, aChromaHeight, aPlayer->mChroma.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace

int MpegVideoIsPlaying()
{
    VideoPlayer& aPlayer = Player();
    return (aPlayer.mPlaying && !aPlayer.mEnded) ? 1 : 0;
}

int MpegVideoOpen(const char* thePath)
{
    VideoPlayer& aPlayer = Player();
    ReleasePlayer(aPlayer);

    std::string aFile = ResolveMovie(thePath);
    if (aFile.empty())
        return 1;

    aPlayer.mMovie = plm_create_with_filename(aFile.c_str());
    if (aPlayer.mMovie == nullptr)
        return 1;
    if (plm_get_width(aPlayer.mMovie) <= 0 || plm_get_height(aPlayer.mMovie) <= 0)
    {
        ReleasePlayer(aPlayer);
        return 1;
    }

    plm_set_video_decode_callback(aPlayer.mMovie, OnVideo, &aPlayer);
    int aRate = plm_get_samplerate(aPlayer.mMovie);
    if (aRate > 0 && plm_get_num_audio_streams(aPlayer.mMovie) > 0)
    {
        aPlayer.mSampleRate = aRate;
        alGenSources(1, &aPlayer.mSource);
        if (aPlayer.mSource != 0)
        {
            alGenBuffers(kQueuedBuffers, aPlayer.mBuffers);
            aPlayer.mBufferCount = kQueuedBuffers;
            alSourcei(aPlayer.mSource, AL_LOOPING, AL_FALSE);
            plm_set_audio_enabled(aPlayer.mMovie, 1);
            plm_set_audio_decode_callback(aPlayer.mMovie, OnAudio, &aPlayer);
            // One queued buffer is 1152 frames. Keep that much audio ahead of
            // the picture so the source does not starve between draws.
            plm_set_audio_lead_time(aPlayer.mMovie, 1152.0 / (double)aRate);
        }
    }
    else
    {
        plm_set_audio_enabled(aPlayer.mMovie, 0);
    }

    aPlayer.mOpened = true;
    return 0;
}

int MpegVideoPlay()
{
    VideoPlayer& aPlayer = Player();
    if (!aPlayer.mOpened || aPlayer.mMovie == nullptr)
        return 1;

    aPlayer.mEnded = false;
    aPlayer.mPlaying = true;
    aPlayer.mLastTick = GetTickCount();
    // A zero delta only drains already-decoded frames. The file starts with
    // headers, so the first picture arrives on a small step.
    plm_decode(aPlayer.mMovie, 1.0 / 30.0);
    return aPlayer.mYTexture == 0 ? 1 : 0;
}

void MpegVideoClose()
{
    ReleasePlayer(Player());
}

void MpegVideoAddEventListener(MpegVideoEventListener theListener)
{
    if (theListener == nullptr)
        return;
    auto& aListeners = Player().mEventListeners;
    if (std::find(aListeners.begin(), aListeners.end(), theListener) == aListeners.end())
        aListeners.push_back(theListener);
}

void MpegVideoRemoveEventListener(MpegVideoEventListener theListener)
{
    auto& aListeners = Player().mEventListeners;
    aListeners.erase(std::remove(aListeners.begin(), aListeners.end(), theListener),
                     aListeners.end());
}

void MpegVideoDraw(Sexy::Graphics* theGraphics, int theX, int theY,
                   int theWidth, int theHeight)
{
    VideoPlayer& aPlayer = Player();
    if (theGraphics == nullptr || !aPlayer.mPlaying || aPlayer.mMovie == nullptr)
        return;

    DWORD aNow = GetTickCount();
    double aElapsed = (double)(aNow - aPlayer.mLastTick) / 1000.0;
    aPlayer.mLastTick = aNow;
    if (aElapsed < 0.0)
        aElapsed = 0.0;
    if (aElapsed > 0.1)
        aElapsed = 0.1;
    plm_decode(aPlayer.mMovie, aElapsed);
    bool aPlaybackCompleted = !aPlayer.mEnded && plm_has_ended(aPlayer.mMovie);
    if (aPlaybackCompleted)
        aPlayer.mEnded = true;

    // DrawScreen translates by the centering origin before widgets draw, so
    // a quad at (-240, -60) lands at the window origin. Add that shift here:
    // the YUV path does not go through Graphics::DrawImage.
    int aShiftX = (int)theGraphics->mTransX;
    int aShiftY = (int)theGraphics->mTransY;
    TRect<int> aDest(theX + aShiftX, theY + aShiftY, theWidth, theHeight);
    if (aPlayer.mYTexture != 0 && Sexy::GLRenderDevice::sCurrent != nullptr)
        Sexy::GLRenderDevice::sCurrent->DrawYuv(aPlayer.mYTexture, aPlayer.mCbTexture,
                                            aPlayer.mCrTexture, aPlayer.mWidth,
                                            aPlayer.mHeight, aDest);

    // A callback may close the player or unregister itself. Finish using the
    // movie and textures first, then dispatch a snapshot exactly once.
    if (aPlaybackCompleted)
    {
        auto aListeners = aPlayer.mEventListeners;
        int aCompletedEvent = 13;
        for (MpegVideoEventListener aListener : aListeners)
            aListener(&aCompletedEvent);
    }
}
