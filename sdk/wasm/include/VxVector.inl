/**
 * @file VxVector.inl
 * @brief Inline implementations for VxVector and VxVector4.
 *
 * Part 1: Scalar operator / utility bodies (moved from VxVector.h).
 * Part 2: SIMD-accelerated high-level operations on VxVector / VxVector4.
 *
 * Included automatically at the bottom of VxVector.h - do not include directly.
 */

#pragma once

#include "VxSIMD.h"

#if defined(VX_SIMD_SSE)
VX_SIMD_INLINE void VxSIMDNormalizeVector(VxVector *v) noexcept;
VX_SIMD_INLINE float VxSIMDDotVector(const VxVector *a, const VxVector *b) noexcept;
VX_SIMD_INLINE void VxSIMDCrossVector(VxVector *result, const VxVector *a, const VxVector *b) noexcept;
VX_SIMD_INLINE void VxSIMDReflectVector(VxVector *result, const VxVector *incident, const VxVector *normal) noexcept;
VX_SIMD_INLINE float VxSIMDLengthVector(const VxVector *v) noexcept;
VX_SIMD_INLINE float VxSIMDLengthSquaredVector(const VxVector *v) noexcept;
VX_SIMD_INLINE float VxSIMDDotVector4(const VxVector4 *a, const VxVector4 *b) noexcept;
#endif

// =============================================================================
// Part 1 - Scalar Inline Implementations
// =============================================================================

