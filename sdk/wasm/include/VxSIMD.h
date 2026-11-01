/**
 * @file VxSIMD.h
 * @brief Public SIMD helpers and high-level vector/matrix utility wrappers for VxMath.
 *
 * @remarks
 * This header follows a DirectXMath-like API style and provides:
 * - compile-time SIMD capability macros
 * - runtime SIMD feature detection helpers
 * - inline SIMD primitives implemented in `VxSIMD.inl` and higher-level typed wrappers in per-type `.inl` files
 */

#ifndef VXSIMD_H
#define VXSIMD_H

#include "VxMathDefines.h"

// SIMD configuration and CPU feature detection

#include <cstdlib>
#include <cstdint>
#include <cfloat>
#include <cmath>
#include <cstring>

/*
 * Build-system kill switch.
 * When VX_SIMD_NONE is defined we keep this header usable, but force-disable
 * all compile-time SIMD paths (VX_SIMD_SSE / VX_SIMD_SSE2 / ...).
 */
#if defined(VX_SIMD_NONE)
#define VX_SIMD_FORCE_DISABLED 1
#endif

// Platform detection
#if defined(_MSC_VER)
#define VX_SIMD_MSVC 1
#include <malloc.h>
#elif defined(__GNUC__) || defined(__clang__)
#define VX_SIMD_GCC 1
#endif

// Architecture detection
#if defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__) || defined(__amd64__)
#define VX_SIMD_X64 1
#define VX_SIMD_X86 1   /* x64 is an x86 family for our needs */
#elif defined(_M_IX86) || defined(__i386__)
#define VX_SIMD_X86 1
#elif defined(_M_ARM64) || defined(__aarch64__)
#define VX_SIMD_ARM64 1
#define VX_SIMD_ARM 1
#elif defined(_M_ARM) || defined(__arm__)
#define VX_SIMD_ARM 1
#endif

/* x86/x64 intrinsic API.  Native builds expose the real x86 headers on x86
 * targets; SIMDe builds expose the same API portably on non-x86 targets. */
#if (defined(VX_SIMD_X86) || defined(VX_SIMD_USE_SIMDE)) && !defined(VX_SIMD_FORCE_DISABLED)

/* Use SIMDe native aliases only for the explicit portable provider.  MinGW
 * headers can expose GCC intrinsic declarations through WinSDK headers; native
 * aliases there may collide with intrinsic function names. */
#if defined(VX_SIMD_USE_SIMDE)
#if !defined(__MINGW32__) && !defined(__MINGW64__)
#define SIMDE_ENABLE_NATIVE_ALIASES
#endif

/* Keep diagnostics from bundled SIMDe headers local to the dependency.  In
 * particular, MSVC ARM64 warns about an intentional narrowing conversion in
 * simde_mm_sad_pu8.  Project code remains subject to warning C4244. */
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#define VX_SIMD_RESTORE_MSVC_WARNINGS 1
#endif

/*
 * By default, prefer IEEE-ish behavior so SIMD matches scalar code
 * more closely. Projects that want maximum throughput can opt in.
 */
#if !defined(VX_SIMD_USE_SIMDE_FAST_MATH)
#define VX_SIMD_USE_SIMDE_FAST_MATH 0
#endif

/* Optional SIMDe performance knobs (opt-in). */
#if VX_SIMD_USE_SIMDE_FAST_MATH
#if !defined(SIMDE_NO_FAST_NANS)
#define SIMDE_FAST_NANS
#endif
#if !defined(SIMDE_NO_FAST_ROUND_MODE)
#define SIMDE_FAST_ROUND_MODE
#endif
#if !defined(SIMDE_NO_FAST_EXCEPTIONS)
#define SIMDE_FAST_EXCEPTIONS
#endif
#endif
#endif // VX_SIMD_USE_SIMDE

