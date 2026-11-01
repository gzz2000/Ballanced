/**
 * @file VxSIMD.inl
 * @brief Low-level SIMD primitives operating on raw __m128 registers.
 *
 * @remarks
 * This file provides the foundational SIMD building blocks (load/store,
 * dot product, cross product, normalization, reciprocal sqrt, basic
 * arithmetic) used internally by the per-type .inl files.
 *
 * Included automatically by VxSIMD.h - do not include directly.
 *
 * Follows the DirectXMath pattern of separating raw vector math from
 * high-level type wrappers.
 */

#pragma once

#if defined(VX_SIMD_SSE)

// ============================================================================
// Vector Load / Store
// ============================================================================

/**
 * @brief Loads 3 floats into a SIMD register (w = 0).
 */
VX_SIMD_INLINE __m128 VxSIMDLoadFloat3(const float *ptr) noexcept {
    return _mm_set_ps(0.0f, ptr[2], ptr[1], ptr[0]);
}

/**
 * @brief Stores the first 3 components of a SIMD register.
 */
VX_SIMD_INLINE void VxSIMDStoreFloat3(float *ptr, __m128 v) noexcept {
    _mm_storel_epi64(reinterpret_cast<__m128i *>(ptr), _mm_castps_si128(v));
    _mm_store_ss(ptr + 2, _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)));
}

/**
 * @brief Loads 4 floats (unaligned) into a SIMD register.
 */
VX_SIMD_INLINE __m128 VxSIMDLoadFloat4(const float *ptr) noexcept {
    return _mm_loadu_ps(ptr);
}

/**
 * @brief Stores 4 floats (unaligned) from a SIMD register.
 */
VX_SIMD_INLINE void VxSIMDStoreFloat4(float *ptr, __m128 v) noexcept {
    _mm_storeu_ps(ptr, v);
}

/**
 * @brief Loads 4 aligned floats into a SIMD register.
 */
VX_SIMD_INLINE __m128 VxSIMDLoadFloat4Aligned(const float *ptr) noexcept {
    return _mm_load_ps(ptr);
}

/**
 * @brief Stores 4 aligned floats from a SIMD register.
 */
VX_SIMD_INLINE void VxSIMDStoreFloat4Aligned(float *ptr, __m128 v) noexcept {
    _mm_store_ps(ptr, v);
}

// ============================================================================
// Dot Product
// ============================================================================

/**
 * @brief 3-component dot product (result broadcast to all lanes).
 */
VX_SIMD_INLINE __m128 VxSIMDDotProduct3(__m128 a, __m128 b) noexcept {
#if defined(VX_SIMD_SSE4_1)
    return _mm_dp_ps(a, b, 0x7F);
#else
    __m128 mul = _mm_mul_ps(a, b);
    __m128 sum_xy = _mm_add_ss(mul, _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(1, 1, 1, 1)));
    __m128 sum_xyz = _mm_add_ss(sum_xy, _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 2, 2, 2)));
    return _mm_shuffle_ps(sum_xyz, sum_xyz, _MM_SHUFFLE(0, 0, 0, 0));
#endif
}

/**
 * @brief 4-component dot product (result broadcast to all lanes).
 */
VX_SIMD_INLINE __m128 VxSIMDDotProduct4(__m128 a, __m128 b) noexcept {
#if defined(VX_SIMD_SSE4_1)
    return _mm_dp_ps(a, b, 0xFF);
#else
    __m128 mul = _mm_mul_ps(a, b);
    __m128 shuf = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 3, 0, 1));
    __m128 sums = _mm_add_ps(mul, shuf);
    shuf = _mm_movehl_ps(shuf, sums);
    sums = _mm_add_ps(sums, shuf);
    return _mm_shuffle_ps(sums, sums, _MM_SHUFFLE(0, 0, 0, 0));
#endif
}

// ============================================================================
// Cross Product
// ============================================================================

/**
 * @brief 3-component cross product.
 */
VX_SIMD_INLINE __m128 VxSIMDCrossProduct3(__m128 a, __m128 b) noexcept {
    __m128 a_yzx = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 b_zxy = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 1, 0, 2));
    __m128 a_zxy = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 1, 0, 2));
    __m128 b_yzx = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
    return VX_FNMADD_PS(a_zxy, b_yzx, _mm_mul_ps(a_yzx, b_zxy));
}