inline VxVector::VxVector() : x(0), y(0), z(0) {}
inline VxVector::VxVector(float f) : x(f), y(f), z(f) {}
inline VxVector::VxVector(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
inline VxVector::VxVector(const float f[3]) : x(f[0]), y(f[1]), z(f[2]) {}
inline float VxVector::SquareMagnitude() const {
#if defined(VX_SIMD_SSE)
    return VxSIMDLengthSquaredVector(this);
#else
    return x * x + y * y + z * z;
#endif
}
inline float VxVector::Magnitude() const {
#if defined(VX_SIMD_SSE)
    return VxSIMDLengthVector(this);
#else
    return sqrtf(SquareMagnitude());
#endif
}
inline float VxVector::Dot(const VxVector &iV) const {
#if defined(VX_SIMD_SSE)
    return VxSIMDDotVector(this, &iV);
#else
    return x * iV.x + y * iV.y + z * iV.z;
#endif
}
inline VxVector VxVector::operator+(float s) const { return VxVector(x + s, y + s, z + s); }
inline VxVector VxVector::operator-(float s) const { return VxVector(x - s, y - s, z - s); }
inline void VxVector::Normalize() {
#if defined(VX_SIMD_SSE)
    VxSIMDNormalizeVector(this);
#else
    const float magSq = SquareMagnitude();
    if (magSq > EPSILON) {
        const float invMag = 1.0f / sqrtf(magSq);
        x *= invMag;
        y *= invMag;
        z *= invMag;
    }
#endif
}
inline void VxVector::Set(float _x, float _y, float _z) { x = _x; y = _y; z = _z; }
inline const float &VxVector::operator[](int i) const { return *((&x) + i); }
inline float &VxVector::operator[](int i) { return *((&x) + i); }
inline VxVector &VxVector::operator+=(const VxVector &v) {
#if defined(VX_SIMD_SSE)
    __m128 lhs = VxSIMDLoadFloat3(&x);
    __m128 rhs = VxSIMDLoadFloat3(&v.x);
    VxSIMDStoreFloat3(&x, _mm_add_ps(lhs, rhs));
#else
    x += v.x; y += v.y; z += v.z;
#endif
    return *this;
}
inline VxVector &VxVector::operator-=(const VxVector &v) {
#if defined(VX_SIMD_SSE)
    __m128 lhs = VxSIMDLoadFloat3(&x);
    __m128 rhs = VxSIMDLoadFloat3(&v.x);
    VxSIMDStoreFloat3(&x, _mm_sub_ps(lhs, rhs));
#else
    x -= v.x; y -= v.y; z -= v.z;
#endif
    return *this;
}
inline VxVector &VxVector::operator*=(const VxVector &v) {
#if defined(VX_SIMD_SSE)
    __m128 lhs = VxSIMDLoadFloat3(&x);
    __m128 rhs = VxSIMDLoadFloat3(&v.x);
    VxSIMDStoreFloat3(&x, _mm_mul_ps(lhs, rhs));
#else
    x *= v.x; y *= v.y; z *= v.z;
#endif
    return *this;
}
inline VxVector &VxVector::operator/=(const VxVector &v) {
#if defined(VX_SIMD_SSE)
    __m128 lhs = VxSIMDLoadFloat3(&x);
    __m128 rhs = VxSIMDLoadFloat3(&v.x);
    VxSIMDStoreFloat3(&x, _mm_div_ps(lhs, rhs));
#else
    x /= v.x; y /= v.y; z /= v.z;
#endif
    return *this;
}
inline VxVector &VxVector::operator*=(float s) {
#if defined(VX_SIMD_SSE)
    __m128 lhs = VxSIMDLoadFloat3(&x);
    VxSIMDStoreFloat3(&x, _mm_mul_ps(lhs, _mm_set1_ps(s)));
#else
    x *= s; y *= s; z *= s;
#endif
    return *this;
}
inline VxVector &VxVector::operator/=(float s) {
    float temp = 1.0f / s;
#if defined(VX_SIMD_SSE)
    __m128 lhs = VxSIMDLoadFloat3(&x);
    VxSIMDStoreFloat3(&x, _mm_mul_ps(lhs, _mm_set1_ps(temp)));
#else
    x *= temp; y *= temp; z *= temp;
#endif
    return *this;
}

inline const VxVector operator+(const VxVector &v) { return v; }
inline const VxVector operator-(const VxVector &v) { return VxVector(-v.x, -v.y, -v.z); }
inline const VxVector operator+(const VxVector &v1, const VxVector &v2) { VxVector r = v1; r += v2; return r; }
inline const VxVector operator-(const VxVector &v1, const VxVector &v2) { VxVector r = v1; r -= v2; return r; }
inline const VxVector operator*(const VxVector &v1, const VxVector &v2) { VxVector r = v1; r *= v2; return r; }
inline const VxVector operator/(const VxVector &v1, const VxVector &v2) { VxVector r = v1; r /= v2; return r; }
inline int operator<(const VxVector &v1, const VxVector &v2) {
#if defined(VX_SIMD_SSE)
    __m128 a = VxSIMDLoadFloat3(&v1.x);
    __m128 b = VxSIMDLoadFloat3(&v2.x);
    return ((_mm_movemask_ps(_mm_cmplt_ps(a, b)) & 0x7) == 0x7) ? 1 : 0;
#else
    return v1[0] < v2[0] && v1[1] < v2[1] && v1[2] < v2[2];
#endif
}
inline int operator<=(const VxVector &v1, const VxVector &v2) {
#if defined(VX_SIMD_SSE)
    __m128 a = VxSIMDLoadFloat3(&v1.x);
    __m128 b = VxSIMDLoadFloat3(&v2.x);
    return ((_mm_movemask_ps(_mm_cmple_ps(a, b)) & 0x7) == 0x7) ? 1 : 0;
#else
    return v1[0] <= v2[0] && v1[1] <= v2[1] && v1[2] <= v2[2];
#endif
}
inline const VxVector operator*(const VxVector &v, float s) { VxVector r = v; r *= s; return r; }
inline const VxVector operator*(float s, const VxVector &v) { VxVector r = v; r *= s; return r; }
inline const VxVector operator/(const VxVector &v, float s) { VxVector r = v; r /= s; return r; }
inline int operator==(const VxVector &v1, const VxVector &v2) {
#if defined(VX_SIMD_SSE)
    __m128 a = VxSIMDLoadFloat3(&v1.x);
    __m128 b = VxSIMDLoadFloat3(&v2.x);
    return ((_mm_movemask_ps(_mm_cmpeq_ps(a, b)) & 0x7) == 0x7) ? 1 : 0;
#else
    return ((v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z));
#endif
}
inline int operator!=(const VxVector &v1, const VxVector &v2) { return !(v1 == v2); }
inline float SquareMagnitude(const VxVector &v) { return v.SquareMagnitude(); }
inline float Magnitude(const VxVector &v) { return v.Magnitude(); }
inline float InvSquareMagnitude(const VxVector &v) { return 1.0f / SquareMagnitude(v); }
inline float InvMagnitude(const VxVector &v) { return 1.0f / Magnitude(v); }
inline const VxVector Normalize(const VxVector &v) { return v * InvMagnitude(v); }
inline const VxVector Normalize(const VxVector *vect) { return Normalize(*vect); }
inline float DotProduct(const VxVector &v1, const VxVector &v2) {
#if defined(VX_SIMD_SSE)
    return VxSIMDDotVector(&v1, &v2);
#else
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
#endif
}
inline const VxVector CrossProduct(const VxVector &Vect1, const VxVector &Vect2) {
    VxVector result;
#if defined(VX_SIMD_SSE)
    VxSIMDCrossVector(&result, &Vect1, &Vect2);
#else
    result = VxVector(
        Vect1.y * Vect2.z - Vect1.z * Vect2.y,
        Vect1.z * Vect2.x - Vect1.x * Vect2.z,
        Vect1.x * Vect2.y - Vect1.y * Vect2.x
    );
#endif
    return result;
}
inline const VxVector Reflect(const VxVector &v1, const VxVector &Norm) {
    VxVector result;
#if defined(VX_SIMD_SSE)
    VxSIMDReflectVector(&result, &v1, &Norm);
#else
    float dp2 = 2.0f * DotProduct(v1, Norm);
    result = VxVector(v1.x - dp2 * Norm.x, v1.y - dp2 * Norm.y, v1.z - dp2 * Norm.z);
#endif
    return result;
}
inline const VxVector Rotate(const VxVector &v1, const VxVector &v2, float angle) {
    const float axisMagSq = v2.x * v2.x + v2.y * v2.y + v2.z * v2.z;
    if (axisMagSq <= EPSILON) {
        return v1;
    }
    const float axisInvMag = 1.0f / sqrtf(axisMagSq);
    const float nx = v2.x * axisInvMag;
    const float ny = v2.y * axisInvMag;
    const float nz = v2.z * axisInvMag;

    const float sinAngle = sinf(angle);
    const float cosAngle = cosf(angle);
    const float oneMinusCos = 1.0f - cosAngle;

    const float nx2 = nx * nx, ny2 = ny * ny, nz2 = nz * nz;
    const float nx_ny_omc = nx * ny * oneMinusCos;
    const float nx_nz_omc = nx * nz * oneMinusCos;
    const float ny_nz_omc = ny * nz * oneMinusCos;
    const float sin_nx = sinAngle * nx;
    const float sin_ny = sinAngle * ny;
    const float sin_nz = sinAngle * nz;

    return VxVector(
        v1.x * (cosAngle + oneMinusCos * nx2) + v1.y * (nx_ny_omc + sin_nz) + v1.z * (nx_nz_omc - sin_ny),
        v1.x * (nx_ny_omc - sin_nz) + v1.y * (cosAngle + oneMinusCos * ny2) + v1.z * (ny_nz_omc + sin_nx),
        v1.x * (nx_nz_omc + sin_ny) + v1.y * (ny_nz_omc - sin_nx) + v1.z * (cosAngle + oneMinusCos * nz2)
    );
}

inline const VxVector Rotate(const VxVector *v1, const VxVector *v2, float angle) {
    return Rotate(*v1, *v2, angle);
}

/**
 * @brief Returns a vector with each component set to its absolute value.
 */
inline const VxVector Absolute(const VxVector &v) {
    VxVector result = v;
    result.Absolute();
    return result;
}
inline void VxVector::Absolute() {
#if defined(VX_SIMD_SSE)
    __m128 vec = VxSIMDLoadFloat3(&x);
    vec = _mm_and_ps(vec, VX_SIMD_ABS_MASK);
    VxSIMDStoreFloat3(&x, vec);
#else
    x = XAbs(x); y = XAbs(y); z = XAbs(z);
#endif
}

/**
 * @brief Calculates the minimum value among the components of a vector.
 */
inline float Min(const VxVector &v) {
#if defined(VX_SIMD_SSE)
    __m128 vec = VxSIMDLoadFloat3(&v.x);
    __m128 y = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 z = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2, 2, 2, 2));
    __m128 m = _mm_min_ss(vec, y);
    m = _mm_min_ss(m, z);
    float result;
    _mm_store_ss(&result, m);
    return result;
