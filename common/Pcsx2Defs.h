// SPDX-FileCopyrightText: 2002-2023 PCSX2 Dev Team
// SPDX-License-Identifier: LGPL-3.0+

#pragma once

#include "Pcsx2Types.h"
#include <cstddef>

// --------------------------------------------------------------------------------------
// Dev / Debug conditionals - Consts for using if() statements instead of uglier #ifdef.
// --------------------------------------------------------------------------------------
#ifdef PCSX2_DEVBUILD
static constexpr bool IsDevBuild = true;
#else
static constexpr bool IsDevBuild = false;
#endif

#ifdef PCSX2_DEBUG
static constexpr bool IsDebugBuild = true;
#else
static constexpr bool IsDebugBuild = false;
#endif

// Defines the memory page size for the target platform at compilation.  All supported platforms
// (which means Intel only right now) have a 4k granularity.
static constexpr unsigned int __pagesize = 0x1000;
static constexpr unsigned int __pageshift = 12;
static constexpr unsigned int __pagemask = __pagesize - 1;

// --------------------------------------------------------------------------------------
//  Microsoft Visual Studio
// --------------------------------------------------------------------------------------
#ifdef _MSC_VER

#define __forceinline_odr __forceinline
#define __noinline __declspec(noinline)
#define __noreturn __declspec(noreturn)

#define RESTRICT __restrict
#define ASSUME(x) __assume(x)

#else

// --------------------------------------------------------------------------------------
//  GCC / Clang Compilers Section
// --------------------------------------------------------------------------------------

// SysV ABI passes vector parameters through registers unconditionally.
#ifndef _WIN32
#define __vectorcall
#endif

// Inlining note: GCC needs ((unused)) attributes defined on inlined functions to suppress
// warnings when a static inlined function isn't used in the scope of a single file (which
// happens *by design* like all the friggen time >_<)

// __forceinline_odr is for member functions that are defined in headers. MSVC can't specify
// inline and __forceinline at the same time, but it required to not get ODR errors in GCC.

#define __forceinline __attribute__((always_inline, unused))
#define __forceinline_odr __forceinline inline
#define __noinline __attribute__((noinline))
#define __noreturn __attribute__((noreturn))

#define RESTRICT __restrict__

#define ASSUME(x) \
	do \
	{ \
		if (!(x)) \
			__builtin_unreachable(); \
	} while (0)

#endif

// --------------------------------------------------------------------------------------
// __releaseinline / __ri -- a forceinline macro that is enabled for RELEASE/PUBLIC builds ONLY.
// --------------------------------------------------------------------------------------
// This is useful because forceinline can make certain types of debugging problematic since
// functions that look like they should be called won't breakpoint since their code is
// inlined, and it can make stack traces confusing or near useless.
//
// Use __releaseinline for things which are generally large functions where trace debugging
// from Devel builds is likely useful; but which should be inlined in an optimized Release
// environment.
//
#define __fi __forceinline
#ifdef PCSX2_DEVBUILD
#define __ri
#else
#define __ri __fi
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// Safe deallocation macros -- checks pointer validity (non-null) when needed, and sets
// pointer to null after deallocation.

#define safe_delete(ptr) (delete (ptr), (ptr) = nullptr)
#define safe_delete_array(ptr) (delete[] (ptr), (ptr) = nullptr)
#define safe_free(ptr) (std::free(ptr), (ptr) = nullptr)

// --------------------------------------------------------------------------------------
//  ImplementEnumOperators  (macro)
// --------------------------------------------------------------------------------------
// This macro implements ++/-- operators for any conforming enumeration.  In order for an
// enum to conform, it must have _FIRST and _COUNT members defined, and must have a full
// compliment of sequential members (no custom assignments) --- looking like so:
//
// enum Dummy {
//    Dummy_FIRST,
//    Dummy_Item = Dummy_FIRST,
//    Dummy_Crap,
//    Dummy_COUNT
// };
//
// The macro also defines utility functions for bounds checking enumerations:
//   EnumIsValid(value);   // returns TRUE if the enum value is between FIRST and COUNT.
//   EnumAssert(value);
//
// It also defines a *prototype* for converting the enumeration to a string.  Note that this
// method is not implemented!  You must implement it yourself if you want to use it:
//   EnumToString(value);
//
#define ImplementEnumOperators(enumName) \
	static __fi enumName& operator++(enumName& src) \
	{ \
		src = (enumName)((int)src + 1); \
		return src; \
	} \
\
	static __fi enumName& operator--(enumName& src) \
	{ \
		src = (enumName)((int)src - 1); \
		return src; \
	} \
\
	static __fi enumName operator++(enumName& src, int) \
	{ \
		enumName orig = src; \
		src = (enumName)((int)src + 1); \
		return orig; \
	} \
\
	static __fi enumName operator--(enumName& src, int) \
	{ \
		enumName orig = src; \
		src = (enumName)((int)src - 1); \
		return orig; \
	} \
\
	static __fi bool operator<(const enumName& left, const pxEnumEnd_t&) { return (int)left < enumName##_COUNT; } \
	static __fi bool operator!=(const enumName& left, const pxEnumEnd_t&) { return (int)left != enumName##_COUNT; } \
	static __fi bool operator==(const enumName& left, const pxEnumEnd_t&) { return (int)left == enumName##_COUNT; } \
\
	static __fi bool EnumIsValid(enumName id) \
	{ \
		return ((int)id >= enumName##_FIRST) && ((int)id < enumName##_COUNT); \
	} \
\
	extern const char* EnumToString(enumName id)

class pxEnumEnd_t
{
};
static const pxEnumEnd_t pxEnumEnd = {};

// --------------------------------------------------------------------------------------
//  DeclareNoncopyableObject
// --------------------------------------------------------------------------------------
// This macro provides an easy and clean method for ensuring objects are not copyable.
// Simply add the macro to the head or tail of your class declaration, and attempts to
// copy the class will give you a moderately obtuse compiler error.
//
#ifndef DeclareNoncopyableObject
#define DeclareNoncopyableObject(classname) \
public: \
	classname(const classname&) = delete; \
	classname& operator=(const classname&) = delete
#endif

// --------------------------------------------------------------------------------------
//  Handy Human-readable constants for common immediate values (_16kb -> _4gb)

static constexpr sptr _1kb = 1024 * 1;
static constexpr sptr _4kb = _1kb * 4;
static constexpr sptr _16kb = _1kb * 16;
static constexpr sptr _32kb = _1kb * 32;
static constexpr sptr _64kb = _1kb * 64;
static constexpr sptr _128kb = _1kb * 128;
static constexpr sptr _256kb = _1kb * 256;

static constexpr s64 _1mb = 1024 * 1024;
static constexpr s64 _8mb = _1mb * 8;
static constexpr s64 _16mb = _1mb * 16;
static constexpr s64 _32mb = _1mb * 32;
static constexpr s64 _64mb = _1mb * 64;
static constexpr s64 _256mb = _1mb * 256;
static constexpr s64 _1gb = _1mb * 1024;
static constexpr s64 _4gb = _1gb * 4;

// Disable some spammy warnings which wx appeared to disable.
// We probably should fix these at some point.
#ifdef _MSC_VER
#pragma warning(disable : 4244) // warning C4244: 'initializing': conversion from 'uptr' to 'uint', possible loss of data
#pragma warning(disable : 4267) // warning C4267: 'initializing': conversion from 'size_t' to 'uint', possible loss of data
#endif
