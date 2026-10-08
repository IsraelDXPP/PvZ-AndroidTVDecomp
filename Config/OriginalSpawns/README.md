# 统一关卡配置

只加载一个 `properties/levels.xml`。本目录的 [levels.xml](levels.xml) 包含冒险 50 关的两个周目、所有已定义的模式（0–93）、生存阶段，以及同一文件中的原版数据表和规则来源。94 是原枚举空洞，没有对应关卡。

构建 Lawn 项目会将文件复制到 exe 旁的 `assets/properties/levels.xml`；如果已有 `assets/files/`，则复制到其 `properties/`。使用 `--assets` 时，将文件放到所指定资源根的 `properties/levels.xml`。不需要二周目文件或额外规则文件。重新构建会更新默认目录里的配置。

## 选择记录

`Level` 通过 `mode`、`id`、`adventureCycle` 和 `survivalStage` 选择。模式、关卡和阶段支持精确整数、闭区间（例如 `0-1`）、`*`。冒险周目为 `first`、`repeat`、`any`，按存档是否已通关冒险判断；阶段从 0 开始，生存和坚不可摧换阶段时重新选择。读档也重新加载当前记录。

优先使用 `priority` 较大的记录（默认 0）；相同优先级按精确选择器优于区间、区间优于通配符累计评分。最终最优记录同分则报错，不依赖文件排列顺序。没有匹配、文件缺失或当前记录解析失败时，清除旧配置并回到原版规则；解析错误写入 stderr。

## 继承与逐波覆盖

```xml
<PVZ schemaVersion="2">
  <Defaults spawnRules="native" flagRules="native"
            allowedZombies="native" waveCount="native" />
  <Level mode="20" id="*" waveCount="25" name="重型武器">
    <Wave index="0" points="0" />
    <!-- 未填写的波保留原版预算、旗帜与出怪规则 -->
  </Level>
  <Level mode="2" id="*" survivalStage="*" />
  <Level mode="2" id="*" survivalStage="0" waveCount="10" />
</PVZ>
```

`spawnRules="native"` 保留未指定的字段。`waveCount`、`allowedZombies` 和每波 `points` 可写 `native`；缺省也继承。稀疏 `Wave index` 只覆盖指定波，不把总波数改为覆盖条数。旧 XML 不写 `spawnRules` 时使用 `legacy`：波数由 Wave 推导、允许列表来自 XML，冒险旗帜由 XML 决定，保留旧文件行为。

| 属性 | 用途 |
| --- | --- |
| `waveCount` | 波数 0–100；`native` 继承原版 |
| `allowedZombies` | 空格分隔的僵尸类型 0–33；`native` 保留存档和阶段判断 |
| `scene` | 场景枚举 0–9；未写时使用原版场景 |
| `initialSunMoney` | 初始阳光；native 策略允许显式 0 |
| `skipShopSeedTutorial` | 跳过 1-1 商店道具教学，避免教学额外大群出怪 |
| `firstWaveDelay` | 首波等待，游戏更新次数，1–600000 |
| `flagRules` | `native` 原版旗帜；`xml` 仅由 XML 指定；`legacy` 旧文件策略 |
| `nativeFlagWave` | 是否执行旗帜波的预算倍率和固定追加，默认 true |
| `nativePointMultiplier` | 是否执行关卡的点数倍率，默认 true |
| `nativeIntroductions` | 是否执行冒险新僵尸追加，默认 true |
| `nativeLastWave` | 是否执行冒险最后一波补怪，默认 true |

每个 `Wave` 可用 `index`（0–99，不写则接上一波）、`points`（允许 0）、`flag="0|1|native"`、`mustHaveZombies`、`afterNativeZombies`、`nextWaveDelay`。两种必出列表合计最多 50 只；索引不能超过显式波数。`mustHaveZombies` 在旗帜处理前加入并扣点，`afterNativeZombies` 在原版固定追加之后、随机填充之前加入并扣点。`nextWaveDelay` 覆盖该波之后的等待，但原版教学暂停、血量提前出怪和大波提示仍有效。

原有的 `initPlantSetting="[列 行 种子]"`、`initTomb="[列 行]"`、`initSeeds`、`forbiddenPlants`、`initSlotLimit`、`initFogColumns`、`zombieVisible`、`winCodeType` 和 `winCodeCondition` 继续解析；各字段仍交给现有玩法代码处理。特殊种子按钮和 I-Zombie 种子在 `initSeeds` 中可使用原版 ID 55–79。

## 原版配置

一周目预算为 `waveIndex / 3 + 1`；二周目通常为 `2 * waveIndex / 5 + 1`，保龄球和锤僵尸例外。一周目 1-1 为 4 波、预算 `1 1 1 2`、无旗帜，跳过商店道具教学后总计 5 只普通僵尸。

`points` 始终是旗帜处理和特殊关卡倍率前的预算。新僵尸和最后一波补怪没有在 XML 中重复追加。`PutInMissingZombies` 已按汇编修正为补齐当前波缺少的全部允许类型，跳过雪人。

重型武器（mode=20）明确列出 25 波，预算 `3 * waveIndex / 4` 向下取整，第一波为 0。原版允许普通、撑杆、橄榄球、跳跳、巨人，雪人仍受存档许可控制；空中出怪和炮台沿用 TV 专属控制器。双人冒险、合作、对战及其他 TV 模式也有记录。

生存阶段 0 列出原版预算，后续阶段保留随阶段增长的预算和抽选僵尸。砸罐子、I-Zombie、锤僵尸、僵王、禅境等仍由各自原版控制器实际运行，不能把这些玩法替换成普通 Wave 刷怪。`controller` 和 `name` 是说明属性，加载器不使用它们选择控制器。

91（Buttered Popcorn）和 93（Pool Party）是参考 AndroidTV 工程的扩展模式，也列出了其波数和预算；完整扩展玩法需要对应控制器，本次配置加载不新增这些玩法。

`OriginalSpawnRules` 子树记录原版表、地址、阶段顺序与动态条件，仅供核对，不参与运行选择；因此运行仍只需这个 XML。

## 重新生成

从仓库根运行 `python Tools/export_original_spawns.py`。脚本读取重建代码中的原版表和模式枚举，只生成本目录的一个 `levels.xml`。
