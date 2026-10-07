# SCBP v1

SCBP（Shader Compiled Binary Package）把同一个 `.sc` 着色器针对多个渲染后端生成的 **bgfx shaderc 二进制**封装在一个文件中。顶点和片元阶段分别打包，游戏端约定读取 `main.vert.scbp`、`main.frag.scbp`。计算阶段也可以单独打包。

游戏端由 `BgfxRenderDevice` 从 `assets/shaders` 加载包，按实际 renderer 选择分段。`setup.env` 的 `SEXY_VIDEO_DRIVER` 可设置 `auto、dx11、dx12、gl、vk、metal`。

项目当前使用的着色器源码和已生成包位于 [Shaders](../Shaders/README.md)，目标掩码为 `0x17`，支持 DX11/12、OpenGL、Vulkan、Metal。

## 编译与打包

要求 Python 3.10 或更新版本，仅使用标准库。脚本在项目的 `Tools/pack_scbp.py` 中；它调用已安装的 bgfx `shaderc`，不会修改 bgfx 的工具目录。编译器优先从项目 `vcpkg_installed` 中查找，兼容 MSBuild 产生的双层 triplet 目录，也检查 `VCPKG_ROOT/installed`、`PATH`。可以通过 `--shaderc` 显式指定。

例如源文件在 `shaders/`，其中包含 `main.vert.sc`、`main.frag.sc`、`varying.def.sc`：

```powershell
python Tools/pack_scbp.py build shaders/main.vert.sc shaders/main.frag.sc --targets dx,metal --output-dir assets/shaders
```

只编译所选的两个后端，输出 `assets/shaders/main.vert.scbp` 和 `assets/shaders/main.frag.scbp`。也可以直接指定 32 位掩码，下面命令与上面等价：

```powershell
python Tools/pack_scbp.py build shaders/main.vert.sc shaders/main.frag.sc --flags 0x3 --output-dir assets/shaders
```

单个源文件可用 `-o` 指定输出；不指定输出路径时，在源文件旁生成同名 `.scbp`。阶段从 `.vert.sc`、`.frag.sc`、`.comp.sc` 推断，其他名称需要 `--type vertex|fragment|compute`。

编译时自动添加源文件所在目录和已安装的 `include/bgfx` 搜索路径，额外目录通过重复 `-i PATH` 指定。顶点/片元阶段默认使用源文件旁的 `varying.def.sc`，也可以用 `--varyingdef PATH` 指定；计算阶段不要求这个文件。

其他编译选项：

- `--profile metal=metal23-14`：覆盖某一后端的 shaderc profile。
- `--platform metal=ios`：覆盖该后端的编译目标系统。
- `--define NAME=VALUE`：传入预处理宏，可重复使用。
- `-O 0|1|2|3`：优化级别，默认为 3。
- `--debug`、`--werror`：开启调试信息、将编译警告视为错误。

profile 覆盖必须仍属于所选后端的语言类别。DX 分段限定为 `s_4_0`/`s_5_0`，保持 D3D11/D3D12 共用语义；不能把 SPIR-V 等输出标成 DX。

默认 profile 和 platform 见下表。宿主机的 shaderc 必须支持所选目标，DirectX 编译通常需要 Windows。Metal 分段是 shaderc 生成的 bgfx 二进制，其中可以包含 MSL 源码；它不是 Apple 的 `.metallib`。若要区分 macOS/iOS 的 profile 或平台宏，应分别生成对应发行包。

任何一个所选后端或源文件编译失败，均不写入输出包；不会静默丢弃失败目标。全部编译成功后，用临时文件替换输出。

## 打包现有二进制与检查

不同宿主机生成的 shaderc `.bin` 也可以直接组合。例如：

```powershell
python Tools/pack_scbp.py pack --type vertex --binary dx=build/main.vert.dx.bin --binary metal=build/main.vert.metal.bin --flags 0x3 -o build/main.vert.scbp
python Tools/pack_scbp.py inspect build/main.vert.scbp
```

`pack` 不重新编译；必须提供完整的 bgfx shaderc 输出，不能直接传入裸 DXBC、SPIR-V、MSL 或 GLSL。未指定 `--flags`/`--targets` 时，按提供的二进制自动生成掩码；指定时要求目标恰好匹配。各分段必须属于同一阶段、使用相同的 bgfx shader 二进制版本。`inspect` 校验头部、表格、边界、阶段标签并列出实际分段。

## 32 位目标标志

标志表示**渲染后端**，不是操作系统，也不使用 `bgfx::RendererType` 的枚举序号。位定义永久固定，每个位对应一个分段。

