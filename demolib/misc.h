#pragma once

#include <algorithm>
#include <cfloat>
#include <cstdlib>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h>

extern "C" NTSYSAPI void NTAPI DbgPrint(const char* msg, ...);

#define ASSERT_MSG(x, ...)                                                                                                       \
	if (!(x))                                                                                                                    \
	{                                                                                                                            \
		extern void DECLSPEC_NORETURN ErrorMessage(int code, const char*, ...);                                                  \
		ErrorMessage(STATUS_ASSERTION_FAILURE, "ASSERT(" #x ") failed:\n" __VA_ARGS__);                                          \
	}
#define ASSERT(x) ASSERT_MSG(x, "(no message given)");

typedef uint8_t byte;

// Get the size of an array
template <typename T, typename S, S N> constexpr S ArraySize(const T (&arr)[N])
{
	return N;
}
