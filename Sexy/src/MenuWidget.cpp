// MenuWidget -- reconstructed from libGameMain.so.
//
// Every raw this-relative offset, vptr store and hand-written vtable slot call
// the transcription carried is gone: each member access below is the named
// member the header recovered and each virtual dispatch is an ordinary call.

#include "MenuWidget.h"

#include "ButtonListener.h"
#include "Graphics.h"
#include "Image.h"
#include "MenuParser.h"
#include "SexyAppBase.h"
#include "Widget.h"
#include "WidgetAnimator.h"
#include "WidgetContainer.h"
#include "WidgetManager.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// ==========================================================================
// Sexy::MenuWidget::~MenuWidget()
// @001691AC  132 bytes
// ==========================================================================
Sexy::MenuWidget::~MenuWidget()
{
  // The two vptr stores, the mSourceFileName release and the Widget destructor
  // are the compiler's. The parser is reference counted, so its release is
  // explicit -- the dump decrements +0x4C and, when the count reaches zero,
  // deletes it through the parser's vtable.
  Sexy::MenuParser *aParser = mMenuParser;
  if ( aParser != NULL )
  {
    if ( --aParser->mRefCount <= 0 )
      delete aParser;
  }
}

// ==========================================================================
// Sexy::MenuWidget::RetainWidgetsOnExit()
// @00169138  4 bytes
// ==========================================================================
bool Sexy::MenuWidget::RetainWidgetsOnExit()
{
  return false;
}

// ==========================================================================
// Sexy::MenuWidget::OnExit()
// @0016913C  2 bytes
// ==========================================================================
void Sexy::MenuWidget::OnExit()
{
  // An empty override in the binary too: it is the hook subclasses use.
}

// ==========================================================================
// Sexy::MenuWidget::UpdateExit()
// @002AF1A0  38 bytes
// ==========================================================================
bool Sexy::MenuWidget::UpdateExit()
{
  // The widget list is circular and its header node is mWidgets' own first
  // link, so the walk ends when the node pointer comes back to that address.
  // The exit is finished as soon as no child still has an animator running.
  const void *aHeader = &mWidgets.mNext;

  for ( WidgetListLink *aNode = mWidgets.mNext; aNode != aHeader; aNode = aNode->mNext )
  {
    if ( static_cast<WidgetListNode *>(aNode)->mWidget->GetAnimator() != NULL )
      return false;
  }

  return true;
}

// ==========================================================================
// Sexy::MenuWidget::Draw(Sexy::Graphics *)
// @002AF1C8  60 bytes
// ==========================================================================
void Sexy::MenuWidget::Draw(Sexy::Graphics * theGraphics)
{
  if ( mImage != NULL )
    theGraphics->DrawImage(mImage, mWidth / 2 - mImage->mWidth / 2, mHeight / 2 - mImage->mHeight / 2);
}

// ==========================================================================
// Sexy::MenuWidget::Enter()
// @002AF204  196 bytes
// ==========================================================================
void Sexy::MenuWidget::Enter()
{
  // Vtable byte 8 of a WidgetAnimator is its slot [2], Init. The menu itself
  // takes the entry animator (state 1), or the idle one (state 0), or none.
  WidgetAnimator *aAnimator = GetAnimatorForState(Sexy::WidgetAnimationState(1));
  if ( aAnimator != NULL )
  {
    aAnimator->Init(Sexy::WidgetAnimationState(1));
    SetAnimator(aAnimator);
  }
  else
  {
    WidgetAnimator *aIdleAnimator = GetAnimatorForState(Sexy::WidgetAnimationState(0));
    if ( aIdleAnimator != NULL )
    {
      aIdleAnimator->Init(Sexy::WidgetAnimationState(0));
      SetAnimator(aIdleAnimator);
    }
    else
    {
      SetAnimator(NULL);
    }
  }

  // Every child is enabled and gets the same treatment.
  const void *aHeader = &mWidgets.mNext;
  for ( WidgetListLink *aNode = mWidgets.mNext; aNode != aHeader; aNode = aNode->mNext )
  {
    Widget *aWidget = static_cast<WidgetListNode *>(aNode)->mWidget;
    aWidget->SetDisabled(false);

    WidgetAnimator *aChildAnimator = aWidget->GetAnimatorForState(Sexy::WidgetAnimationState(1));
    if ( aChildAnimator != NULL )
    {
      aChildAnimator->Init(Sexy::WidgetAnimationState(1));
      aWidget->SetAnimator(aChildAnimator);
    }
    else
    {
      WidgetAnimator *aIdle = aWidget->GetAnimatorForState(Sexy::WidgetAnimationState(0));
      if ( aIdle == NULL )
      {
        aWidget->SetAnimator(NULL);
      }
      else
      {
        // The dump asks for the idle animator a second time and installs that
        // second answer; a plain getter, so the two are the same object.
        aIdle->Init(Sexy::WidgetAnimationState(0));
        aWidget->SetAnimator(aWidget->GetAnimatorForState(Sexy::WidgetAnimationState(0)));
      }
    }
  }

  mExitFinished = false;
}

