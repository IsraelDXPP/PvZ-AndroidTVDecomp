// MessageWidget -- reconstructed from libGameMain.so.
//
// The bodies below are the binary's own code, rewritten in ordinary C++:
//
//   * every `this + N` word access is the named member of MessageWidget.h;
//   * the hand-written vtable-slot calls are real virtual calls
//     (`Font::StringWidthUnicode`, `Font::CharWidth`, `Font::GetLineSpacing`);
//   * the libstdc++ COW-string refcount dance the dump spells out at every
//     temporary (`v - 12`, `v - 4`, `__ldrex`/`__strex`, `operator delete`) is
//     gone: `Sexy::String` / `std::wstring` own their storage and the compiler
//     emits the same release;
//   * the constructor no longer stores a recon vtable address (the vptr is the
//     compiler's) and registers its synchronised block through
//     `mSyncBlocks.push_back` instead of `vector::_M_insert_aux`.
//
// Signatures in MessageWidget.h corrected against the binary (and against the
// sibling class in SimpleMessageWidget.h, which spells these the same way):
// GetFont returns Sexy::Font *, and LayoutReanimText / Update /
// DrawReanimatedText are void. mTextReanimID holds ReanimationID values, not
// Reanimation pointers -- the constructor zeroes it, the writers store
// `mApp->ReanimationGetID(...)` into it and the readers hand it straight back to
// `mApp->ReanimationTryToGet(ReanimationID)`.

#include "MessageWidget.h"

#include "game/Board.h"
#include "Font.h"
#include "FontData.h"
#include "Graphics.h"
#include "LawnApp.h"
#include "NativeDisplay.h"
#include "Reanimation.h"
#include "ReanimatorTransform.h"
#include "SexyAppBase.h"

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

namespace
{

std::vector<int> MessageCharacters(const char *theLabel)
{
    Sexy::String aText(theLabel);
    std::vector<int> aCharacters;
    unsigned int aByteIndex = 0;
    while (aByteIndex < aText.size())
        aCharacters.push_back(ReadCodePoint(aText, aByteIndex));
    return aCharacters;
}

} // namespace

// ==========================================================================
// MessageWidget::~MessageWidget()
// @001A3B90  40 bytes
// ==========================================================================
MessageWidget::~MessageWidget()
{
    // ClearReanim detaches the reanimation this widget is showing, if any. The
    // only other pointer the binary releases is SyncObject's own mSyncBlocks
    // vector, which the base destructor handles.
    ClearReanim();
}

// ==========================================================================
// MessageWidget::ClearReanim()
// @001A3B68  40 bytes
// ==========================================================================
Reanimation *MessageWidget::ClearReanim()
{
    Reanimation *aResult = nullptr;

    for (int i = 0; i < MAX_MESSAGE_LENGTH; ++i)
    {
        aResult = mApp->ReanimationTryToGet(mTextReanimID[i]);
        if (aResult != nullptr)
        {
            aResult->ReanimationDie();
            mTextReanimID[i] = REANIMATIONID_NULL;
        }
    }
    return aResult;
}

// ==========================================================================
// MessageWidget::ClearLabel()
// @001A3BEC  40 bytes
// ==========================================================================
MessageWidget *MessageWidget::ClearLabel()
{
    if (mReanimType == REANIM_NONE)
    {
        mDuration = 0;
    }
    else
    {
        // Clamp the remaining display time so the label still gets its slide-off
        // window.
        int aSlideOffDeadline = mSlideOffTime + 101;
        if (mDuration > aSlideOffDeadline)
            mDuration = aSlideOffDeadline;
    }
    return this;
}

// ==========================================================================
// MessageWidget::GetFont()
// @001A3C14  54 bytes
// ==========================================================================
Sexy::Font *MessageWidget::GetFont()
{
    // The binary shifts 1 by (mMessageStyle - 1) and tests the result against two
    // constant masks, which is a switch over the styles compiled to a bit test:
    // 0x37FFF selects the big House of Terror face (every style in 1..18 except
    // SLOT_MACHINE), and the second test -- bit 15 -- is SLOT_MACHINE itself.
    unsigned int aStyleBitIndex = (unsigned int)mMessageStyle - 1;
    if (aStyleBitIndex > 0x11)
        return nullptr;

    unsigned int aStyleBit = 1u << aStyleBitIndex;
    if ((aStyleBit & 0x37FFFu) != 0)
        return FONT_HOUSEOFTERROR28;
    if ((aStyleBit & 0x8000u) != 0)
        return FONT_HOUSEOFTERROR16;
    return nullptr;
}

