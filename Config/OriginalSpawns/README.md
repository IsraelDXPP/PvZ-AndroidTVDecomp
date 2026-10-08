# TV 与 NormalLevel 原版出怪 XML

提供两个独立的分发文件，均使用 `schemaVersion="3"`：

| 文件 | 冒险数据 | 1-1 一周目 | 1-1 二周目 |
| --- | --- | --- | --- |
| [levels.tv.xml](levels.tv.xml) | 用户提供的 TV 原版 levels.xml | 4 波，预算 1、1、2、2，6 只普通僵尸 | 沿用 TV XML，4 波 |
| [levels.pc.xml](levels.pc.xml) | 参考工程 NormalLevel 恢复的内置原版算法 | 4 波，预算 1、1、1、2，5 只普通僵尸 | 20 波，预算 2*index/5+1，第 10、20 波为旗帜波 |

两份都包含完整模式和生存阶段，以及重型武器、双人、合作等 TV 专属模式。差异集中在冒险的场景、波数、预算、允许列表和周目选择。PC 文件指用户要求的 NormalLevel 方案：参考工程通过禁用 TV XML、调用原二进制算法恢复出怪，并非对独立 PC 可执行文件的再次提取。两份都显式跳过 1-1 商店种子教学，追加规则仍完全由 XML 动作执行。

分发时任选一份，复制并改名为资源目录的 `properties/levels.xml`。游戏每次运行只加载该文件，两份之间没有引用或依赖。枚举 94 是空洞；91、93 是参考工程扩展，并在 XML 中标明。

构建默认将 `levels.tv.xml` 部署成 exe 旁的 `assets/properties/levels.xml`；已有 `assets/files/` 时使用其 properties 目录。构建原版方案时加入 `/p:LevelConfigurationVariant=pc`，TV 方案为 `/p:LevelConfigurationVariant=tv`。构建不会修改两份源配置。分发后更换出怪只需替换运行资源中的 levels.xml。

源文件缺少 `</PVZ>`，转换时只在内存中补齐；原文件未改。`tv-adventure-source.txt` 保存原始字节供重新导出，运行不读取它。冒险的场景、允许列表、波数、每波 points、flag 和 mustHaveZombies 均保留。原 TV 文件的 mode=0 扩为 0-1，让双人冒险共享配置；TV 文件的两个周目都采用这份 TV XML。PC 文件按 NormalLevel 分别还原两个周目。1-1 是 4 波，预算 1、1、2、2，共 6 只普通僵尸。明确跳过商店种子教学，取消其第四波的全类型额外僵尸。

## 与旧格式的行为差异

旧扩展格式以开关控制 C++ 中的旗帜追加、登场追加、末波补齐和关卡倍率。新格式把这些规则写成 XML 动作；代码只解释动作，不再按关卡号硬塞僵尸。原 nativeFlagWave、nativePointMultiplier、nativeIntroductions、nativeLastWave 和 afterNativeZombies 已由动作替代，不应继续使用。

`points` 仍是必出列表、旗帜和倍率处理前的预算，不等于僵尸数量。`mustHaveZombies` 先按顺序加入并扣除点数，然后执行动作。允许列表只约束随机 Fill，Spawn、Missing 和 mustHaveZombies 可以显式指定列表以外的类型，保留原 TV 文件的行为。最多 100 波，每波最多 50 个条目。

配置缺失或无法选择时清空旧配置，不再回退到代码里的固定追加。XML 模式没有 Fill 就不会随机填充；允许列表为空或没有符合抽选条件的类型时也不会凭空补普通僵尸。旧不带 spawnRules 的 XML 仍解析顺序 Wave 和原有场景/种子/墓碑属性，但不恢复已经移除的代码追加。

## 选择、默认值和动作

Level 选择器为 mode、id、adventureCycle、survivalStage。数字支持整数、闭区间或 *；adventureCycle 为 first/repeat/any。priority 默认 0，较大者优先，然后比较精确程度，最终最优同分会报错。生存换阶段和读档重新选择配置。Defaults 必须在全部 Level 前。

```xml
<PVZ schemaVersion="3">
  <Defaults spawnRules="xml" flagRules="xml" allowedZombies="" waveCount="0">
    <Fill />
  </Defaults>
  <Level mode="0" id="1" waveCount="2" allowedZombies="0 2">
    <Wave index="0" points="2" />
    <Wave index="1" points="0" replaceActions="true">
      <Spawn types="4" count="2" />
    </Wave>
  </Level>
</PVZ>
```