// ============================================================================
// Reciprocal Square Root / Reciprocal
// ============================================================================

/**
 * @brief Fast reciprocal square root (approx).
 */
VX_SIMD_INLINE __m128 VxSIMDReciprocalSqrt(__m128 v) noexcept {
    return _mm_rsqrt_ps(v);
}

/**
 * @brief Accurate reciprocal square root (Newton-Raphson refinement).
 */
VX_SIMD_INLINE __m128 VxSIMDReciprocalSqrtAccurate(__m128 v) noexcept {
#if defined(VX_SIMD_USE_SIMDE)
    return _mm_div_ps(VX_SIMD_ONE, _mm_sqrt_ps(v));
#else
    __m128 rsqrt = _mm_rsqrt_ps(v);
    __m128 halfV = _mm_mul_ps(VX_SIMD_NR_HALF, v);
    __m128 rsqrt_sq = _mm_mul_ps(rsqrt, rsqrt);
    __m128 correction = VX_FNMADD_PS(halfV, rsqrt_sq, VX_SIMD_NR_THREE_HALF);
    return _mm_mul_ps(rsqrt, correction);
#endif
}

/**
 * @brief Accurate reciprocal (1/x) with Newton-Raphson refinement.
 */
VX_SIMD_INLINE __m128 VxSIMDReciprocalAccurate(__m128 v) noexcept {
#if defined(VX_SIMD_USE_SIMDE)
    return _mm_div_ps(VX_SIMD_ONE, v);
#else
    __m128 rcp = _mm_rcp_ps(v);
    __m128 two = _mm_set1_ps(2.0f);
    return _mm_mul_ps(rcp, VX_FNMADD_PS(v, rcp, two));
#endif
}

// ============================================================================
// Normalization
// ============================================================================

VX_SIMD_INLINE __m128 VxSIMDNormalizeChecked(__m128 v, __m128 dot) noexcept {
    __m128 mask = _mm_cmpgt_ps(dot, VX_SIMD_EPSILON);
    __m128 safeDot = _mm_max_ps(dot, VX_SIMD_EPSILON);
    __m128 invLen = VxSIMDReciprocalSqrtAccurate(safeDot);
    __m128 normalized = _mm_mul_ps(v, invLen);
    __m128 keepOriginal = _mm_andnot_ps(mask, v);
    __m128 useNormalized = _mm_and_ps(mask, normalized);
    return _mm_or_ps(keepOriginal, useNormalized);
}

/**
 * @brief Normalizes a 3D vector register.
 */
VX_SIMD_INLINE __m128 VxSIMDNormalize3(__m128 v) noexcept {
    return VxSIMDNormalizeChecked(v, VxSIMDDotProduct3(v, v));
}

/**
 * @brief Normalizes a 4D vector register.
 */
VX_SIMD_INLINE __m128 VxSIMDNormalize4(__m128 v) noexcept {
    return VxSIMDNormalizeChecked(v, VxSIMDDotProduct4(v, v));
}

// ============================================================================
// Matrix-Vector Multiplication (raw float* matrix)
// ============================================================================

/**
 * @brief Matrix * vec3 with translation (affine transform).
 */
VX_SIMD_INLINE __m128 VxSIMDMatrixMultiplyVector3(const float *mat, __m128 v) noexcept {
    __m128 r0 = _mm_loadu_ps(&mat[0]);
    __m128 r1 = _mm_loadu_ps(&mat[4]);
    __m128 r2 = _mm_loadu_ps(&mat[8]);
    __m128 r3 = _mm_loadu_ps(&mat[12]);

    __m128 v_x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 v_y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 v_z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));

    // Original VxMath.dll 0x2428396a–0x2428399a adds x, y, z, then
    // translation. Reassociation changes coordinates under cancellation,
    // including the ball's local position during a transformer animation.
    // Keep the original separate SSE products/additions (no FMA).
    __m128 result = _mm_add_ps(_mm_mul_ps(r0, v_x), _mm_mul_ps(r1, v_y));
    result = _mm_add_ps(result, _mm_mul_ps(r2, v_z));
    result = _mm_add_ps(result, r3);
    return result;
}

