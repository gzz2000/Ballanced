/**
 * @file VxRay.inl
 * @brief Inline implementations for VxRay.
 *
 * Part 1: Scalar inline bodies (moved from VxRay.h).
 * Part 2: SIMD-accelerated ray operations.
 *
 * Included automatically at the bottom of VxRay.h - do not include directly.
 */

#pragma once

#if defined(VX_SIMD_SSE)

#include "VxSIMD.h"

VX_SIMD_INLINE void VxSIMDRayTransform(VxRay *dest, const VxRay *ray, const VxMatrix *mat) noexcept;
VX_SIMD_INLINE void VxSIMDRayInterpolate(VxVector *point, const VxRay *ray, float t) noexcept;
VX_SIMD_INLINE float VxSIMDRaySquareDistance(const VxRay *ray, const VxVector *point) noexcept;

#endif

// =============================================================================
// Part 1 - Scalar Inline Implementations
// =============================================================================

inline VxRay::VxRay() : m_Origin(0.0f, 0.0f, 0.0f), m_Direction(0.0f, 0.0f, 0.0f) {}

inline VxRay::VxRay(const VxVector &start, const VxVector &end) : m_Origin(start), m_Direction(end - start) {}

inline VxRay::VxRay(const VxVector &start, const VxVector &dir, int *dummy) : m_Origin(start), m_Direction(dir) {
    (void) dummy;
}

inline void VxRay::Transform(VxRay &dest, const VxMatrix &mat) {
#if defined(VX_SIMD_SSE)
    VxSIMDRayTransform(&dest, this, &mat);
#else
    Vx3DMultiplyMatrixVector(&dest.m_Origin, mat, &m_Origin);
    Vx3DRotateVector(&dest.m_Direction, mat, &m_Direction);
#endif
}

inline void VxRay::Interpolate(VxVector &p, float t) const {
#if defined(VX_SIMD_SSE)
    VxSIMDRayInterpolate(&p, this, t);
#else
    p = m_Origin + m_Direction * t;
#endif
}

inline float VxRay::SquareDistance(const VxVector &p) const {
#if defined(VX_SIMD_SSE)
    return VxSIMDRaySquareDistance(this, &p);
#else
    VxVector v = p - m_Origin;
    const float a = SquareMagnitude(v);
    const float dirSq = SquareMagnitude(m_Direction);
    if (dirSq <= EPSILON) {
        return a;
    }
    const float ps = DotProduct(v, m_Direction);
    const float sq = a - (ps * ps) / dirSq;
    return (sq > 0.0f) ? sq : 0.0f;
#endif
}

inline float VxRay::Distance(const VxVector &p) const {
    return sqrtf(SquareDistance(p));
}

inline bool VxRay::operator==(const VxRay &iRay) const {
    return (m_Origin == iRay.m_Origin) && (m_Direction == iRay.m_Direction);
}

inline const VxVector &VxRay::GetOrigin() const {
    return m_Origin;
}

inline VxVector &VxRay::GetOrigin() {
    return m_Origin;
}

inline const VxVector &VxRay::GetDirection() const {
    return m_Direction;
}

inline VxVector &VxRay::GetDirection() {
    return m_Direction;
}

// =============================================================================
// Part 2 - SIMD-Accelerated Ray Operations
// =============================================================================

#if defined(VX_SIMD_SSE)

VX_SIMD_INLINE void VxSIMDRayTransform(VxRay *dest, const VxRay *ray, const VxMatrix *mat) noexcept {
    VxSIMDMultiplyMatrixVector(&dest->m_Origin, mat, &ray->m_Origin);
    VxSIMDRotateVector(&dest->m_Direction, mat, &ray->m_Direction);
}

VX_SIMD_INLINE void VxSIMDRayInterpolate(VxVector *point, const VxRay *ray, float t) noexcept {
    __m128 origin = VxSIMDLoadFloat3(&ray->m_Origin.x);
    __m128 dir = VxSIMDLoadFloat3(&ray->m_Direction.x);
    __m128 tv = _mm_set1_ps(t);
    __m128 out = VX_FMADD_PS(dir, tv, origin);
    VxSIMDStoreFloat3(&point->x, out);
}

VX_SIMD_INLINE float VxSIMDRaySquareDistance(const VxRay *ray, const VxVector *point) noexcept {
    const __m128 p = VxSIMDLoadFloat3(&point->x);
    const __m128 o = VxSIMDLoadFloat3(&ray->m_Origin.x);
    const __m128 d = VxSIMDLoadFloat3(&ray->m_Direction.x);
    const __m128 v = _mm_sub_ps(p, o);
    const __m128 aa = VxSIMDDotProduct3(v, v);
    const __m128 dd = VxSIMDDotProduct3(d, d);

    const float ddScalar = _mm_cvtss_f32(dd);
    if (ddScalar <= EPSILON) {
        return _mm_cvtss_f32(aa);
    }

    const __m128 ps = VxSIMDDotProduct3(v, d);
    __m128 sq = _mm_sub_ss(aa, _mm_div_ss(_mm_mul_ss(ps, ps), dd));
    sq = _mm_max_ss(sq, _mm_setzero_ps());
    return _mm_cvtss_f32(sq);
}

#endif // VX_SIMD_SSE
