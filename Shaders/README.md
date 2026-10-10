# 游戏着色器

这些 `.sc` 是原 GL 后端内嵌 GLSL 的 bgfx 版本。保留当前工程的仿射变换、像素到裁剪空间公式、纹理染色、alpha test、纯色输出和 BT.601 视频转换。普通/叠加绘制通过混合状态切换。

原版对应依据：内嵌普通顶点/片元着色器位于 `libGameMain.so` 的 `0x592cb8`/`0x592e40`，纹理颜色乘顶点颜色；`GLGraphics::Flush` 位于 `0x4762c8`，`SetColor` 位于 `0x476148`，使用二维位置、二维 UV 和每顶点 RGBA。这里沿用当前工程的浮点顶点，不使用原版另一条压缩整数顶点路径的 `1/16`、`1/8192` 解码。

## 产物与平台

| 文件 | 用途 | 配对 |
| --- | --- | --- |
| `main.vert.scbp` | 通用顶点变换、UV 和颜色传递 | 以下三种片元共用 |
| `main.frag.scbp` | 纹理、顶点颜色调制、染色、alpha test、植物滤镜 | `main.vert.scbp` |
| `solid.frag.scbp` | 纯色矩形、线条等 | `main.vert.scbp` |
| `yuv.frag.scbp` | MPEG 视频的 Y/Cb/Cr 转 RGB | `main.vert.scbp` |

全部文件使用 SCBP v1，`target_flags = 0x0000001F`。每个包有 DX、Metal、OpenGL、OpenGLES、Vulkan 五个二进制分段，覆盖这些平台的 bgfx 渲染后端。DX11/12 共用 `s_5_0` 分段。协议定义见 [Tools/scbp.md](../Tools/scbp.md)。

源码和已生成 `.scbp` 保存在本目录。bgfx 后端从 `<assets>/shaders` 读取这些包。构建不生成或复制 shader，需要时自行将四个包放入资源目录的 `shaders` 子目录；若资源在 `assets/files`，着色器仍在 `assets/shaders`。

在游戏资源根的 `setup.env` 设置 `SEXY_VIDEO_DRIVER=auto|dx11|dx12|gl|vk|metal`，启动时按实际 renderer 从包中选择对应分段。指定后端失败或包缺失会报告初始化错误。

## 重新生成

在项目根目录运行：

```powershell
python Tools/pack_scbp.py build Shaders/main.vert.sc Shaders/main.frag.sc Shaders/solid.frag.sc Shaders/yuv.frag.sc --flags 0x1F --werror
python Tools/pack_scbp.py inspect Shaders/main.vert.scbp
python Tools/pack_scbp.py inspect Shaders/main.frag.scbp
```

自动使用项目已安装的 bgfx shaderc 和 `varying.def.sc`。CMake 配置时追加 `-DVCPKG_MANIFEST_FEATURES=shader-tools` 可安装宿主机 shaderc。四个包全部编译成功后才替换产物。

## bgfx 绑定约定

| 名称 | bgfx 类型/属性 | 数据 |
| --- | --- | --- |
| `a_position` | `Attrib::Position`，2 个 Float | 当前二维像素位置 |
| `a_texcoord0` | `Attrib::TexCoord0`，2 个 Float | 纹理 UV |
| `a_color0` | `Attrib::Color0`，4 个 Uint8，normalized | 当前顶点 RGBA；沿用现有预乘约定 |
| `u_transform` | `UniformType::Mat3` | 当前仿射矩阵，默认单位阵 |
| `u_viewport` | `UniformType::Vec4` | `(width, height, 0, 0)` |
| `u_blitParams` | `UniformType::Vec4` | `(colorize ? 1 : 0, alphaTest ? 1 : 0, alphaRef, FilterEffect + 1)`，默认全 0 |
| `s_texture` | `UniformType::Sampler`，stage 0 | 纹理绘制的 RGBA 图像 |
| `s_y` | `UniformType::Sampler`，stage 0 | 视频 Y 平面，R8 |
| `s_cb` | `UniformType::Sampler`，stage 1 | 视频 Cb 平面，R8 |
| `s_cr` | `UniformType::Sampler`，stage 2 | 视频 Cr 平面，R8 |

`u_transform` 使用数学上的列向量乘法，上传为 column-major 的 9 个 float。`SexyMatrix3::m` 是 row-major，因此上传前按 `m00,m10,m20,m01,m11,m21,m02,m12,m22` 排列。着色器不会再次预乘顶点颜色。

视频路径沿用当前 `DrawYuv` 的单位变换矩阵，不将顶点颜色乘入视频输出。纯色和视频片元仍声明相同的完整 varying 接口，以匹配通用顶点着色器的 bgfx interface hash。

着色器不设置混合、裁剪、纹理采样或深度状态。普通预乘 alpha 混合使用 `ONE / INV_SRC_ALPHA`，叠加绘制使用 `ONE / ONE`，由 `BgfxRenderDevice` 设置。

植物滤镜按原版 `FilterEffectDoLumSat`（`0x206a38`）处理 HSL：褪色滤镜的亮度和饱和度分别乘 1.8、0.2，轻度褪色分别乘 1.2、0.3，白色滤镜保留 alpha。`u_blitParams.w` 为 0 时关闭，为 1、2、3 时分别应用这三种滤镜。IZ 纸板按原版 PC `IZombieDrawPlant`（`0x42b690`）先用白色滤镜画三次轮廓：偏移 `(4,4)`、颜色 `(122,86,58)`，偏移 `(2,2)` 和 `(-2,-2)`、颜色 `(171,135,107)`；最后关闭滤镜，在原坐标用 `(255,201,160)` 调制植物原图。共用软件渲染路径使用相同的变换。