// ==========================================================================
// MessageWidget::IsBeingDisplayed()
// @001A3C58  12 bytes
// ==========================================================================
bool MessageWidget::IsBeingDisplayed()
{
    return mDuration != 0;
}

// ==========================================================================
// MessageWidget::MessageWidget(LawnApp *)
// @001A3CF8  134 bytes
// ==========================================================================
MessageWidget::MessageWidget(LawnApp *theApp)
{
    // The object synchronises its own derived state: one block running from
    // mLabel to the end of the object. The binary builds the descriptor on the
    // stack, pushes it into the SyncObject base's vector (the dump's
    // `_M_insert_aux` at `this + 4`), and then zero-fills the same range.
    const size_t aStateSize = MakeSyncBlock(mLabel, mMessageStyleNext).mSize;
    mSyncBlocks.push_back(SyncBlockInfo{&mLabel, aStateSize});
    memset(mLabel, 0, (size_t)aStateSize);

    mApp = theApp;
    mDuration = 0;
    mMessageStyle = MESSAGE_STYLE_OFF;
    mReanimType = REANIM_NONE;
    mSlideOffTime = 100;
    mFontScale = 1.0f;
    mMessageStyleNext = MESSAGE_STYLE_OFF;

    for (int i = 0; i < MAX_MESSAGE_LENGTH; ++i)
        mTextReanimID[i] = REANIMATIONID_NULL;
}

// ==========================================================================
// MessageWidget::LayoutReanimText()
// @001A3E2C  778 bytes
// ==========================================================================
void MessageWidget::LayoutReanimText()
{
    // TV 0x1A3E2C lays out decoded characters, one animation per character.
    Sexy::Font *aFont = GetFont();
    std::vector<int> aCharacters = MessageCharacters(mLabel);

    mSlideOffTime = (int)aCharacters.size() + 100;

    // Measure every '\n'-separated line: the reanimated glyphs are laid out one
    // line at a time and each line is centred on its own width.
    std::vector<float> aLineWidths;
    int aLineStart = 0;

    int aTextLength = (int)aCharacters.size();
    for (int i = 0; i <= aTextLength; ++i)
    {
        if (i < aTextLength && aCharacters[i] != '\n')
            continue;

        Sexy::String aLine =
            CodePointsToUtf8(std::vector<int>(aCharacters.begin() + aLineStart, aCharacters.begin() + i));
        aLineWidths.push_back((float)aFont->StringWidth(aLine) * mFontScale);
        aLineStart = i + 1;
    }

    // The first line starts at the left edge of the screen, shifted 120px further
    // left while the app is not in its wide layout.
    float aLineX = -(aLineWidths[0] * 0.5f);
    if (!mApp->mPlatformDriverOrQueue->mWideScreen)
        aLineX -= 120.0f;

    if (aTextLength <= 0)
        return;

    // One Reanimation per glyph; the pen advances by the glyph's own width and
    // every '\n' restarts it on the next line's centred position.
    float aLineY = 0.0f;
    int aLineIndex = 0;
    for (int i = 0;;)
    {
        Reanimation *aReanim = mApp->AddReanimation(aLineX, aLineY, 0, mReanimType);
        aReanim->mIsAttachment = true;
        aReanim->PlayReanim("anim_enter", REANIM_PLAY_ONCE_AND_HOLD, 0, 0.0);
        mTextReanimID[i] = mApp->ReanimationGetID(aReanim);

        aLineX += (float)aFont->CharWidth(aCharacters[i]) * mFontScale;

        if (aCharacters[i] == '\n')
        {
            ++aLineIndex;
            aLineX = -(aLineWidths[aLineIndex] * 0.5f);
            ++i;
            aLineY += (float)aFont->GetLineSpacing() * mFontScale;
        }
        else
        {
            ++i;
        }

        if (i >= aTextLength)
            break;
    }
}

