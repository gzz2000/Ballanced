/**
 * @file VxQuaternion.inl
 * @brief Inline implementations for VxQuaternion.
 *
 * Part 1: Scalar inline bodies (moved from VxQuaternion.h).
 * Part 2: SIMD-accelerated high-level operations on VxQuaternion.
 *
 * Included automatically at the bottom of VxQuaternion.h - do not include directly.
 */

#pragma once

#include "VxSIMD.h"

#if defined(VX_SIMD_SSE)
VX_SIMD_INLINE float VxSIMDDotQuaternion(const VxQuaternion *a, const VxQuaternion *b) noexcept;
VX_SIMD_INLINE float VxSIMDMagnitudeQuaternion(const VxQuaternion *q) noexcept;
VX_SIMD_INLINE void VxSIMDConjugateQuaternion(VxQuaternion *result, const VxQuaternion *q) noexcept;
VX_SIMD_INLINE void VxSIMDDivideQuaternion(VxQuaternion *result, const VxQuaternion *p, const VxQuaternion *q) noexcept;
VX_SIMD_INLINE void VxSIMDScaleQuaternion(VxQuaternion *result, const VxQuaternion *q, float scale) noexcept;
VX_SIMD_INLINE void VxSIMDNormalizeQuaternion(VxQuaternion *q) noexcept;
VX_SIMD_INLINE void VxSIMDMultiplyQuaternion(VxQuaternion *result, const VxQuaternion *a, const VxQuaternion *b) noexcept;
VX_SIMD_INLINE void VxSIMDSlerpQuaternion(VxQuaternion *result, float t, const VxQuaternion *a, const VxQuaternion *b) noexcept;
#endif

// =============================================================================
// Part 1 - Scalar Inline Implementations
// =============================================================================

inline VxQuaternion::VxQuaternion() {
    x = y = z = 0.0f;
    w = 1.0f;
}

inline VxQuaternion::VxQuaternion(const VxVector &Vector, float Angle) {
    FromRotation(Vector, Angle);
}

inline VxQuaternion::VxQuaternion(float X, float Y, float Z, float W) {
    x = X;
    y = Y;
    z = Z;
    w = W;
}

inline VxQuaternion VxQuaternion::operator+(const VxQuaternion &q) const {
    return VxQuaternion(x + q.x, y + q.y, z + q.z, w + q.w);
}

inline VxQuaternion VxQuaternion::operator-(const VxQuaternion &q) const {
    return VxQuaternion(x - q.x, y - q.y, z - q.z, w - q.w);
}

inline VxQuaternion VxQuaternion::operator*(const VxQuaternion &q) const {
    return Vx3DQuaternionMultiply(*this, q);
}

inline VxQuaternion VxQuaternion::operator/(const VxQuaternion &q) const {
    return Vx3DQuaternionDivide(*this, q);
}

inline VxQuaternion &VxQuaternion::operator*=(float s) {
    x *= s;
    y *= s;
    z *= s;
    w *= s;
    return *this;
}

inline VxQuaternion VxQuaternion::operator-() const {
    return VxQuaternion(-x, -y, -z, -w);
}

inline VxQuaternion VxQuaternion::operator+() const {
    return *this;
}

inline int operator==(const VxQuaternion &q1, const VxQuaternion &q2) {
#if defined(VX_SIMD_SSE)
    __m128 a = VxSIMDLoadFloat4(&q1.x);
    __m128 b = VxSIMDLoadFloat4(&q2.x);
    return (_mm_movemask_ps(_mm_cmpeq_ps(a, b)) == 0xF) ? 1 : 0;
#else
    return q1.x == q2.x && q1.y == q2.y && q1.z == q2.z && q1.w == q2.w;
#endif
}

inline int operator!=(const VxQuaternion &q1, const VxQuaternion &q2) {
    return q1.x != q2.x || q1.y != q2.y || q1.z != q2.z || q1.w != q2.w;
}

inline VxQuaternion operator*(float s, const VxQuaternion &q) {
    VxQuaternion result;
#if defined(VX_SIMD_SSE)
    VxSIMDScaleQuaternion(&result, &q, s);
#else
    result = VxQuaternion(q.x * s, q.y * s, q.z * s, q.w * s);
#endif
    return result;
}

