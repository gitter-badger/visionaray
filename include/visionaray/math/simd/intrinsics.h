// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

//--------------------------------------------------------------------------------------------------
// Detect architecture
//

// SSE2 is always available on 64-bit platforms
#if defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#define VSNRAY_SIMD_ON_64_BIT 1
#endif

//--------------------------------------------------------------------------------------------------
// Detect instruction set
//

#define VSNRAY_SIMD_ISA_SSE         10
#define VSNRAY_SIMD_ISA_SSE2        20
#define VSNRAY_SIMD_ISA_SSE3        30
#define VSNRAY_SIMD_ISA_SSSE3       31
#define VSNRAY_SIMD_ISA_SSE4_1      41
#define VSNRAY_SIMD_ISA_SSE4_2      42
#define VSNRAY_SIMD_ISA_AVX         50
#define VSNRAY_SIMD_ISA_AVX2        60
#define VSNRAY_SIMD_ISA_AVX_512     70

#ifndef VSNRAY_SIMD_ISA
#if defined(__AVX2__)
#define VSNRAY_SIMD_ISA VSNRAY_SIMD_ISA_AVX2
#elif defined(__AVX__)
#define VSNRAY_SIMD_ISA VSNRAY_SIMD_ISA_AVX
#elif defined(__SSE4_2__)
#define VSNRAY_SIMD_ISA VSNRAY_SIMD_ISA_SSE4_2
#elif defined(__SSE4_1__)
#define VSNRAY_SIMD_ISA VSNRAY_SIMD_ISA_SSE4_1
#elif defined(__SSSE3__)
#define VSNRAY_SIMD_ISA VSNRAY_SIMD_ISA_SSSE3
#elif defined(__SSE3__)
#define VSNRAY_SIMD_ISA VSNRAY_SIMD_ISA_SSE3
#elif defined(__SSE2__) || defined(VSNRAY_SIMD_ON_64_BIT)
#define VSNRAY_SIMD_ISA VSNRAY_SIMD_ISA_SSE2
#else
#define VSNRAY_SIMD_ISA 0
#endif
#endif

// Intel Short Vector Math Library available?
#ifndef VSNRAY_SIMD_HAS_SVML
#if defined(__INTEL_COMPILER)
#define VSNRAY_SIMD_HAS_SVML 1
#endif
#endif

//--------------------------------------------------------------------------------------------------
// SSE #include's
//

#if VSNRAY_SIMD_ISA >= VSNRAY_SIMD_ISA_SSE2
#include <emmintrin.h>
#endif
#if VSNRAY_SIMD_ISA >= VSNRAY_SIMD_ISA_SSE3
#include <pmmintrin.h>
#endif
#if VSNRAY_SIMD_ISA >= VSNRAY_SIMD_ISA_SSSE3
#include <tmmintrin.h>
#endif
#if VSNRAY_SIMD_ISA >= VSNRAY_SIMD_ISA_SSE4_1
#include <smmintrin.h>
#endif
#if VSNRAY_SIMD_ISA >= VSNRAY_SIMD_ISA_SSE4_2
#include <nmmintrin.h>
#endif
#if VSNRAY_SIMD_ISA >= VSNRAY_SIMD_ISA_AVX
#include <immintrin.h>
#endif
#if VSNRAY_SIMD_ISA >= VSNRAY_SIMD_ISA_AVX_512
#include <zmmintrin.h>
#endif
