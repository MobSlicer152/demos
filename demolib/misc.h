#pragma once

#define _NO_CRT_STDIO_INLINE

#include <algorithm>
#include <cerrno>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <omp.h>
#include <random>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h>

extern "C" NTSYSAPI void NTAPI DbgPrint(const char* msg, ...);
extern "C" NTSYSAPI ULONG NTAPI RtlUniform(PULONG seed);
#else
#include <unistd.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>

#ifdef __INTELLISENSE__
#define FORCEINLINE
#define __declspec(x)
#else
#define FORCEINLINE __forceinline
#endif
#define STATUS_ASSERTION_FAILURE EXIT_FAILURE
#define DECLSPEC_NORETURN __declspec(noreturn)
#define DECLSPEC_ALIGN(x) __declspec(align(x))

struct PaletteColor
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
};
#endif

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

// Get the size of an array
template <typename T, typename S, S N> constexpr S ArraySize(const std::array<T, N>& arr)
{
	return N;
}