#else
    return XMin(v.x, v.y, v.z);
#endif
}

/**
 * @brief Calculates the maximum value among the components of a vector.
 */
inline float Max(const VxVector &v) {
#if defined(VX_SIMD_SSE)
    __m128 vec = VxSIMDLoadFloat3(&v.x);
    __m128 y = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 z = _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2, 2, 2, 2));
    __m128 m = _mm_max_ss(vec, y);
    m = _mm_max_ss(m, z);
    float result;
    _mm_store_ss(&result, m);
    return result;
#else
    float ret = v.x;
    if (ret < v.y) ret = v.y;
    if (ret < v.z) ret = v.z;
    return ret;
#endif
}

/**
 * @brief Component-wise minimum of two vectors.
 */
inline const VxVector Minimize(const VxVector &v1, const VxVector &v2) {
    VxVector result;
#if defined(VX_SIMD_SSE)
    __m128 a = VxSIMDLoadFloat3(&v1.x);
    __m128 b = VxSIMDLoadFloat3(&v2.x);
    VxSIMDStoreFloat3(&result.x, _mm_min_ps(a, b));
#else
    result = VxVector(XMin(v1[0], v2[0]), XMin(v1[1], v2[1]), XMin(v1[2], v2[2]));
#endif
    return result;
}

/**
 * @brief Component-wise maximum of two vectors.
 */
inline const VxVector Maximize(const VxVector &v1, const VxVector &v2) {
    VxVector result;
#if defined(VX_SIMD_SSE)
    __m128 a = VxSIMDLoadFloat3(&v1.x);
    __m128 b = VxSIMDLoadFloat3(&v2.x);
    VxSIMDStoreFloat3(&result.x, _mm_max_ps(a, b));
#else
    result = VxVector(XMax(v1[0], v2[0]), XMax(v1[1], v2[1]), XMax(v1[2], v2[2]));
#endif
    return result;
}

/**
 * @brief Linear interpolation between two vectors.
 */
