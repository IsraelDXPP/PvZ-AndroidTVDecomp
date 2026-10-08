#!/usr/bin/env python3
"""Export standalone TV and NormalLevel/PC spawn configurations in schema 3."""

from __future__ import annotations

import argparse
import copy
import hashlib
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


MODE_POOLS = {
    17: [27, 28], 18: [0, 2, 4, 3, 5], 20: [0, 3, 7, 18, 23],
    21: [0, 2, 4, 6, 7, 5], 22: [0, 2, 4, 14, 12, 15],
    25: [0, 2, 4, 6, 7, 5], 26: [0, 2, 7, 11], 27: [0, 4, 7, 16],
    28: [0, 2, 4, 7], 29: [13, 12], 30: [0, 2, 14, 3], 31: [0, 2, 4],
    32: [0, 2, 4, 6, 7, 5, 15, 3, 14, 21], 33: [27, 28, 29, 30, 31, 32],
    34: [0, 2, 4, 3, 5, 8, 6], 35: [18], 38: [0, 4, 2, 7, 3, 15],
    40: [0, 2, 4, 6, 7, 15], 42: [16], 45: [0, 2, 4, 6, 16],
    48: [0, 2, 14, 16], 49: [0, 4, 2, 21],
    77: [0, 2, 4, 3, 12, 13, 14], 78: [0, 2, 4, 3, 12, 13, 14],
    79: [0, 2, 4, 3, 1], 80: [0, 2, 4, 3, 1, 5, 6, 7, 8, 9],
    81: [0, 2, 4, 3, 1, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14],
    82: [0, 2, 4, 3, 1, 5, 6, 7, 8, 9, 15, 16, 17, 18, 19],
    83: [0, 2, 1, 4, 3, 12, 13, 23],
    88: [0, 2, 4, 3, 12, 13, 14], 89: [0, 2, 4, 3, 12, 13, 14],
    90: [0, 4, 2, 6, 7, 5, 15, 20],
}


def number_list(values: list[int]) -> str:
    return " ".join(map(str, values))


def pipeline(multiplier: int = 1, war: bool = False) -> list[ET.Element]:
    actions = [ET.Element("Scale", flagOnly="true", numerator="5", denominator="2")]
    if not war:
        actions.extend([
            ET.Element("Spawn", flagOnly="true", types="0", budgetCountLimit="8"),
            ET.Element("Spawn", flagOnly="true", types="1"),
        ])
    if multiplier != 1:
        actions.append(ET.Element("Scale", numerator=str(multiplier)))
    actions.append(ET.Element("Fill"))
    return actions


def extra_policy(parent: ET.Element, scene: int, level: int = 0, mode: int = 0) -> None:
    pool_count, pool_points = (2, 3) if level in {21, 22, 31, 32} else (3, 5) if level in {23, 24, 25, 33, 34, 35} else (3, 7)
    sky_count, sky_points = (2, 3) if level in {41, 42} else (3, 5) if level in {43, 44, 45} else (3, 7)
    disabled = mode in {17, 33}
    ET.SubElement(parent, "ExtraSpawns", graves="false" if disabled else "true",
        types="0 2" if scene == 1 else "0 2 4", carrier="20", bobsledFallback="0 0 0 0",
        poolCount=str(pool_count if scene in {2, 3} and not disabled else 0), poolPoints=str(pool_points),
        skyCount=str(sky_count if (scene in {4, 5} or mode == 20) and not disabled else 0), skyPoints=str(sky_points),
        skyEveryWave="true" if mode == 20 else "false", graveMinimumAge="100",
        gravesOnFlag="true" if mode == 46 else "false", graveCountdownChance="true" if mode == 46 else "false")


def whack_policy(parent: ET.Element) -> None:
    whack = ET.SubElement(parent, "WhackSpawns", rollRange="30", waveDelay="2000",
        initialGraves="9", graveTarget="5", finalCount="20", finalTypes="2 4",
        firstMinimumDelay="100", firstMaximumDelay="200", lastMinimumDelay="30", lastMaximumDelay="60", delayEndWave="12")
    two = [0, 30, 10, 10, 15, 18]
    three = [0, 0, 0, 0, 10, 13]
    pail = [0, 0, 0, 10, 15, 15]
    cone = [0, 0, 30, 30, 30, 30]
    # The native type roll is also Rand(30). The second threshold can exceed 30.
    for index in range(6):
        cone_weight = min(cone[index], 30 - pail[index])
        ET.SubElement(whack, "Stage", twoWeight=str(min(two[index], 30 - three[index])), threeWeight=str(three[index]),
            types="4 2 0", weights=number_list([pail[index], cone_weight, 30 - pail[index] - cone_weight]),
            threeTypes="2", threeWeights="30")


