#include "ShaderPackage.h"
#include "SDLFile.h"
#include <SDL3/SDL.h>
#include <filesystem>
#include <cstring>

namespace Sexy {
namespace {
uint32_t Read32(const unsigned char* bytes)
{
    return uint32_t(bytes[0]) | (uint32_t(bytes[1]) << 8)
        | (uint32_t(bytes[2]) << 16) | (uint32_t(bytes[3]) << 24);
}
uint32_t RendererFlag(bgfx::RendererType::Enum renderer)
{
    switch (renderer)
    {
    case bgfx::RendererType::Direct3D11: case bgfx::RendererType::Direct3D12: return 1;
    case bgfx::RendererType::Metal: return 2;
    case bgfx::RendererType::OpenGL: return 4;
    case bgfx::RendererType::OpenGLES: return 8;
    case bgfx::RendererType::Vulkan: return 16;
    default: return 0;
    }
}
}
bool SelectShaderBinary(const std::vector<unsigned char>& package, ShaderStage stage,
                        bgfx::RendererType::Enum renderer,
                        std::vector<unsigned char>& binary, std::string& error)
{
    binary.clear();
    error.clear();
    auto fail = [&error](const char* text) { error = text; return false; };
    if (package.size() < 16 || package.size() > UINT32_MAX
        || std::memcmp(package.data(), "SCBP", 4) != 0) return fail("invalid SCBP header");
    if (Read32(package.data() + 4) != 1) return fail("unsupported SCBP version");
    if (Read32(package.data() + 8) != static_cast<uint32_t>(stage)) return fail("wrong shader stage");
    uint32_t flags = Read32(package.data() + 12);
    if (!flags || (flags & ~uint32_t(31))) return fail("invalid backend mask");
    uint32_t requested = RendererFlag(renderer);
    if (!(requested & flags)) return fail("SCBP does not contain the active renderer");
    size_t count = 0;
    for (uint32_t bit = 1; bit <= 16; bit <<= 1) if (flags & bit) ++count;
    size_t offset = 16 + count * 8;
    if (package.size() < offset) return fail("truncated segment table");
    const char* tag = stage == ShaderStage::Vertex ? "VSH" : stage == ShaderStage::Fragment ? "FSH" : "CSH";
    size_t entry = 16;
    unsigned char version = 0;
    size_t selectedOffset = 0, selectedSize = 0;
    for (uint32_t bit = 1; bit <= 16; bit <<= 1)
    {
        if (!(flags & bit)) continue;
        uint32_t segmentOffset = Read32(package.data() + entry);
        uint32_t size = Read32(package.data() + entry + 4);
        if (segmentOffset != offset || size < 12 || offset > package.size()
            || size > package.size() - offset) return fail("invalid segment bounds");
        if (std::memcmp(package.data() + offset, tag, 3)) return fail("wrong bgfx shader tag");
        unsigned char currentVersion = package[offset + 3];
        if (entry != 16 && version != currentVersion) return fail("mixed bgfx shader versions");
        version = currentVersion;
        if (bit == requested) { selectedOffset = offset; selectedSize = size; }
        offset += size;
        entry += 8;
    }
    if (offset != package.size()) return fail("unexpected trailing bytes");
    binary.assign(package.begin() + selectedOffset, package.begin() + selectedOffset + selectedSize);
    return true;
}
bgfx::ShaderHandle LoadShaderPackage(const std::string& name, ShaderStage stage)
{
    std::filesystem::path root = std::filesystem::u8path(SDLFile::GetAssetRoot());
    if (root.filename() == "files") root = root.parent_path();
    root /= "shaders";
    std::string path = (root / std::filesystem::u8path(name)).u8string();
    SDL_PathInfo info{};
    if (!SDL_GetPathInfo(path.c_str(), &info) || info.type != SDL_PATHTYPE_FILE || info.size < 16 || info.size > UINT32_MAX)
    { SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot open shader %s", path.c_str()); return BGFX_INVALID_HANDLE; }
    size_t size = 0;
    void* loaded = SDL_LoadFile(path.c_str(), &size);
    if (!loaded) return BGFX_INVALID_HANDLE;
    const unsigned char* bytes = static_cast<const unsigned char*>(loaded);
    std::vector<unsigned char> package(bytes, bytes + size);
    SDL_free(loaded);
    std::vector<unsigned char> binary;
    std::string error;
    if (!SelectShaderBinary(package, stage, bgfx::getRendererType(), binary, error))
    { SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: %s", name.c_str(), error.c_str()); return BGFX_INVALID_HANDLE; }
    bgfx::ShaderHandle shader = bgfx::createShader(bgfx::copy(binary.data(), static_cast<uint32_t>(binary.size())));
    if (bgfx::isValid(shader)) bgfx::setName(shader, name.c_str());
    return shader;
}
}