/* SSE (x64 implies SSE2 baseline; for 32-bit MSVC use _M_IX86_FP) */
#if defined(VX_SIMD_USE_SIMDE) \
      || defined(VX_SIMD_X64) \
      || (defined(_MSC_VER) && defined(_M_IX86_FP) && _M_IX86_FP >= 1) \
      || defined(__SSE__)
#define VX_SIMD_SSE 1
#  if defined(VX_SIMD_USE_SIMDE)
#    include <simde/x86/sse.h>
#  else
#    include <xmmintrin.h>
#  endif
#endif

/* SSE2 */
#if defined(VX_SIMD_USE_SIMDE) \
      || defined(VX_SIMD_X64) \
      || (defined(_MSC_VER) && defined(_M_IX86_FP) && _M_IX86_FP >= 2) \
      || defined(__SSE2__)
#define VX_SIMD_SSE2 1
#  if defined(VX_SIMD_USE_SIMDE)
#    include <simde/x86/sse2.h>
#  else
#    include <emmintrin.h>
#  endif
#endif

#if defined(VX_SIMD_X86) && !defined(VX_SIMD_USE_SIMDE) && !defined(VX_SIMD_SSE2)
#error "VxMath requires SSE2 on x86/x64 builds."
#endif

/*
 * Fine-grained feature blocks.
 *
 * Each block accepts both compiler-predefined macros (e.g. __SSE4_1__, set
 * by GCC/Clang -msse4.1) and macros injected by the build system
 * (VX_SIMD_SSE4_1=1, set by vx_apply_simd_flags for compilers like MSVC
 * that do not predefine __SSEx__ even when the corresponding ISA is active).
 *
 * SIMDe headers carry their own include guards, so including them more than
 * once is always safe.
 */

/* SSE3 */
#if defined(__SSE3__) || defined(VX_SIMD_SSE3)
#  ifndef VX_SIMD_SSE3
#    define VX_SIMD_SSE3 1
#  endif
#  if defined(VX_SIMD_USE_SIMDE)
#    include <simde/x86/sse3.h>
#  else
#    include <pmmintrin.h>
#  endif
#endif

/* SSSE3 */
#if defined(__SSSE3__) || defined(VX_SIMD_SSSE3)
#  ifndef VX_SIMD_SSSE3
#    define VX_SIMD_SSSE3 1
#  endif
#  if defined(VX_SIMD_USE_SIMDE)
#    include <simde/x86/ssse3.h>
#  else
#    include <tmmintrin.h>
#  endif
#elif defined(VX_SIMD_SSE2) && defined(_MSC_VER) && !defined(VX_SIMD_USE_SIMDE)
/* MSVC exposes SSSE3 intrinsic declarations without a separate /arch:SSSE3
 * switch.  Keep the declarations available for runtime-dispatched kernels
 * without advertising SSSE3 as a compile-time target capability. */
#  include <tmmintrin.h>
#elif defined(VX_SIMD_SSE2) && defined(VX_SIMD_USE_SIMDE)
/* Always pull in ssse3.h via SIMDe emulation: blit kernels use
 * _mm_shuffle_epi8 at SSE2 baseline and rely on SIMDe to emulate it
 * when the requested provider can execute it portably.  The runtime
 * dispatcher still keeps SSSE3 kernels disabled unless that mode is selected. */
#  include <simde/x86/ssse3.h>
#endif

/* SSE4.1 */
#if defined(__SSE4_1__) || defined(VX_SIMD_SSE4_1)
#  ifndef VX_SIMD_SSE4_1
#    define VX_SIMD_SSE4_1 1
#  endif
#  if defined(VX_SIMD_USE_SIMDE)
#    include <simde/x86/sse4.1.h>
#  else
#    include <smmintrin.h>
#  endif
#endif

/* SSE4.2 */
#if defined(__SSE4_2__) || defined(VX_SIMD_SSE4_2)
#  ifndef VX_SIMD_SSE4_2
#    define VX_SIMD_SSE4_2 1
#  endif
#  if defined(VX_SIMD_USE_SIMDE)
#    include <simde/x86/sse4.2.h>
#  else
#    include <nmmintrin.h>
#  endif
#endif