Level 内的动作替换 Defaults 的整套动作；Wave 内的动作替换 Level 的整套动作。`replaceActions="true"` 可以显式设置空动作列表。Wave 的 index 从 0 开始；不写时顺序递增。flag 只标记大波，是否加旗帜僵尸由 Spawn 决定。

| 动作 | 效果 |
| --- | --- |
| Scale numerator="5" denominator="2" | 剩余预算乘分数，向零取整 |
| Spawn types="0 2" count="3" | 按列表顺序生成，整份列表重复 3 次，并扣点 |
| Spawn types="0" budgetCountLimit="8" | 重复次数取最近一次 Scale 前预算与 8 的较小值，最低 0；用于原版旗帜波 |
| Missing types="0 2 4" | 仅补齐当前波缺少的指定类型，扣点 |
| Fill | 从 allowedZombies 按原版权重、最早波次和预算限制填充剩余预算 |

动作可附 `flagOnly="true"`、`lastOnly="true"`、`yetiUnkilledOnly="true"`。雪人条件同时要求存档许可且未杀死后重开。蹦极突袭的旗帜波明确写 Spawn types=20 count=5，不再执行通用动作。

生存的未指定波预算为：

`floor((index + pointIndexOffset + stage * pointStageStride) * pointNumerator / pointDenominator) + pointOffset`

默认参数为 1/3、offset=1、其余 0。显式 Wave points 优先。生存后续阶段设置 2/5 和阶段波数作为 stride；坚不可摧增加 indexOffset=10。allowedZombies="survival" 明确选择原版按阶段抽取允许类型的算法；普通记录使用明确数字列表。allowYeti 明确控制是否根据存档额外开放随机雪人，默认 false。

## 专用玩法数据

这些节点都参与运行，不再只是说明：

| 节点 | 可定制内容 |
| --- | --- |
| ExtraSpawns | graves、graveMinimumAge、graveCountdownChance、gravesOnFlag；poolCount/poolPoints；skyCount/skyPoints、skyEveryWave；types、carrier、bobsledFallback |
| WhackSpawns | initialGraves、graveTarget、waveDelay、rollRange、finalCount/finalTypes；首尾最小/最大召唤间隔及 delayEndWave |
| WhackSpawns/Stage | 按当前波阶段选择；twoWeight、threeWeight 定义 2/3 只概率，其余为 1 只；types/weights 定义僵尸分布，threeTypes/threeWeights 可覆盖一次出 3 只时的分布 |
| BossSpawns | type 指定开场僵王；Spawn 按 maximumAge、row 和 types 选择召唤，按文件顺序取第一个匹配项 |
| Vases | minimumStage/maximumStage、excludedColumns、leafCount；Pot 的 kind=plant/zombie/sun、plant/zombie、count；stageDelta/stageDivisor/maximumCount 支持无尽增长 |
| Mound | 对战墓堆的 minimumLevel/maximumLevel、types |
| Summons | backupDancer、imp、bossBungee/bossBungeeCount、aquarium、squirrel、bobsled；类型 -1 禁用对应召唤 |

ExtraSpawns 未声明的数量默认 0、墓碑补怪默认关闭。types 是补怪候选池，仍受本关允许列表约束。重型武器明确声明每波空降 3 只、预算 7，以及允许列表 0 3 7 18 23；第一波的常规预算为 0，空降独立执行。蹦极载体也由 carrier 指定。保留原版地形落点、动画和玩法控制器；召唤类型需适合对应的动画动作。

锤僵尸的墓碑位置仍按原版随机选格，初始数量由 initialGraves 指定。已修复原随机函数只有 15 位而权重为 100000 的问题，恢复原版 31 位 Mersenne Twister，避免墓碑总是填满前两列。

原属性 initPlantSetting、initTomb、initSeeds、forbiddenPlants、initSlotLimit、initFogColumns、zombieVisible、winCodeType、winCodeCondition 继续支持。firstWaveDelay/nextWaveDelay 单位为游戏更新次数。控制器的动画、地形、玩家主动操作（例如 I-Zombie 放置）继续由玩法代码运行，出怪配置通过本文件选择。

## 重新导出

`python Tools/export_original_spawns.py`

指定另一份原 TV 冒险文件：`python Tools/export_original_spawns.py --tv-source "C:/path/levels.xml"`。默认生成本目录的 levels.tv.xml 和 levels.pc.xml；也可加 --profile tv 或 --profile pc 单独生成。构建会将所选方案复制为资源目录里的 levels.xml，手动定制请改对应源配置，或使用独立 --assets 目录。
