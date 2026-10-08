# PlantsVsZombies-AndroidTV 触控代码

来源项目：https://github.com/ZombieYetis/PlantsVsZombies-AndroidTV

Copyright (C) 2023-2026 PvZ TV Touch Team

许可证：GNU General Public License，版本 3 或任何后续版本（GPL-3.0-or-later）。原项目许可证全文保存在本目录的 `LICENSE`。

2026-10-08：参考用户提供的 PlantsVsZombies-AndroidTV-main 源码，将触控交互移植、改写为 Windows SDL 鼠标输入。保留本项目的对象模型、原版坐标、存档格式与种植规则；没有移植 Android Hook、JNI、多点触控、联网或回放功能。

参考源码位于原项目 `app/src/main/cpp/PvZ/src/Lawn/Board/Board.cpp` 和 `app/src/main/cpp/PvZ/src/Lawn/Widget/`。移植与适配涉及本项目 `Lawn/src/game/` 下的 Board、SeedChooserScreen、ImitaterDialog、ChallengeScreen、StoreScreen、AlmanacDialog、MailScreen、HelpTextScreen、HouseChooserDialog、VSSetupMenu 和 WaitForSecondPlayerDialog 及相应声明。GamepadControls、BaseGamepadControls、ZenGardenControls、TreeOfWisdomControls、CursorObject、CursorPreview、SeedBank 与 HelpBarWidget 增加了与鼠标交互配合的逻辑；`Sexy` 平台层和控件层增加了鼠标事件转发、坐标映射、捕获和按钮分发支持。

上述从原项目移植或派生的代码按 GPL-3.0-or-later 提供。原有 MIT 代码的版权与许可声明继续适用于原有部分；包含这些 GPL 部分的组合程序分发须遵守 GPL。

2026-10-08：继续移植 Board 顶部菜单按钮的位置、可用状态与点击行为，并修复商店翻页点击入口的图片资源查询。

2026-10-08：菜单下方增加小道具栏显示开关，并按原版解锁条件限制选卡界面的商店和图鉴按钮。

This code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this code. If not, see https://www.gnu.org/licenses/.
