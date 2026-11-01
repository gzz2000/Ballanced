/**
 * @file VxFrustum.inl
 * @brief Inline implementations for VxFrustum plus SIMD-accelerated frustum and
 *        box-projection operations.
 *
 * Part 1: Scalar inline bodies (moved from VxFrustum.h).
 * Part 2: SIMD-accelerated frustum update, compute vertices, transform,
 *         and box projection utilities (TransformBox2D, ProjectBoxZExtents).
 *
 * Included automatically at the bottom of VxFrustum.h - do not include directly.
 */

#pragma once

#include "VxSIMD.h"
#include "VxRect.h"

#if defined(VX_SIMD_SSE)
VX_SIMD_INLINE void VxSIMDFrustumUpdate(VxFrustum *frustum) noexcept;
VX_SIMD_INLINE void VxSIMDFrustumComputeVertices(const VxFrustum *frustum, VxVector *vertices) noexcept;
VX_SIMD_INLINE void VxSIMDFrustumTransform(VxFrustum *frustum, const VxMatrix *invWorldMat) noexcept;
#endif

// =============================================================================
// Part 1 - Scalar Inline Implementations
// =============================================================================

inline VxVector &VxFrustum::GetOrigin() {
    return m_Origin;
}

inline const VxVector &VxFrustum::GetOrigin() const {
    return m_Origin;
}

inline VxVector &VxFrustum::GetRight() {
    return m_Right;
}

inline const VxVector &VxFrustum::GetRight() const {
    return m_Right;
}

inline VxVector &VxFrustum::GetUp() {
    return m_Up;
}

inline const VxVector &VxFrustum::GetUp() const {
    return m_Up;
}

inline VxVector &VxFrustum::GetDir() {
    return m_Dir;
}

inline const VxVector &VxFrustum::GetDir() const {
    return m_Dir;
}

inline float &VxFrustum::GetRBound() {
    return m_RBound;
}

inline const float &VxFrustum::GetRBound() const {
    return m_RBound;
}

inline float &VxFrustum::GetUBound() {
    return m_UBound;
}

inline const float &VxFrustum::GetUBound() const {
    return m_UBound;
}

inline float &VxFrustum::GetDMin() {
    return m_DMin;
}

inline const float &VxFrustum::GetDMin() const {
    return m_DMin;
}

inline float &VxFrustum::GetDMax() {
    return m_DMax;
}

inline const float &VxFrustum::GetDMax() const {
    return m_DMax;
}

inline float VxFrustum::GetDRatio() const {
    return m_DRatio;
}

inline float VxFrustum::GetRF() const {
    return m_RF;
}

inline float VxFrustum::GetUF() const {
    return m_UF;
}

inline const VxPlane &VxFrustum::GetNearPlane() const {
    return m_NearPlane;
}

inline const VxPlane &VxFrustum::GetFarPlane() const {
    return m_FarPlane;
}

inline const VxPlane &VxFrustum::GetLeftPlane() const {
    return m_LeftPlane;
}

inline const VxPlane &VxFrustum::GetRightPlane() const {
    return m_RightPlane;
}

inline const VxPlane &VxFrustum::GetUpPlane() const {
    return m_UpPlane;
}

inline const VxPlane &VxFrustum::GetBottomPlane() const {
    return m_BottomPlane;
}

inline VxFrustum::VxFrustum()
    : m_Origin(VxVector::axis0()),
      m_Right(VxVector::axisX()),
      m_Up(VxVector::axisY()),
      m_Dir(VxVector::axisZ()),
      m_RBound(1.0f),
      m_UBound(1.0f),
      m_DMin(1.0f),
      m_DMax(2.0f),
      m_DRatio(0.0f),
      m_RF(0.0f),
      m_UF(0.0f) { Update(); }