// ==========================================================================
// MessageWidget::SetLabel(Sexy::String const&,MessageStyle)
// @001A4178  430 bytes
// ==========================================================================
void MessageWidget::SetLabel(Sexy::String const &theLabel, MessageStyle theStyle)
{
    Sexy::String aTranslated;
    TodStringTranslate(&aTranslated, theLabel);

    if (mReanimType != REANIM_NONE && mDuration > 0)
    {
        // A message is already up: queue this one and shorten the current one so it
        // slides off before the new label starts.
        mMessageStyleNext = theStyle;
        strcpy(mLabelNext, aTranslated.c_str());

        int aSlideOffDeadline = mSlideOffTime + 101;
        if (mDuration > aSlideOffDeadline)
            mDuration = aSlideOffDeadline;
        return;
    }

    ClearReanim();
    strcpy(mLabel, aTranslated.c_str());
    mMessageStyle = theStyle;
    mReanimType = REANIM_NONE;

    // How long the label stays up, by style. The binary selects these with a chain
    // of bit tests over `a3`; the switch below is that chain's result.
    bool aKeepsFontScale = false;
    switch ((int)theStyle)
    {
        case MESSAGE_STYLE_TUTORIAL_LEVEL1:
        case MESSAGE_STYLE_TUTORIAL_LEVEL2:
        case MESSAGE_STYLE_TUTORIAL_LATER:
        case MESSAGE_STYLE_HINT_FAST:
        case MESSAGE_STYLE_HINT_TALL_FAST:
        case MESSAGE_STYLE_HINT_TALL_UNLOCKMESSAGE:
        case MESSAGE_STYLE_BIG_MIDDLE_FAST:
            mDuration = 500;
            break;

        case MESSAGE_STYLE_TUTORIAL_LEVEL1_STAY:
        case MESSAGE_STYLE_TUTORIAL_LATER_STAY:
        case MESSAGE_STYLE_HINT_STAY:
        case MESSAGE_STYLE_HINT_TALL_STAY:
            mDuration = 10000;
            break;

        case MESSAGE_STYLE_HINT_LONG:
        case MESSAGE_STYLE_HINT_TALL_LONG:
        case MESSAGE_STYLE_BIG_MIDDLE:
        case MESSAGE_STYLE_ZEN_GARDEN_LONG:
            mDuration = 1500;
            break;

        case MESSAGE_STYLE_HOUSE_NAME:
            mDuration = 250;
            break;

        case MESSAGE_STYLE_HUGE_WAVE:
            mDuration = 750;
            // 98 is the "text fades on" reanimation this style spells its letters with.
            mReanimType = REANIM_TEXT_FADE_ON;
            if (mApp->GetLocale().compare("zh_CN") == 0)
                mFontScale = 1.6f;
            LayoutReanimText();
            // This branch jumps past the scale reset below, so the 1.6f it just wrote
            // (or the 1.0f from the constructor) survives.
            aKeepsFontScale = true;
            break;

        case MESSAGE_STYLE_SLOT_MACHINE:
            mDuration = 750;
            break;

        default:
            break;
    }

    if (!aKeepsFontScale)
        mFontScale = 1.0f;

    mDisplayTime = mDuration;
}