inline const VxVector Interpolate(float step, const VxVector &v1, const VxVector &v2) {
    VxVector result;
#if defined(VX_SIMD_SSE)
    __m128 a = VxSIMDLoadFloat3(&v1.x);
    __m128 b = VxSIMDLoadFloat3(&v2.x);
    __m128 t = _mm_set1_ps(step);
    VxSIMDStoreFloat3(&result.x, VX_FMADD_PS(_mm_sub_ps(b, a), t, a));
#else
    result = VxVector(v1.x + (v2.x - v1.x) * step,
                      v1.y + (v2.y - v1.y) * step,
                      v1.z + (v2.z - v1.z) * step);
#endif
    return result;
}

// --- VxVector4 operators ---------------------------------------------

inline VxVector4::VxVector4() { x = y = z = w = 0.0f; }
inline VxVector4::VxVector4(float f) { x = y = z = w = f; }
inline VxVector4::VxVector4(float _x, float _y, float _z, float _w) { x = _x; y = _y; z = _z; w = _w; }
inline VxVector4::VxVector4(const float f[4]) { x = f[0]; y = f[1]; z = f[2]; w = f[3]; }
inline VxVector4 &VxVector4::operator=(const VxVector &v) { x = v.x; y = v.y; z = v.z; return *this; }
inline float VxVector4::Dot(const VxVector4 &iV) const {
    return x * iV.x + y * iV.y + z * iV.z;
}
inline VxVector4 VxVector4::operator+(float s) const { return VxVector4(x + s, y + s, z + s, w + s); }
inline VxVector4 VxVector4::operator-(float s) const { return VxVector4(x - s, y - s, z - s, w - s); }

