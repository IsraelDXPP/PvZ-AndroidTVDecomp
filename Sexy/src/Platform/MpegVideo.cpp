// MpegVideo.cpp -- PL_MPEG picture decode and SDL3 soundtrack for the intro.
// File access stays in SDL so movies can use UTF-8 paths and Android assets.
#define PLM_NO_STDIO
#define PL_MPEG_IMPLEMENTATION
#include "third_party/pl_mpeg.h"

#include "MpegVideo.h"

#include "Graphics.h"
#include "SDLFile.h"
#include "ReconHelpers.h"
#include "Bgfx/BgfxRenderDevice.h"

#include <SDL3/SDL.h>

#include <cstring>
#include <limits>
#include <algorithm>
#include <string>
#include <vector>

namespace
{

struct VideoPlayer
{
    plm_t* mMovie;
    SDL_IOStream* mMovieFile;
    bool mOpened;
    bool mPlaying;
    bool mEnded;
    Uint64 mLastTick;
    std::vector<unsigned char> mLuma;
    std::vector<unsigned char> mChroma;
    bgfx::TextureHandle mYTexture;
    bgfx::TextureHandle mCbTexture;
    bgfx::TextureHandle mCrTexture;
    int mWidth;
    int mHeight;
    int mChromaWidth;
    int mChromaHeight;
    SDL_AudioStream* mAudio;
    int mSampleRate;
    std::vector<MpegVideoEventListener> mEventListeners;