def boss_policy(parent: ET.Element) -> None:
    boss = ET.SubElement(parent, "BossSpawns", type="25")
    for age, types in [(3499, [0]), (7999, [2]), (12499, [4])]:
        ET.SubElement(boss, "Spawn", maximumAge=str(age), types=number_list(types))
    # gBossZombieList @0x697630, verified from the TV binary.
    pool = [2, 4, 7, 3, 15, 21, 12, 22, 18, 5, 6, 23]
    ET.SubElement(boss, "Spawn", row="0", types=number_list(pool[:11]))
    ET.SubElement(boss, "Spawn", types=number_list(pool))


def load_tv_source(source: Path) -> tuple[ET.Element, str]:
    raw = source.read_bytes()
    text = raw.decode("utf-8-sig")
    if not text.rstrip().endswith("</PVZ>"):
        text += "\n</PVZ>\n"
    original = ET.fromstring(text)
    records = original.findall("Level")
    if len(records) != 50 or {int(record.get("id", "0")) for record in records} != set(range(1, 51)):
        raise ValueError("The supplied TV source must contain adventure levels 1 through 50")
    return original, hashlib.sha256(raw).hexdigest()


def vase_policy(parent: ET.Element, mode: int = 0) -> None:
    # Challenge::ScaryPotterPopulate @0x187008, including adventure's three rounds.
    def layout(columns: list[int], pots: list[tuple[int, int, int, int]], leaf: int = 2, **attributes: str) -> ET.Element:
        element = ET.SubElement(parent, "Vases", excludedColumns=number_list(columns), leafCount=str(leaf), **attributes)
        for kind, zombie, plant, count in pots:
            options = dict(kind={1: "plant", 2: "zombie", 3: "sun"}[kind], count=str(count))
            if zombie >= 0:
                options["zombie"] = str(zombie)
            if plant >= 0:
                options["plant"] = str(plant)
            ET.SubElement(element, "Pot", options)
        return element
    if mode == 0:
        layout(list(range(6)), [(1,-1,0,5), (1,-1,17,5), (2,0,-1,4), (2,4,-1,1)], 0, minimumStage="0", maximumStage="0")
        layout(list(range(5)), [(1,-1,0,4), (1,-1,5,5), (1,-1,17,4), (2,0,-1,5), (2,4,-1,1), (2,7,-1,1)], minimumStage="1", maximumStage="1")
        layout(list(range(4)), [(1,-1,0,5), (1,-1,5,5), (1,-1,12,5), (2,0,-1,6), (2,4,-1,2), (2,8,-1,1), (2,15,-1,1)], 3, minimumStage="2", maximumStage="2")
    elif mode == 70:
        element = layout([0,1], [(1,-1,53,6), (1,-1,5,2), (1,-1,0,1), (1,-1,18,2), (1,-1,17,5),
            (1,-1,4,1), (1,-1,3,1), (1,-1,25,1), (3,-1,-1,1), (2,0,-1,8), (2,4,-1,5), (2,15,-1,1), (2,23,-1,1)])
        element.findall("Pot")[9].attrib.update(stageDelta="-1", stageDivisor="10", maximumCount="8")
        element.findall("Pot")[12].attrib.update(stageDelta="1", stageDivisor="10", maximumCount="9")
    elif mode in VASE_LAYOUTS:
        columns, pots = VASE_LAYOUTS[mode]
        layout(columns, pots)


def mound_policy(parent: ET.Element, mode: int) -> None:
    if mode == 76:
        rows = [(0,0,[0]), (1,1,[6]), (2,2,[3]), (3,3,[7]), (4,4,[23])]
    else:
        rows = [(0,0,[0]), (1,1,[5,2]), (2,2,[3,7]), (3,3,[6,15]), (4,1000000,[23,22,12])]
    for minimum, maximum, types in rows:
        ET.SubElement(parent, "Mound", minimumLevel=str(minimum), maximumLevel=str(maximum), types=number_list(types))


