// BgfxRenderDevice.ios.mm -- the Metal layer bgfx needs on iOS.
//
// bgfx's iOS backend expects the CAMetalLayer of the view that SDL3 created,
// the same handle its own examples pass in bgfx::PlatformData::nwh. SDL3
// exposes the UIWindow through SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER; the
// window's root view controller owns the layer.
//
// Objective-C++ because the UIKit headers are Objective-C only. The file is
// compiled for iOS builds alone.

#import <UIKit/UIKit.h>

extern "C" void *LawnGetIOSMetalLayer(void *theUIWindow)
{
    if (theUIWindow == nullptr)
    {
        return nullptr;
    }
    UIWindow *aWindow = (__bridge UIWindow *)theUIWindow;
    UIViewController *aController = aWindow.rootViewController;
    if (aController == nil)
    {
        return nullptr;
    }
    UIView *aView = aController.view;
    if (aView == nil)
    {
        return nullptr;
    }
    return (__bridge void *)aView.layer;
}