inline VxFrustum::VxFrustum(const VxVector &origin, const VxVector &right, const VxVector &up, const VxVector &dir,
                             float nearplane, float farplane, float fov, float aspectratio)
    : m_Origin(origin),
      m_Right(right),
      m_Up(up),
      m_Dir(dir),
      m_RBound(tanf(fov * 0.5f) * nearplane),
      m_UBound(tanf(fov * 0.5f) * nearplane * aspectratio),
      m_DMin(nearplane),
      m_DMax(farplane),
      m_DRatio(0.0f),
      m_RF(0.0f),
      m_UF(0.0f) { Update(); }

inline XDWORD VxFrustum::Classify(const VxVector &v) const {
    XDWORD flags = 0;
    if (GetNearPlane().Classify(v) > 0.0f) flags |= VXCLIP_FRONT;
    else if (GetFarPlane().Classify(v) > 0.0f) flags |= VXCLIP_BACK;
    if (GetLeftPlane().Classify(v) > 0.0f) flags |= VXCLIP_LEFT;
    else if (GetRightPlane().Classify(v) > 0.0f) flags |= VXCLIP_RIGHT;
    if (GetBottomPlane().Classify(v) > 0.0f) flags |= VXCLIP_BOTTOM;
    else if (GetUpPlane().Classify(v) > 0.0f) flags |= VXCLIP_TOP;
    return flags;
}

inline float VxFrustum::Classify(const VxBbox &v) const {
    float cumul = 1.0f;
    float f = m_NearPlane.Classify(v);
    if (f > 0.0f) return f;
    cumul *= f;
    f = m_FarPlane.Classify(v);
    if (f > 0.0f) return f;
    cumul *= f;
    f = m_LeftPlane.Classify(v);
    if (f > 0.0f) return f;
    cumul *= f;
    f = m_RightPlane.Classify(v);
    if (f > 0.0f) return f;
    cumul *= f;
    f = m_UpPlane.Classify(v);
    if (f > 0.0f) return f;
    cumul *= f;
    f = m_BottomPlane.Classify(v);
    if (f > 0.0f) return f;
    cumul *= f;
    return -cumul;
}

inline float VxFrustum::Classify(const VxBbox &b, const VxMatrix &mat) const {
    float cumul = 1.0f;

    VxVector axis[4];
    axis[0] = mat[0] * ((b.Max.x - b.Min.x) * 0.5f);
    axis[1] = mat[1] * ((b.Max.y - b.Min.y) * 0.5f);
    axis[2] = mat[2] * ((b.Max.z - b.Min.z) * 0.5f);
    VxVector v = b.GetCenter();
    Vx3DMultiplyMatrixVector(axis + 3, mat, &v);

    float f = XClassify(axis, m_NearPlane);
    if (f > 0.0f) return f;
    cumul *= f;
    f = XClassify(axis, m_FarPlane);
    if (f > 0.0f) return f;
    cumul *= f;
    f = XClassify(axis, m_LeftPlane);
    if (f > 0.0f) return f;
    cumul *= f;
    f = XClassify(axis, m_RightPlane);
    if (f > 0.0f) return f;
    cumul *= f;
    f = XClassify(axis, m_UpPlane);
    if (f > 0.0f) return f;
    cumul *= f;
    f = XClassify(axis, m_BottomPlane);
    if (f > 0.0f) return f;
    cumul *= f;
    return -cumul;
}

inline XBOOL VxFrustum::IsInside(const VxVector &v) const {
    if (GetNearPlane().Classify(v) > 0.0f) return FALSE;
    if (GetFarPlane().Classify(v) > 0.0f) return FALSE;
    if (GetLeftPlane().Classify(v) > 0.0f) return FALSE;
    if (GetRightPlane().Classify(v) > 0.0f) return FALSE;
    if (GetBottomPlane().Classify(v) > 0.0f) return FALSE;
    if (GetUpPlane().Classify(v) > 0.0f) return FALSE;
    return TRUE;
}