inline VxVector4 &VxVector4::operator+=(const VxVector4 &v) {
#if defined(VX_SIMD_SSE)
    __m128 lhs = VxSIMDLoadFloat4((const float *) this);
    __m128 rhs = VxSIMDLoadFloat4((const float *) &v);
    VxSIMDStoreFloat4((float *) this, _mm_add_ps(lhs, rhs));
#else
    x += v.x; y += v.y; z += v.z; w += v.w;
#endif
    return *this;
}
inline VxVector4 &VxVector4::operator-=(const VxVector4 &v) {
#if defined(VX_SIMD_SSE)
    __m128 lhs = VxSIMDLoadFloat4((const float *) this);
    __m128 rhs = VxSIMDLoadFloat4((const float *) &v);
    VxSIMDStoreFloat4((float *) this, _mm_sub_ps(lhs, rhs));
#else
    x -= v.x; y -= v.y; z -= v.z; w -= v.w;
#endif
    return *this;
}
inline VxVector4 &VxVector4::operator*=(const VxVector4 &v) {
#if defined(VX_SIMD_SSE)
    __m128 lhs = VxSIMDLoadFloat4((const float *) this);
    __m128 rhs = VxSIMDLoadFloat4((const float *) &v);
    VxSIMDStoreFloat4((float *) this, _mm_mul_ps(lhs, rhs));
#else
    x *= v.x; y *= v.y; z *= v.z; w *= v.w;
#endif
    return *this;
}
inline VxVector4 &VxVector4::operator/=(const VxVector4 &v) {
#if defined(VX_SIMD_SSE)
    __m128 lhs = VxSIMDLoadFloat4((const float *) this);
    __m128 rhs = VxSIMDLoadFloat4((const float *) &v);
    VxSIMDStoreFloat4((float *) this, _mm_div_ps(lhs, rhs));
#else
    x /= v.x; y /= v.y; z /= v.z; w /= v.w;
#endif
    return *this;
}
inline VxVector4 &VxVector4::operator+=(const VxVector &v) {
#if defined(VX_SIMD_SSE)
    __m128 lhs = VxSIMDLoadFloat4((const float *) this);
    __m128 rhs = _mm_setr_ps(v.x, v.y, v.z, 0.0f);
    VxSIMDStoreFloat4((float *) this, _mm_add_ps(lhs, rhs));
#else
    x += v.x; y += v.y; z += v.z;
#endif
    return *this;
}
inline VxVector4 &VxVector4::operator-=(const VxVector &v) {
#if defined(VX_SIMD_SSE)
    __m128 lhs = VxSIMDLoadFloat4((const float *) this);
    __m128 rhs = _mm_setr_ps(v.x, v.y, v.z, 0.0f);
    VxSIMDStoreFloat4((float *) this, _mm_sub_ps(lhs, rhs));
#else
    x -= v.x; y -= v.y; z -= v.z;
#endif
    return *this;
}
inline VxVector4 &VxVector4::operator*=(const VxVector &v) {
#if defined(VX_SIMD_SSE)
    __m128 lhs = VxSIMDLoadFloat4((const float *) this);
    __m128 rhs = _mm_setr_ps(v.x, v.y, v.z, 1.0f);
    VxSIMDStoreFloat4((float *) this, _mm_mul_ps(lhs, rhs));
#else
    x *= v.x; y *= v.y; z *= v.z;
#endif
    return *this;
}
inline VxVector4 &VxVector4::operator/=(const VxVector &v) {
#if defined(VX_SIMD_SSE)
    __m128 lhs = VxSIMDLoadFloat4((const float *) this);
    __m128 rhs = _mm_setr_ps(v.x, v.y, v.z, 1.0f);
    VxSIMDStoreFloat4((float *) this, _mm_div_ps(lhs, rhs));
#else
    x /= v.x; y /= v.y; z /= v.z;
#endif
    return *this;
}
inline VxVector4 &VxVector4::operator*=(float s) {
#if defined(VX_SIMD_SSE)
    __m128 lhs = VxSIMDLoadFloat4((const float *) this);
    VxSIMDStoreFloat4((float *) this, _mm_mul_ps(lhs, _mm_set1_ps(s)));
#else
    x *= s; y *= s; z *= s; w *= s;
#endif
    return *this;
}
inline VxVector4 &VxVector4::operator/=(float s) {
    float temp = 1.0f / s;
#if defined(VX_SIMD_SSE)
    __m128 lhs = VxSIMDLoadFloat4((const float *) this);
    VxSIMDStoreFloat4((float *) this, _mm_mul_ps(lhs, _mm_set1_ps(temp)));
#else
    x *= temp; y *= temp; z *= temp; w *= temp;
#endif
    return *this;
}
inline const VxVector4 operator+(const VxVector4 &v) { return v; }
inline const VxVector4 operator-(const VxVector4 &v) { return VxVector4(-v.x, -v.y, -v.z, -v.w); }
inline const VxVector4 operator+(const VxVector4 &v1, const VxVector4 &v2) { VxVector4 r = v1; r += v2; return r; }
inline const VxVector4 operator-(const VxVector4 &v1, const VxVector4 &v2) { VxVector4 r = v1; r -= v2; return r; }
inline const VxVector4 operator*(const VxVector4 &v1, const VxVector4 &v2) { VxVector4 r = v1; r *= v2; return r; }
inline const VxVector4 operator/(const VxVector4 &v1, const VxVector4 &v2) { VxVector4 r = v1; r /= v2; return r; }
inline const VxVector4 operator*(const VxVector4 &v, float s) { VxVector4 r = v; r *= s; return r; }
inline const VxVector4 operator*(float s, const VxVector4 &v) { VxVector4 r = v; r *= s; return r; }
inline const VxVector4 operator/(const VxVector4 &v, float s) { VxVector4 r = v; r /= s; return r; }
inline int operator==(const VxVector4 &v1, const VxVector4 &v2) {
#if defined(VX_SIMD_SSE)
    __m128 a = VxSIMDLoadFloat4((const float *) &v1);
    __m128 b = VxSIMDLoadFloat4((const float *) &v2);
    return (_mm_movemask_ps(_mm_cmpeq_ps(a, b)) == 0xF) ? 1 : 0;
#else
    return ((v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z) && (v1.w == v2.w));
#endif
}
inline int operator!=(const VxVector4 &v1, const VxVector4 &v2) { return !(v1 == v2); }
inline void VxVector4::Set(float _x, float _y, float _z, float _w) { x = _x; y = _y; z = _z; w = _w; }
inline void VxVector4::Set(float _x, float _y, float _z) { x = _x; y = _y; z = _z; }
inline const float &VxVector4::operator[](int i) const { return *((&x) + i); }
inline float &VxVector4::operator[](int i) { return *((&x) + i); }
#if !defined(_MSC_VER)
inline VxVector4::operator float *() const { return (float *) &x; }
#else
inline VxVector4::operator float *() const { return (float *) &v[0]; }
#endif

// --- VxCompressedVector bodies -----------------------------------------------

/**
 * @brief Performs spherical linear interpolation between two compressed vectors.
 */
inline VxCompressedVectorOld::VxCompressedVectorOld() { xa = ya = 0; }

inline VxCompressedVectorOld::VxCompressedVectorOld(float _x, float _y, float _z) { Set(_x, _y, _z); }

inline void VxCompressedVectorOld::Slerp(float step, VxCompressedVectorOld &v1, VxCompressedVectorOld &v2) {
    int v1y = ((int) v1.ya + 16384) & 16383;
    int v2y = ((int) v2.ya + 16384) & 16383;
    v2y = (v2y - v1y);
    if (v2y > 8192) v2y = 16384 - v2y;
    else if (v2y < -8192) v2y = 16384 + v2y;
    xa = (int) ((float) v1.xa + (float) (v2.xa - v1.xa) * step);
    ya = (int) ((float) v1y + (float) v2y * step);
}

inline VxCompressedVectorOld &VxCompressedVectorOld::operator=(const VxVector &v) {
    Set(v.x, v.y, v.z);
    return *this;
}

inline void VxCompressedVectorOld::Set(float X, float Y, float Z) {
    xa = -radToAngle((float) asin(Y));
    ya = radToAngle((float) atan2(X, Z));
}