/* AVX */
#if defined(__AVX__) || defined(VX_SIMD_AVX) || defined(VX_SIMD_AVX2)
#  ifndef VX_SIMD_AVX
#    define VX_SIMD_AVX 1
#  endif
#  if defined(VX_SIMD_USE_SIMDE)
#    include <simde/x86/avx.h>
#  else
#    include <immintrin.h>
#  endif
#endif

/* AVX2 */
#if defined(__AVX2__) || defined(VX_SIMD_AVX2)
#  ifndef VX_SIMD_AVX2
#    define VX_SIMD_AVX2 1
#  endif
#  if defined(VX_SIMD_USE_SIMDE)
#    include <simde/x86/avx2.h>
#  else
#    include <immintrin.h>
#  endif
#endif

/* FMA (used for the VX_FMADD_PS / VX_FNMADD_PS macro fallbacks) */
#if defined(__FMA__) || defined(VX_SIMD_FMA)
#  ifndef VX_SIMD_FMA
#    define VX_SIMD_FMA 1
#  endif
#  if defined(VX_SIMD_USE_SIMDE)
#    include <simde/x86/fma.h>
#  else
#    include <immintrin.h>
#  endif
#endif

#endif // (VX_SIMD_X86 || VX_SIMD_USE_SIMDE) && !VX_SIMD_FORCE_DISABLED

#if defined(VX_SIMD_RESTORE_MSVC_WARNINGS)
#pragma warning(pop)
#undef VX_SIMD_RESTORE_MSVC_WARNINGS
#endif

// ============================================================================
// FMA Macros (like DirectXMath XM_FMADD_PS)
// ============================================================================
// These macros use FMA instructions when available, falling back to mul+add.
// FMA provides better precision (single rounding) and can be faster on modern CPUs.

#if defined(VX_SIMD_FMA)
// FMA3: a * b + c  (fused multiply-add, single rounding)
#define VX_FMADD_PS(a, b, c)  _mm_fmadd_ps((a), (b), (c))
// FMA3: -(a * b) + c  (negated multiply-add)
#define VX_FNMADD_PS(a, b, c) _mm_fnmadd_ps((a), (b), (c))
// FMA3: a * b - c  (fused multiply-subtract)
#define VX_FMSUB_PS(a, b, c)  _mm_fmsub_ps((a), (b), (c))
// FMA3: -(a * b) - c  (negated multiply-subtract)
#define VX_FNMSUB_PS(a, b, c) _mm_fnmsub_ps((a), (b), (c))
#else
// Fallback: separate mul and add (two roundings, but still correct)
#define VX_FMADD_PS(a, b, c)  _mm_add_ps(_mm_mul_ps((a), (b)), (c))
#define VX_FNMADD_PS(a, b, c) _mm_sub_ps((c), _mm_mul_ps((a), (b)))
#define VX_FMSUB_PS(a, b, c)  _mm_sub_ps(_mm_mul_ps((a), (b)), (c))
#define VX_FNMSUB_PS(a, b, c) _mm_sub_ps(_mm_set1_ps(0.0f), _mm_add_ps(_mm_mul_ps((a), (b)), (c)))
#endif

// ============================================================================
// SIMD Helper Macros
// ============================================================================

#define VX_ALIGNED_MALLOC(size, alignment) VxNewAligned(size, alignment)
#define VX_ALIGNED_FREE(ptr) VxDeleteAligned(ptr)

// Common SIMD alignments
#define VX_ALIGN_SSE VX_ALIGN(16)

// Force inline for SIMD functions (like DirectXMath)
#if defined(VX_SIMD_MSVC)
#define VX_SIMD_INLINE __forceinline
#elif defined(VX_SIMD_GCC)
#define VX_SIMD_INLINE inline __attribute__((always_inline))
#else
#define VX_SIMD_INLINE inline
#endif

// ============================================================================
// SIMD Type Definitions
// ============================================================================