inline void VxFrustum::Update() {
    m_DRatio = m_DMax / m_DMin;
    m_RF = m_RBound * m_DMax * -2.0f;
    m_UF = m_UBound * m_DMax * -2.0f;

    VxVector nearDirVec = m_Dir * m_DMin;
    VxVector upVec = m_Up * m_UBound;
    VxVector rightVec = m_Right * m_RBound;

    VxVector nbl_rel = nearDirVec - rightVec - upVec;
    VxVector ntl_rel = nearDirVec - rightVec + upVec;
    VxVector nbr_rel = nearDirVec + rightVec - upVec;
    VxVector ntr_rel = nearDirVec + rightVec + upVec;

    VxVector nbl = m_Origin + nbl_rel;
    VxVector farVertex = m_Origin + ntr_rel * m_DRatio;

    VxVector nearNormal = -m_Dir;
    m_NearPlane.Create(nearNormal, nbl);
    m_FarPlane.Create(m_Dir, farVertex);

    VxVector leftNormal = CrossProduct(nbl_rel, ntl_rel);
    leftNormal.Normalize();
    m_LeftPlane.Create(leftNormal, m_Origin);

    VxVector rightNormal = CrossProduct(ntr_rel, nbr_rel);
    rightNormal.Normalize();
    m_RightPlane.Create(rightNormal, m_Origin);

    VxVector bottomNormal = CrossProduct(nbr_rel, nbl_rel);
    bottomNormal.Normalize();
    m_BottomPlane.Create(bottomNormal, m_Origin);

    VxVector topNormal = CrossProduct(ntl_rel, ntr_rel);
    topNormal.Normalize();
    m_UpPlane.Create(topNormal, m_Origin);
}

inline void VxFrustum::ComputeVertices(VxVector vertices[8]) const {
#if defined(VX_SIMD_SSE)
    VxSIMDFrustumComputeVertices(this, vertices);
#else
    VxVector nearDirVec = m_Dir * m_DMin;
    VxVector rightVec = m_Right * m_RBound;
    VxVector upVec = m_Up * m_UBound;

    VxVector leftVec = nearDirVec - rightVec;
    VxVector rightVec2 = nearDirVec + rightVec;

    vertices[0] = leftVec - upVec;
    vertices[1] = leftVec + upVec;
    vertices[2] = rightVec2 + upVec;
    vertices[3] = rightVec2 - upVec;

    for (int i = 0; i < 4; i++) {
        VxVector nearVec = vertices[i];
        vertices[i + 4] = m_Origin + nearVec * m_DRatio;
        vertices[i] += m_Origin;
    }
#endif
}

inline void VxFrustum::Transform(const VxMatrix &invworldmat) {
#if defined(VX_SIMD_SSE)
    VxSIMDFrustumTransform(this, &invworldmat);
#else
    m_Right *= m_RBound;
    m_Up *= m_UBound;
    m_Dir *= m_DMin;

    VxVector newOrigin;
    Vx3DMultiplyMatrixVector(&newOrigin, invworldmat, &m_Origin);
    m_Origin = newOrigin;

    VxVector resultVectors[3];
    Vx3DRotateVectorMany(resultVectors, invworldmat, &m_Right, 3, sizeof(VxVector));

    const float newRBound = Magnitude(resultVectors[0]);
    const float newUBound = Magnitude(resultVectors[1]);
    const float newDMin   = Magnitude(resultVectors[2]);

    m_RBound = newRBound;
    m_UBound = newUBound;
    m_DMax   = newDMin * m_DRatio;
    m_DMin   = newDMin;

    if (newRBound > 0.0f) resultVectors[0] /= newRBound; else resultVectors[0] = VxVector::axis0();
    if (newUBound > 0.0f) resultVectors[1] /= newUBound; else resultVectors[1] = VxVector::axis0();
    if (newDMin   > 0.0f) resultVectors[2] /= newDMin;   else resultVectors[2] = VxVector::axis0();

    m_Right = resultVectors[0];
    m_Up    = resultVectors[1];
    m_Dir   = resultVectors[2];

    Update();
#endif
}

