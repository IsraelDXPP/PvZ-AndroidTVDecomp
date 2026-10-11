# Console achievement condition audit

Reviewed on 2026-10-09 against the original TV decompilation exports and the achievement restoration in the user-supplied `PlantsVsZombies-AndroidTV-main` project. The TV exports retain some progress markers but do not contain a complete platform achievement service. The table distinguishes conditions supported directly by the original code from those restored using the reference project.

| Achievement | Unlock conditions and boundaries | Reference |
| --- | --- | --- |
| Home Lawn Security | Complete Adventure's final level, 5-10. An existing Adventure completion marker can also grant it retroactively. | Adventure completion logic; reference `Board::FadeOutLevel` |
| Master of Morticulture | Own all 49 plants: 40 Adventure plants, 8 upgrades, and the Imitater. Zen Garden potted plants do not count. | Original `LawnApp::HasSeedType` (`0x2B756C`); reference `StoreScreen::PurchaseItem` |
| Alive and Planting | Complete at least 40 flags in single-player Survival: Endless (pool). Count flags, not arbitrary survival rounds. Progress is also saved on completion or exit. | Original `Board::SurvivalSaveScore` (`0x15AF08`), `GetSurvivalFlagsCompleted` (`0x15AEA8`) |
| Soil Your Plants | Plant 10 Peashooters in one regular planting level. They need not remain alive simultaneously. Conveyors, the Zen Garden, preset plants, and the Imitater do not count. Reset the counter when initializing each level. | Reference `Board::DoPlantingAchievementCheck`, `Board::InitLevel`; regular planting modes follow original `IsSeedPlantingLevel` (`0x2B5350`) |
| Close Shave | Win after using the lawn mower in every active row. The reference excludes Survival and Versus. Levels without mowers must not grant it through `0 == 0`. | Reference `Board::FadeOutLevel`; the original function (`0x15D61C`) also retains a mower completion marker excluding Versus |
| Nom Nom Nom | Win a regular level planting only Sunflowers, Wall-nuts, and Chompers; all three types need not be used. Exclude Survival, Versus, conveyors, Vasebreaker, I, Zombie, and levels without regular planting. The Imitater is a separate seed and is not allowed. | Reference `Board::InitLevel`, `DoPlantingAchievementCheck`, `FadeOutLevel` |
| Versus | The same player wins 5 consecutive Versus matches; a loss resets the streak. Switching plant/zombie sides does not change ownership. Grant it to the profile bound to that player's controller. | Original `VSResultsMenu::InitFromBoard` (`0x1916FC`) and the reference five-win streak check |
| Crash of the Titan | Defeat a regular or red-eyed Gargantuar from an actual wave; exclude I, Zombie. Regular/cooperative play follows the original participant progress rules. Versus grants it to the plant-side controller's profile. | Original `Zombie::DieNoLoot` (`0x1B1660`) |
| ZFF's 4 Evr | Complete cooperative Wall-nut Bowling. Save it for both participating profiles. | Reference `Board::FadeOutLevel` |
| Shop | Spend a total of 25,000 coins at Crazy Dave's. Internal units are 10 coins, so the threshold is 2,500. Adding coins is not spending. Canceled operations or unchanged house selections do not count. Update spending once per purchase. | Reference `LawnPlayerInfo::AddCoins`; original store purchase debit path (`0x18F128`) |
| Explodonator | One Cherry Bomb must actually kill at least 10 zombies. Do not accumulate multiple explosions or count damaged targets as kills. Grant it to the player who planted the bomb; mouse actions use the primary profile. | Original `Plant::DoSpecial` (`0x16DDB8`), `KillAllZombiesInRadius` (`0x15AAC0`) |
| Smarty Branches | Grow the Tree of Wisdom to 100 feet. An existing height record of at least 100 can also grant it retroactively. | Original `Challenge::TreeOfWisdomGrow` (`0x1853D4`); reference implementation of the same function |

All unlocks are saved in the profile's `mAchievements`. Later losses, side switches, or plant sales do not revoke them. Opening the achievements/house view grants only achievements that can be established from persistent records. Achievements requiring data from a particular level are recorded by the corresponding game events; historical level outcomes are not inferred.

When launched with `-tod` or `--debug-keys`, lowercase `u` at the main menu is the manual full-unlock path in both Debug and Release: it sets all 12 achievement flags and saves them with the existing mode unlocks. It does not purchase house blueprints.

## Buying houses

After completing Adventure, Crazy Dave's shop shows five pages: seed slots/upgrades, plant upgrades, garden tools, Tree of Wisdom tools, and house blueprints. Original internal page numbers are retained, skipping the `NUM_STORE_PAGES` sentinel. The house page is shown as page 5/5.

The first blueprint for each of the other four houses costs 8,000 coins. Each later upgrade adds 8,000 coins, up to level five. The default house already has its first three appearances; its two paid upgrades cost 24,000 and 32,000 coins. Purchasing a blueprint selects that house; the house view's Choose House action also offers switching. The shop's Change House action costs 300 coins, charged only after confirming a switch to a different owned house.