#if defined(VX_SIMD_SSE)
typedef __m128 vx_simd_float4;
typedef __m128i vx_simd_int4;
#endif

// ============================================================================
// Global SIMD Constants (like DirectXMath g_XMOne, etc.)
// ============================================================================
// These constants avoid repeated _mm_set calls in hot paths.

#if defined(VX_SIMD_SSE)

// Common scalar values broadcast to all lanes
#define VX_SIMD_ONE       _mm_set1_ps(1.0f)
#define VX_SIMD_HALF      _mm_set1_ps(0.5f)
#define VX_SIMD_QUARTER   _mm_set1_ps(0.25f)
#define VX_SIMD_TWO       _mm_set1_ps(2.0f)
#define VX_SIMD_THREE     _mm_set1_ps(3.0f)
#define VX_SIMD_FOUR      _mm_set1_ps(4.0f)
#define VX_SIMD_NEG_ONE   _mm_set1_ps(-1.0f)
#define VX_SIMD_ZERO      _mm_setzero_ps()

// Newton-Raphson rsqrt constants
#define VX_SIMD_NR_HALF       _mm_set1_ps(0.5f)
#define VX_SIMD_NR_THREE_HALF _mm_set1_ps(1.5f)

// Epsilon values for comparisons
#define VX_SIMD_EPSILON   _mm_set1_ps(EPSILON)

// Identity matrix rows
#define VX_SIMD_IDENTITY_R0 _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f)
#define VX_SIMD_IDENTITY_R1 _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f)
#define VX_SIMD_IDENTITY_R2 _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f)
#define VX_SIMD_IDENTITY_R3 _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f)

// Identity quaternion {0, 0, 0, 1}
#define VX_SIMD_QUAT_IDENTITY _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f)

// Sign masks for quaternion operations
#define VX_SIMD_SIGN_MASK     _mm_castsi128_ps(_mm_set1_epi32(static_cast<int>(0x80000000)))
#define VX_SIMD_ABS_MASK      _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF))

// Quaternion multiplication sign masks
#define VX_SIMD_QUAT_SIGN1 _mm_setr_ps(1.0f, -1.0f, 1.0f, -1.0f)
#define VX_SIMD_QUAT_SIGN2 _mm_setr_ps(1.0f, 1.0f, -1.0f, -1.0f)
#define VX_SIMD_QUAT_SIGN3 _mm_setr_ps(-1.0f, 1.0f, 1.0f, -1.0f)

// Pi-related constants for trigonometric approximations
#define VX_SIMD_PI        _mm_set1_ps(3.14159265358979323846f)
#define VX_SIMD_TWO_PI    _mm_set1_ps(6.28318530717958647692f)
#define VX_SIMD_HALF_PI   _mm_set1_ps(1.57079632679489661923f)
#define VX_SIMD_INV_PI    _mm_set1_ps(0.31830988618379067154f)
#define VX_SIMD_INV_TWO_PI _mm_set1_ps(0.15915494309189533577f)

#endif // VX_SIMD_SSE

// ============================================================================
// CPU Feature Detection (Runtime)
// ============================================================================

/**
 * @brief CPU feature flags for runtime detection
 */
struct VxSIMDFeatures {
    bool SSE = false;
    bool SSE2 = false;
    bool SSE3 = false;
    bool SSSE3 = false;
    bool SSE4_1 = false;
    bool SSE4_2 = false;
    bool AVX = false;
    bool AVX2 = false;
    bool FMA = false;
    bool AVX512F = false;
    bool XSAVE = false;
    bool OSXSAVE = false;
};

// CPU feature detection at runtime
#if defined(VX_SIMD_X86)
#if defined(VX_SIMD_MSVC)
#include <intrin.h>
#else
#include <cpuid.h>
#endif