inline VxQuaternion operator*(const VxQuaternion &q, float s) {
    return s * q;
}

inline float Magnitude(const VxQuaternion &q) {
#if defined(VX_SIMD_SSE)
    return VxSIMDMagnitudeQuaternion(&q);
#else
    return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
#endif
}

inline float DotProduct(const VxQuaternion &q1, const VxQuaternion &q2) {
#if defined(VX_SIMD_SSE)
    return VxSIMDDotQuaternion(&q1, &q2);
#else
    return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
#endif
}

inline const float &VxQuaternion::operator[](int i) const {
    return *((&x) + i);
}

inline float &VxQuaternion::operator[](int i) {
    return *((&x) + i);
}

inline VxQuaternion Vx3DQuaternionConjugate(const VxQuaternion &Quat) {
    VxQuaternion result;
#if defined(VX_SIMD_SSE)
    VxSIMDConjugateQuaternion(&result, &Quat);
#else
    result = VxQuaternion(-Quat.x, -Quat.y, -Quat.z, Quat.w);
#endif
    return result;
}

namespace VxQuaternionDetail {
inline bool NeedsWideProduct(const VxQuaternion &a, const VxQuaternion &b);
inline VxQuaternion MultiplyWide(const VxQuaternion &a, const VxQuaternion &b);
inline VxQuaternion DivideWide(const VxQuaternion &p, const VxQuaternion &q);
}

inline VxQuaternion Vx3DQuaternionMultiply(const VxQuaternion &QuatL, const VxQuaternion &QuatR) {
    VxQuaternion result;
#if defined(VX_SIMD_SSE)
    VxSIMDMultiplyQuaternion(&result, &QuatL, &QuatR);
#else
    if (VxQuaternionDetail::NeedsWideProduct(QuatL, QuatR))
        return VxQuaternionDetail::MultiplyWide(QuatL, QuatR);
    result = VxQuaternion(
        QuatL.x * QuatR.w + QuatL.w * QuatR.x + QuatL.y * QuatR.z - QuatL.z * QuatR.y,
        QuatL.y * QuatR.w + QuatL.w * QuatR.y + QuatL.z * QuatR.x - QuatL.x * QuatR.z,
        QuatL.z * QuatR.w + QuatL.w * QuatR.z + QuatL.x * QuatR.y - QuatL.y * QuatR.x,
        QuatL.w * QuatR.w - QuatL.x * QuatR.x - QuatL.y * QuatR.y - QuatL.z * QuatR.z
    );
#endif
    return result;
}

inline VxQuaternion Vx3DQuaternionDivide(const VxQuaternion &P, const VxQuaternion &Q) {
    VxQuaternion result;
#if defined(VX_SIMD_SSE)
    VxSIMDDivideQuaternion(&result, &P, &Q);
#else
    if (VxQuaternionDetail::NeedsWideProduct(P, Q))
        return VxQuaternionDetail::DivideWide(P, Q);
    // 0x2429D650 has its own accumulation order, including z,y,x,w for
    // the scalar component. Multiplying conjugate(Q) * P reassociates it.
    result = VxQuaternion(
        Q.w*P.x - Q.x*P.w - P.z*Q.y + P.y*Q.z,
        P.y*Q.w - P.w*Q.y - P.x*Q.z + P.z*Q.x,
        P.z*Q.w - P.w*Q.z - P.y*Q.x + P.x*Q.y,
        P.z*Q.z + P.y*Q.y + P.x*Q.x + Q.w*P.w);
#endif
    return result;
}

inline void VxQuaternion::Multiply(const VxQuaternion &Quat) {
    *this = Vx3DQuaternionMultiply(*this, Quat);
}