VASE_LAYOUTS = {
    52: ([0, 1, 2, 3], [
        (1, -1, 0, 5),
        (1, -1, 5, 5),
        (1, -1, 17, 5),
        (2, 0, -1, 6),
        (2, 4, -1, 3),
        (2, 15, -1, 1),
    ]),
    54: ([0, 1, 2, 8], [
        (1, -1, 53, 7),
        (1, -1, 5, 3),
        (1, -1, 3, 3),
        (1, -1, 4, 2),
        (2, 0, -1, 6),
        (2, 4, -1, 3),
        (2, 15, -1, 1),
    ]),
    56: ([0, 1, 2], [
        (1, -1, 53, 6),
        (1, -1, 5, 4),
        (1, -1, 17, 2),
        (1, -1, 12, 3),
        (1, -1, 3, 3),
        (2, 0, -1, 8),
        (2, 4, -1, 2),
        (2, 8, -1, 1),
        (2, 15, -1, 1),
    ]),
    58: ([0, 1], [
        (1, -1, 8, 11),
        (1, -1, 12, 4),
        (1, -1, 53, 4),
        (2, 15, -1, 8),
        (2, 0, -1, 7),
        (2, 7, -1, 1),
    ]),
    60: ([0, 1], [
        (1, -1, 53, 6),
        (1, -1, 30, 3),
        (1, -1, 17, 4),
        (1, -1, 12, 2),
        (1, -1, 5, 2),
        (1, -1, 31, 3),
        (2, 0, -1, 6),
        (2, 4, -1, 5),
        (2, 15, -1, 1),
        (2, 7, -1, 3),
    ]),
    62: ([0, 1], [
        (1, -1, 53, 7),
        (1, -1, 17, 2),
        (1, -1, 23, 5),
        (1, -1, 18, 2),
        (1, -1, 22, 4),
        (2, 0, -1, 7),
        (2, 3, -1, 5),
        (2, 7, -1, 2),
        (2, 15, -1, 1),
    ]),
    64: ([0, 1, 2], [
        (1, -1, 21, 13),
        (1, -1, 3, 3),
        (1, -1, 17, 3),
        (2, 0, -1, 10),
        (2, 4, -1, 1),
    ]),
    66: ([0, 1], [
        (1, -1, 8, 7),
        (1, -1, 23, 3),
        (1, -1, 17, 5),
        (1, -1, 53, 4),
        (2, 15, -1, 8),
        (2, 0, -1, 4),
        (2, 18, -1, 4),
    ]),
    68: ([0, 1], [
        (1, -1, 53, 6),
        (1, -1, 5, 2),
        (1, -1, 0, 2),
        (1, -1, 18, 2),
        (1, -1, 17, 5),
        (1, -1, 4, 1),
        (1, -1, 3, 1),
        (1, -1, 25, 1),
        (2, 0, -1, 8),
        (2, 4, -1, 5),
        (2, 15, -1, 1),
        (2, 23, -1, 1),
    ]),
}

def normal_adventure_records(tables: SpawnTables) -> ET.Element:
    # NormalLevel disables XML in the reference project's Board::IsLevelDataLoaded
    # and LawnApp::LoadLevelConfiguration, then calls the binary's wave picker.
    # gZombieWaves @0x695C38; permission table @0x694120; picker @0x161870.
    original = ET.Element("PVZ", version="1.2.0.1073")
    for repeat in (False, True):
        for level in range(1, 51):
            count = 6 if level == WHACK_LEVEL else tables.wave_counts[level - 1]
            if repeat and level != WHACK_LEVEL and level not in MINI_BOSS_LEVELS:
                count = 20 if count <= 9 else count + 10
            scene = 1 if level == 35 else 5 if level == 50 else (level - 1) // 10
            pool = [definition.zombie_type for definition in tables.definitions
                if definition.zombie_type != YETI_TYPE and definition.starting_level <= level
                and definition.random_weight > 0 and tables.permissions[definition.zombie_type][level - 1]]
            if level == WHACK_LEVEL:
                pool = [0, 2, 4]
            elif repeat and level == BOWLING_LEVEL:
                pool = [0, 2, 4, 3, 5]
            element = ET.SubElement(original, "Level", mode="0", id=str(level), scene=str(scene),
                adventureCycle="repeat" if repeat else "first", allowedZombies=number_list(pool),
                allowYeti="true" if repeat and level != WHACK_LEVEL else "false")
            for index in range(count):
                points = 2 * index // 5 + 1 if repeat and level not in {BOWLING_LEVEL, WHACK_LEVEL} else index // 3 + 1
                interval = count if not repeat and count < 10 else 10
                flag = not (level == 1 and not repeat) and index % interval == interval - 1
                ET.SubElement(element, "Wave", points=str(points), flag="1" if flag else "0")
    return original