| Bit | 掩码 | 脚本名称 | bgfx 渲染后端 | 默认 platform | 顶点/片元 profile | 计算 profile |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | `0x00000001` | `dx` | Direct3D11、Direct3D12 | `windows` | `s_5_0` | `s_5_0` |
| 1 | `0x00000002` | `metal` | Metal | `osx` | `metal` | `metal` |
| 2 | `0x00000004` | `opengl` | OpenGL | `linux` | `150` | `430` |
| 3 | `0x00000008` | `opengles` | OpenGLES | `android` | `300_es` | `310_es` |
| 4 | `0x00000010` | `vulkan` | Vulkan | `linux` | `spirv` | `spirv` |
| 5–31 | — | — | 保留供后续后端扩展 | — | — | — |

例如 `dx | metal = 0x00000003`，包中只含 DirectX 和 Metal 两份内容；`0x0000001f` 包含表中全部五种。当前工具拒绝零掩码、未知位及超出 `uint32` 范围的值。

OpenGL 图形阶段默认使用 GLSL 150，可供 OpenGL 3.3 上下文使用；当前已安装 shaderc 的内置 GLSL 优化器支持该版本，但不接受 GLSL 330。计算阶段走 shaderc 的计算着色器编译路径，使用 GLSL 430。

## 二进制布局

全部整数均为**无符号、小端序**。文件中没有 C/C++ 结构体填充，不得通过结构体大小或裸指针强转来解析。SCBP 格式版本与 bgfx shader 二进制版本是两件事。

固定头部长度为 16 字节：

| 文件偏移 | 长度 | 字段 | 含义 |
| --- | --- | --- | --- |
| 0 | 4 | `magic` | ASCII `SCBP`，字节 `53 43 42 50` |
| 4 | 4 | `version` | `uint32`，当前为 1 |
| 8 | 4 | `stage` | `uint32`：1=vertex、2=fragment、3=compute |
| 12 | 4 | `target_flags` | `uint32` 目标后端掩码 |

头部之后为分段表。条目数 `N = popcount(target_flags)`，**按置位 bit 从低到高排序**，每个条目占 8 字节：

| 条目内偏移 | 长度 | 字段 | 含义 |
| --- | --- | --- | --- |
| 0 | 4 | `offset` | `uint32`，分段在整个文件中的绝对字节偏移 |
| 4 | 4 | `size` | `uint32`，完整 shaderc 二进制的字节数 |

分段数据紧接表格，也按同样顺序连续存放，没有压缩、补齐字节或尾随数据。第一个分段的 offset 为 `16 + 8 * N`，后续 offset 等于前一个 offset 加 size。文件总大小不得超过 `0xffffffff` 字节。

以 DX+Metal 为例：

```text
0..15    SCBP header: flags=0x00000003
16..23   DX entry:    offset=32, size=dx_size
24..31   Metal entry: offset=32+dx_size, size=metal_size
32..     DX shaderc binary, then Metal shaderc binary
```

## 后续游戏端加载约定

bgfx 初始化后，按当前 `bgfx::getRendererType()` 映射到上表的单个位，分别加载 `main.vert.scbp` 和 `main.frag.scbp`：

1. 验证 magic、版本、期望阶段、已知且非零的 target_flags，以及完整分段表。
2. 确认当前后端位存在。不存在时报告“该包不支持当前渲染后端”；不可取其他后端的分段代替。
3. 当前后端的表格索引为 `popcount(target_flags & (backend_bit - 1))`。
4. 验证所有分段的连续布局与大小。先确认 `offset <= file_size`，再用 `size <= file_size - offset` 检查边界，避免加法溢出和无符号减法下溢。不得接受截断、重叠、表内偏移或尾随数据。
5. 顶点/片元分段应分别有 shaderc 的 `VSH`/`FSH` 前缀，后一个字节为 bgfx shader 二进制版本。使用匹配当前 bgfx 库的 shaderc 重新生成包。
6. 用 `bgfx::copy(segment_bytes, segment_size)` 复制所选分段，再传入 `bgfx::createShader`。随后用顶点/片元 handle 创建 program。不要把整个 SCBP 文件传给 `createShader`。

读取层从资源目录取得文件字节即可；SCBP 不依赖 Windows 文件 API。游戏端无需 Python 或 shaderc，也不会在运行时重新编译 `.sc`。

## 工具验证

```powershell
python -m unittest discover -s Tools/tests -v
```

测试覆盖固定字节布局、稀疏位掩码、损坏/截断分段、阶段和版本错误，以及失败时保留旧输出。检测到本地 shaderc 时，还会实际编译 DX+Metal 顶点/片元包、验证重打包一致性，并检查各默认后端的图形和计算 profile；未安装 shaderc 时跳过编译集成测试。
