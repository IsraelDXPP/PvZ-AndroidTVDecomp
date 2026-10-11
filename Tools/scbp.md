# SCBP v1

SCBP (Shader Compiled Binary Package) combines **bgfx shaderc binaries** for multiple rendering backends into one file per shader stage. Vertex and fragment stages are packaged separately; the game loads `main.vert.scbp` and `main.frag.scbp`. Compute stages can also be packaged separately.

`BgfxRenderDevice` loads packages from `assets/shaders` and selects the segment for the actual renderer. Set `SEXY_VIDEO_DRIVER` in `setup.env` to `auto`, `dx11`, `dx12`, `gl`, `vk`, or `metal`.

The project's current sources and generated packages are in [Shaders](../Shaders/README.md). Their target mask is `0x1F`, covering DX11/12, OpenGL, OpenGLES, Vulkan, and Metal.

## Compilation and packaging

Requires Python 3.10 or newer and uses only the standard library. `Tools/pack_scbp.py` invokes an installed bgfx `shaderc` without modifying bgfx's tool directory. Compiler discovery searches the project's `vcpkg_installed` first, including the nested triplet layout created by MSBuild, then `VCPKG_ROOT/installed` and `PATH`. Override discovery with `--shaderc`.

For example, suppose `shaders/` contains `main.vert.sc`, `main.frag.sc`, and `varying.def.sc`:

```powershell
python Tools/pack_scbp.py build shaders/main.vert.sc shaders/main.frag.sc --targets dx,metal --output-dir assets/shaders
```

This compiles only the two selected backends and writes `assets/shaders/main.vert.scbp` and `assets/shaders/main.frag.scbp`. The equivalent command using a 32-bit mask is:

```powershell
python Tools/pack_scbp.py build shaders/main.vert.sc shaders/main.frag.sc --flags 0x3 --output-dir assets/shaders
```

For one source, `-o` specifies the output file. Without an output path, a matching `.scbp` is generated beside the source. Stage detection uses `.vert.sc`, `.frag.sc`, or `.comp.sc`; other names require `--type vertex|fragment|compute`.

Compilation automatically adds the source directory and installed `include/bgfx` to the include search path. Repeat `-i PATH` for more directories. Vertex/fragment stages default to `varying.def.sc` beside the source; override it with `--varyingdef PATH`. Compute stages do not require it.

Other compiler options:

- `--profile metal=metal23-14`: override a backend's shaderc profile.
- `--platform metal=ios`: override that backend's target platform.
- `--define NAME=VALUE`: pass a preprocessor definition; may be repeated.
- `-O 0|1|2|3`: optimization level, default 3.
- `--debug`, `--werror`: enable debug information or treat warnings as errors.

Profile overrides must remain in the selected backend's language family. DX segments are limited to `s_4_0` / `s_5_0` to retain shared D3D11/D3D12 semantics. Do not label SPIR-V or another backend's output as DX.

Default profiles and platforms are listed below. Host shaderc must support each selected target; DirectX compilation usually requires Windows. A Metal segment is a bgfx binary produced by shaderc and may contain MSL source. It is not an Apple `.metallib`. Generate separate distribution packages when macOS/iOS profiles or platform definitions need to differ.

If any selected backend or source fails, no output packages are written; failing targets are never silently omitted. After all compilations succeed, temporary files replace the outputs.

## Packing existing binaries and inspection

Combine shaderc `.bin` files produced on different hosts:

```powershell
python Tools/pack_scbp.py pack --type vertex --binary dx=build/main.vert.dx.bin --binary metal=build/main.vert.metal.bin --flags 0x3 -o build/main.vert.scbp
python Tools/pack_scbp.py inspect build/main.vert.scbp
```

`pack` does not recompile. Supply complete bgfx shaderc outputs, not raw DXBC, SPIR-V, MSL, or GLSL. Without `--flags` / `--targets`, the mask is inferred from the supplied binaries; an explicit mask must match exactly. Segments must share the same stage and bgfx shader binary version. `inspect` checks the header, table, boundaries, and stage tags, then lists the actual segments.

## 32-bit target flags

Flags identify **rendering backends**, not operating systems or `bgfx::RendererType` enum ordinals. Bit assignments are permanent; each set bit corresponds to one segment.