// ==========================================================================
// Sexy::MenuWidget::Exit()
// @002AF2C8  138 bytes
// ==========================================================================
void Sexy::MenuWidget::Exit()
{
  if ( mIsFading )
    return;

  // The exit animator is state 2, and it is installed without being Init'ed
  // first -- unlike Enter's, which is.
  WidgetAnimator *aAnimator = GetAnimatorForState(Sexy::WidgetAnimationState(2));
  if ( aAnimator != NULL )
    SetAnimator(aAnimator);
  else
    SetAnimator(NULL);

  const void *aHeader = &mWidgets.mNext;
  for ( WidgetListLink *aNode = mWidgets.mNext; aNode != aHeader; aNode = aNode->mNext )
  {
    Widget *aWidget = static_cast<WidgetListNode *>(aNode)->mWidget;
    aWidget->SetDisabled(true);

    WidgetAnimator *aChildAnimator = aWidget->GetAnimatorForState(Sexy::WidgetAnimationState(2));
    if ( aChildAnimator != NULL )
    {
      aChildAnimator->Init(Sexy::WidgetAnimationState(2));
      aWidget->SetAnimator(aChildAnimator);
    }
    else
    {
      aWidget->SetAnimator(NULL);
    }
  }

  mIsFading = true;
}

// ==========================================================================
// Sexy::MenuWidget::CreateMenuParser(void)
// @002AF354  96 bytes
// ==========================================================================
int Sexy::MenuWidget::CreateMenuParser()
{
  // Drop the parser this widget already holds; the count is the parser's own
  // and reaching zero frees it.
  if ( mMenuParser != NULL )
  {
    if ( --mMenuParser->mRefCount <= 0 )
      delete mMenuParser;

    mMenuParser = NULL;
  }

  Sexy::MenuParser *aParser = new MenuParser(this);
  if ( aParser != mMenuParser )
  {
    if ( mMenuParser != NULL )
    {
      if ( --mMenuParser->mRefCount <= 0 )
        delete mMenuParser;
    }

    // The widget takes its own reference on the new parser.
    if ( aParser != NULL )
      ++aParser->mRefCount;

    mMenuParser = aParser;
  }

  // The dump hands back the pointer it read before the new parser was adopted,
  // which the release above has already cleared. LoadMenuFile, the only caller,
  // discards it.
  return 0;
}

// ==========================================================================
// Sexy::MenuWidget::MenuWidget()
// @002AF3D8  116 bytes
// ==========================================================================
Sexy::MenuWidget::MenuWidget()
{
  // Widget's constructor, the two vptr stores (the primary table and the
  // ButtonListener sub-object's at +0x100) and mSourceFileName's construction
  // are the compiler's.
  mMenuParser = NULL;
  mImage = NULL;
  mWidgetManager = NULL;
  mInitialFocusId = -1;

  // The widget covers the whole screen to begin with.
  Resize(((SexyAppBase *)gSexyAppBase)->mScreenBounds);

  // The dump seeds the member with the shared empty rep and then assigns the
  // empty C string at 0x57D7EA over it; both produce the empty string.
  mSourceFileName = "";

  mIsFading = false;
  mExitFinished = false;
}