inline bool VxFrustum::operator==(const VxFrustum &iFrustum) const {
    return (m_Origin == iFrustum.m_Origin) &&
           (m_Right == iFrustum.m_Right) &&
           (m_Up == iFrustum.m_Up) &&
           (m_Dir == iFrustum.m_Dir) &&
           (m_RBound == iFrustum.m_RBound) &&
           (m_UBound == iFrustum.m_UBound) &&
           (m_DMin == iFrustum.m_DMin) &&
           (m_DMax == iFrustum.m_DMax);
}

inline float VxFrustum::XClassify(const VxVector axis[4], const VxPlane &plane) {
    return plane.XClassify(axis);
}

// =============================================================================
// Part 2 - SIMD-Accelerated Frustum & Box Projection Operations
// =============================================================================

#if defined(VX_SIMD_SSE)

#include "VxSIMD.h"

VX_SIMD_INLINE void VxSIMDFrustumUpdate(VxFrustum *frustum) noexcept {
    frustum->Update();
}

VX_SIMD_INLINE void VxSIMDFrustumComputeVertices(const VxFrustum *frustum, VxVector *vertices) noexcept {
    const __m128 dir = VxSIMDLoadFloat3(&frustum->GetDir().x);
    const __m128 right = VxSIMDLoadFloat3(&frustum->GetRight().x);
    const __m128 up = VxSIMDLoadFloat3(&frustum->GetUp().x);
    const __m128 origin = VxSIMDLoadFloat3(&frustum->GetOrigin().x);
    const __m128 dMin = _mm_set1_ps(frustum->GetDMin());
    const __m128 rBound = _mm_set1_ps(frustum->GetRBound());
    const __m128 uBound = _mm_set1_ps(frustum->GetUBound());
    const __m128 dRatio = _mm_set1_ps(frustum->GetDRatio());

    const __m128 nearDirVec = _mm_mul_ps(dir, dMin);
    const __m128 rightVec = _mm_mul_ps(right, rBound);
    const __m128 upVec = _mm_mul_ps(up, uBound);

    const __m128 leftVec = _mm_sub_ps(nearDirVec, rightVec);
    const __m128 rightVec2 = _mm_add_ps(nearDirVec, rightVec);

    __m128 nearCorners[4];
    nearCorners[0] = _mm_sub_ps(leftVec, upVec);
    nearCorners[1] = _mm_add_ps(leftVec, upVec);
    nearCorners[2] = _mm_add_ps(rightVec2, upVec);
    nearCorners[3] = _mm_sub_ps(rightVec2, upVec);

    for (int i = 0; i < 4; ++i) {
        const __m128 farVec = _mm_add_ps(origin, _mm_mul_ps(nearCorners[i], dRatio));
        const __m128 nearVec = _mm_add_ps(origin, nearCorners[i]);
        VxSIMDStoreFloat3(&vertices[i].x, nearVec);
        VxSIMDStoreFloat3(&vertices[i + 4].x, farVec);
    }
}

