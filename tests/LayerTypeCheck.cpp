// Includes the shipped public headers and checks the enum / value / virtual
// classification. A virtual call on BaseAppDriver and SoundInstance must hit
// the override, which is the same dispatch the Windows driver and sound path use.

#include "ReconTypes.h"
#include "BaseAppDriver.h"
#include "InputConnectManager.h"
#include "IInputDriver.h"
#include "Graphics3D.h"
#include "PIValue.h"
#include "ScrollbarWidget.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "SexyTypes.h"
#include "BridgeApp.h"
#include "game/Board.h"
#include "SeedChooserScreen.h"
#include "DataSync.h"
#include "TodFoley.h"

#include <stdio.h>
#include <type_traits>

static int gFailures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL %s\n", #cond); \
        ++gFailures; \
    } \
} while (0)

template <class T, class = void>
struct HasMVtable : std::false_type {};

template <class T>
struct HasMVtable<T, std::void_t<decltype(std::declval<T>().mVtable)>> : std::true_type {};

template <class T>
void ReportEnum(const char* name)
{
    const bool isEnum = std::is_enum<T>::value;
    const bool oneWord = sizeof(T) == sizeof(int);
    CHECK(isEnum);
    CHECK(oneWord);
    CHECK(!HasMVtable<T>::value);
    if (isEnum && oneWord)
        printf("%s enum\n", name);
}

// A multi-word record must not be reported as an enum. The check runs before
// the classification line, so a wrong conversion cannot look classified.
template <class T>
void ReportRecord(const char* name, size_t theWords)
{
    const bool isEnum = std::is_enum<T>::value;
    const bool sized = sizeof(T) == theWords * sizeof(int);
    CHECK(!isEnum);
    CHECK(sized);
    CHECK(!HasMVtable<T>::value);
    if (!isEnum && sized)
        printf("%s record\n", name);
}

template <class T, class Value>
void ReportValue(const char* name)
{
    using Field = decltype(std::declval<T>().mValue);
    const bool isEnum = std::is_enum<T>::value;
    const bool fieldOk = std::is_same<Field, Value>::value;
    CHECK(!isEnum);
    CHECK(fieldOk);
    CHECK(!HasMVtable<T>::value);
    if (!isEnum && fieldOk)
        printf("%s value\n", name);
}

template <class T>
void ReportVirtual(const char* name)
{
    const bool polymorphic = std::is_polymorphic<T>::value;
    const bool noHandVtable = !HasMVtable<T>::value;
    CHECK(polymorphic);
    CHECK(noHandVtable);
    if (polymorphic && noHandVtable)
        printf("%s virtual\n", name);
}

struct ProbeDriver : Sexy::BaseAppDriver
{
    int Startup() override { return 42; }
};

struct ProbeSound : Sexy::SoundInstance
{
    int mPlays = 0;

    void Release() override {}
    void SetBaseVolume(double) override {}
    void SetBasePan(int) override {}
    void SetBaseRate(double) override {}
    void AdjustPitch(double) override {}
    void SetVolume(double) override {}
    void SetMasterVolumeIdx(int) override {}
    void SetPan(int) override {}
    void Play(bool, bool) override { ++mPlays; }
    void Stop() override {}
    bool IsPlaying() override { return true; }
    bool IsReleased() override { return false; }
};