// ==========================================================================
// Sexy::MenuWidget::MenuWidget(Sexy::String)
// @002AF490  120 bytes
// ==========================================================================
Sexy::MenuWidget::MenuWidget(Sexy::String theFileName)
{
  mMenuParser = NULL;
  mImage = NULL;
  mWidgetManager = NULL;
  mInitialFocusId = -1;

  Resize(((SexyAppBase *)gSexyAppBase)->mScreenBounds);

  mSourceFileName = theFileName;

  mIsFading = false;
  mExitFinished = false;
}

// ==========================================================================
// Sexy::MenuWidget::LoadMenuFile(Sexy::String)
// @002AF548  62 bytes
// ==========================================================================
int Sexy::MenuWidget::LoadMenuFile(Sexy::String theFileName)
{
  // Vtable byte 44 is WidgetContainer's slot [11], RemoveAllWidgets: the old
  // menu's widgets are deleted, recursively, before the new file is parsed.
  RemoveAllWidgets(true, true, NULL);

  mSourceFileName = theFileName;

  Sexy::MenuParser *aParser = mMenuParser;
  if ( aParser == NULL )
  {
    // Vtable byte 508 is this class's own slot [127], CreateMenuParser.
    CreateMenuParser();
    aParser = mMenuParser;
  }

  // Vtable byte 52 is DescParser::LoadDescriptor, slot [13].
  return aParser->LoadDescriptor(mSourceFileName);
}

// ==========================================================================
// Sexy::MenuWidget::Update()
// @002AF588  96 bytes
// ==========================================================================
void Sexy::MenuWidget::Update()
{
  Widget::Update();

  if ( !mIsFading || mExitFinished )
    return;

  // Vtable bytes 488, 500 and 504 are this class's own slots [122] UpdateExit,
  // [125] RetainWidgetsOnExit and [126] OnExit.
  if ( !UpdateExit() )
    return;

  if ( !RetainWidgetsOnExit() )
  {
    mWidgetManager->RemoveWidget(this);
    ((SexyAppBase *)gSexyAppBase)->SwitchScreenMode();
  }

  mExitFinished = true;
  OnExit();
}

// ==========================================================================
// Sexy::MenuWidget::DrawAll(Sexy::ModalFlags *,Sexy::Graphics *)
// @002AF5F0  52 bytes
// ==========================================================================
void Sexy::MenuWidget::DrawAll(Sexy::ModalFlags * theFlags, Sexy::Graphics * theGraphics)
{
  const void *aHeader = &mWidgets.mNext;
  for ( WidgetListLink *aNode = mWidgets.mNext; aNode != aHeader; aNode = aNode->mNext )
    static_cast<WidgetListNode *>(aNode)->mWidget->DrawPre(theGraphics);

  WidgetContainer::DrawAll(theFlags, theGraphics);
}

// ==========================================================================
// Sexy::MenuWidget::AddedToManager(Sexy::WidgetManager *)
// @002AF624  138 bytes
// ==========================================================================
void Sexy::MenuWidget::AddedToManager(Sexy::WidgetManager * theManager)
{
  mWidgetManager = theManager;

  if ( !mSourceFileName.empty() )
  {
    // The dump copy-constructs the name before handing it over, so a failure
    // below still reports the original.
    String aFileName(mSourceFileName);
    if ( !LoadMenuFile(aFileName) )
      OutputDebug("Error in menu file '%s'\n", mSourceFileName.c_str());
  }

  WidgetContainer::AddedToManager(theManager);

  // Vtable byte 492 is this class's own slot [123], Enter.
  Enter();
}

// ==========================================================================
// Sexy::MenuWidget::RemovedFromManager(Sexy::WidgetManager *)
// @002AF6C8  26 bytes
// ==========================================================================
void Sexy::MenuWidget::RemovedFromManager(Sexy::WidgetManager * theManager)
{
  WidgetContainer::RemovedFromManager(theManager);
  RemoveAllWidgets(true, true, theManager);
}

// ==========================================================================
// Sexy::MenuWidget::GotFocus()
// @002AF6E4  4 bytes
// ==========================================================================
void Sexy::MenuWidget::GotFocus()
{
  Widget::GotFocus();
}

// ==========================================================================
// Sexy::MenuWidget::AddWidget(Sexy::Widget *)
// @002AF6E8  4 bytes
// ==========================================================================
void Sexy::MenuWidget::AddWidget(Sexy::Widget * theWidget)
{
  WidgetContainer::AddWidget(theWidget);
}
