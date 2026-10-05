// SettingsDialog -- reconstructed from libGameMain.so.
//
// Stage 2: the bodies are the IDA decompilation rewritten as ordinary C++
// against the reconstructed headers. Every offset, vtable slot and virtual call
// was resolved against the decompilation at the address in each section banner.

#include "SettingsDialog.h"

#include "Dialog.h"
#include "GameSlider.h"
#include "Graphics.h"
#include "Image.h"
// The label font is an ImageFont and the scale is set through its own slot 28,
// which Sexy::Font does not declare.
#include "ImageFont.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "PlayerInfo.h"
#include "SexyAppBase.h"
#include "TodFoley.h"
#include "WidgetManager.h"
// ==========================================================================
// SettingsDialog::GetPreferredHeight(int)
// @00168054  14 bytes
// ==========================================================================
int SettingsDialog::GetPreferredHeight(int theWidth)
{
  // The dialog's preferred height is the options background's width: the art is
  // square and the dialog is drawn on it. The binary reads the image's +0x24
  // (Image::mWidth) directly.
  return (IMAGE_OPTIONS_MENUBACK)->GetWidth();
}

// ==========================================================================
// SettingsDialog::CheckboxChecked(int,bool)
// @0016806C  2 bytes
// ==========================================================================
void SettingsDialog::CheckboxChecked(int theId, bool theChecked)
{
  // Empty in the binary too: this dialog has no checkboxes, but CheckboxListener
  // is one of its bases, so the override exists.
}

// ==========================================================================
// SettingsDialog::ButtonPress(int)
// @00168078  28 bytes
// ==========================================================================
void SettingsDialog::ButtonPress(int theId)
{
  // Slot 169 (byte 0x2A4) of LawnApp's vtable is LawnApp::PlaySample(int, bool),
  // which HIDES SexyAppBase::PlaySample(int). The click sound is the same for
  // every button, so the id is ignored.
  mApp->PlaySample(SOUND_GRAVEBUTTON, true);
}

// ==========================================================================
// SettingsDialog::~SettingsDialog()
// @001681B4  100 bytes
// ==========================================================================
SettingsDialog::~SettingsDialog()
{
  // The binary frees each widget through slot 1 of its vtable (the deleting
  // destructor) in this order: sound slider, music slider, back button, device
  // button. The four vptr stores it also performs are the compiler's own and are
  // left to the implicit base destructor call.
  delete mSoundSlider;
  delete mMusicSlider;
  delete mBackButton;
  delete mSelectDeviceButton;
}

// ==========================================================================
// SettingsDialog::GameButtonDown(Sexy::GamepadButton,int,unsigned int)
// @00168268  34 bytes
// ==========================================================================
void SettingsDialog::GameButtonDown(int theButton, int thePlayerIndex, unsigned int theFlags)
{
  // 5 is GAMEPAD_BUTTON_START and 7 is GAMEPAD_BUTTON_B; they differ only in
  // bit 1, so clearing that bit folds the two into one test. Either one leaves
  // the dialog the same way the back button does, unless the profile save has
  // already failed or a modifier is held down.
  if ( !mSaveFailed && (theButton == 5 || theButton == 7) && (theFlags & 1) == 0 )
    ButtonDepress(3);
}

// ==========================================================================
// SettingsDialog::SliderVal(int,double)
// @00168290  122 bytes
// ==========================================================================
void SettingsDialog::SliderVal(int theId, double theValue)
{
  // Slot 85 (byte 0x154) of the app's vtable is SetMusicVolume, slot 86 (byte
  // 0x158) is SetSfxVolume. The music slider's range is 0..1; the sound slider's
  // is 0..1.5384616, so its value is scaled back down by 0.65 on the way in.
  if ( theId == 0 )
  {
    mApp->SetMusicVolume(theValue);
    mApp->mSoundSystem->RehookupSoundWithMusicVolume();
    mApp->mPlayerInfo->mMusicVolume = theValue;
  }
  else if ( theId == 1 )
  {
    mApp->SetSfxVolume(theValue * 0.65);
    mApp->mSoundSystem->RehookupSoundWithMusicVolume();
    mApp->mPlayerInfo->mSoundVolume = theValue;
  }
}

