// Native::NotificationManager -- reconstructed from libGameMain.so.
//
// Plain class: no base class and no virtual function. All six methods are
// 12-byte thunks (two of them, in the tail of the dump, are 16-byte thunks in a
// second translation unit), so the class itself carries no logic and the
// binaries' implementation lives in the Java/native bridge.
//
// Size 0x8, taken from the allocation sites the transcription recorded. No
// transcribed body reads or writes the storage, so both words are unknown.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_NOTIFICATIONMANAGER_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_NOTIFICATIONMANAGER_H

#include "ReconTypes.h"

namespace Native
{

class NotificationManager
{
public:
    int create();
    int getFeatures();
    int getName();
    int setAlias();
    int setTags();
    int cancelAllNotifications();
};

} // namespace Native

// Layout check: two four-byte words.

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_NOTIFICATIONMANAGER_H
