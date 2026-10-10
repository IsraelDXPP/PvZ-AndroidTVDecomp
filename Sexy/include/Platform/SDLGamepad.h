// SDLGamepad -- SDL3 behind Sexy::IInputDriver.
//
// SDL controller ids are the devices. A connected pad becomes key events the
// game already understands; sticks are read back through GetAxisInfo.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_SDLGAMEPAD_H
#define LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_SDLGAMEPAD_H

#include "IInputDriver.h"
#include "InputDriver.h"

#include <SDL3/SDL_gamepad.h>
#include <vector>

namespace Sexy
{

class SDLGamepadInterface;

class SDLGamepadDriver : public InputDriver
{
public:
    SDLGamepadDriver();
    virtual ~SDLGamepadDriver();

    virtual IInputDriver *Init();
    virtual int OnStart(Sexy::SexyAppBase *theApp, Sexy::InputManager *theManager);
    virtual int OnStop();

    SDLGamepadInterface *mInterface;
};

IInputDriver *CreateSDLGamepad(InputDriver *theOwner);

} // namespace Sexy

#endif // LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_SDLGAMEPAD_H
