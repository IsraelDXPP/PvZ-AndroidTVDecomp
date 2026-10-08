#!/usr/bin/env python3
"""Export all TV game modes and both adventure cycles into one levels.xml."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import xml.etree.ElementTree as ET


REPOSITORY = Path(__file__).resolve().parent.parent
MINI_BOSS_LEVELS = {10, 20, 30}
WHACK_LEVEL = 15
BOWLING_LEVEL = 5
YETI_TYPE = 19


@dataclass(frozen=True)
class ZombieDefinition:
    zombie_type: int
    name: str
    point_value: int
    starting_level: int
    minimum_wave: int
    random_weight: int


@dataclass(frozen=True)
class SpawnTables:
    wave_counts: list[int]
    permissions: list[list[int]]
    definitions: list[ZombieDefinition]


def read_tables(repository: Path) -> SpawnTables:
    globals_text = (repository / "Sexy/src/ReconGlobals.cpp").read_text(encoding="utf-8")
    definitions_text = (repository / "Sexy/src/ReconVariadic.cpp").read_text(encoding="utf-8")
    wave_match = re.search(r"const int gZombieWaves\[50\]\s*=\s*\{(.*?)\};", globals_text, re.S)
    permission_match = re.search(
        r"const bool gZombieAllowedLevels\[NUM_ZOMBIE_TYPES\]\[50\]\s*=\s*\{(.*?)\};",
        globals_text,
        re.S,
    )
    definition_match = re.search(
        r"static ZombieDefinition gZombieDefs\[NUM_ZOMBIE_TYPES\]\s*=\s*\{(.*?)\};",
        definitions_text,
        re.S,
    )
    if not wave_match or not permission_match or not definition_match:
        raise ValueError("Cannot find the reconstructed native spawn tables")

    wave_counts = [int(value) for value in re.findall(r"\d+", wave_match.group(1))]
    permissions = [
        [int(value) for value in re.findall(r"\d+", row)]
        for row in re.findall(r"\{([^{}]+)\}", permission_match.group(1))
    ]
    definitions = []
    for row in re.findall(r"\{([^{}]+)\}", definition_match.group(1)):
        columns = [column.strip() for column in row.split(",")]
        definitions.append(
            ZombieDefinition(
                zombie_type=int(columns[0]),
                name=columns[6].strip('"'),
                point_value=int(columns[2]),
                starting_level=int(columns[3]),
                minimum_wave=int(columns[4]),
                random_weight=int(columns[5]),
            )
        )
    if len(wave_counts) != 50 or len(permissions) != 34 or len(definitions) != 34:
        raise ValueError("Expected 50 adventure levels and 34 native zombie types")
    if any(len(row) != 50 for row in permissions):
        raise ValueError("Every permission row must contain exactly 50 levels")
    if any(definition.zombie_type != index for index, definition in enumerate(definitions)):
        raise ValueError("Zombie definitions must be indexed by their native type")
    return SpawnTables(wave_counts, permissions, definitions)


def wave_count(tables: SpawnTables, level: int, repeat: bool) -> int:
    if level == WHACK_LEVEL:
        return 6
    count = tables.wave_counts[level - 1]
    if repeat and level not in MINI_BOSS_LEVELS:
        return 20 if count <= 9 else count + 10
    return count


def can_spawn_yeti(level: int, repeat: bool) -> bool:
    # Used only for the explanatory introduction table. Runtime permissions
    # remain native so that profile flag 32768 is also respected.
    return repeat and level >= 40


def allowed_zombies(tables: SpawnTables, level: int, repeat: bool) -> list[int]:
    if level == WHACK_LEVEL:
        return [0, 2, 4]
    if repeat and level == BOWLING_LEVEL:
        return [0, 2, 3, 4, 5]
    return [
        definition.zombie_type
        for definition in tables.definitions
        if (
            can_spawn_yeti(level, repeat)
            if definition.zombie_type == YETI_TYPE
            else definition.starting_level <= level
            and definition.random_weight > 0
            and tables.permissions[definition.zombie_type][level - 1] != 0
        )
    ]


def base_points(level: int, wave_index: int, repeat: bool) -> int:
    if not repeat or level in {BOWLING_LEVEL, WHACK_LEVEL}:
        return wave_index // 3 + 1
    return 2 * wave_index // 5 + 1


def is_flag_wave(level: int, wave_index: int, count: int, repeat: bool) -> bool:
    if not repeat and level == 1:
        return False
    interval = 10 if repeat or count >= 10 else count
    return wave_index % interval == interval - 1


def point_multiplier(level: int) -> int:
    if level in {5, 25}:
        return 4
    if level in {10, 20, 30, 40}:
        return 3
    if level == 45:
        return 2
    return 1


def introduced_zombie(tables: SpawnTables, level: int, repeat: bool) -> int | None:
    if level == 1:
        return None
    return next(
        (
            definition.zombie_type
            for definition in tables.definitions
            if definition.starting_level == level
            and (definition.zombie_type != YETI_TYPE or can_spawn_yeti(level, repeat))
        ),
        None,
    )


def introduced_wave_indices(zombie_type: int | None, count: int) -> list[int]:
    if zombie_type is None or zombie_type == 10:
        return []
    if zombie_type in {16, 17}:
        return sorted({6, count - 1})
    if zombie_type == YETI_TYPE:
        return [count // 2]
    return sorted({count // 2, count - 1})


def scene_for_level(level: int) -> int:
    if level == 35:
        return 1
    if level == 50:
        return 5
    return (level - 1) // 10


def write_xml(root: ET.Element, destination: Path) -> None:
    ET.indent(root, space="  ")
    ET.ElementTree(root).write(destination, encoding="utf-8", xml_declaration=True)
    with destination.open("a", encoding="utf-8", newline="\n") as stream:
        stream.write("\n")


def build_levels(tables: SpawnTables, repeat: bool) -> ET.Element:
    root = ET.Element("AdventureRecords")
    cycle = "repeat" if repeat else "first"
    root.append(ET.Comment(" 冒险二周目。 " if repeat else " 冒险一周目。 "))
    for level in range(1, 51):
        count = wave_count(tables, level, repeat)
        world, stage = divmod(level - 1, 10)
        root.append(ET.Comment(f" {world + 1}-{stage + 1}：{count} 波；原版预算倍率 {point_multiplier(level)}。 "))
        element = ET.SubElement(
            root,
            "Level",
            id=str(level),
            mode="0-1",
            adventureCycle=cycle,
            scene=str(scene_for_level(level)),
            waveCount=str(count),
            allowedZombies="native",
        )
        if level == 1:
            element.set("skipShopSeedTutorial", "true")
        if level == 15:
            element.append(ET.Comment(" 锤僵尸由 Challenge::WhackAZombieSpawning 出怪，下面的 points 不控制实际出怪数量。 "))
        elif level == 35:
            element.append(ET.Comment(" 砸罐子由罐子内容决定出怪，常规波次不用于实际刷怪。 "))
        elif level == 50:
            element.append(ET.Comment(" 僵王关由 Boss 的召唤逻辑出怪，常规波次不用于实际刷怪。 "))

        introduction = introduced_zombie(tables, level, repeat)
        introduction_indices = introduced_wave_indices(introduction, count)
        for wave_index in range(count):
            flag = is_flag_wave(level, wave_index, count, repeat)
            if level == 45 and flag:
                element.append(ET.Comment(" 原版代码将这波替换为 5 只蹦极僵尸，跳过普通旗帜波预算和新僵尸追加。 "))
            elif wave_index in introduction_indices:
                condition = "（未杀死雪人并重开时）" if introduction == YETI_TYPE else ""
                element.append(ET.Comment(f" 原版代码在本波追加僵尸 {introduction}{condition}；不再写入 mustHaveZombies。 "))
            wave_attributes = {"index": str(wave_index), "points": str(base_points(level, wave_index, repeat)), "flag": "1" if flag else "0"}
            ET.SubElement(element, "Wave", wave_attributes)
    return root


def rule(parent: ET.Element, name: str, description: str, **attributes: str) -> ET.Element:
    element = ET.SubElement(parent, name, attributes)
    element.text = description
    return element


def build_rules(tables: SpawnTables) -> ET.Element:
    root = ET.Element("OriginalSpawnRules", version="2", waveIndexBase="0")
    rule(root, "Scope", "原版 TV 模式的配置和动态规则；预算供原生加权选怪使用，不是某次随机运行的固定僵尸名单。此节点仅记录来源，不参与 Level 选择。")
    sources = ET.SubElement(root, "Sources")
    ET.SubElement(sources, "Native", function="Board::PickZombieWaves", address="0x161870")
    ET.SubElement(sources, "Native", table="gZombieWaves", address="0x695C38", entries="50")
    ET.SubElement(sources, "Native", table="gZombieAllowedLevels", address="0x694120", rows="34", integersPerRow="51")
    ET.SubElement(sources, "Native", table="ZombieDefinition", address="0x697660", entries="34")
    ET.SubElement(sources, "Reference", function="Board::IsFlagWave", file="PvZ/src/Lawn/Board/Board.cpp", line="4616")
    ET.SubElement(sources, "Reference", function="MainMenu::SyncProfile", file="PvZ/src/Lawn/Widget/MainMenu.cpp", line="372")

    profiles = ET.SubElement(root, "Profiles")
    first = ET.SubElement(profiles, "Profile", name="first-adventure", file="levels.xml", hasFinishedAdventure="0")
    rule(first, "WaveCount", "使用内置表；2-5 锤僵尸例外，始终 6 波。", expression="nativeWaveCounts[level - 1]")
    rule(first, "BasePoints", "所有冒险关卡的基础点数。", expression="waveIndex // 3 + 1")
    rule(first, "Flags", "1-1 无旗帜；少于 10 波的关卡以最后一波为旗帜，其余每 10 波一旗。")
    repeat = ET.SubElement(profiles, "Profile", name="repeat-adventure", file="levels.xml", hasFinishedAdventure="1", profileFlag32768="0", killedYetiAndRestarted="0")
    rule(repeat, "WaveCount", "1-10、2-10、3-10 保持内置波数；2-5 始终 6 波；其他关卡原波数不超过 9 时改为 20，否则增加 10。")
    rule(repeat, "BasePoints", "二周目公式；1-5 保龄球和 2-5 锤僵尸继续使用 waveIndex // 3 + 1。", expression="2 * waveIndex // 5 + 1")
    rule(repeat, "Flags", "每 10 波一旗，短于 10 波时没有旗帜。", expression="waveIndex % 10 == 9")

    definitions = ET.SubElement(root, "ZombieDefinitions")
    for definition in tables.definitions:
        allowed_levels = [str(index + 1) for index, allowed in enumerate(tables.permissions[definition.zombie_type]) if allowed]
        ET.SubElement(definitions, "Zombie", id=str(definition.zombie_type), name=definition.name,
                      pointValue=str(definition.point_value), startingLevel=str(definition.starting_level),
                      minimumWave=str(definition.minimum_wave), randomWeight=str(definition.random_weight),
                      nativeAllowedLevels=" ".join(allowed_levels))
    ET.SubElement(root, "NativeWaveCounts", values=" ".join(map(str, tables.wave_counts)))

    permissions = ET.SubElement(root, "AllowedZombieRules")
    rule(permissions, "Normal", "原生允许表为真、已达到 startingLevel、randomWeight 非零才允许；雪人单独判断。")
    rule(permissions, "Yeti", "已通关冒险，并且当前关卡至少为 4-10，或者存档标志 32768 已设置。", expression="hasFinishedAdventure and (profileFlag32768 or level >= 40)")
    rule(permissions, "WhackAZombie", "两个周目都交给 Challenge 初始化允许种类。", level="15", zombies="0 2 4")
    rule(permissions, "RepeatBowling", "二周目 1-5 改用 Challenge 的保龄球种类。", level="5", zombies="0 2 3 4 5")

    order = ET.SubElement(root, "WaveGenerationOrder")
    rule(order, "Reset", "每波清空计数；读取本波基础 points。", step="1")
    rule(order, "BungeeFlag", "5-5 的旗帜波固定加入 5 只蹦极，然后结束本波；其他步骤不再执行。", step="2", level="45", zombies="20 20 20 20 20")
    rule(order, "XmlMustHave", "XML 的 mustHaveZombies 在旗帜倍率之前加入并扣点；冒险记录不重复填写原生追加。", step="3")
    rule(order, "Flag", "预算乘 2.5 后向零取整；先加 min(原预算, 8) 只普通僵尸，再加 1 只旗帜僵尸，逐只扣点。", step="4", budgetExpression="int(basePoints * 2.5) - min(basePoints, 8) - 1")
    rule(order, "SpecialMultiplier", "对旗帜添加之后剩余的预算应用关卡倍率；不能把倍率提前乘进 XML。", step="5")
    rule(order, "Introduction", "按 Introductions 追加新僵尸并扣点。", step="6")
    rule(order, "LastWave", "5-10 最后一波先加巨人；所有冒险最后一波随后执行 LastWaveMissing。", step="7", bossLevel="50", bossExtraZombie="23")
    rule(order, "XmlAfterNative", "afterNativeZombies 在原版固定追加之后、随机填充之前加入并扣点。", step="8")
    rule(order, "RandomFill", "预算大于 0 且已排入数量小于 50 时加权随机选怪；类型必须允许、minimumWave 不大于 waveIndex + 1、pointValue 不大于剩余预算。每只扣 pointValue。", step="9", maxZombiesPerWave="50")

    multipliers = ET.SubElement(root, "SpecialMultipliers")
    for level in (5, 10, 20, 25, 30, 40, 45):
        ET.SubElement(multipliers, "Level", id=str(level), multiplier=str(point_multiplier(level)))

    introductions = ET.SubElement(root, "Introductions")
    rule(introductions, "Selection", "1-1 不选择新僵尸；其余关卡按类型顺序取第一个 startingLevel 等于关卡号的类型；不能出雪人时跳过雪人。与 allowedZombies 筛选分开执行。")
    rule(introductions, "Placement", "气球、矿工在 waveIndex=6 和最后一波追加；雪人在 waveCount // 2 且未杀死后重开时追加；泳圈不追加；其余在 waveCount // 2 和最后一波追加。")
    for repeat_profile in (False, True):
        profile = ET.SubElement(introductions, "Profile", name="repeat-adventure" if repeat_profile else "first-adventure")
        for level in range(1, 51):
            zombie_type = introduced_zombie(tables, level, repeat_profile)
            count = wave_count(tables, level, repeat_profile)
            indices = [index for index in introduced_wave_indices(zombie_type, count)
                       if not (level == 45 and is_flag_wave(level, index, count, repeat_profile))]
            if indices:
                attributes = {"id": str(level), "zombie": str(zombie_type), "waveIndices": " ".join(map(str, indices))}
                if zombie_type == YETI_TYPE:
                    attributes["condition"] = "not killedYetiAndRestarted"
                ET.SubElement(profile, "Level", attributes)

    rule(root, "LastWaveMissing", "Board::PutInMissingZombies（0x1521C4）遍历全部 34 类；雪人除外，对当前波尚未加入且 CanZombieSpawnOnLevel 为真的每一类追加一只并扣点。依赖当前波计数，无法用静态 mustHaveZombies 完整表达。")
    controllers = ET.SubElement(root, "NativeSpawnControllers")
    rule(controllers, "Level", "锤僵尸按墓碑、计时器和 Challenge 状态连续刷怪，points 不决定实际数量。", id="15", controller="Challenge::WhackAZombieSpawning")
    rule(controllers, "Level", "砸罐子的僵尸来自罐子内容，常规波次不触发。", id="35", controller="ScaryPotter")
    rule(controllers, "Level", "僵王及其召唤逻辑负责出怪，常规波次不触发。", id="50", controller="Zombie::BossSpawnAttack")

    timing = ET.SubElement(root, "SpawnTiming", controller="Board::UpdateZombieSpawning", unit="gameUpdateTicks")
    rule(timing, "FirstWave", "一般为 1800；一周目 1-2 为 5000；教学状态可暂停计时。")
    rule(timing, "NormalInterval", "下一波普通等待为 2500..3099；3-5 小僵尸关固定为 750。")
    rule(timing, "BeforeFlag", "下一波为旗帜时等待 4500；保龄球不使用这项等待；临近旗帜还有 750 的大波提示倒计时。")
    rule(timing, "EarlyNextWave", "当前波刷出超过 400 次更新，剩余血量不超过初始血量的随机 50%..65%，且等待大于 200 时，将等待缩短到 200。")

    compatibility = ET.SubElement(root, "Compatibility")
    rule(compatibility, "ShopTutorial", "1-1 的 skipShopSeedTutorial=true 在加载时设置存档教学标志，匹配参考项目 normalLevel；第四波不会追加全部种类。")
    rule(compatibility, "RepeatSelection", "adventureCycle 按是否通关冒险自动选择；survivalStage 按当前阶段自动选择。")
    rule(compatibility, "YetiState", "allowedZombies=native 保留实时存档判断，不冻结雪人许可。")
    rule(compatibility, "MissingZombieImplementation", "PutInMissingZombies 已按原汇编修正为遍历全部类型，跳过雪人。")
    return root


def mode_names(repository: Path) -> dict[int, str]:
    text = (repository / "Sexy/include/ReconTypes.h").read_text(encoding="utf-8")
    block = re.search(r"enum GameMode\s*\{(.*?)\};", text, re.S)
    if not block:
        raise ValueError("Cannot find GameMode")
    return {int(value): name for name, value in re.findall(r"(GAMEMODE_\w+)\s*=\s*(\d+)", block.group(1))}


def challenge_wave_count(mode: int) -> int:
    if mode in {44, 50, 51}:
        return 0
    if mode == 20:
        return 25
    if mode == 31:
        return 12
    if mode in {17, 18, 22, 27, 42, 45, 46, 79, 80, 91, 93}:
        return 20
    if mode in {26, 28, 33, 34, 35, 47, 48, 49, 81, 82, 83}:
        return 30
    if mode == 76:
        return 5
    return 40


def challenge_scene(mode: int) -> int | None:
    # Board::PickBackground @0x15FFE8; 91/93 follow the reference extension.
    groups = {
        0: {2, 7, 12, 17, 18, 19, 20, 23, 34, 37, 38, 39, 40, 41, 43, 47, 50, 74, 75, 79, 83, 84},
        1: {3, 8, 13, 21, 25, 27, 31, 46, *range(52, 72), 80, 85},
        2: {4, 9, 14, 26, 29, 30, 32, 33, 72, 73, 77, 78, 81, 86, 89, 91, 93},
        3: {5, 10, 15, 22, 42, 48, 90},
        4: {6, 11, 16, 28, 35, 45, 49, 82, 87},
        5: {36, 88},
        7: {44},
        8: {24},
        9: {51},
    }
    return next((scene for scene, modes in groups.items() if mode in modes), None)


def controller_for_mode(mode: int) -> str:
    if mode == 20:
        return "Challenge::UpdateZombieSpawning / Board::SpawnZombieWave (sky)"
    if mode == 31:
        return "Challenge::WhackAZombieSpawning"
    if mode in {36, 88}:
        return "Zombie::BossSpawnAttack"
    if mode in range(52, 72):
        return "ScaryPotter" if mode % 2 == 0 else "IZombie"
    if mode in {44, 51}:
        return "ZenGarden"
    return "native"


def build_manifest(tables: SpawnTables) -> ET.Element:
    root = ET.Element("PVZ", version="1.2.0.1073", schemaVersion="2")
    root.append(ET.Comment(" 单一运行配置：所有定义的 TV 模式、冒险两个周目和生存全部阶段。索引从 0 开始。 "))
    root.append(ET.Comment(" points 是旗帜处理和关卡倍率之前的预算；动态许可与特殊控制器继承原版。 "))
    ET.SubElement(root, "Defaults", spawnRules="native", flagRules="native", allowedZombies="native", waveCount="native")
    for repeat in (False, True):
        root.extend(build_levels(tables, repeat))
    root.append(ET.Comment(" 挑战、生存、解谜、双人及 TV 专属模式。未枚举的 mode=94 是原枚举空洞。 "))
    for mode, name in mode_names(REPOSITORY).items():
        if mode <= 1:
            continue
        attributes = {"mode": str(mode), "id": "*", "name": name, "controller": controller_for_mode(mode)}
        scene = challenge_scene(mode)
        if scene is not None:
            attributes["scene"] = str(scene)
        if 2 <= mode <= 16 or mode == 32:
            attributes["survivalStage"] = "*"
            element = ET.SubElement(root, "Level", attributes)
            element.append(ET.Comment(" 后续阶段预算按阶段增长，允许列表按阶段抽选；缺省 Wave 继承原版，不会变成零波。 "))
            attributes = {**attributes, "survivalStage": "0", "waveCount": "10" if mode <= 6 or mode == 32 else "20"}
            element = ET.SubElement(root, "Level", attributes)
            for index in range(int(attributes["waveCount"])):
                points = 2 * (index + 10) // 5 + 1 if mode == 32 else index // 3 + 1
                ET.SubElement(element, "Wave", index=str(index), points=str(points))
            continue
        count = challenge_wave_count(mode)
        attributes["waveCount"] = str(count)
        element = ET.SubElement(root, "Level", attributes)
        if mode == 20:
            element.append(ET.Comment(" 重型武器：25 波；3*index/4 向下取整；原版允许普通、撑杆、橄榄球、跳跳、巨人（0 3 7 18 23），雪人按存档动态追加；保留空中出怪和炮台逻辑。 "))
        if mode in {91, 93}:
            element.append(ET.Comment(" 参考 AndroidTV 工程新增模式；这里只提取场景、波数和预算，玩法仍依赖模式控制器实现。 "))
        for index in range(count):
            points = 3 * index // 4 if mode == 20 else 4 if mode == 76 else 4 * index // 5 + 1 if mode in {91, 93} else index // 3 + 1
            wave_attributes = {"index": str(index), "points": str(points)}
            if mode == 91 and index == count - 1:
                wave_attributes["afterNativeZombies"] = "23"
            ET.SubElement(element, "Wave", wave_attributes)
    root.append(build_rules(tables))
    return root


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, default=REPOSITORY / "Config/OriginalSpawns")
    options = parser.parse_args()
    tables = read_tables(REPOSITORY)
    options.output_dir.mkdir(parents=True, exist_ok=True)
    destination = options.output_dir / "levels.xml"
    write_xml(build_manifest(tables), destination)
    print(destination)


if __name__ == "__main__":
    main()
