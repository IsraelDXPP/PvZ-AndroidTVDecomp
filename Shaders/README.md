# Game shaders

These `.sc` files are bgfx versions of the GLSL embedded in the original GL backend. They preserve the current affine transforms, pixel-to-clip-space formula, texture tinting, alpha testing, solid-color output, and BT.601 video conversion. Normal and additive draws use different blend states.

Original references: the regular vertex/fragment shaders embedded in `libGameMain.so` are at `0x592cb8` / `0x592e40`, with texture color multiplied by vertex color. `GLGraphics::Flush` is at `0x4762c8`, and `SetColor` at `0x476148`; they use 2D positions, 2D UVs, and per-vertex RGBA. This implementation retains the project's floating-point vertices rather than the original compressed integer path's `1/16` and `1/8192` decoding.

## Packages and platforms

| File | Purpose | Pairing |
| --- | --- | --- |
| `main.vert.scbp` | Vertex transforms, UVs, and color | Shared by all three fragment shaders below |
| `main.frag.scbp` | Textures, vertex-color modulation, tinting, alpha testing, and plant filters | `main.vert.scbp` |
| `solid.frag.scbp` | Solid-color rectangles, lines, and other primitives | `main.vert.scbp` |
| `yuv.frag.scbp` | MPEG video Y/Cb/Cr-to-RGB conversion | `main.vert.scbp` |

All packages use SCBP v1 with `target_flags = 0x0000001F`. Each contains DX, Metal, OpenGL, OpenGLES, and Vulkan segments for the corresponding bgfx renderers. DX11/12 share the `s_5_0` segment. See [Tools/scbp.md](../Tools/scbp.md) for the format.

Sources and generated `.scbp` files are stored here. The bgfx backend reads packages from `<assets>/shaders`. Builds do not generate or copy shaders. Place the four packages in the resource directory's `shaders` subdirectory yourself. When resources use `assets/files`, shaders remain in `assets/shaders`.

Set `SEXY_VIDEO_DRIVER=auto|dx11|dx12|gl|vk|metal` in the game asset root's `setup.env`. Startup selects the segment for the actual renderer. An unavailable requested backend or missing package causes an initialization error.

## Regeneration

Run from the repository root:

```powershell
python Tools/pack_scbp.py build Shaders/main.vert.sc Shaders/main.frag.sc Shaders/solid.frag.sc Shaders/yuv.frag.sc --flags 0x1F --werror
python Tools/pack_scbp.py inspect Shaders/main.vert.scbp
python Tools/pack_scbp.py inspect Shaders/main.frag.scbp
```

The script discovers the project's installed bgfx shaderc and `varying.def.sc`. Add `-DVCPKG_MANIFEST_FEATURES=shader-tools` when configuring CMake to install host shaderc. Outputs are replaced only after all four packages compile successfully.

## bgfx binding contract

| Name | bgfx type/attribute | Data |
| --- | --- | --- |
| `a_position` | `Attrib::Position`, 2 Float components | Current 2D pixel position |
| `a_texcoord0` | `Attrib::TexCoord0`, 2 Float components | Texture UV |
| `a_color0` | `Attrib::Color0`, 4 normalized Uint8 components | Current vertex RGBA, retaining the existing premultiplication convention |
| `u_transform` | `UniformType::Mat3` | Current affine transform; identity by default |
| `u_viewport` | `UniformType::Vec4` | `(width, height, 0, 0)` |
| `u_blitParams` | `UniformType::Vec4` | `(colorize ? 1 : 0, alphaTest ? 1 : 0, alphaRef, FilterEffect + 1)`; all zeros by default |
| `s_texture` | `UniformType::Sampler`, stage 0 | RGBA image for texture draws |
| `s_y` | `UniformType::Sampler`, stage 0 | Video Y plane, R8 |
| `s_cb` | `UniformType::Sampler`, stage 1 | Video Cb plane, R8 |
| `s_cr` | `UniformType::Sampler`, stage 2 | Video Cr plane, R8 |

`u_transform` uses mathematical column-vector multiplication and is uploaded as nine column-major floats. `SexyMatrix3::m` is row-major, so upload order is `m00,m10,m20,m01,m11,m21,m02,m12,m22`. Shaders do not premultiply vertex color again.

Video retains `DrawYuv`'s identity transform and does not multiply output by vertex color. Solid and video fragment shaders declare the same complete varying interface to match the shared vertex shader's bgfx interface hash.

Shaders do not set blend, scissor, texture sampling, or depth state. `BgfxRenderDevice` sets normal premultiplied alpha blending to `ONE / INV_SRC_ALPHA` and additive blending to `ONE / ONE`.

Plant filters follow the original `FilterEffectDoLumSat` (`0x206a38`) HSL behavior: washed-out lightness and saturation are multiplied by 1.8 and 0.2; mild washing uses 1.2 and 0.3; the white filter preserves alpha. `u_blitParams.w` of 0 disables filters; 1, 2, and 3 select these three filters respectively. I, Zombie cardboard follows the original PC `IZombieDrawPlant` (`0x42b690`): draw three white-filter outlines at `(4,4)` with `(122,86,58)`, then `(2,2)` and `(-2,-2)` with `(171,135,107)`. Finally, disable the filter and modulate the original plant at its original position with `(255,201,160)`. The shared software-rendering path uses the same transformation.