inline VxCompressedVector::VxCompressedVector() { xa = ya = 0; }

inline VxCompressedVector::VxCompressedVector(float _x, float _y, float _z) { Set(_x, _y, _z); }

inline void VxCompressedVector::Slerp(float step, VxCompressedVector &v1, VxCompressedVector &v2) {
    int coef = (int) (65536.0f * step);
    int v1y = ((int) v1.ya + 16384) & 16383;
    int v2y = ((int) v2.ya + 16384) & 16383;
    v2y = (v2y - v1y);
    if (v2y > 8192) v2y = 16384 - v2y;
    else if (v2y < -8192) v2y = 16384 + v2y;
    xa = (short int) ((int) v1.xa + (((v2.xa - v1.xa) * coef) >> 16));
    ya = (short int) (v1y + ((v2y * coef) >> 16));
}

inline VxCompressedVector &VxCompressedVector::operator=(const VxVector &v) {
    Set(v.x, v.y, v.z);
    return *this;
}

inline void VxCompressedVector::Set(float X, float Y, float Z) {
    xa = (short int) -radToAngle((float) asin(Y));
    ya = (short int) radToAngle((float) atan2(X, Z));
}

inline VxCompressedVectorOld &VxCompressedVectorOld::operator=(const VxCompressedVector &v) {
    xa = (int) v.xa;
    ya = (int) v.ya;
    return *this;
}

inline VxCompressedVector &VxCompressedVector::operator=(const VxCompressedVectorOld &v) {
    xa = (short int) v.xa;
    ya = (short int) v.ya;
    return *this;
}

// =============================================================================
// Part 2 - SIMD High-Level VxVector / VxVector4 Operations
// =============================================================================

#if defined(VX_SIMD_SSE)

#include "VxSIMD.h"

VX_SIMD_INLINE void VxSIMDNormalizeVector(VxVector *v) noexcept {
    __m128 vec = VxSIMDLoadFloat3(&v->x);
    __m128 dot = VxSIMDDotProduct3(vec, vec);
    __m128 mask = _mm_cmpgt_ps(dot, VX_SIMD_EPSILON);
    __m128 safeDot = _mm_max_ps(dot, VX_SIMD_EPSILON);
    __m128 invLen = VxSIMDReciprocalSqrtAccurate(safeDot);
    __m128 normalized = _mm_mul_ps(vec, invLen);
    __m128 keepOriginal = _mm_andnot_ps(mask, vec);
    __m128 useNormalized = _mm_and_ps(mask, normalized);
    vec = _mm_or_ps(keepOriginal, useNormalized);
    VxSIMDStoreFloat3(&v->x, vec);
}

VX_SIMD_INLINE void VxSIMDAddVector(VxVector *result, const VxVector *a, const VxVector *b) noexcept {
    __m128 aVec = VxSIMDLoadFloat3(&a->x);
    __m128 bVec = VxSIMDLoadFloat3(&b->x);
    __m128 resultVec = _mm_add_ps(aVec, bVec);
    VxSIMDStoreFloat3(&result->x, resultVec);
}

VX_SIMD_INLINE void VxSIMDSubtractVector(VxVector *result, const VxVector *a, const VxVector *b) noexcept {
    __m128 aVec = VxSIMDLoadFloat3(&a->x);
    __m128 bVec = VxSIMDLoadFloat3(&b->x);
    __m128 resultVec = _mm_sub_ps(aVec, bVec);
    VxSIMDStoreFloat3(&result->x, resultVec);
}

VX_SIMD_INLINE void VxSIMDScaleVector(VxVector *result, const VxVector *v, float scalar) noexcept {
    __m128 vec = VxSIMDLoadFloat3(&v->x);
    __m128 scaleVec = _mm_set1_ps(scalar);
    __m128 resultVec = _mm_mul_ps(vec, scaleVec);
    VxSIMDStoreFloat3(&result->x, resultVec);
}

VX_SIMD_INLINE float VxSIMDDotVector(const VxVector *a, const VxVector *b) noexcept {
    const __m128 aVec = VxSIMDLoadFloat3(&a->x);
    const __m128 bVec = VxSIMDLoadFloat3(&b->x);
    return _mm_cvtss_f32(VxSIMDDotProduct3(aVec, bVec));
}

VX_SIMD_INLINE void VxSIMDCrossVector(VxVector *result, const VxVector *a, const VxVector *b) noexcept {
    __m128 aVec = VxSIMDLoadFloat3(&a->x);
    __m128 bVec = VxSIMDLoadFloat3(&b->x);
    __m128 resultVec = VxSIMDCrossProduct3(aVec, bVec);
    VxSIMDStoreFloat3(&result->x, resultVec);
}

VX_SIMD_INLINE float VxSIMDLengthVector(const VxVector *v) noexcept {
    const __m128 vec = VxSIMDLoadFloat3(&v->x);
    return _mm_cvtss_f32(_mm_sqrt_ss(VxSIMDDotProduct3(vec, vec)));
}