namespace VxQuaternionDetail {
inline bool NeedsWideProduct(const VxQuaternion &a, const VxQuaternion &b) {
    float minimumA = FLT_MAX, minimumB = FLT_MAX, maximumA = 0.0f, maximumB = 0.0f;
    for (int i = 0; i < 4; ++i) {
        const float av = std::fabs(a[i]), bv = std::fabs(b[i]);
        if (!std::isfinite(av) || !std::isfinite(bv)) return true;
        if (av > maximumA) maximumA = av;
        if (bv > maximumB) maximumB = bv;
        if (av != 0.0f && av < minimumA) minimumA = av;
        if (bv != 0.0f && bv < minimumB) minimumB = bv;
    }
    if (maximumA == 0.0f || maximumB == 0.0f) return false;
    // Conservatively bound all sixteen products and every four-term sum.
    return static_cast<double>(maximumA)*maximumB > static_cast<double>(FLT_MAX)/4.0 ||
           static_cast<double>(minimumA)*minimumB < FLT_MIN;
}

inline float FourProductsWide(float a0, float b0, float a1, float b1,
                              float a2, float b2, float a3, float b3) {
    const double p0 = static_cast<double>(a0)*b0;
    const double p1 = static_cast<double>(a1)*b1;
    const double p2 = static_cast<double>(a2)*b2;
    const double p3 = static_cast<double>(a3)*b3;
    return static_cast<float>(((p0+p1)+p2)+p3);
}

inline VxQuaternion MultiplyWide(const VxQuaternion &a, const VxQuaternion &b) {
    // 0x2429D5C0 / 0x2429C7B0 retain every intermediate in x87 registers.
    return VxQuaternion(
        FourProductsWide(a.x,b.w, a.w,b.x, a.y,b.z, -a.z,b.y),
        FourProductsWide(a.y,b.w, a.w,b.y, a.z,b.x, -a.x,b.z),
        FourProductsWide(a.z,b.w, a.w,b.z, a.x,b.y, -a.y,b.x),
        FourProductsWide(a.w,b.w, -a.x,b.x, -a.y,b.y, -a.z,b.z));
}

inline VxQuaternion DivideWide(const VxQuaternion &p, const VxQuaternion &q) {
    // Same mathematical quotient as conjugate(q)*p, but a distinct order.
    return VxQuaternion(
        FourProductsWide(q.w,p.x, -q.x,p.w, -p.z,q.y, p.y,q.z),
        FourProductsWide(p.y,q.w, -p.w,q.y, -p.x,q.z, p.z,q.x),
        FourProductsWide(p.z,q.w, -p.w,q.z, -p.y,q.x, p.x,q.y),
        FourProductsWide(p.z,q.z, p.y,q.y, p.x,q.x, q.w,p.w));
}

inline uint32_t TrigProductWord(const uint32_t (&product)[38], int bit) {
    const int word = bit / 32, shift = bit % 32;
    uint32_t value = product[word] >> shift;
    if (shift != 0 && word + 1 < 38) value |= product[word + 1] << (32 - shift);
    return value;
}

inline void SinCosWide(double angle, double &sine, double &cosine) {
    const double magnitude = std::fabs(angle);
    if (magnitude < 8192.0 || !std::isfinite(magnitude)) {
        sine = std::sin(angle);
        cosine = std::cos(angle);
        return;
    }
    // Win32 CRT reduction can lose the phase of large finite arguments.
    // Multiply the exact 53-bit significand by floor((2/pi)*2^1152).
    // Even at DBL_MAX the omitted tail contributes less than 2^-128.
    // These little-endian words are reproducible with the Decimal generator
    // in tests/quaternion_reference/generate_trigonometric_reference.py.
    static const uint32_t twoOverPi[36] = {
        0x1f8d5d08u, 0x6bfb5fb1u, 0x8a5292eau, 0x3d0739f7u,
        0xebe5f17bu, 0x7527bac7u, 0x9e5fea2du, 0x4f463f66u,
        0x27cb09b7u, 0x6d367ecfu, 0x5a0a6d1fu, 0xef2f118bu,
        0xde05980fu, 0x1ff897ffu, 0xbdf9283bu, 0x9c845f8bu,
        0x835339f4u, 0x3991d639u, 0xb45f7e41u, 0xe99c7026u,
        0x2ebb4484u, 0xe88235f5u, 0xb129a73eu, 0xfe1deb1cu,
        0x09d1921cu, 0x06492eeau, 0x424dd2e0u, 0xb7246e3au,
        0xdebbc561u, 0xfe5163abu, 0x3c439041u, 0xdb629599u,
        0xf534ddc0u, 0xfc2757d1u, 0x4e441529u, 0xa2f9836eu
    };
    int exponent;
    const uint64_t significand = static_cast<uint64_t>(std::ldexp(std::frexp(magnitude, &exponent), 53));
    uint32_t product[38] = {};
    for (int part = 0; part < 2; ++part) {
        const uint32_t factor = static_cast<uint32_t>(significand >> (32 * part));
        uint64_t carry = 0;
        for (int i = 0; i < 36; ++i) {
            const uint64_t value = static_cast<uint64_t>(twoOverPi[i])*factor + product[i + part] + carry;
            product[i + part] = static_cast<uint32_t>(value);
            carry = value >> 32;
        }
        product[36 + part] = static_cast<uint32_t>(carry);
    }
    // Only the quadrant and the leading 64 fractional bits are needed.
    // For this branch binaryPoint is in [181,1191], within product's bounds.
    const int binaryPoint = 1152 + 53 - exponent;
    const unsigned int quadrant = TrigProductWord(product, binaryPoint) & 3u;
    const double fraction = std::ldexp(static_cast<double>(TrigProductWord(product, binaryPoint - 32)), -32)
                          + std::ldexp(static_cast<double>(TrigProductWord(product, binaryPoint - 64)), -64);
    const double reduced = fraction * 1.57079632679489661923;
    const double s = std::sin(reduced), c = std::cos(reduced);
    switch (quadrant) {
    case 0: sine = s; cosine = c; break;
    case 1: sine = c; cosine = -s; break;
    case 2: sine = -s; cosine = -c; break;
    default: sine = -c; cosine = s; break;
    }
    if (angle < 0.0) sine = -sine;
}

template <typename SineType>
inline void RotationSinCos(float angle, SineType &sine, float &cosine) {
    double wideSine, wideCosine;
    SinCosWide(angle, wideSine, wideCosine);
    sine = static_cast<SineType>(wideSine);
    cosine = static_cast<float>(wideCosine);
}

inline double InterpolationDot(const VxQuaternion &a, const VxQuaternion &b) {
    const double z = static_cast<double>(a.z)*b.z;
    const double y = static_cast<double>(a.y)*b.y;
    const double x = static_cast<double>(a.x)*b.x;
    const double w = static_cast<double>(a.w)*b.w;
    return ((z+y)+x)+w;
}

inline VxQuaternion Interpolate(double t, const VxQuaternion &a, const VxQuaternion &b) {
    const double dot = InterpolationDot(a, b);
    const double sign = dot < 0.0 ? -1.0 : 1.0;
    const double cosine = dot < 0.0 ? -dot : dot;
    VxQuaternion result;
    if (t == 0.0) return a;
    if (t == 1.0) {
        for (int i = 0; i < 4; ++i) result[i] = static_cast<float>(sign*b[i]);
        return result;
    }
    if (!(cosine < 0.99)) {
        // Preserve the close-angle linear convention. This form retains
        // equal endpoints even when 1-t can no longer represent the unit term.
        for (int i = 0; i < 4; ++i)
            result[i] = static_cast<float>(a[i] + t*(sign*b[i] - a[i]));
    } else {
        const double angle = std::acos(cosine);
        const double phase = t*angle;
        double phaseSine, phaseCosine;
        SinCosWide(phase, phaseSine, phaseCosine);
        const double inverseSine = 1.0/std::sin(angle);
        // Rotate in the endpoint plane. Evaluating (1-t)*angle separately
        // loses the phase offset during large extrapolation.
        for (int i = 0; i < 4; ++i) {
            const double tangent = (sign*b[i] - cosine*a[i])*inverseSine;
            result[i] = static_cast<float>(a[i]*phaseCosine + tangent*phaseSine);
        }
    }
    return result;
}

inline double SquaredLengthWide(float x, float y, float z, float w = 0.0f) {
    // All finite float squares and their sum fit in double, including
    // subnormal inputs and inputs whose squared length exceeds FLT_MAX.
    const double x2 = static_cast<double>(x)*x;
    const double y2 = static_cast<double>(y)*y;
    const double z2 = static_cast<double>(z)*z;
    const double w2 = static_cast<double>(w)*w;
    return ((x2+y2)+z2)+w2;
}

inline double LengthWide(float x, float y, float z, float w = 0.0f) {
    return std::sqrt(SquaredLengthWide(x, y, z, w));
}

inline void NormalizeWide(VxQuaternion *q) {
    const double length = LengthWide(q->x, q->y, q->z, q->w);
    if (length == 0.0) {
        *q = VxQuaternion();
        return;
    }
    // Keep the reciprocal wide until each normalized component is stored.
    const double inverse = 1.0/length;
    q->x = static_cast<float>(q->x*inverse);
    q->y = static_cast<float>(q->y*inverse);
    q->z = static_cast<float>(q->z*inverse);
    q->w = static_cast<float>(q->w*inverse);
}
} // namespace VxQuaternionDetail