/**
 * @brief Matrix rotation * vec3 (no translation).
 */
VX_SIMD_INLINE __m128 VxSIMDMatrixRotateVector3(const float *mat, __m128 v) noexcept {
    __m128 r0 = _mm_loadu_ps(&mat[0]);
    __m128 r1 = _mm_loadu_ps(&mat[4]);
    __m128 r2 = _mm_loadu_ps(&mat[8]);

    __m128 v_x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 v_y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 v_z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));

    __m128 result = _mm_mul_ps(r2, v_z);
    result = VX_FMADD_PS(r1, v_y, result);
    result = VX_FMADD_PS(r0, v_x, result);
    return result;
}

/**
 * @brief Matrix * vec4.
 */
VX_SIMD_INLINE __m128 VxSIMDMatrixMultiplyVector4(const float *mat, __m128 v) noexcept {
    __m128 r0 = _mm_loadu_ps(&mat[0]);
    __m128 r1 = _mm_loadu_ps(&mat[4]);
    __m128 r2 = _mm_loadu_ps(&mat[8]);
    __m128 r3 = _mm_loadu_ps(&mat[12]);

    __m128 v_x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 v_y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 v_z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
    __m128 v_w = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3));

    __m128 result = VX_FMADD_PS(r2, v_z, _mm_mul_ps(r3, v_w));
    result = VX_FMADD_PS(r1, v_y, result);
    result = VX_FMADD_PS(r0, v_x, result);
    return result;
}

// ============================================================================
// Register-Level Arithmetic
// ============================================================================

VX_SIMD_INLINE __m128 VxSIMDAdd3(__m128 a, __m128 b) noexcept {
    return _mm_add_ps(a, b);
}

VX_SIMD_INLINE __m128 VxSIMDSubtract3(__m128 a, __m128 b) noexcept {
    return _mm_sub_ps(a, b);
}

VX_SIMD_INLINE __m128 VxSIMDScale(__m128 v, float scalar) noexcept {
    return _mm_mul_ps(v, _mm_set1_ps(scalar));
}

VX_SIMD_INLINE float VxSIMDLength3(__m128 v) noexcept {
    __m128 dot = VxSIMDDotProduct3(v, v);
    __m128 len = _mm_sqrt_ss(dot);
    float result;
    _mm_store_ss(&result, len);
    return result;
}

VX_SIMD_INLINE float VxSIMDLengthSquared3(__m128 v) noexcept {
    __m128 dot = VxSIMDDotProduct3(v, v);
    float result;
    _mm_store_ss(&result, dot);
    return result;
}

VX_SIMD_INLINE __m128 VxSIMDLerp(__m128 a, __m128 b, float t) noexcept {
    __m128 tVec = _mm_set1_ps(t);
    __m128 diff = _mm_sub_ps(b, a);
    return VX_FMADD_PS(diff, tVec, a);
}

VX_SIMD_INLINE __m128 VxSIMDMin(__m128 a, __m128 b) noexcept {
    return _mm_min_ps(a, b);
}

VX_SIMD_INLINE __m128 VxSIMDMax(__m128 a, __m128 b) noexcept {
    return _mm_max_ps(a, b);
}

VX_SIMD_INLINE __m128 VxSIMDReflect3(__m128 incident, __m128 normal) noexcept {
    __m128 dot = VxSIMDDotProduct3(incident, normal);
    __m128 two_dot = _mm_add_ps(dot, dot);
    return VX_FNMADD_PS(two_dot, normal, incident);
}

// ============================================================================
// Raw Quaternion Operations (__m128 xyzw layout)
// ============================================================================

VX_SIMD_INLINE __m128 VxSIMDQuaternionNormalize(__m128 q) noexcept {
    __m128 magSq = VxSIMDDotProduct4(q, q);
    __m128 isZero = _mm_cmpeq_ps(magSq, _mm_setzero_ps());
    int zeroMask = _mm_movemask_ps(isZero);
    if (zeroMask & 1) {
        return VX_SIMD_QUAT_IDENTITY;
    }
    __m128 invNorm = VxSIMDReciprocalSqrtAccurate(magSq);
    return _mm_mul_ps(q, invNorm);
}