VX_SIMD_INLINE void VxSIMDFrustumTransform(VxFrustum *frustum, const VxMatrix *invWorldMat) noexcept {
    VxVector &right = frustum->GetRight();
    VxVector &up = frustum->GetUp();
    VxVector &dir = frustum->GetDir();
    VxVector &origin = frustum->GetOrigin();
    float &rBound = frustum->GetRBound();
    float &uBound = frustum->GetUBound();
    float &dMin = frustum->GetDMin();
    float &dMax = frustum->GetDMax();
    const float dRatio = frustum->GetDRatio();

    const float *matData = reinterpret_cast<const float *>(&(*invWorldMat)[0][0]);
    const __m128 rightScaled = _mm_mul_ps(VxSIMDLoadFloat3(&right.x), _mm_set1_ps(rBound));
    const __m128 upScaled = _mm_mul_ps(VxSIMDLoadFloat3(&up.x), _mm_set1_ps(uBound));
    const __m128 dirScaled = _mm_mul_ps(VxSIMDLoadFloat3(&dir.x), _mm_set1_ps(dMin));

    const __m128 newOrigin = VxSIMDMatrixMultiplyVector3(matData, VxSIMDLoadFloat3(&origin.x));
    VxSIMDStoreFloat3(&origin.x, newOrigin);

    const __m128 rotatedRight = VxSIMDMatrixRotateVector3(matData, rightScaled);
    const __m128 rotatedUp = VxSIMDMatrixRotateVector3(matData, upScaled);
    const __m128 rotatedDir = VxSIMDMatrixRotateVector3(matData, dirScaled);

    const float newRBound = VxSIMDLength3(rotatedRight);
    const float newUBound = VxSIMDLength3(rotatedUp);
    const float newDMin = VxSIMDLength3(rotatedDir);

    rBound = newRBound;
    uBound = newUBound;
    dMin = newDMin;
    dMax = newDMin * dRatio;

    if (newRBound > 0.0f) {
        VxSIMDStoreFloat3(&right.x, _mm_mul_ps(rotatedRight, _mm_set1_ps(1.0f / newRBound)));
    } else {
        right = VxVector::axis0();
    }
    if (newUBound > 0.0f) {
        VxSIMDStoreFloat3(&up.x, _mm_mul_ps(rotatedUp, _mm_set1_ps(1.0f / newUBound)));
    } else {
        up = VxVector::axis0();
    }
    if (newDMin > 0.0f) {
        VxSIMDStoreFloat3(&dir.x, _mm_mul_ps(rotatedDir, _mm_set1_ps(1.0f / newDMin)));
    } else {
        dir = VxVector::axis0();
    }

    frustum->Update();
}

//-------------------------------------------------------------------------------------
// Box Projection Operations
//-------------------------------------------------------------------------------------