    VideoPlayer()
        : mMovie(nullptr)
        , mMovieFile(nullptr)
        , mOpened(false)
        , mPlaying(false)
        , mEnded(false)
        , mLastTick(0)
        , mYTexture(BGFX_INVALID_HANDLE)
        , mCbTexture(BGFX_INVALID_HANDLE)
        , mCrTexture(BGFX_INVALID_HANDLE)
        , mWidth(0)
        , mHeight(0)
        , mChromaWidth(0)
        , mChromaHeight(0)
        , mAudio(nullptr)
        , mSampleRate(44100)
    {
    }
};

VideoPlayer& Player()
{
    static VideoPlayer aPlayer;
    return aPlayer;
}

void ReleaseAudio(VideoPlayer& player)
{
    if (player.mAudio)
    {
        SDL_DestroyAudioStream(player.mAudio);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
    player.mAudio = nullptr;
}

void DeletePlane(bgfx::TextureHandle& texture)
{ if (bgfx::isValid(texture)) bgfx::destroy(texture); texture = BGFX_INVALID_HANDLE; }

bgfx::TextureHandle MakePlane(int width, int height, const unsigned char*)
{ return bgfx::createTexture2D(uint16_t(width), uint16_t(height), false, 1,
    bgfx::TextureFormat::R8, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP); }

void UploadPlane(bgfx::TextureHandle texture, int width, int height, const unsigned char* pixels)
{ if (bgfx::isValid(texture)) bgfx::updateTexture2D(texture, 0, 0, 0, 0, uint16_t(width), uint16_t(height),
    bgfx::copy(pixels, uint32_t(width * height))); }

void ReleasePlayer(VideoPlayer& thePlayer)
{
    ReleaseAudio(thePlayer);
    thePlayer.mChroma.clear();
    thePlayer.mLuma.clear();
    DeletePlane(thePlayer.mYTexture);
    DeletePlane(thePlayer.mCbTexture);
    DeletePlane(thePlayer.mCrTexture);
    if (thePlayer.mMovie != nullptr)
    {
        plm_destroy(thePlayer.mMovie);
        thePlayer.mMovie = nullptr;
    }
    if (thePlayer.mMovieFile != nullptr)
    {
        SDL_CloseIO(thePlayer.mMovieFile);
        thePlayer.mMovieFile = nullptr;
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

    if (Sexy::SDLFile::FileExists(aPath))
        return Sexy::SDLFile::ResolvePath(aPath);

    // TitleScreen builds "<external dir>/movies/intro.mpg" and then replaces
    // the whole string with the relative name. Either spelling should open.
    const char* aMovies = std::strstr(aPath.c_str(), "movies/");
    if (aMovies != nullptr && Sexy::SDLFile::FileExists(aMovies))
        return Sexy::SDLFile::ResolvePath(aMovies);
    return {};
}

void LoadMovieBytes(plm_buffer_t* theBuffer, void* theUser)
{
    SDL_IOStream* aFile = static_cast<SDL_IOStream*>(theUser);
    // Match PL_MPEG's file loader without changing the total file length at
    // EOF; signal_end() is intended for ring buffers and would overwrite it.
    if (theBuffer->discard_read_bytes)
        plm_buffer_discard_read_bytes(theBuffer);
    const size_t aAvailable = theBuffer->capacity - theBuffer->length;
    const size_t aRead = SDL_ReadIO(aFile, theBuffer->bytes + theBuffer->length, aAvailable);
    theBuffer->length += aRead;
    if (aRead == 0)
        theBuffer->has_ended = TRUE;
}

void SeekMovieBytes(plm_buffer_t*, size_t theOffset, void* theUser)
{
    SDL_IOStream* aFile = static_cast<SDL_IOStream*>(theUser);
    SDL_SeekIO(aFile, static_cast<Sint64>(theOffset), SDL_IO_SEEK_SET);
}

size_t TellMovieBytes(plm_buffer_t*, void* theUser)
{
    SDL_IOStream* aFile = static_cast<SDL_IOStream*>(theUser);
    const Sint64 aPosition = SDL_TellIO(aFile);
    return aPosition >= 0 ? static_cast<size_t>(aPosition) : 0;
}

void OnAudio(plm_t*, plm_samples_t* samples, void* user)
{
    auto* player = static_cast<VideoPlayer*>(user);
    if (player && player->mAudio && samples && samples->count)
        SDL_PutAudioStreamData(player->mAudio, samples->interleaved, int(samples->count * 2 * sizeof(float)));
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
    int aChromaWidth = (aWidth + 1) / 2;
    int aChromaHeight = (aHeight + 1) / 2;
    if (aWidth <= 0 || aHeight <= 0 || aChromaWidth <= 0 || aChromaHeight <= 0)
        return;
    if (theFrame->y.data == nullptr || theFrame->cb.data == nullptr || theFrame->cr.data == nullptr)
        return;

    if (Sexy::BgfxRenderDevice::sCurrent == nullptr)
        return;

    // MPEG-1 stores chroma at half resolution. The planes are not packed, so
    // copy each one tight before the upload. The shader turns them into RGB.
    if (aPlayer->mWidth != aWidth || aPlayer->mHeight != aHeight
        || aPlayer->mChromaWidth != aChromaWidth || aPlayer->mChromaHeight != aChromaHeight
        || !bgfx::isValid(aPlayer->mYTexture))
    {
        DeletePlane(aPlayer->mYTexture);
        DeletePlane(aPlayer->mCbTexture);
        DeletePlane(aPlayer->mCrTexture);
        aPlayer->mWidth = aWidth;
        aPlayer->mHeight = aHeight;
        aPlayer->mChromaWidth = aChromaWidth;
        aPlayer->mChromaHeight = aChromaHeight;
        aPlayer->mChroma.assign((size_t)aChromaWidth * (size_t)aChromaHeight, 0);
        aPlayer->mLuma.resize(size_t(aWidth) * aHeight);
        aPlayer->mYTexture = MakePlane(aWidth, aHeight, nullptr);
        aPlayer->mCbTexture = MakePlane(aChromaWidth, aChromaHeight, nullptr);
        aPlayer->mCrTexture = MakePlane(aChromaWidth, aChromaHeight, nullptr);
    }

    // pl_mpeg pads planes to macroblock dimensions; upload only visible rows.
    for (int row = 0; row < aHeight; ++row)
        std::memcpy(aPlayer->mLuma.data() + size_t(row) * aWidth,
                    theFrame->y.data + size_t(row) * theFrame->y.width, aWidth);
    UploadPlane(aPlayer->mYTexture, aWidth, aHeight, aPlayer->mLuma.data());
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

    aPlayer.mMovieFile = p_fopen(aFile.c_str(), "rb");
    if (aPlayer.mMovieFile == nullptr)
        return 1;
    const Sint64 aSize = SDL_GetIOSize(aPlayer.mMovieFile);
    if (aSize <= 0 || static_cast<Uint64>(aSize) > std::numeric_limits<size_t>::max())
    {
        ReleasePlayer(aPlayer);
        return 1;
    }
    plm_buffer_t* aBuffer = plm_buffer_create_with_callbacks(LoadMovieBytes, SeekMovieBytes,
                                                           TellMovieBytes, static_cast<size_t>(aSize),
                                                           aPlayer.mMovieFile);
    if (aBuffer == nullptr)
    {
        ReleasePlayer(aPlayer);
        return 1;
    }
    aPlayer.mMovie = plm_create_with_buffer(aBuffer, 1);
    if (aPlayer.mMovie == nullptr)
    {
        plm_buffer_destroy(aBuffer);
        ReleasePlayer(aPlayer);
        return 1;
    }
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
        SDL_AudioSpec spec{SDL_AUDIO_F32, 2, aRate};
        if (SDL_InitSubSystem(SDL_INIT_AUDIO))
        {
            aPlayer.mAudio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
            if (!aPlayer.mAudio) SDL_QuitSubSystem(SDL_INIT_AUDIO);
        }
        if (aPlayer.mAudio)
        {
            SDL_ResumeAudioStreamDevice(aPlayer.mAudio);
            plm_set_audio_enabled(aPlayer.mMovie, 1);
            plm_set_audio_decode_callback(aPlayer.mMovie, OnAudio, &aPlayer);
            plm_set_audio_lead_time(aPlayer.mMovie, 1152.0 / double(aRate));
        }
    }
    if (!aPlayer.mAudio) plm_set_audio_enabled(aPlayer.mMovie, 0);

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
    aPlayer.mLastTick = SDL_GetTicks();
    // A zero delta only drains already-decoded frames. The file starts with
    // headers, so the first picture arrives on a small step.
    plm_decode(aPlayer.mMovie, 1.0 / 30.0);
    return !bgfx::isValid(aPlayer.mYTexture) ? 1 : 0;
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

    Uint64 aNow = SDL_GetTicks();
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
    if (bgfx::isValid(aPlayer.mYTexture) && Sexy::BgfxRenderDevice::sCurrent != nullptr)
        Sexy::BgfxRenderDevice::sCurrent->DrawYuv(aPlayer.mYTexture, aPlayer.mCbTexture,
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