inline void VxQuaternion::Normalize() {
#if defined(VX_SIMD_SSE)
    VxSIMDNormalizeQuaternion(this);
#else
    VxQuaternionDetail::NormalizeWide(this);
#endif
}

inline VxQuaternion Slerp(float t, const VxQuaternion &Quat1, const VxQuaternion &Quat2) {
    return VxQuaternionDetail::Interpolate(t, Quat1, Quat2);
}

inline VxQuaternion Squad(float t, const VxQuaternion &Quat1, const VxQuaternion &Quat1Out, const VxQuaternion &Quat2In,
                          const VxQuaternion &Quat2) {
    VxQuaternion slerpA = Slerp(t, Quat1Out, Quat2In);
    VxQuaternion slerpB = Slerp(t, Quat1, Quat2);
    const double blendFactor = 2.0 * t * (1.0 - static_cast<double>(t));
    return VxQuaternionDetail::Interpolate(blendFactor, slerpB, slerpA);
}

inline VxQuaternion LnDif(const VxQuaternion &P, const VxQuaternion &Q) {
    VxQuaternion div = Vx3DQuaternionDivide(Q, P);
    return Ln(div);
}

inline VxQuaternion Ln(const VxQuaternion &Quat) {
    const double magnitude = VxQuaternionDetail::LengthWide(Quat.x, Quat.y, Quat.z);
    // Skip division for zero or unordered lengths; retain double precision
    // through the angle and component scaling.
    const double scale = magnitude > 0.0 ?
        (std::atan2(magnitude, static_cast<double>(Quat.w))/magnitude) : magnitude;
    return VxQuaternion(static_cast<float>(scale*Quat.x),
                        static_cast<float>(scale*Quat.y),
                        static_cast<float>(scale*Quat.z), 0.0f);
}