VX_SIMD_INLINE XBOOL VxSIMDTransformBox2D(const VxMatrix *worldProjection, const VxBbox *box, VxRect *extents, const VxRect *screenSize, VXCLIP_FLAGS *orClipFlags, VXCLIP_FLAGS *andClipFlags) noexcept {
    if (!box || !box->IsValid()) {
        if (orClipFlags) *orClipFlags = (VXCLIP_FLAGS) VXCLIP_ALL;
        if (andClipFlags) *andClipFlags = (VXCLIP_FLAGS) VXCLIP_ALL;
        return FALSE;
    }

    __m128 verts[8];
    const float dx = box->Max.x - box->Min.x;
    const float dy = box->Max.y - box->Min.y;
    const float dz = box->Max.z - box->Min.z;

    const __m128 col0 = _mm_loadu_ps((const float *) &(*worldProjection)[0][0]);
    const __m128 col1 = _mm_loadu_ps((const float *) &(*worldProjection)[1][0]);
    const __m128 col2 = _mm_loadu_ps((const float *) &(*worldProjection)[2][0]);
    const __m128 col3 = _mm_loadu_ps((const float *) &(*worldProjection)[3][0]);

    const __m128 minX = _mm_shuffle_ps(_mm_set_ss(box->Min.x), _mm_set_ss(box->Min.x), _MM_SHUFFLE(0, 0, 0, 0));
    const __m128 minY = _mm_shuffle_ps(_mm_set_ss(box->Min.y), _mm_set_ss(box->Min.y), _MM_SHUFFLE(0, 0, 0, 0));
    const __m128 minZ = _mm_shuffle_ps(_mm_set_ss(box->Min.z), _mm_set_ss(box->Min.z), _MM_SHUFFLE(0, 0, 0, 0));
    verts[0] = _mm_add_ps(
        _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(minX, col0), _mm_mul_ps(minY, col1)),
            col3),
        _mm_mul_ps(minZ, col2));

    const __m128 deltaX = _mm_mul_ps(col0, _mm_set1_ps(dx));
    const __m128 deltaY = _mm_mul_ps(col1, _mm_set1_ps(dy));
    const int vertexCount = (dz == 0.0f) ? 4 : 8;

    if (vertexCount == 4) {
        verts[1] = _mm_add_ps(verts[0], deltaX);
        verts[2] = _mm_add_ps(verts[0], deltaY);
        verts[3] = _mm_add_ps(verts[1], deltaY);
    } else {
        const __m128 deltaZ = _mm_mul_ps(col2, _mm_set1_ps(dz));
        verts[1] = _mm_add_ps(verts[0], deltaZ);
        verts[2] = _mm_add_ps(verts[0], deltaY);
        verts[3] = _mm_add_ps(verts[1], deltaY);
        verts[4] = _mm_add_ps(verts[0], deltaX);
        verts[5] = _mm_add_ps(verts[4], deltaZ);
        verts[6] = _mm_add_ps(verts[4], deltaY);
        verts[7] = _mm_add_ps(verts[5], deltaY);
    }

    XDWORD allOr = 0;
    XDWORD allAnd = 0xFFFFFFFFu;
    const __m128 zero = _mm_setzero_ps();

    for (int i = 0; i < vertexCount; ++i) {
        __m128 v = verts[i];
        __m128 w = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3));
        __m128 negW = _mm_sub_ps(zero, w);

        __m128 x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));

        XDWORD flags = 0;
        if (_mm_comilt_ss(x, negW)) flags |= VXCLIP_LEFT;
        if (_mm_comigt_ss(x, w)) flags |= VXCLIP_RIGHT;
        if (_mm_comilt_ss(y, negW)) flags |= VXCLIP_BOTTOM;
        if (_mm_comigt_ss(y, w)) flags |= VXCLIP_TOP;
        if (_mm_comilt_ss(z, zero)) flags |= VXCLIP_FRONT;
        if (_mm_comigt_ss(z, w)) flags |= VXCLIP_BACK;

        allOr |= flags;
        allAnd &= flags;
    }

    if (extents && screenSize) {
        extents->left = 1.0e10f;
        extents->top = 1.0e10f;
        extents->right = -1.0e10f;
        extents->bottom = -1.0e10f;
    }

    if (extents && screenSize && (allAnd & VXCLIP_ALL) == 0) {
        __m128 minXY = _mm_set1_ps(1.0e10f);
        __m128 maxXY = _mm_set1_ps(-1.0e10f);

        const float halfWidth = (screenSize->right - screenSize->left) * 0.5f;
        const float halfHeight = (screenSize->bottom - screenSize->top) * 0.5f;
        const float centerX = halfWidth + screenSize->left;
        const float centerY = halfHeight + screenSize->top;

        __m128 viewScale = _mm_setr_ps(halfWidth, -halfHeight, 0.0f, 0.0f);
        __m128 viewOffset = _mm_setr_ps(centerX, centerY, 0.0f, 0.0f);

        for (int i = 0; i < vertexCount; ++i) {
            __m128 v = verts[i];
            __m128 w = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3));
            __m128 invW = _mm_rcp_ss(w);
            invW = _mm_and_ps(_mm_shuffle_ps(invW, invW, _MM_SHUFFLE(0, 0, 0, 0)), VX_SIMD_ABS_MASK);

            __m128 projected = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(v, viewScale), invW), viewOffset);
            minXY = _mm_min_ps(minXY, projected);
            maxXY = _mm_max_ps(maxXY, projected);
        }

        alignas(16) float minf[4], maxf[4];
        _mm_store_ps(minf, minXY);
        _mm_store_ps(maxf, maxXY);

        extents->left = minf[0];
        extents->bottom = maxf[1];
        extents->right = maxf[0];
        extents->top = minf[1];
    }

    if ((allOr & VXCLIP_FRONT) != 0 && extents && screenSize) {
        if ((allOr & VXCLIP_LEFT) != 0) extents->left = screenSize->left;
        if ((allOr & VXCLIP_RIGHT) != 0) extents->right = screenSize->right;
        if ((allOr & VXCLIP_TOP) != 0) extents->top = screenSize->top;
        if ((allOr & VXCLIP_BOTTOM) != 0) extents->bottom = screenSize->bottom;
    }

    if (orClipFlags) *orClipFlags = (VXCLIP_FLAGS) allOr;
    if (andClipFlags) *andClipFlags = (VXCLIP_FLAGS) allAnd;
    return (allAnd & VXCLIP_ALL) == 0;
}