VX_SIMD_INLINE float VxSIMDLengthSquaredVector(const VxVector *v) noexcept {
    const __m128 vec = VxSIMDLoadFloat3(&v->x);
    return _mm_cvtss_f32(VxSIMDDotProduct3(vec, vec));
}

VX_SIMD_INLINE float VxSIMDDistanceVector(const VxVector *a, const VxVector *b) noexcept {
    const __m128 aVec = VxSIMDLoadFloat3(&a->x);
    const __m128 bVec = VxSIMDLoadFloat3(&b->x);
    const __m128 diff = _mm_sub_ps(aVec, bVec);
    return _mm_cvtss_f32(_mm_sqrt_ss(VxSIMDDotProduct3(diff, diff)));
}

VX_SIMD_INLINE void VxSIMDLerpVector(VxVector *result, const VxVector *a, const VxVector *b, float t) noexcept {
    __m128 aVec = VxSIMDLoadFloat3(&a->x);
    __m128 bVec = VxSIMDLoadFloat3(&b->x);
    __m128 tVec = _mm_set1_ps(t);
    __m128 diff = _mm_sub_ps(bVec, aVec);
    __m128 resultVec = VX_FMADD_PS(diff, tVec, aVec);
    VxSIMDStoreFloat3(&result->x, resultVec);
}

VX_SIMD_INLINE void VxSIMDReflectVector(VxVector *result, const VxVector *incident, const VxVector *normal) noexcept {
    __m128 iVec = VxSIMDLoadFloat3(&incident->x);
    __m128 nVec = VxSIMDLoadFloat3(&normal->x);
    __m128 dotVec = VxSIMDDotProduct3(iVec, nVec);
    __m128 twoVec = VX_SIMD_TWO;
    __m128 factor = _mm_mul_ps(twoVec, dotVec);
    __m128 reflection = VX_FNMADD_PS(factor, nVec, iVec);
    VxSIMDStoreFloat3(&result->x, reflection);
}

VX_SIMD_INLINE void VxSIMDMinimizeVector(VxVector *result, const VxVector *a, const VxVector *b) noexcept {
    __m128 aVec = VxSIMDLoadFloat3(&a->x);
    __m128 bVec = VxSIMDLoadFloat3(&b->x);
    __m128 resultVec = _mm_min_ps(aVec, bVec);
    VxSIMDStoreFloat3(&result->x, resultVec);
}

VX_SIMD_INLINE void VxSIMDMaximizeVector(VxVector *result, const VxVector *a, const VxVector *b) noexcept {
    __m128 aVec = VxSIMDLoadFloat3(&a->x);
    __m128 bVec = VxSIMDLoadFloat3(&b->x);
    __m128 resultVec = _mm_max_ps(aVec, bVec);
    VxSIMDStoreFloat3(&result->x, resultVec);
}

VX_SIMD_INLINE void VxSIMDRotateVector(VxVector *result, const VxMatrix *mat, const VxVector *v) noexcept {
    __m128 vec = VxSIMDLoadFloat3(&v->x);
    const float *matData = reinterpret_cast<const float *>(mat);
    __m128 res = VxSIMDMatrixRotateVector3(matData, vec);
    VxSIMDStoreFloat3(&result->x, res);
}

// --- VxVector4 SIMD ---------------------------------------------------------

VX_SIMD_INLINE void VxSIMDAddVector4(VxVector4 *result, const VxVector4 *a, const VxVector4 *b) noexcept {
    __m128 aVec = VxSIMDLoadFloat4((const float *) a);
    __m128 bVec = VxSIMDLoadFloat4((const float *) b);
    __m128 resultVec = _mm_add_ps(aVec, bVec);
    VxSIMDStoreFloat4((float *) result, resultVec);
}

VX_SIMD_INLINE void VxSIMDSubtractVector4(VxVector4 *result, const VxVector4 *a, const VxVector4 *b) noexcept {
    __m128 aVec = VxSIMDLoadFloat4((const float *) a);
    __m128 bVec = VxSIMDLoadFloat4((const float *) b);
    __m128 resultVec = _mm_sub_ps(aVec, bVec);
    VxSIMDStoreFloat4((float *) result, resultVec);
}

VX_SIMD_INLINE void VxSIMDScaleVector4(VxVector4 *result, const VxVector4 *v, float scalar) noexcept {
    __m128 vec = VxSIMDLoadFloat4((const float *) v);
    __m128 scaleVec = _mm_set1_ps(scalar);
    __m128 resultVec = _mm_mul_ps(vec, scaleVec);
    VxSIMDStoreFloat4((float *) result, resultVec);
}

VX_SIMD_INLINE float VxSIMDDotVector4(const VxVector4 *a, const VxVector4 *b) noexcept {
    __m128 aVec = VxSIMDLoadFloat4((const float *) a);
    __m128 bVec = VxSIMDLoadFloat4((const float *) b);
    __m128 dotVec = VxSIMDDotProduct4(aVec, bVec);
    float result;
    _mm_store_ss(&result, dotVec);
    return result;
}