VX_SIMD_INLINE __m128 VxSIMDQuaternionMultiply(__m128 qa, __m128 qb) noexcept {
    // Match 0x2429D5C0: for xyz, add the two scalar/vector products,
    // then the positive cross term, then subtract the negative cross term.
    const __m128 negateW = _mm_setr_ps(0.0f, 0.0f, 0.0f, -0.0f);
    const __m128 a = _mm_mul_ps(qa, _mm_shuffle_ps(qb, qb, _MM_SHUFFLE(3, 3, 3, 3)));
    const __m128 b = _mm_xor_ps(negateW,
        _mm_mul_ps(_mm_shuffle_ps(qa, qa, _MM_SHUFFLE(0, 3, 3, 3)),
                   _mm_shuffle_ps(qb, qb, _MM_SHUFFLE(0, 2, 1, 0))));
    const __m128 c = _mm_xor_ps(negateW,
        _mm_mul_ps(_mm_shuffle_ps(qa, qa, _MM_SHUFFLE(1, 0, 2, 1)),
                   _mm_shuffle_ps(qb, qb, _MM_SHUFFLE(1, 1, 0, 2))));
    const __m128 d = _mm_mul_ps(_mm_shuffle_ps(qa, qa, _MM_SHUFFLE(2, 1, 0, 2)),
                                _mm_shuffle_ps(qb, qb, _MM_SHUFFLE(2, 0, 2, 1)));
    return _mm_sub_ps(_mm_add_ps(_mm_add_ps(a, b), c), d);
}

VX_SIMD_INLINE __m128 VxSIMDQuaternionConjugate(__m128 q) noexcept {
    static const __m128 conjugateMask = _mm_setr_ps(-1.0f, -1.0f, -1.0f, 1.0f);
    return _mm_mul_ps(q, conjugateMask);
}

// ============================================================================
// Raw Matrix Operations (float[16])
// ============================================================================

VX_SIMD_INLINE void VxSIMDMatrixMultiply(float *result, const float *a, const float *b) noexcept {
    const __m128 a0 = _mm_loadu_ps(a + 0);
    const __m128 a1 = _mm_loadu_ps(a + 4);
    const __m128 a2 = _mm_loadu_ps(a + 8);
    const __m128 a3 = _mm_loadu_ps(a + 12);

    for (int i = 0; i < 4; ++i) {
        const __m128 bRow = _mm_loadu_ps(b + i * 4);
        const __m128 b_x = _mm_shuffle_ps(bRow, bRow, _MM_SHUFFLE(0, 0, 0, 0));
        const __m128 b_y = _mm_shuffle_ps(bRow, bRow, _MM_SHUFFLE(1, 1, 1, 1));
        const __m128 b_z = _mm_shuffle_ps(bRow, bRow, _MM_SHUFFLE(2, 2, 2, 2));
        const __m128 b_w = _mm_shuffle_ps(bRow, bRow, _MM_SHUFFLE(3, 3, 3, 3));

        __m128 res = VX_FMADD_PS(a2, b_z, _mm_mul_ps(a3, b_w));
        res = VX_FMADD_PS(a1, b_y, res);
        res = VX_FMADD_PS(a0, b_x, res);
        _mm_storeu_ps(result + i * 4, res);
    }
}

VX_SIMD_INLINE void VxSIMDMatrixTranspose(float *result, const float *src) noexcept {
    __m128 r0 = _mm_loadu_ps(src + 0);
    __m128 r1 = _mm_loadu_ps(src + 4);
    __m128 r2 = _mm_loadu_ps(src + 8);
    __m128 r3 = _mm_loadu_ps(src + 12);

    _MM_TRANSPOSE4_PS(r0, r1, r2, r3);

    _mm_storeu_ps(result + 0, r0);
    _mm_storeu_ps(result + 4, r1);
    _mm_storeu_ps(result + 8, r2);
    _mm_storeu_ps(result + 12, r3);
}

// ============================================================================
// VxMath Utility Operations
// ============================================================================

