// Fixed-width scalar types used by the reconstruction. Object pointers and
// object layouts use their native C++ types on every target architecture.
#ifndef LAWNPROJECTTV_SEXY_INCLUDE_RUNTIME_SEXYTYPES_H
#define LAWNPROJECTTV_SEXY_INCLUDE_RUNTIME_SEXYTYPES_H

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <utility>
#include <type_traits>
#include "ArmIntrinsics.h"

using _WORD = std::uint16_t;
using _BYTE = std::uint8_t;
using _QWORD = std::uint64_t;
using __int64_t_ = std::int64_t;
using _BOOL4 = int;
using _BOOL1 = unsigned char;
using _UNKNOWN = void;

#include "IdaMacros.h"

#endif // LAWNPROJECTTV_SEXY_INCLUDE_RUNTIME_SEXYTYPES_H