VX_SIMD_INLINE void VxSIMDLerpVector4(VxVector4 *result, const VxVector4 *a, const VxVector4 *b, float t) noexcept {
    __m128 aVec = VxSIMDLoadFloat4((const float *) a);
    __m128 bVec = VxSIMDLoadFloat4((const float *) b);
    __m128 tVec = _mm_set1_ps(t);
    __m128 diff = _mm_sub_ps(bVec, aVec);
    __m128 resultVec = VX_FMADD_PS(diff, tVec, aVec);
    VxSIMDStoreFloat4((float *) result, resultVec);
}

// --- Array operations --------------------------------------------------------

VX_SIMD_INLINE void VxSIMDInterpolateFloatArray(float *result, const float *a, const float *b, float factor, int count) noexcept {
    const __m128 factorVec = _mm_set1_ps(factor);

    const int simdCount = count & ~3;
    for (int i = 0; i < simdCount; i += 4) {
        const __m128 aVec = _mm_loadu_ps(a + i);
        const __m128 bVec = _mm_loadu_ps(b + i);
        const __m128 diff = _mm_sub_ps(bVec, aVec);
        const __m128 resultVec = VX_FMADD_PS(diff, factorVec, aVec);
        _mm_storeu_ps(result + i, resultVec);
    }

    for (int i = simdCount; i < count; ++i) {
        result[i] = a[i] + (b[i] - a[i]) * factor;
    }
}

VX_SIMD_INLINE void VxSIMDInterpolateVectorArray(void *result, const void *a, const void *b, float factor, int count, XDWORD strideResult, XDWORD strideInput) noexcept {
    const __m128 factorVec = _mm_set1_ps(factor);

    if (strideResult == static_cast<XDWORD>(sizeof(VxVector)) &&
        strideInput == static_cast<XDWORD>(sizeof(VxVector))) {
        const VxVector *vecA = static_cast<const VxVector *>(a);
        const VxVector *vecB = static_cast<const VxVector *>(b);
        VxVector *vecResult = static_cast<VxVector *>(result);

        int i = 0;
        for (; i + 3 < count; i += 4) {
            __m128 a0 = VxSIMDLoadFloat3(&vecA[i + 0].x);
            __m128 b0 = VxSIMDLoadFloat3(&vecB[i + 0].x);
            __m128 a1 = VxSIMDLoadFloat3(&vecA[i + 1].x);
            __m128 b1 = VxSIMDLoadFloat3(&vecB[i + 1].x);
            __m128 a2 = VxSIMDLoadFloat3(&vecA[i + 2].x);
            __m128 b2 = VxSIMDLoadFloat3(&vecB[i + 2].x);
            __m128 a3 = VxSIMDLoadFloat3(&vecA[i + 3].x);
            __m128 b3 = VxSIMDLoadFloat3(&vecB[i + 3].x);

            VxSIMDStoreFloat3(&vecResult[i + 0].x, VX_FMADD_PS(_mm_sub_ps(b0, a0), factorVec, a0));
            VxSIMDStoreFloat3(&vecResult[i + 1].x, VX_FMADD_PS(_mm_sub_ps(b1, a1), factorVec, a1));
            VxSIMDStoreFloat3(&vecResult[i + 2].x, VX_FMADD_PS(_mm_sub_ps(b2, a2), factorVec, a2));
            VxSIMDStoreFloat3(&vecResult[i + 3].x, VX_FMADD_PS(_mm_sub_ps(b3, a3), factorVec, a3));
        }

        for (; i < count; ++i) {
            __m128 va = VxSIMDLoadFloat3(&vecA[i].x);
            __m128 vb = VxSIMDLoadFloat3(&vecB[i].x);
            VxSIMDStoreFloat3(&vecResult[i].x, VX_FMADD_PS(_mm_sub_ps(vb, va), factorVec, va));
        }
        return;
    }

    const char *srcA = static_cast<const char *>(a);
    const char *srcB = static_cast<const char *>(b);
    char *dst = static_cast<char *>(result);

    for (int i = 0; i < count; ++i) {
        const VxVector *vecA = reinterpret_cast<const VxVector *>(srcA);
        const VxVector *vecB = reinterpret_cast<const VxVector *>(srcB);
        VxVector *vecResult = reinterpret_cast<VxVector *>(dst);

        __m128 aVec = VxSIMDLoadFloat3(&vecA->x);
        __m128 bVec = VxSIMDLoadFloat3(&vecB->x);
        __m128 diff = _mm_sub_ps(bVec, aVec);
        __m128 resultVec = VX_FMADD_PS(diff, factorVec, aVec);
        VxSIMDStoreFloat3(&vecResult->x, resultVec);

        srcA += strideInput;
        srcB += strideInput;
        dst += strideResult;
    }
}

#endif // VX_SIMD_SSE
