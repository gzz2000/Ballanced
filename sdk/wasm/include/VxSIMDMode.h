#ifndef VXSIMDMODE_H
#define VXSIMDMODE_H

// Global SIMD override modes (used by VxSetSIMDOverride).
// Values are fixed for API/ABI stability.
#define VX_SIMD_MODE_AUTO   0
#define VX_SIMD_MODE_NONE   1
#define VX_SIMD_MODE_SSE2   2
#define VX_SIMD_MODE_SSSE3  3
#define VX_SIMD_MODE_SSE4_1 4
#define VX_SIMD_MODE_AVX    5
#define VX_SIMD_MODE_AVX2   6

#endif // VXSIMDMODE_H