int main()
{
    ReportRecord<FoleyInstance>("FoleyInstance", 5);
    ReportEnum<MiscStat>("MiscStat");
    ReportEnum<BUTTON_ALIGNMENT_X>("BUTTON_ALIGNMENT_X");
    ReportEnum<PIParticleInstance>("PIParticleInstance");
    ReportEnum<RenderItem>("RenderItem");
    ReportEnum<SeedChooserScreen::SeedDir>("SeedChooserScreen::SeedDir");
    ReportRecord<TextExtents>("TextExtents", 6);
    ReportEnum<AdviceType>("AdviceType");
    ReportEnum<AlmanacDialog_SeedDir>("AlmanacDialog_SeedDir");
    ReportRecord<AttacherInfo>("AttacherInfo", 4);
    CHECK((std::is_same<decltype(std::declval<AttacherInfo>().mReanimName), Sexy::String>::value));
    CHECK((std::is_same<decltype(std::declval<AttacherInfo>().mTrackName), Sexy::String>::value));
    CHECK((std::is_same<decltype(std::declval<AttacherInfo>().mLowFPS), float>::value));
    CHECK((std::is_same<decltype(std::declval<AttacherInfo>().mLoopType), int>::value));
    CHECK((std::is_same<decltype(std::declval<FoleyInstance>().mInstance), Sexy::SoundInstance*>::value));
    CHECK((std::is_same<decltype(std::declval<TextExtents>().mHeight), int>::value));
    ReportEnum<BaseGamepadControls_MovementState>("BaseGamepadControls_MovementState");
    ReportEnum<BossPart>("BossPart");
    ReportEnum<ChallengePage>("ChallengePage");
    ReportEnum<ChallengeState>("ChallengeState");
    ReportEnum<ChosenSeed>("ChosenSeed");
    ReportEnum<CoinMotion>("CoinMotion");
    ReportEnum<CreditsPhase>("CreditsPhase");
    ReportEnum<DrawVariation>("DrawVariation");
    ReportEnum<E_DAVESINGLEHELP>("E_DAVESINGLEHELP");
    ReportEnum<E_SHOP_PACKET>("E_SHOP_PACKET");
    ReportEnum<FilterEffect>("FilterEffect");
    ReportEnum<GameStats_MiscStat>("GameStats_MiscStat");
    ReportEnum<GridItemState>("GridItemState");
    ReportEnum<GridItemType>("GridItemType");
    ReportEnum<HelpBarWidget_HelpAlign>("HelpBarWidget_HelpAlign");
    ReportEnum<Mailbox_MailMessage>("Mailbox_MailMessage");
    ReportEnum<MainMenu_MenuScene>("MainMenu_MenuScene");
    ReportEnum<MusicTune>("MusicTune");
    ReportEnum<ParticleEffect>("ParticleEffect");
    ReportEnum<ParticleSystemID>("ParticleSystemID");
    ReportEnum<PlantPriority>("PlantPriority");
    ReportEnum<PlayerFlags>("PlayerFlags");
    ReportEnum<ProjectileType>("ProjectileType");
    ReportEnum<ReanimationID>("ReanimationID");
    ReportEnum<RenderLayer>("RenderLayer");
    ReportEnum<SeedChooserScreen_SeedDir>("SeedChooserScreen_SeedDir");
    ReportEnum<StoreItem>("StoreItem");
    ReportEnum<StoreScreen_ItemDir>("StoreScreen_ItemDir");
    ReportEnum<TrashPileType>("TrashPileType");
    ReportEnum<VSSetupMenu_VSSetupState>("VSSetupMenu_VSSetupState");
    ReportEnum<BlendFactor>("BlendFactor");
    ReportEnum<CmpFunc>("CmpFunc");
    ReportEnum<DrawMode>("DrawMode");
    ReportEnum<PlantWeapon>("PlantWeapon");
    ReportEnum<Sexy::PixelFormat>("Sexy::PixelFormat");
    ReportEnum<Sexy::WidgetAnimationState>("Sexy::WidgetAnimationState");
    ReportEnum<Sexy::LogLevel>("Sexy::LogLevel");
    ReportEnum<Sexy::SexyCommonApp_E_TESTAD_MODE>("Sexy::SexyCommonApp_E_TESTAD_MODE");
    ReportEnum<Sexy::SexyCommonApp_POST_TYPE>("Sexy::SexyCommonApp_POST_TYPE");
    ReportEnum<Sexy::SexyCommonApp_E_MESSAGEBOX_TYPE>("Sexy::SexyCommonApp_E_MESSAGEBOX_TYPE");

    ReportValue<Sexy::HRenderContext, int>("Sexy::HRenderContext");
    ReportValue<Sexy::InputAxisInfo, float>("Sexy::InputAxisInfo");
    ReportValue<Sexy::DriverAxisSample, float>("Sexy::DriverAxisSample");
    ReportValue<Sexy::PIKeyframe, float>("Sexy::PIKeyframe");
    ReportValue<Sexy::ScrollbarWidget, double>("Sexy::ScrollbarWidget");
    ReportValue<DataSync::PointerToIntNode, int>("DataSync::PointerToIntNode");
    ReportValue<DataSync::IntToPointerNode, void*>("DataSync::IntToPointerNode");
    ReportValue<BoardRenderItem, int>("BoardRenderItem");
    ReportValue<SCoinItem, int>("SCoinItem");
    ReportValue<TMLongCard, int>("TMLongCard");
    ReportValue<_DWORD, unsigned int>("_DWORD");

    ReportVirtual<Sexy::BaseAppDriver>("Sexy::BaseAppDriver");
    ReportVirtual<Sexy::IResStreamsDriver>("Sexy::IResStreamsDriver");
    ReportVirtual<Sexy::IAudioDriver>("Sexy::IAudioDriver");
    ReportVirtual<Sexy::IHttpDriver>("Sexy::IHttpDriver");
    ReportVirtual<Sexy::IFileDriver>("Sexy::IFileDriver");
    ReportVirtual<Sexy::InputConnectManager>("Sexy::InputConnectManager");
    ReportVirtual<Sexy::IInputDriver>("Sexy::IInputDriver");
    ReportVirtual<Sexy::SoundManager>("Sexy::SoundManager");
    ReportVirtual<Sexy::SoundInstance>("Sexy::SoundInstance");

    CHECK(!std::is_polymorphic<Native::BridgeApp>::value);
    CHECK(!HasMVtable<Native::BridgeApp>::value);
    if (!std::is_polymorphic<Native::BridgeApp>::value && !HasMVtable<Native::BridgeApp>::value)
        printf("Native::BridgeApp value\n");

    ProbeDriver probe;
    Sexy::BaseAppDriver* driver = &probe;
    const int started = driver->Startup();
    CHECK(started == 42);
    printf("BaseAppDriver::Startup virtual %d\n", started);

    ProbeSound probeSound;
    Sexy::SoundInstance* sound = &probeSound;
    CHECK(sound->IsPlaying());
    sound->Play(false, false);
    CHECK(probeSound.mPlays == 1);
    printf("SoundInstance::Play virtual %d\n", probeSound.mPlays);

    printf("failures %d\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}
