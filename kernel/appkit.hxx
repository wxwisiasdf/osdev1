#ifndef USER_HXX
#define USER_HXX 1

#include <cstdint>
#include "vendor.hxx"

namespace AppKit
{
struct ProgramInfo
{
    int (*entryPoint)(char32_t[]) = nullptr;
    uint8_t icon[16 * 16][3] = {};
} ALIGN(2048);
}

#endif