// ==========================================================================
// SettingsDialog::SettingsDialog(LawnApp *)
// @001686B0  1074 bytes
// ==========================================================================
SettingsDialog::SettingsDialog(LawnApp * theApp)
  : LawnDialog(theApp, nullptr, 4, true, "[SETTINGS]", "", "", 0)
{
  // The base constructor runs first, exactly as it does in the binary. The three
  // libstdc++ COW string constructions (Sexy::StringCtor @0x2ED3A4) and the
  // refcount cleanups that followed them are the temporaries for "[SETTINGS]" and
  // two empty strings, which the base's by-reference parameters now take as
  // literals. The four vptr stores the dump carries afterwards are the compiler's
  // own: this class's primary table, and the ButtonListener (+0x100),
  // SliderListener (+0x2FC) and CheckboxListener (+0x300) sub-objects.
  mApp = theApp;

  Sexy::Color aBackColor(255, 255, 100);
  SetColor(3, aBackColor);

  // MakeButton takes the ButtonListener sub-object and the Widget sub-object; the
  // binary's `this + 256` is the ButtonListener one, which is the implicit base
  // conversion of `this`.
  mBackButton = (GameButton *)MakeButton(3, this, this, Sexy::String("[BACK]"));

  // The authoritative table types both sliders `Sexy::Widget *`, so the
  // GameSlider-typed local is what carries the SetValue call.
  GameSlider *aMusicSlider = new GameSlider(IMAGE_OPTIONS_SLIDERSLOT,
                                            IMAGE_OPTIONS_SLIDERKNOB2,
                                            0, this, nullptr);
  mMusicSlider = aMusicSlider;
  // Slot 81 (byte 0x144) of the app's vtable is GetMusicVolume. The music slider's
  // range is 0..1, so the volume is clamped into it before being handed over.
  double aMusicVolume = mApp->GetMusicVolume();
  if ( aMusicVolume > 1.0 )
    aMusicVolume = 1.0;
  if ( aMusicVolume < 0.0 )
    aMusicVolume = 0.0;
  aMusicSlider->SetValue(aMusicVolume);

  GameSlider *aSoundSlider = new GameSlider(IMAGE_OPTIONS_SLIDERSLOT,
                                            IMAGE_OPTIONS_SLIDERKNOB2,
                                            1, this, nullptr);
  mSoundSlider = aSoundSlider;
  // Slot 82 (byte 0x148) is GetSfxVolume. The sound slider's range is 0..1.5384616
  // (1 / 0.65), which is what SliderVal scales back down on the way out.
  aSoundSlider->SetValue(mApp->GetSfxVolume() * 1.53846159);

  mSelectDeviceButton = (GameButton *)MakeButton(2, this, this, Sexy::String("[SELECT_DEVICE]"));

  // Slot 59 (byte 0xEC) is Widget::SetFocusLinks(up, down, left, right) and slot 42
  // (byte 0xA8) is Widget::SetVisible. The device button only exists in the full
  // build, so it starts hidden and the chain is rewired around it below.
  mSelectDeviceButton->SetFocusLinks(mSoundSlider, mBackButton, nullptr, nullptr);
  mSelectDeviceButton->SetVisible(false);
  mMusicSlider->SetFocusLinks(mBackButton, mSoundSlider, nullptr, nullptr);

  if ( mApp->IsFullVersion() )
  {
    mBackButton->SetFocusLinks(mSelectDeviceButton, mMusicSlider, nullptr, nullptr);
    mSoundSlider->SetFocusLinks(mMusicSlider, mSelectDeviceButton, nullptr, nullptr);
  }
  else
  {
    mBackButton->SetFocusLinks(mSoundSlider, mMusicSlider, nullptr, nullptr);
    mSoundSlider->SetFocusLinks(mMusicSlider, mBackButton, nullptr, nullptr);
  }

  // The label font is the one "[MUSIC]" and "[SOUND_FX]" are drawn with; its scale
  // comes from the OPTION_DLG_LABEL_FONT_SCALE property and defaults to 1.
  ((Sexy::ImageFont *)FONT_DWARVENTODCRAFT18)->SetScale(
      mApp->GetDouble("OPTION_DLG_LABEL_FONT_SCALE", 1.0));

  mSaveFailed = false;
}

// ==========================================================================
// SettingsDialog::AddedToManager(Sexy::WidgetManager *)
// @00168BEC  68 bytes
// ==========================================================================
void SettingsDialog::AddedToManager(Sexy::WidgetManager * theWidgetManager)
{
  Sexy::Dialog::AddedToManager(theWidgetManager);
  // Slot 6 (byte 0x18) of the container's vtable is WidgetContainer::AddWidget.
  // The device button is only added when the build has one.
  AddWidget(mBackButton);
  AddWidget(mSoundSlider);
  AddWidget(mMusicSlider);
  if ( mApp->IsFullVersion() )
    AddWidget(mSelectDeviceButton);
}