inline VxQuaternion Exp(const VxQuaternion &Quat) {
    const double magnitude = VxQuaternionDetail::LengthWide(Quat.x, Quat.y, Quat.z);
    double sine, cosine;
    VxQuaternionDetail::SinCosWide(magnitude, sine, cosine);
    const double scale = magnitude == 0.0 ? 1.0 : sine/magnitude;
    return VxQuaternion(static_cast<float>(scale*Quat.x),
                        static_cast<float>(scale*Quat.y),
                        static_cast<float>(scale*Quat.z),
                        static_cast<float>(cosine));
}

// =============================================================================
// Part 2 - SIMD-Accelerated Quaternion Operations
// =============================================================================

#if defined(VX_SIMD_SSE)

#include "VxSIMD.h"

VX_SIMD_INLINE float VxSIMDDotQuaternion(const VxQuaternion *a, const VxQuaternion *b) noexcept {
    __m128 qa = VxSIMDLoadFloat4(&a->x);
    __m128 qb = VxSIMDLoadFloat4(&b->x);
    __m128 dot = VxSIMDDotProduct4(qa, qb);
    float result;
    _mm_store_ss(&result, dot);
    return result;
}

VX_SIMD_INLINE float VxSIMDMagnitudeQuaternion(const VxQuaternion *q) noexcept {
    return VxSIMDDotQuaternion(q, q);
}

VX_SIMD_INLINE void VxSIMDConjugateQuaternion(VxQuaternion *result, const VxQuaternion *q) noexcept {
    __m128 quat = VxSIMDLoadFloat4(&q->x);
    __m128 conjugate = VxSIMDQuaternionConjugate(quat);
    VxSIMDStoreFloat4(&result->x, conjugate);
}

