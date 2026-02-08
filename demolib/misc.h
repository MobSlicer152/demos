#pragma once

#include <algorithm>
#include <cfloat>
#include <cstdlib>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h>

#define ASSERT(x)

// Get the size of an array
template <typename T, typename S, S N> constexpr S ArraySize(const T (&arr)[N])
{
    return N;
}
