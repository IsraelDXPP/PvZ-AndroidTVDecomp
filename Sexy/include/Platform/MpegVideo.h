// MpegVideo -- decode the title intro with PL_MPEG.
//
// The Android build played movies/intro.mp4 through the AG platform library.
// This port plays the converted MPEG-1 program stream movies/intro.mpg:
// PL_MPEG decodes the picture, OpenAL plays the soundtrack, and TitleScreen
// draws the current frame. The file has to be MPEG-1 video and MP2 audio in
// an MPEG program stream. MPEG-4 and AAC stay in the original file and are
// not what this player opens.

#pragma once

namespace Sexy
{
class Graphics;
}

// Non-zero while a movie is open and has not reached its last frame.
int MpegVideoIsPlaying();

// Opens thePath. A missing file, or a file that is not an MPEG program
// stream, returns non-zero. 0 means the picture can be played.
int MpegVideoOpen(const char* thePath);

// Starts picture and soundtrack from the beginning. 0 on success.
int MpegVideoPlay();

void MpegVideoClose();

// AG video events use 13 for playback completion. Callbacks run on the
// drawing thread, after the decoder has returned.
using MpegVideoEventListener = int* (*)(int*);
void MpegVideoAddEventListener(MpegVideoEventListener theListener);
void MpegVideoRemoveEventListener(MpegVideoEventListener theListener);

// Presents the frame that matches the clock. TitleScreen calls this while
// the intro state is showing a movie.
void MpegVideoDraw(Sexy::Graphics* theGraphics, int theX, int theY,
                   int theWidth, int theHeight);