// ==========================================================================
// MessageWidget::Update()
// @001A4350  364 bytes
// ==========================================================================
void MessageWidget::Update()
{
    Board *aBoard = mApp->mBoard;
    if (aBoard == nullptr || aBoard->mPaused)
        return;

    if (mDuration > 0 && mDuration < 10000)
    {
        --mDuration;
        if (mDuration == 0)
        {
            MessageStyle aNextStyle = mMessageStyleNext;
            mMessageStyle = MESSAGE_STYLE_OFF;
            if (aNextStyle != MESSAGE_STYLE_OFF)
            {
                // The queued label is promoted now that the current one has expired.
                Sexy::String aNextLabel(mLabelNext);
                SetLabel(aNextLabel, aNextStyle);
                mMessageStyleNext = MESSAGE_STYLE_OFF;
            }
        }
    }

    int aCharCount = (int)MessageCharacters(mLabel).size();
    if (aCharCount <= 0)
        return;

    for (int i = 0;;)
    {
        Reanimation *aReanim = mApp->ReanimationTryToGet(mTextReanimID[i]);
        if (aReanim == nullptr)
            return;

        if (mReanimType == REANIM_TEXT_FADE_ON)
        {
            // The huge-wave text holds its fade-on rate while it is still sliding in,
            // and only starts counting down once the label has begun to leave.
            if (mDuration > mSlideOffTime)
            {
                aReanim->SetAnimRate(60.0);
            }
            else
            {
                if (mDuration == mSlideOffTime)
                    aReanim->PlayReanim("anim_leave", REANIM_PLAY_ONCE_AND_HOLD, 0, 0.0);
                aReanim->SetAnimRate(
                    TodAnimateCurveFloat(0, 50, 100 * (mSlideOffTime - mDuration) - i, 0.0f, 40.0f, 1));
            }
        }
        else if (mDuration <= mSlideOffTime)
        {
            if (mDuration == mSlideOffTime)
                aReanim->PlayReanim("anim_leave", REANIM_PLAY_ONCE_AND_HOLD, 0, 0.0);
            aReanim->SetAnimRate(TodAnimateCurveFloat(0, 50, (mSlideOffTime - mDuration) - i, 0.0f, 40.0f, 1));
        }
        else
        {
            aReanim->SetAnimRate(TodAnimateCurveFloat(0, 50, mDisplayTime - mDuration - i, 0.0f, 40.0f, 1));
        }

        ++i;
        aReanim->Update();
        if (i >= aCharCount)
            return;
    }
}

// ==========================================================================
// MessageWidget::DrawReanimatedText(Sexy::Graphics *,Sexy::Font *,Sexy::Color const&,float)
// @001A44E0  642 bytes
// ==========================================================================
void MessageWidget::DrawReanimatedText(
    Sexy::Graphics *theGraphics, Sexy::Font *theFont, Color const &theColor, float thePos)
{
    std::vector<int> aCharacters = MessageCharacters(mLabel);

    int aTextLength = (int)aCharacters.size();
    float aFontScale = mFontScale;

    for (int i = 0; i < aTextLength; ++i)
    {
        Reanimation *aReanim = mApp->ReanimationTryToGet(mTextReanimID[i]);
        if (aReanim == nullptr)
            return;

        ReanimatorTransform aTransform;
        aReanim->GetCurrentTransform(2, &aTransform);

        // The color's alpha is scaled by the glyph's own fade, then rounded and
        // clamped; a glyph that has faded out entirely ends the draw.
        float aScaledAlpha = (float)theColor.mAlpha * aTransform.mAlpha;
        int aAlpha = (int)(aScaledAlpha <= 0.0f ? aScaledAlpha - 0.5f : aScaledAlpha + 0.5f);
        if (aAlpha <= 0)
            return;
        if (aAlpha >= 255)
            aAlpha = 255;

        Color aColor = theColor;
        aColor.mAlpha = aAlpha;
        aTransform.mScaleX *= aFontScale;
        aTransform.mScaleY *= aFontScale;

        // The reanimation's overlay matrix carries the drift the glyph has already
        // travelled; the text is positioned relative to the left edge of the board.
        float aOverlayX = aReanim->mOverlayMatrix.m02;
        aTransform.mTransX = (aOverlayX + aTransform.mTransX) - (float)LawnApp::FULLSCREEN_RECT.mX;
        aTransform.mTransY = ((aTransform.mTransY + aReanim->mOverlayMatrix.m12) + -300.0f) + thePos;
        if (mReanimType == REANIM_TEXT_FADE_ON && mDisplayTime - mDuration < mSlideOffTime)
            aTransform.mTransX += (1.0f - aReanim->mAnimTime) * aOverlayX;

        // One glyph per draw call, placed by the transform.
        Sexy::SexyMatrix3 aMatrix;
        aReanim->MatrixFromTransform(aTransform, aMatrix);

        Sexy::String aGlyph = CodePointsToUtf8({aCharacters[i]});
        TodDrawStringMatrix(theGraphics, theFont, aMatrix, aGlyph, aColor);
    }
}