VX_SIMD_INLINE void VxSIMDDivideQuaternion(VxQuaternion *result, const VxQuaternion *p, const VxQuaternion *q) noexcept {
    if (VxQuaternionDetail::NeedsWideProduct(*p, *q)) {
        *result = VxQuaternionDetail::DivideWide(*p, *q);
        return;
    }
    const __m128 qp = VxSIMDLoadFloat4(&p->x), qq = VxSIMDLoadFloat4(&q->x);
    const __m128 negateXYZ = _mm_setr_ps(-0.0f, -0.0f, -0.0f, 0.0f);
    const __m128 a = _mm_mul_ps(_mm_shuffle_ps(qp, qp, _MM_SHUFFLE(2, 2, 1, 0)),
                                _mm_shuffle_ps(qq, qq, _MM_SHUFFLE(2, 3, 3, 3)));
    const __m128 b = _mm_xor_ps(negateXYZ,
        _mm_mul_ps(_mm_shuffle_ps(qp, qp, _MM_SHUFFLE(1, 3, 3, 3)),
                   _mm_shuffle_ps(qq, qq, _MM_SHUFFLE(1, 2, 1, 0))));
    const __m128 c = _mm_xor_ps(negateXYZ,
        _mm_mul_ps(_mm_shuffle_ps(qp, qp, _MM_SHUFFLE(0, 1, 0, 2)),
                   _mm_shuffle_ps(qq, qq, _MM_SHUFFLE(0, 0, 2, 1))));
    const __m128 d = _mm_mul_ps(_mm_shuffle_ps(qp, qp, _MM_SHUFFLE(3, 0, 2, 1)),
                                _mm_shuffle_ps(qq, qq, _MM_SHUFFLE(3, 1, 0, 2)));
    const __m128 divided = _mm_add_ps(_mm_add_ps(_mm_add_ps(a, b), c), d);
    VxSIMDStoreFloat4(&result->x, divided);
}

VX_SIMD_INLINE void VxSIMDScaleQuaternion(VxQuaternion *result, const VxQuaternion *q, float scale) noexcept {
    __m128 quat = VxSIMDLoadFloat4(&q->x);
    __m128 scaled = _mm_mul_ps(quat, _mm_set1_ps(scale));
    VxSIMDStoreFloat4(&result->x, scaled);
}

VX_SIMD_INLINE void VxSIMDNormalizeQuaternion(VxQuaternion *q) noexcept {
    if (VxQuaternionDetail::NeedsWideProduct(*q, *q)) {
        VxQuaternionDetail::NormalizeWide(q);
        return;
    }
    const __m128 quat = _mm_loadu_ps(&q->x);
    const __m128 squares = _mm_mul_ps(quat, quat);
    // Match ((x*x + y*y) + z*z) + w*w, without a horizontal reassociation.
    __m128 sum = _mm_add_ss(squares, _mm_shuffle_ps(squares, squares, _MM_SHUFFLE(1, 1, 1, 1)));
    sum = _mm_add_ss(sum, _mm_shuffle_ps(squares, squares, _MM_SHUFFLE(2, 2, 2, 2)));
    sum = _mm_add_ss(sum, _mm_shuffle_ps(squares, squares, _MM_SHUFFLE(3, 3, 3, 3)));
    if (_mm_cvtss_f32(sum) == 0.0f) {
        *q = VxQuaternion();
        return;
    }
    const __m128 inverse = _mm_div_ss(_mm_set_ss(1.0f), _mm_sqrt_ss(sum));
    VxQuaternion result;
    _mm_storeu_ps(&result.x, _mm_mul_ps(quat, _mm_shuffle_ps(inverse, inverse, 0)));
    *q = result;
}

VX_SIMD_INLINE void VxSIMDMultiplyQuaternion(VxQuaternion *result, const VxQuaternion *a, const VxQuaternion *b) noexcept {
    if (VxQuaternionDetail::NeedsWideProduct(*a, *b)) {
        *result = VxQuaternionDetail::MultiplyWide(*a, *b);
        return;
    }
    __m128 qa = _mm_loadu_ps(&a->x);
    __m128 qb = _mm_loadu_ps(&b->x);
    __m128 r = VxSIMDQuaternionMultiply(qa, qb);
    _mm_storeu_ps(&result->x, r);
}

VX_SIMD_INLINE void VxSIMDSlerpQuaternion(VxQuaternion *result, float t, const VxQuaternion *a, const VxQuaternion *b) noexcept {
    *result = VxQuaternionDetail::Interpolate(t, *a, *b);
}

#endif // VX_SIMD_SSE
