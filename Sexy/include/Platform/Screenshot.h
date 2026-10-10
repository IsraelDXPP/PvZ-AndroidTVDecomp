// Screenshot.h -- offscreen capture of the game's back buffer.
//
// Verification of a software renderer should not depend on grabbing the
// desktop: the back buffer already holds exactly what was drawn, so we write
// that buffer straight to a file. This makes captures deterministic, immune to
// window occlusion/DPI/scaling, and usable from a headless test run.
//
// Two entry points:
//   SaveScreenshot()          -- explicit, used by tests and the --screenshot flag
//   ScreenshotHotkey::Poll()  -- F12 at runtime, checked once per frame

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_SCREENSHOT_H
#define LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_SCREENSHOT_H

#include "CoreTypes.h"
#include "MemoryImage.h"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace Sexy
{

class Screenshot
{
public:
    // Write a 32-bit ARGB surface to a PNG (or BMP if the path ends in .bmp).
    // Returns false if the image is empty or the file could not be written.
    static bool Save(MemoryImage *theImage, const std::string &thePath);

    // Convenience: derive a numbered filename in the screenshots/ folder,
    // e.g. screenshots/shot_0001.png, and bump the counter.
    static bool SaveNext(MemoryImage *theImage);

    // Number of screenshots taken this session.
    static int GetCount();

    // Write the raw ARGB buffer next to the PNG for exact byte-level diffing.
    static bool SaveRaw(MemoryImage *theImage, const std::string &thePath);

private:
    static int sCounter;
};

// Runtime hotkey handling (F12 by default).
class ScreenshotHotkey
{
public:
    // Query before reading the GPU target; Poll records the key state afterward.
    static bool IsCaptureRequested(bool theKeyDown);

    // Call once per frame with the current key state. Captures on the
    // rising edge only, so holding the key does not spam files.
    static void Poll(MemoryImage *theScreen, bool theKeyDown);

private:
    static bool sWasDown;
};

} // namespace Sexy

#endif // LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_SCREENSHOT_H