VX_SIMD_INLINE void VxSIMDCopyDwordAlignedBlock(void *dst, const void *src, XDWORD sizeBytes) noexcept {
    XBYTE *dstBytes = static_cast<XBYTE *>(dst);
    const XBYTE *srcBytes = static_cast<const XBYTE *>(src);

    XDWORD offset = 0;
    for (; offset + 16 <= sizeBytes; offset += 16) {
        const __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(srcBytes + offset));
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dstBytes + offset), chunk);
    }

    for (; offset < sizeBytes; offset += 4) {
        std::memcpy(dstBytes + offset, srcBytes + offset, sizeof(XDWORD));
    }
}

VX_SIMD_INLINE XBOOL VxSIMDFillStructure(int count, void *dst, XDWORD stride, XDWORD sizeSrc, const void *src) noexcept {
    if (!src || !dst || !count || !sizeSrc || !stride || (sizeSrc & 3) != 0) {
        return FALSE;
    }

    XBYTE *dstBytes = static_cast<XBYTE *>(dst);
    if (sizeSrc == 16) {
        const __m128i pattern = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
        for (int i = 0; i < count; ++i) {
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dstBytes), pattern);
            dstBytes += stride;
        }
        return TRUE;
    }

    for (int i = 0; i < count; ++i) {
        VxSIMDCopyDwordAlignedBlock(dstBytes, src, sizeSrc);
        dstBytes += stride;
    }
    return TRUE;
}

VX_SIMD_INLINE XBOOL VxSIMDCopyStructure(int count, void *dst, XDWORD outStride, XDWORD sizeSrc, const void *src, XDWORD inStride) noexcept {
    if (!src || !dst || !count || !sizeSrc || !outStride || !inStride || (sizeSrc & 3) != 0) {
        return FALSE;
    }

    XBYTE *dstBytes = static_cast<XBYTE *>(dst);
    const XBYTE *srcBytes = static_cast<const XBYTE *>(src);

    for (int i = 0; i < count; ++i) {
        VxSIMDCopyDwordAlignedBlock(dstBytes, srcBytes, sizeSrc);
        dstBytes += outStride;
        srcBytes += inStride;
    }

    return TRUE;
}

VX_SIMD_INLINE XBOOL VxSIMDIndexedCopy(const VxStridedData &dst, const VxStridedData &src, XDWORD sizeSrc, const int *indices, int indexCount) noexcept {
    if (indexCount == 0) {
        return FALSE;
    }
    if (sizeSrc == 0 || (sizeSrc & 3) != 0) {
        return FALSE;
    }
    if (dst.Ptr == nullptr || src.Ptr == nullptr || indices == nullptr) {
        return FALSE;
    }

    XBYTE *dstBytes = static_cast<XBYTE *>(dst.Ptr);
    const XBYTE *srcBase = static_cast<const XBYTE *>(src.Ptr);

    for (int i = 0; i < indexCount; ++i) {
        const XBYTE *srcBytes = srcBase + (indices[i] * src.Stride);
        VxSIMDCopyDwordAlignedBlock(dstBytes, srcBytes, sizeSrc);
        dstBytes += dst.Stride;
    }

    return TRUE;
}

VX_SIMD_INLINE XBOOL VxSIMDPtInRect(const CKRECT *rect, const CKPOINT *pt) noexcept {
    const __m128i pointXYXY = _mm_setr_epi32(pt->x, pt->x, pt->y, pt->y);
    const __m128i minXYXY = _mm_setr_epi32(rect->left, rect->left, rect->top, rect->top);
    const __m128i maxXYXY = _mm_setr_epi32(rect->right, rect->right, rect->bottom, rect->bottom);

    const __m128i geMin = _mm_or_si128(_mm_cmpgt_epi32(pointXYXY, minXYXY), _mm_cmpeq_epi32(pointXYXY, minXYXY));
    const __m128i leMax = _mm_or_si128(_mm_cmpgt_epi32(maxXYXY, pointXYXY), _mm_cmpeq_epi32(maxXYXY, pointXYXY));
    const __m128i insideMask = _mm_and_si128(geMin, leMax);

    return (_mm_movemask_ps(_mm_castsi128_ps(insideMask)) == 0xF) ? TRUE : FALSE;
}

#endif // VX_SIMD_SSE