def build_manifest(tables: SpawnTables, source: Path, profile: str = "tv") -> ET.Element:
    if profile == "tv":
        original, checksum = load_tv_source(source)
        root = ET.Element("PVZ", version=original.get("version", "1.2.0.1073"), schemaVersion="3", tvSourceSHA256=checksum)
        root.append(ET.Comment(" TV 方案：冒险保留用户提供的 TV XML。完整独立文件，分发时改名 levels.xml。波次索引从 0 开始。 "))
    elif profile == "pc":
        original = normal_adventure_records(tables)
        root = ET.Element("PVZ", version=original.get("version"), schemaVersion="3", spawnSource="NormalLevel")
        root.append(ET.Comment(" 原版方案：参考工程 NormalLevel 恢复的内置冒险算法，一周目和二周目分别配置。保留全部 TV 模式，分发时改名 levels.xml。 "))
    else:
        raise ValueError("Unknown spawn profile")
    defaults = ET.SubElement(root, "Defaults", spawnRules="xml", flagRules="xml", waveCount="0", allowedZombies="")
    defaults.extend(pipeline())
    ET.SubElement(defaults, "Summons", backupDancer="9", imp="24", bossBungee="20", bossBungeeCount="3",
        aquarium="11", squirrel="0", bobsled="13")
    for record in original.findall("Level"):
        level = int(record.get("id"))
        element = copy.deepcopy(record)
        element.set("mode", "0-1")
        element.set("waveCount", str(len(record.findall("Wave"))))
        root.append(element)
        count = int(element.get("waveCount"))
        multiplier = 4 if level in {5, 25} else 3 if level in {10, 20, 30, 40} else 2 if level == 45 else 1
        actions = pipeline(multiplier)
        if multiplier != 1:
            element.extend(copy.deepcopy(actions))
        if level == 1:
            element.set("skipShopSeedTutorial", "true")
        scene = int(element.get("scene", "0"))
        extra_policy(element, scene, level)
        if level == 15:
            whack_policy(element)
        if level == 50:
            boss_policy(element)
        if level == 35:
            vase_policy(element)
        introduction = next((definition.zombie_type for definition in tables.definitions
            if definition.starting_level == level and level > 1), None)
        missing = [definition.zombie_type for definition in tables.definitions
            if definition.zombie_type != YETI_TYPE and definition.starting_level <= level
            and definition.random_weight > 0 and tables.permissions[definition.zombie_type][level - 1]]
        for index, wave in enumerate(element.findall("Wave")):
            wave.set("index", str(index))
            flag = wave.get("flag", "0") == "1"
            wave.set("flag", "1" if flag else "0")
            if level == 45 and flag:
                wave.set("replaceActions", "true")
                ET.SubElement(wave, "Spawn", types="20", count="5")
                continue
            additions = []
            introduced_here = introduction is not None and introduction != 10 and (
                index in {6, count - 1} if introduction in {16, 17} else
                index == count // 2 if introduction == YETI_TYPE else index in {count // 2, count - 1})
            if introduced_here:
                attributes = {"types": str(introduction)}
                if introduction == YETI_TYPE:
                    attributes["yetiUnkilledOnly"] = "true"
                additions.append(ET.Element("Spawn", attributes))
            if index == count - 1:
                if level == 50:
                    additions.append(ET.Element("Spawn", types="23"))
                if missing:
                    additions.append(ET.Element("Missing", types=number_list(missing)))
            if additions:
                wave.set("replaceActions", "true")
                wave.extend(copy.deepcopy(actions[:-1]))
                wave.extend(additions)
                wave.append(copy.deepcopy(actions[-1]))

    for mode, name in mode_names(REPOSITORY).items():
        if mode <= 1:
            continue
        scene = challenge_scene(mode)
        stage_mode = 2 <= mode <= 16 or mode == 32
        count = (10 if mode <= 6 or mode == 32 else 20) if stage_mode else challenge_wave_count(mode)
        stages = ["*", "0"] if 2 <= mode <= 16 else ["*"]
        for stage in stages:
            attributes = dict(mode=str(mode), id="*", name=name, waveCount=str(count), allowYeti="false" if mode in {26, 31} else "true")
            if scene is not None:
                attributes["scene"] = str(scene)
            if stage_mode:
                attributes["survivalStage"] = stage
            pool = [0, 2] if 2 <= mode <= 6 else MODE_POOLS.get(mode, [0, 2, 4])
            attributes["allowedZombies"] = "survival" if (2 <= mode <= 16 and stage == "*") or mode in {84, 85, 86, 87} else number_list(pool)
            element = ET.SubElement(root, "Level", attributes)
            if stage_mode and (stage != "0" or mode == 32):
                element.set("pointNumerator", "2")
                element.set("pointDenominator", "5")
                element.set("pointStageStride", str(count))
                if mode == 32:
                    element.set("pointIndexOffset", "10")
                if mode != 32:
                    element.set("firstWaveDelay", "600")
            multiplier = 6 if mode == 28 else 4 if mode in {18, 26, 34, 83} else 2 if mode in {22, 27, 47, 49} else 1
            actions = pipeline(multiplier, mode in {17, 33})
            if multiplier != 1 or mode in {17, 33}:
                element.extend(copy.deepcopy(actions))
            extra_policy(element, scene if scene is not None else 0, mode=mode)
            if mode == 31:
                whack_policy(element)
            if mode in {36, 88}:
                boss_policy(element)
            if mode in range(52, 71, 2):
                vase_policy(element, mode)
            if mode in {75, 76}:
                mound_policy(element, mode)
            if mode in {91, 93}:
                element.append(ET.Comment(" 参考工程扩展模式；并非 TV 原版发售模式。 "))
            for index in range(count):
                wave_attributes = dict(index=str(index), flag="1" if mode == 76 or index % 10 == 9 else "0")
                if not stage_mode or stage == "0":
                    points = 3 * index // 4 if mode == 20 else 4 if mode == 76 else 4 * index // 5 + 1 if mode in {91, 93} else index // 3 + 1
                    wave_attributes["points"] = str(points)
                wave = ET.SubElement(element, "Wave", wave_attributes)
                if mode == 49 and wave_attributes["flag"] == "1":
                    wave.set("replaceActions", "true")
                    ET.SubElement(wave, "Spawn", types="20", count="5")
                elif mode == 28:
                    types = [21] * 10 if index % 10 == 5 else [15] * 10 if index % 10 == 8 else [23] * 3 if index == 19 else [23] * 5 if index == 29 else []
                    if types:
                        wave.set("replaceActions", "true")
                        wave.extend(copy.deepcopy(actions[:-1]))
                        ET.SubElement(wave, "Spawn", types=number_list(types))
                        wave.append(copy.deepcopy(actions[-1]))
                elif mode == 91 and index == count - 1:
                    wave.set("replaceActions", "true")
                    wave.extend(copy.deepcopy(actions[:-1]))
                    ET.SubElement(wave, "Spawn", types="23")
                    wave.append(copy.deepcopy(actions[-1]))
    return root


def write_manifest(root: ET.Element, destination: Path) -> None:
    ET.indent(root, space="  ")
    ET.ElementTree(root).write(destination, encoding="utf-8", xml_declaration=True)
    with destination.open("a", encoding="utf-8", newline="\n") as stream:
        stream.write("\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, default=REPOSITORY / "Config/OriginalSpawns")
    parser.add_argument("--tv-source", type=Path, default=REPOSITORY / "Config/OriginalSpawns/tv-adventure-source.txt")
    parser.add_argument("--profile", choices=("both", "tv", "pc"), default="both")
    options = parser.parse_args()
    tables = read_tables(REPOSITORY)
    options.output_dir.mkdir(parents=True, exist_ok=True)
    profiles = ("tv", "pc") if options.profile == "both" else (options.profile,)
    for profile in profiles:
        destination = options.output_dir / f"levels.{profile}.xml"
        write_manifest(build_manifest(tables, options.tv_source, profile), destination)
        print(destination)


if __name__ == "__main__":
    main()