VX_SIMD_INLINE uint64_t VxReadXCR0() {
#if defined(VX_SIMD_MSVC)
    return static_cast<uint64_t>(_xgetbv(0));
#elif defined(VX_SIMD_GCC)
    uint32_t eax = 0;
    uint32_t edx = 0;
    __asm__ volatile(".byte 0x0f, 0x01, 0xd0" : "=a"(eax), "=d"(edx) : "c"(0));
    return (static_cast<uint64_t>(edx) << 32) | eax;
#else
    return 0;
#endif
}
#endif

/**
 * @brief Detects CPU features at runtime
 * @return A VxSIMDFeatures structure with detected capabilities
 */
VX_SIMD_INLINE VxSIMDFeatures VxDetectSIMDFeatures() {
    VxSIMDFeatures features;

#if defined(VX_SIMD_X86)
    int info[4];

#if defined(VX_SIMD_MSVC)
    __cpuidex(info, 0, 0);
#elif defined(VX_SIMD_GCC)
    __cpuid_count(0, 0, info[0], info[1], info[2], info[3]);
#endif
    int max_id = info[0];

    if (max_id >= 1) {
#if defined(VX_SIMD_MSVC)
        __cpuidex(info, 1, 0);
#elif defined(VX_SIMD_GCC)
        __cpuid_count(1, 0, info[0], info[1], info[2], info[3]);
#endif
        // ECX register (info[2])
        features.SSE3   = (info[2] & (1 << 0)) != 0;
        features.SSSE3  = (info[2] & (1 << 9)) != 0;
        const bool fmaCpu = (info[2] & (1 << 12)) != 0;
        features.SSE4_1 = (info[2] & (1 << 19)) != 0;
        features.SSE4_2 = (info[2] & (1 << 20)) != 0;
        features.XSAVE  = (info[2] & (1 << 26)) != 0;
        features.OSXSAVE= (info[2] & (1 << 27)) != 0;
        const bool avxCpu = (info[2] & (1 << 28)) != 0;

        // EDX register (info[3])
        features.SSE    = (info[3] & (1 << 25)) != 0;
        features.SSE2   = (info[3] & (1 << 26)) != 0;

        bool avxStateEnabled = false;
        if (features.OSXSAVE) {
            const uint64_t xcr0 = VxReadXCR0();
            avxStateEnabled = (xcr0 & 0x6) == 0x6; // XMM and YMM state
        }

        features.AVX = avxCpu && avxStateEnabled;
        features.FMA = fmaCpu && features.AVX;
    }

    if (max_id >= 7) {
#if defined(VX_SIMD_MSVC)
        __cpuidex(info, 7, 0);
#elif defined(VX_SIMD_GCC)
        __cpuid_count(7, 0, info[0], info[1], info[2], info[3]);
#endif
        // EBX register (info[1])
        const bool avx2Cpu = (info[1] & (1 << 5)) != 0;
        const bool avx512fCpu = (info[1] & (1 << 16)) != 0;

        features.AVX2 = avx2Cpu && features.AVX;

        bool avx512StateEnabled = false;
        if (features.OSXSAVE) {
            const uint64_t xcr0 = VxReadXCR0();
            avx512StateEnabled = (xcr0 & 0xE6) == 0xE6; // XMM,YMM,opmask,ZMM_hi256,Hi16_ZMM
        }
        features.AVX512F = avx512fCpu && avx512StateEnabled;
    }
#endif

    return features;
}

/**
 * @brief Gets the cached CPU features (only detects once)
 * @return Reference to the cached CPU features
 */
VX_SIMD_INLINE const VxSIMDFeatures &VxGetSIMDFeatures() {
    static VxSIMDFeatures features = VxDetectSIMDFeatures();
    return features;
}

// ============================================================================
// Forward Declarations for Types
// ============================================================================

struct VxVector;
class VxVector4;
struct VxQuaternion;
class VxMatrix;
class VxRay;
class VxPlane;
class VxRect;
struct Vx2DVector;
struct VxStridedData;
struct VxBbox;
class VxFrustum;

// ============================================================================
// Inline Implementation Include
// ============================================================================

#include "VxSIMD.inl"

#endif // VXSIMD_H
