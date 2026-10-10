set(lawn_module_sources
    src/game/AlmanacDialog.cpp
    src/game/Attachment.cpp
    src/game/AttachmentHolder.cpp
    src/game/AwardScreen.cpp
    src/game/BaseGamepadControls.cpp
    src/game/Board.cpp
    src/game/BottomRotatorMenu.cpp
    src/game/BridgeApp.cpp
    src/game/BuyMoneyDialog.cpp
    src/game/BuySunMoneyDialog.cpp
    src/game/Challenge.cpp
    src/game/ChallengeScreen.cpp
    src/game/CheatCodeDialog.cpp
    src/game/CheatDialog.cpp
    src/game/Coin.cpp
    src/game/ConfirmBackToMainDialog.cpp
    src/game/ContinueDialog.cpp
    src/game/CreditScreen.cpp
    src/game/CreditsTiming.cpp
    src/game/CreditsOverlay.cpp
    src/game/CursorObject.cpp
    src/game/CursorPreview.cpp
    src/game/CustomScrollbarWidget.cpp
    src/game/CutScene.cpp
    src/game/DataReader.cpp
    src/game/DataReaderException.cpp
    src/game/DataSync.cpp
    src/game/DataWriter.cpp
    src/game/DaveHelp.cpp
    src/game/DeckChooserButton.cpp
    src/game/Definition.cpp
    src/game/DriverFactory.cpp
    src/game/DrRpcEngine.cpp
    src/game/EffectSystem.cpp
    src/game/GameButton.cpp
    src/game/GameCenter.cpp
    src/game/GameObject.cpp
    src/game/GameOverDialog.cpp
    src/game/GamepadControls.cpp
    src/game/GameSlider.cpp
    src/game/GameStats.cpp
    src/game/GridItem.cpp
    src/game/HelpBarWidget.cpp
    src/game/HelpOptionsDialog.cpp
    src/game/HelpTextImageWidget.cpp
    src/game/HelpTextScreen.cpp
    src/game/HelpTextWidget.cpp
    src/game/HouseChooserDialog.cpp
    src/game/ImitaterDialog.cpp
    src/game/internalJSONNode.cpp
    src/game/jsonChildren.cpp
    src/game/JSONNode.cpp
    src/game/JSONWorker.cpp
    src/game/LawnApp.cpp
    src/game/LawnDialog.cpp
    src/game/LawnMenuParser.cpp
    src/game/LawnMenuWidget.cpp
    src/game/LawnMower.cpp
    src/game/LawnParseWidget.cpp
    src/game/LawnPlayerInfo.cpp
    src/game/LawnSession.cpp
    src/game/LawnStoneButton.cpp
    src/game/LawnUser.cpp
    src/game/LevelStats.cpp
    src/game/LogComposer.cpp
    src/game/Mailbox.cpp
    src/game/MailScreen.cpp
    src/game/MainMenu.cpp
    src/game/MaskHelpWidget.cpp
    src/game/MessageWidget.cpp
    src/game/Music.cpp
    src/game/Music2.cpp
    src/game/NativeApp.cpp
    src/game/NewLawnButton.cpp
    src/game/NewOptionsDialog.cpp
    src/game/NewUserDialog.cpp
    src/game/NotificationManager.cpp
    src/game/ParseWidget.cpp
    src/game/Plant.cpp
    src/game/PoolEffect.cpp
    src/game/PottedPlant.cpp
    src/game/Projectile.cpp
    src/game/PurchaseLimitDialog.cpp
    src/game/Reanimation.cpp
    src/game/ReanimationButton.cpp
    src/game/ReanimationHolder.cpp
    src/game/ReanimationWidget.cpp
    src/game/ReanimatorCache.cpp
    src/game/ReanimatorDefinition.cpp
    src/game/ReanimatorTransform.cpp
    src/game/ReanimTrackWidgetAnimator.cpp
    src/game/RevivalGameDialog.cpp
    src/game/SaveGameContext.cpp
    src/game/SavingDingusDialog.cpp
    src/game/SavingFileDialog.cpp
    src/game/SavingProfileDialog.cpp
    src/game/SeedBank.cpp
    src/game/SeedChooserScreen.cpp
    src/game/SeedPacket.cpp
    src/game/ServerConfig.cpp
    src/game/SessionWaitingDialog.cpp
    src/game/SettingsDialog.cpp
    src/game/ShareManager.cpp
    src/game/ShopSeedPacket.cpp
    src/game/SlideInOutWidgetAnimator.cpp
    src/game/StartButton.cpp
    src/game/StoreScreen.cpp
    src/game/StoreScreenOverlay.cpp
    src/game/StretchableGameButton.cpp
    src/game/TestMenuWidget.cpp
    src/game/TitleScreen.cpp
    src/game/ToolTipWidget.cpp
    src/game/Trail.cpp
    src/game/TrailDefinition.cpp
    src/game/TrailHolder.cpp
    src/game/TrashBin.cpp
    src/game/TreeOfWisdomControls.cpp
    src/game/TypingCheck.cpp
    src/game/UnlockDialog.cpp
    src/game/UserDialog.cpp
    src/game/VSResultsMenu.cpp
    src/game/VSSetupMenu.cpp
    src/game/WaitFor2PLoadDialog.cpp
    src/game/WaitForLeaderboardsDialog.cpp
    src/game/WaitForRemoteFileDialog.cpp
    src/game/WaitForSecondPlayerDialog.cpp
    src/game/ZenGarden.cpp
    src/game/ZenGardenControls.cpp
    src/game/Zombie.cpp
    src/main.cpp
    src/game/AchievementSystem.cpp
    src/game/AchievementsWidget.cpp
    src/game/LeaderboardsWidget.cpp
)

if(ANDROID)
    add_library(LawnTV SHARED ${lawn_module_sources})
    set_target_properties(LawnTV PROPERTIES OUTPUT_NAME main)
    # Windows Ninja can shorten clang++.exe to an 8.3 path without "++".
    # Select the C++ driver explicitly so the NDK links libc++ and its ABI runtime.
    target_link_options(LawnTV PRIVATE "--driver-mode=g++" "-Wl,--no-undefined")
else()
    add_executable(LawnTV ${lawn_module_sources})
endif()
target_link_libraries(LawnTV PRIVATE LawnTVBuildOptions LawnTVDependencies Sexy TodLib)
set_target_properties(LawnTV PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:LawnTV>"
)
if(WIN32)
    enable_language(RC)
    target_sources(LawnTV PRIVATE Lawn.rc)
    target_include_directories(LawnTV PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    if(MSVC)
        target_link_options(LawnTV PRIVATE /DEBUG "/MAP:$<TARGET_FILE_DIR:LawnTV>/LawnTV.map")
    endif()
endif()
if(ANDROID)
    find_file(lawn_cxx_shared libc++_shared.so
        PATHS "${CMAKE_SYSROOT}/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}"
            "${CMAKE_SYSROOT}/usr/lib/${ANDROID_TOOLCHAIN_NAME}"
        NO_DEFAULT_PATH REQUIRED
    )
    add_custom_command(TARGET LawnTV POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:SDL3::SDL3>" "${lawn_cxx_shared}" "$<TARGET_FILE_DIR:LawnTV>"
        COMMENT "Staging libSDL3.so and libc++_shared.so for SDLActivity"
        VERBATIM
    )
endif()