| Bit | Mask | Script name | bgfx renderer | Default platform | Vertex/fragment profile | Compute profile |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | `0x00000001` | `dx` | Direct3D11, Direct3D12 | `windows` | `s_5_0` | `s_5_0` |
| 1 | `0x00000002` | `metal` | Metal | `osx` | `metal` | `metal` |
| 2 | `0x00000004` | `opengl` | OpenGL | `linux` | `150` | `430` |
| 3 | `0x00000008` | `opengles` | OpenGLES | `android` | `300_es` | `310_es` |
| 4 | `0x00000010` | `vulkan` | Vulkan | `linux` | `spirv` | `spirv` |
| 5-31 | - | - | Reserved for future backends | - | - | - |

For example, `dx | metal = 0x00000003` contains DirectX and Metal only; `0x0000001f` contains all five listed backends. The tool rejects zero masks, unknown bits, and values outside the `uint32` range.

OpenGL graphics stages default to GLSL 150, suitable for an OpenGL 3.3 context. The installed shaderc's built-in GLSL optimizer supports this version but not GLSL 330. Compute shaders use shaderc's compute compilation path with GLSL 430.

## Binary layout

All integers are **unsigned and little-endian**. Files contain no C/C++ structure padding; do not parse them using structure sizes or raw pointer casts. The SCBP format version is independent of the bgfx shader binary version.

The fixed header is 16 bytes:

| File offset | Length | Field | Meaning |
| --- | --- | --- | --- |
| 0 | 4 | `magic` | ASCII `SCBP`, bytes `53 43 42 50` |
| 4 | 4 | `version` | `uint32`, currently 1 |
| 8 | 4 | `stage` | `uint32`: 1=vertex, 2=fragment, 3=compute |
| 12 | 4 | `target_flags` | `uint32` backend mask |

A segment table follows the header. Entry count is `N = popcount(target_flags)`, ordered **from the lowest set bit to the highest**. Each entry occupies 8 bytes:

| Entry offset | Length | Field | Meaning |
| --- | --- | --- | --- |
| 0 | 4 | `offset` | `uint32`, absolute byte offset of the segment in the file |
| 4 | 4 | `size` | `uint32`, byte length of the complete shaderc binary |

Segment data follows the table contiguously in the same order, without compression, padding, or trailing data. The first offset is `16 + 8 * N`; each subsequent offset is the previous offset plus its size. Total file size must not exceed `0xffffffff` bytes.

DX+Metal example:

```text
0..15    SCBP header: flags=0x00000003
16..23   DX entry:    offset=32, size=dx_size
24..31   Metal entry: offset=32+dx_size, size=metal_size
32..     DX shaderc binary, then Metal shaderc binary
```

## Runtime loading contract

After bgfx initialization, map `bgfx::getRendererType()` to the corresponding bit above and load `main.vert.scbp` and `main.frag.scbp` separately:

1. Validate magic, version, expected stage, known nonzero target flags, and the complete segment table.
2. Require the current backend's bit. If absent, report that the package does not support the current renderer. Never substitute another backend's segment.
3. Calculate the backend's table index as `popcount(target_flags & (backend_bit - 1))`.
4. Validate contiguous layout and all sizes. Check `offset <= file_size` before `size <= file_size - offset` to avoid addition overflow and unsigned subtraction underflow. Reject truncation, overlaps, offsets into the table, and trailing data.
5. Vertex/fragment segments must begin with shaderc's `VSH` / `FSH` prefix, followed by the bgfx shader binary version byte. Regenerate packages using shaderc matching the current bgfx library.
6. Copy the selected segment with `bgfx::copy(segment_bytes, segment_size)`, pass it to `bgfx::createShader`, then create the program from the vertex/fragment handles. Do not pass the whole SCBP package to `createShader`.

The read layer only needs to obtain bytes from the asset directory. SCBP does not depend on Windows file APIs. The game requires neither Python nor shaderc and does not recompile `.sc` files at runtime.

## Tool verification

```powershell
python -m unittest discover -s Tools/tests -v
```

Tests cover fixed byte layout, sparse masks, corrupt/truncated segments, stage/version errors, and preserving old outputs on failure. When local shaderc is available, they also compile DX+Metal vertex/fragment packages, verify repacking consistency, and check default graphics/compute profiles for each backend. Compiler integration tests are skipped when shaderc is unavailable.