// ==========================================================================
// MessageWidget::Draw(Sexy::Graphics *)
// @001A47A4  1970 bytes
// ==========================================================================
void MessageWidget::Draw(Sexy::Graphics *theGraphics)
{
    if (mDuration <= 0)
        return;

    Sexy::Font *aFont = GetFont();
    Color aColor(250, 250, 0, 255);
    Color aShadowColor(0, 0, 0, 255);

    // The outline pass is only drawn for the Continuum Bold face.
    Sexy::Font *aOutlineFont = nullptr;
    if (aFont == FONT_CONTINUUMBOLD14)
        aOutlineFont = FONT_CONTINUUMBOLD14OUTLINE;

    // Per-style geometry. The binary fills these in from a jump table and then
    // runs one shared tail; the switch below sets the same values per style.
    int aPosY = 596;
    int aRectX = 400;
    int aRectHeight = 600;
    int aBackdropOffsetY = 0;
    int aBackdropWidth = 0;
    int aAlphaStart = 0;
    bool aUseFadeCurve = false;

    switch ((int)mMessageStyle)
    {
        case MESSAGE_STYLE_TUTORIAL_LEVEL1:
        case MESSAGE_STYLE_TUTORIAL_LEVEL1_STAY:
            aPosY = 400;
            aColor = Color(253, 245, 173, 255);
            aAlphaStart = 192;
            aBackdropOffsetY = -4;
            aBackdropWidth = 110;
            aUseFadeCurve = true;
            break;

        case MESSAGE_STYLE_TUTORIAL_LEVEL2:
        case MESSAGE_STYLE_TUTORIAL_LATER:
        case MESSAGE_STYLE_TUTORIAL_LATER_STAY:
            aPosY = 476;
            aColor = Color(253, 245, 173, 255);
            aAlphaStart = 192;
            aBackdropOffsetY = -4;
            aBackdropWidth = 100;
            aUseFadeCurve = true;
            break;

        case MESSAGE_STYLE_HINT_LONG:
        case MESSAGE_STYLE_HINT_FAST:
        case MESSAGE_STYLE_HINT_STAY:
            aPosY = 487;
            aColor = Color(253, 245, 173, 255);
            aAlphaStart = 192;
            aBackdropOffsetY = -4;
            aBackdropWidth = 85;
            aUseFadeCurve = true;
            break;

        case MESSAGE_STYLE_HINT_TALL_FAST:
        case MESSAGE_STYLE_HINT_TALL_UNLOCKMESSAGE:
        case MESSAGE_STYLE_HINT_TALL_LONG:
        case MESSAGE_STYLE_HINT_TALL_STAY:
            aPosY = 330;
            aColor = Color(253, 245, 173, 255);
            aAlphaStart = 192;
            aBackdropOffsetY = -4;
            aBackdropWidth = 100;
            aUseFadeCurve = true;
            break;

        case MESSAGE_STYLE_BIG_MIDDLE:
        case MESSAGE_STYLE_BIG_MIDDLE_FAST:
            aPosY = 300;
            aColor = Color(253, 245, 173, 255);
            aAlphaStart = 192;
            aBackdropWidth = 110;
            aUseFadeCurve = true;
            break;

        case MESSAGE_STYLE_HOUSE_NAME:
            aPosY = 550;
            aColor = Color(255, 255, 255, 255);
            if (mReanimType == REANIM_NONE)
            {
                // No reanimated letters: fade the whole label in, 15 alpha per tick of the
                // display timer, and let the black outline fade with it.
                int aAlpha = 15 * mDuration;
                if (aAlpha >= 255)
                    aAlpha = 255;
                else if (aAlpha < 0)
                    aAlpha = 0;
                aColor.mAlpha = aAlpha;
                aShadowColor.mAlpha = aAlpha;
            }
            break;

        case MESSAGE_STYLE_HUGE_WAVE:
            aPosY = 330;
            aColor = Color(255, 0, 0, 255);
            break;

        case MESSAGE_STYLE_SLOT_MACHINE:
            // A slim strip with no black backdrop; only its y position differs between
            // the Chinese build and the rest.
            aRectX = 390;
            aRectHeight = 18;
            aAlphaStart = 64;
            aPosY = mApp->GetLocale().compare("zh_CN") == 0 ? 90 : 99;
            aUseFadeCurve = true;
            break;

        case MESSAGE_STYLE_ZEN_GARDEN_LONG:
            aPosY = 330;
            aColor = Color(253, 245, 173, 255);
            aAlphaStart = 192;
            aBackdropOffsetY = -4;
            aBackdropWidth = 55;
            aUseFadeCurve = true;
            break;

        default:
            break;
    }

    // Every path that has glyph reanimations draws those and stops.
    if (mReanimType != REANIM_NONE)
    {
        float aDrawY = (float)aPosY;
        if (aOutlineFont != nullptr)
            DrawReanimatedText(theGraphics, aOutlineFont, Sexy::Color::Black, aDrawY);
        DrawReanimatedText(theGraphics, aFont, aColor, aDrawY);
        return;
    }

    if (aUseFadeCurve)
    {
        aColor.mAlpha = TodAnimateCurve(75, 0, mApp->mBoard->mMainCounter % 75, aAlphaStart, 255, 11);
        aShadowColor.mAlpha = aColor.mAlpha;
    }

    // The label is wrapped into a rect whose left edge follows the board.
    TRect<int> aRect;
    aRect.mX = aRectX - 400 - mApp->mBoard->mX;
    aRect.mY = aPosY - aFont->mAscent;
    aRect.mWidth = 800;
    aRect.mHeight = aRectHeight;

    if (mMessageStyle == MESSAGE_STYLE_SLOT_MACHINE)
    {
        // Keep the TV strip anchor and height (0x1A4C90), centering the current
        // font box inside the strip rather than using its ascent as the top offset.
        aRect.mY = aPosY - aRectHeight / 2;
    }

    Sexy::String aLabel(mLabel);

    if (aBackdropWidth != 0)
    {
        // The band uses the message font and color; only the unbanded path has
        // an optional outline pass (the banded draw is at 0x1A4D8A).
        TRect<int> aBackdrop;
        aBackdrop.mX = (800 - LawnApp::FULLSCREEN_RECT.mWidth) / 2;
        aBackdrop.mY = aPosY;
        aBackdrop.mWidth = LawnApp::FULLSCREEN_RECT.mWidth;
        aBackdrop.mHeight = aBackdropWidth;

        theGraphics->SetColor(Color(0, 0, 0, 128));
        theGraphics->FillRect(aBackdrop);
        aBackdrop.mY += aBackdropOffsetY;
        TodDrawStringWrapped(theGraphics, aLabel, aBackdrop, aFont, aColor, 5, 0);
    }
    else
    {
        if (aOutlineFont != nullptr)
            TodDrawStringWrapped(theGraphics, aLabel, aRect, aOutlineFont, aShadowColor, 5, 0);
        TodDrawStringWrapped(theGraphics, aLabel, aRect, aFont, aColor, 5, 0);
    }

    if (mMessageStyle != MESSAGE_STYLE_HOUSE_NAME)
        return;

    // HOUSE_NAME in survival mode adds the completed-flag counter under the name.
    Sexy::String aFlagsText;
    if (mApp->IsSurvivalMode() && mApp->mBoard->mChallenge->mSurvivalStage > 0)
    {
        int aFlagsCompleted = mApp->mBoard->GetNumWavesPerSurvivalStage() *
            (int)mApp->mBoard->mChallenge->mSurvivalStage / mApp->mBoard->GetNumWavesPerFlag();
        Sexy::String aPluralFlags = mApp->Pluralize(aFlagsCompleted, "[ONE_FLAG]", "[COUNT_FLAGS]");
        const char *aFlagsFormat = aFlagsCompleted <= 1 ? "[FLAGS_COMPLETED]" : "[FLAGS_COMPLETED_PLURAL]";
        Sexy::String aFlagsLabel(aFlagsFormat);
        Sexy::String aFlagsReplaced;
        TodReplaceString(&aFlagsReplaced, &aFlagsLabel, "{FLAGS}", &aPluralFlags);
        aFlagsText = aFlagsReplaced;
    }

    if (!aFlagsText.empty())
    {
        int aFlagsX = 400 - mApp->mBoard->mX;
        TodDrawString(
            theGraphics, aFlagsText, aFlagsX, aPosY + 26, FONT_HOUSEOFTERROR16, Color(224, 187, 98, aColor.mAlpha), 2);
    }
}