VX_SIMD_INLINE void VxSIMDProjectBoxZExtents(const VxMatrix *worldProjection, const VxBbox *box, float *zhMin, float *zhMax) noexcept {
    if (!zhMin || !zhMax) return;
    *zhMin = 1.0e10f;
    *zhMax = -1.0e10f;

    if (!box || !box->IsValid()) {
        return;
    }

    __m128 corners[8];
    corners[0] = VxSIMDMatrixMultiplyVector3((const float *) &(*worldProjection)[0][0], VxSIMDLoadFloat3(&box->Min.x));

    const float dx = box->Max.x - box->Min.x;
    const float dy = box->Max.y - box->Min.y;
    const float dz = box->Max.z - box->Min.z;

    const __m128 col0 = _mm_loadu_ps((const float *) &(*worldProjection)[0][0]);
    const __m128 col1 = _mm_loadu_ps((const float *) &(*worldProjection)[1][0]);
    const __m128 col2 = _mm_loadu_ps((const float *) &(*worldProjection)[2][0]);
    const __m128 deltaX = _mm_mul_ps(col0, _mm_set1_ps(dx));
    const __m128 deltaY = _mm_mul_ps(col1, _mm_set1_ps(dy));
    const __m128 deltaZ = _mm_mul_ps(col2, _mm_set1_ps(dz));

    int vertexCount;
    if (dz <= EPSILON) {
        vertexCount = 4;
        corners[1] = _mm_add_ps(corners[0], deltaX);
        corners[2] = _mm_add_ps(corners[0], deltaY);
        corners[3] = _mm_add_ps(corners[1], deltaY);
    } else if (dx <= EPSILON) {
        vertexCount = 4;
        corners[1] = _mm_add_ps(corners[0], deltaZ);
        corners[2] = _mm_add_ps(corners[0], deltaY);
        corners[3] = _mm_add_ps(corners[1], deltaY);
    } else if (dy <= EPSILON) {
        vertexCount = 4;
        corners[1] = _mm_add_ps(corners[0], deltaZ);
        corners[2] = _mm_add_ps(corners[0], deltaX);
        corners[3] = _mm_add_ps(corners[1], deltaX);
    } else {
        vertexCount = 8;
        corners[1] = _mm_add_ps(corners[0], deltaZ);
        corners[2] = _mm_add_ps(corners[0], deltaY);
        corners[3] = _mm_add_ps(corners[1], deltaY);
        corners[4] = _mm_add_ps(corners[0], deltaX);
        corners[5] = _mm_add_ps(corners[4], deltaZ);
        corners[6] = _mm_add_ps(corners[4], deltaY);
        corners[7] = _mm_add_ps(corners[5], deltaY);
    }

    for (int i = 0; i < vertexCount; ++i) {
        const __m128 v = corners[i];
        const float w = _mm_cvtss_f32(_mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3)));
        const float z = _mm_cvtss_f32(_mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)));

        float projZ = 0.0f;
        if (w >= 0.0f)
            projZ = z / w;

        if (projZ < *zhMin) *zhMin = projZ;
        if (projZ > *zhMax) *zhMax = projZ;
    }
}

#endif // VX_SIMD_SSE