// ==========================================================================
// SettingsDialog::RemovedFromManager(Sexy::WidgetManager *)
// @00168C78  58 bytes
// ==========================================================================
void SettingsDialog::RemovedFromManager(Sexy::WidgetManager * theWidgetManager)
{
  Sexy::Dialog::RemovedFromManager(theWidgetManager);
  // Slot 7 (byte 0x1C) is WidgetContainer::RemoveWidget. Unlike AddedToManager
  // this one is unconditional, so it stays symmetric with the constructor.
  RemoveWidget(mBackButton);
  RemoveWidget(mSoundSlider);
  RemoveWidget(mMusicSlider);
  RemoveWidget(mSelectDeviceButton);
}

// ==========================================================================
// SettingsDialog::Resize(int,int,int,int)
// @00168E1C  116 bytes
// ==========================================================================
void SettingsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
  Sexy::Dialog::Resize(theX, theY, theWidth, theHeight);
  // Slot 50 (byte 0xC8) of each widget's vtable is Widget::Resize. The three rows
  // are stacked 65 pixels apart, starting 140 pixels down; the back button sits
  // below all of them.
  mMusicSlider->Resize(70, 140, 264, 46);
  mSoundSlider->Resize(70, 205, 264, 40);
  mSelectDeviceButton->Resize(70, 270, 264, 48);
  mBackButton->Resize(70, 371, 264, 48);
}

// ==========================================================================
// SettingsDialog::Update()
// @00168E94  54 bytes
// ==========================================================================
void SettingsDialog::Update()
{
  Sexy::Dialog::Update();
}

// ==========================================================================
// SettingsDialog::ButtonDepress(int)
// @00168F34  48 bytes
// ==========================================================================
void SettingsDialog::ButtonDepress(int theId)
{
  Sexy::Dialog::ButtonDepress(theId);
  if ( theId == 3 )
  {
    // Slot 9 (byte 0x24) of the profile's vtable is SaveDetails, which returns
    // whether every userdata file was written. Its result is what blocks the
    // deferred close in Update and the gamepad back button in GameButtonDown.
    mSaveFailed = !mApp->mPlayerInfo->SaveDetails();
    // 1000 is the dialog's standard "close me" id.
    Sexy::Dialog::ButtonDepress(1000);
  }
}

// ==========================================================================
// SettingsDialog::Draw(Sexy::Graphics *)
// @00168F8C  290 bytes
// ==========================================================================
void SettingsDialog::Draw(Sexy::Graphics * theGraphics)
{
  LawnDialog::Draw(theGraphics);

  // The two labels are grey until their slider takes the focus, then green.
  // Widget's +0x77 (mHasFocus) is what the binary tests.
  Sexy::Color aLabelColor(107, 109, 145);
  Sexy::Color aFocusColor(100, 255, 100);

  TodDrawString(theGraphics, Sexy::String("[MUSIC]"), 206, 142, FONT_DWARVENTODCRAFT18,
                mMusicSlider->mHasFocus ? aFocusColor : aLabelColor, 2);
  TodDrawString(theGraphics, Sexy::String("[SOUND_FX]"), 206, 195, FONT_DWARVENTODCRAFT18,
                mSoundSlider->mHasFocus ? aFocusColor : aLabelColor, 2);
}

// ==========================================================================
// SettingsDialog::GotFocus()
// @001690F8  34 bytes
// ==========================================================================
void SettingsDialog::GotFocus()
{
  LawnDialog::GotFocus();
  // +0xDC of the widget is mFocusedChildWidget. Slot 57 (byte 0xE4) is
  // Widget::SetFocus, so the back button takes the focus as soon as the dialog
  // has no focused child of its own.
  if ( mFocusedChildWidget == nullptr )
    SetFocus(mBackButton, true);
}

// ==========================================================================
// SettingsDialog::KeyDown(Sexy::KeyCode)
// @0016911C  26 bytes
// ==========================================================================
int SettingsDialog::KeyDown(Sexy::KeyCode theKey)
{
  // 27 is KEYCODE_ESCAPE; it leaves through the same path as the back button
  // (1001 is the dialog's "cancelled" id). Everything else is the base's.
  if ( theKey != 27 )
    return Sexy::Dialog::KeyDown(theKey);
  Sexy::Dialog::ButtonDepress(1001);
  return 1;
}
