#pragma once
#include <bgfx/bgfx.h>
#include <cstdint>
#include <string>
#include <vector>

namespace Sexy {
enum class ShaderStage : uint32_t { Vertex = 1, Fragment = 2, Compute = 3 };
// All fields are read as little-endian integers, independent of C++ layout.
bool SelectShaderBinary(const std::vector<unsigned char>& thePackage,
                        ShaderStage theStage, bgfx::RendererType::Enum theRenderer,
                        std::vector<unsigned char>& theBinary, std::string& theError);
bgfx::ShaderHandle LoadShaderPackage(const std::string& theName, ShaderStage theStage);
}
