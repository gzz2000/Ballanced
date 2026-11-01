/**
 * @file VxBbox.inl
 * @brief Inline implementations for VxBbox.
 *
 * Part 1: Scalar inline bodies (moved from VxBbox.h).
 * Part 2: SIMD-accelerated bounding-box operations.
 *
 * This file uses the same VXBBOX_IMPL include guard as the original
 * VxBbox.h layout to prevent double-definition.
 *
 * Included automatically at the bottom of VxBbox.h - do not include directly.
 */

#pragma once

// =============================================================================
// Part 1 - Scalar Inline Implementations
// =============================================================================

#include "VxMatrix.h"
#include "VxSIMD.h"

#if defined(VX_SIMD_SSE)
VX_SIMD_INLINE int VxSIMDBboxClassify(const VxBbox *self, const VxBbox *other, const VxVector *point) noexcept;
VX_SIMD_INLINE void VxSIMDBboxClassifyVertices(const VxBbox *self, int count, const XBYTE *vertices, XDWORD stride, XDWORD *flags) noexcept;
VX_SIMD_INLINE void VxSIMDBboxTransformTo(const VxBbox *self, VxVector *points, const VxMatrix *mat) noexcept;
VX_SIMD_INLINE void VxSIMDBboxTransformFrom(VxBbox *dest, const VxBbox *src, const VxMatrix *mat) noexcept;
#endif

inline VxBbox::VxBbox() : Max(-1e6f, -1e6f, -1e6f), Min(1e6f, 1e6f, 1e6f) {}

inline VxBbox::VxBbox(const VxVector &iMin, const VxVector &iMax) : Max(iMax), Min(iMin) {}

inline VxBbox::VxBbox(float value) {
    Max.x = value;
    Max.y = value;
    Max.z = value;
    Min.x = -value;
    Min.y = -value;
    Min.z = -value;
}

inline XBOOL VxBbox::IsValid() const {
#if defined(VX_SIMD_SSE)
    __m128 minV = VxSIMDLoadFloat3(&Min.x);
    __m128 maxV = VxSIMDLoadFloat3(&Max.x);
    __m128 cmp = _mm_cmple_ps(minV, maxV);
    return ((_mm_movemask_ps(cmp) & 0x7) == 0x7) ? TRUE : FALSE;
#else
    if (Min.x > Max.x) return FALSE;
    if (Min.y > Max.y) return FALSE;
    if (Min.z > Max.z) return FALSE;
    return TRUE;
#endif
}

inline VxVector VxBbox::GetSize() const {
    return Max - Min;
}

inline VxVector VxBbox::GetHalfSize() const {
    return (Max - Min) * 0.5f;
}

inline VxVector VxBbox::GetCenter() const {
    return (Max + Min) * 0.5f;
}

inline void VxBbox::SetCorners(const VxVector &min, const VxVector &max) {
    Min = min;
    Max = max;
}

inline void VxBbox::SetDimension(const VxVector &position, const VxVector &size) {
    Min = position;
    Max = position + size;
}

inline void VxBbox::SetCenter(const VxVector &center, const VxVector &halfsize) {
    Min = center - halfsize;
    Max = center + halfsize;
}

inline void VxBbox::Reset() {
    Max.x = -1e6f;
    Max.y = -1e6f;
    Max.z = -1e6f;
    Min.x = 1e6f;
    Min.y = 1e6f;
    Min.z = 1e6f;
}

inline void VxBbox::Merge(const VxBbox &v) {
#if defined(VX_SIMD_SSE)
    __m128 selfMax = VxSIMDLoadFloat3(&Max.x);
    __m128 selfMin = VxSIMDLoadFloat3(&Min.x);
    __m128 otherMax = VxSIMDLoadFloat3(&v.Max.x);
    __m128 otherMin = VxSIMDLoadFloat3(&v.Min.x);
    VxSIMDStoreFloat3(&Max.x, _mm_max_ps(selfMax, otherMax));
    VxSIMDStoreFloat3(&Min.x, _mm_min_ps(selfMin, otherMin));
#else
    Max.x = XMax(v.Max.x, Max.x);
    Max.y = XMax(v.Max.y, Max.y);
    Max.z = XMax(v.Max.z, Max.z);
    Min.x = XMin(v.Min.x, Min.x);
    Min.y = XMin(v.Min.y, Min.y);
    Min.z = XMin(v.Min.z, Min.z);
#endif
}

inline void VxBbox::Merge(const VxVector &v) {
#if defined(VX_SIMD_SSE)
    __m128 selfMax = VxSIMDLoadFloat3(&Max.x);
    __m128 selfMin = VxSIMDLoadFloat3(&Min.x);
    __m128 point = VxSIMDLoadFloat3(&v.x);
    VxSIMDStoreFloat3(&Max.x, _mm_max_ps(selfMax, point));
    VxSIMDStoreFloat3(&Min.x, _mm_min_ps(selfMin, point));
#else
    if (v.x > Max.x) Max.x = v.x;
    if (v.x < Min.x) Min.x = v.x;
    if (v.y > Max.y) Max.y = v.y;
    if (v.y < Min.y) Min.y = v.y;
    if (v.z > Max.z) Max.z = v.z;
    if (v.z < Min.z) Min.z = v.z;
#endif
}

inline XDWORD VxBbox::Classify(const VxVector &iPoint) const {
#if defined(VX_SIMD_SSE)
    __m128 minV = VxSIMDLoadFloat3(&Min.x);
    __m128 maxV = VxSIMDLoadFloat3(&Max.x);
    __m128 p = VxSIMDLoadFloat3(&iPoint.x);
    int less = _mm_movemask_ps(_mm_cmplt_ps(p, minV));
    int greater = _mm_movemask_ps(_mm_cmpgt_ps(p, maxV));
    XDWORD flag = 0;
    if (less & 1) flag |= VXCLIP_LEFT;
    else if (greater & 1) flag |= VXCLIP_RIGHT;
    if (less & 2) flag |= VXCLIP_BOTTOM;
    else if (greater & 2) flag |= VXCLIP_TOP;
    if (less & 4) flag |= VXCLIP_BACK;
    else if (greater & 4) flag |= VXCLIP_FRONT;
    return flag;
#else
    XDWORD flag = 0;
    if (iPoint.x < Min.x) flag |= VXCLIP_LEFT;
    else if (iPoint.x > Max.x) flag |= VXCLIP_RIGHT;
    if (iPoint.y < Min.y) flag |= VXCLIP_BOTTOM;
    else if (iPoint.y > Max.y) flag |= VXCLIP_TOP;
    if (iPoint.z < Min.z) flag |= VXCLIP_BACK;
    else if (iPoint.z > Max.z) flag |= VXCLIP_FRONT;
    return flag;
#endif
}

inline XDWORD VxBbox::Classify(const VxBbox &iBox) const {
#if defined(VX_SIMD_SSE)
    __m128 selfMin = VxSIMDLoadFloat3(&Min.x);
    __m128 selfMax = VxSIMDLoadFloat3(&Max.x);
    __m128 otherMin = VxSIMDLoadFloat3(&iBox.Min.x);
    __m128 otherMax = VxSIMDLoadFloat3(&iBox.Max.x);
    int maxLtMin = _mm_movemask_ps(_mm_cmplt_ps(otherMax, selfMin));
    int minGtMax = _mm_movemask_ps(_mm_cmpgt_ps(otherMin, selfMax));
    XDWORD flag = 0;
    if (maxLtMin & 4) flag |= VXCLIP_BACK;
    else if (minGtMax & 4) flag |= VXCLIP_FRONT;
    if (maxLtMin & 1) flag |= VXCLIP_LEFT;
    else if (minGtMax & 1) flag |= VXCLIP_RIGHT;
    if (maxLtMin & 2) flag |= VXCLIP_BOTTOM;
    else if (minGtMax & 2) flag |= VXCLIP_TOP;
    return flag;
#else
    XDWORD flag = 0;
    if (iBox.Max.z < Min.z) flag |= VXCLIP_BACK;
    else if (iBox.Min.z > Max.z) flag |= VXCLIP_FRONT;
    if (iBox.Max.x < Min.x) flag |= VXCLIP_LEFT;
    else if (iBox.Min.x > Max.x) flag |= VXCLIP_RIGHT;
    if (iBox.Max.y < Min.y) flag |= VXCLIP_BOTTOM;
    else if (iBox.Min.y > Max.y) flag |= VXCLIP_TOP;
    return flag;
#endif
}

inline int VxBbox::Classify(const VxBbox &box2, const VxVector &pt) const {
#if defined(VX_SIMD_SSE)
    return VxSIMDBboxClassify(this, &box2, &pt);
#else
    XDWORD ptFlags = 0;
    if (pt.x < Min.x) ptFlags |= VXCLIP_LEFT; else if (pt.x > Max.x) ptFlags |= VXCLIP_RIGHT;
    if (pt.y < Min.y) ptFlags |= VXCLIP_BOTTOM; else if (pt.y > Max.y) ptFlags |= VXCLIP_TOP;
    if (pt.z < Min.z) ptFlags |= VXCLIP_BACK; else if (pt.z > Max.z) ptFlags |= VXCLIP_FRONT;
    XDWORD box2Flags = 0;
    if (box2.Max.z < Min.z) box2Flags |= VXCLIP_BACK; else if (box2.Min.z > Max.z) box2Flags |= VXCLIP_FRONT;
    if (box2.Max.x < Min.x) box2Flags |= VXCLIP_LEFT; else if (box2.Min.x > Max.x) box2Flags |= VXCLIP_RIGHT;
    if (box2.Max.y < Min.y) box2Flags |= VXCLIP_BOTTOM; else if (box2.Min.y > Max.y) box2Flags |= VXCLIP_TOP;
    if (ptFlags) {
        if (!box2Flags) {
            if (box2.Min.x>=Min.x && box2.Min.y>=Min.y && box2.Min.z>=Min.z &&
                box2.Max.x<=Max.x && box2.Max.y<=Max.y && box2.Max.z<=Max.z) return -1;
            if (Min.x>=box2.Min.x && Min.y>=box2.Min.y && Min.z>=box2.Min.z &&
                Max.x<=box2.Max.x && Max.y<=box2.Max.y && Max.z<=box2.Max.z) {
                return box2.VectorIn(pt) ? -1 : 1;
            }
        }
    } else {
        if (box2Flags) return -1;
        if (Min.x>=box2.Min.x && Min.y>=box2.Min.y && Min.z>=box2.Min.z &&
            Max.x<=box2.Max.x && Max.y<=box2.Max.y && Max.z<=box2.Max.z) return 1;
    }
    return 0;
#endif
}

inline void VxBbox::ClassifyVertices(const int iVcount, XBYTE *iVertices, XDWORD iStride, XDWORD *oFlags) const {
#if defined(VX_SIMD_SSE)
    VxSIMDBboxClassifyVertices(this, iVcount, iVertices, iStride, oFlags);
#else
    const float maxX=Max.x, maxY=Max.y, maxZ=Max.z, minX=Min.x, minY=Min.y, minZ=Min.z;
    for (int i = 0; i < iVcount; i++) {
        const float *v = reinterpret_cast<const float *>(iVertices + i*iStride);
        XDWORD flag = 0;
        if (v[2] < minZ) flag |= VXCLIP_BACK; else if (v[2] > maxZ) flag |= VXCLIP_FRONT;
        if (v[1] < minY) flag |= VXCLIP_BOTTOM; else if (v[1] > maxY) flag |= VXCLIP_TOP;
        if (v[0] < minX) flag |= VXCLIP_LEFT; else if (v[0] > maxX) flag |= VXCLIP_RIGHT;
        oFlags[i] = flag;
    }
#endif
}

inline void VxBbox::ClassifyVerticesOneAxis(const int iVcount, XBYTE *iVertices, XDWORD iStride, const int iAxis, XDWORD *oFlags) const {
    if (iAxis < 0 || iAxis > 2) { for (int i=0;i<iVcount;i++) oFlags[i]=0; return; }
    const float maxVal = *(&Max.x + iAxis), minVal = *(&Min.x + iAxis);
    for (int i = 0; i < iVcount; i++) {
        const float *v = reinterpret_cast<const float *>(iVertices + i*iStride + iAxis*sizeof(float));
        XDWORD flag = 0;
        if (*v < minVal) flag = 1; else if (*v > maxVal) flag = 2;
        oFlags[i] = flag;
    }
}

inline void VxBbox::Intersect(const VxBbox &v) {
#if defined(VX_SIMD_SSE)
    __m128 selfMax = VxSIMDLoadFloat3(&Max.x);
    __m128 selfMin = VxSIMDLoadFloat3(&Min.x);
    __m128 otherMax = VxSIMDLoadFloat3(&v.Max.x);
    __m128 otherMin = VxSIMDLoadFloat3(&v.Min.x);
    VxSIMDStoreFloat3(&Max.x, _mm_min_ps(selfMax, otherMax));
    VxSIMDStoreFloat3(&Min.x, _mm_max_ps(selfMin, otherMin));
#else
    Max.x = XMin(v.Max.x, Max.x);
    Max.y = XMin(v.Max.y, Max.y);
    Max.z = XMin(v.Max.z, Max.z);
    Min.x = XMax(v.Min.x, Min.x);
    Min.y = XMax(v.Min.y, Min.y);
    Min.z = XMax(v.Min.z, Min.z);
#endif
}

inline XBOOL VxBbox::VectorIn(const VxVector &v) const {
#if defined(VX_SIMD_SSE)
    __m128 p = VxSIMDLoadFloat3(&v.x);
    __m128 minV = VxSIMDLoadFloat3(&Min.x);
    __m128 maxV = VxSIMDLoadFloat3(&Max.x);
    __m128 ge = _mm_cmpge_ps(p, minV);
    __m128 le = _mm_cmple_ps(p, maxV);
    __m128 inside = _mm_and_ps(ge, le);
    return ((_mm_movemask_ps(inside) & 0x7) == 0x7) ? TRUE : FALSE;
#else
    if (v.x < Min.x) return FALSE;
    if (v.x > Max.x) return FALSE;
    if (v.y < Min.y) return FALSE;
    if (v.y > Max.y) return FALSE;
    if (v.z < Min.z) return FALSE;
    if (v.z > Max.z) return FALSE;
    return TRUE;
#endif
}

inline XBOOL VxBbox::IsBoxInside(const VxBbox &b) const {
#if defined(VX_SIMD_SSE)
    __m128 bMin = VxSIMDLoadFloat3(&b.Min.x);
    __m128 bMax = VxSIMDLoadFloat3(&b.Max.x);
    __m128 selfMin = VxSIMDLoadFloat3(&Min.x);
    __m128 selfMax = VxSIMDLoadFloat3(&Max.x);
    __m128 ge = _mm_cmpge_ps(bMin, selfMin);
    __m128 le = _mm_cmple_ps(bMax, selfMax);
    __m128 inside = _mm_and_ps(ge, le);
    return ((_mm_movemask_ps(inside) & 0x7) == 0x7) ? TRUE : FALSE;
#else
    if (b.Min.x < Min.x) return 0;
    if (b.Min.y < Min.y) return 0;
    if (b.Min.z < Min.z) return 0;
    if (b.Max.x > Max.x) return 0;
    if (b.Max.y > Max.y) return 0;
    if (b.Max.z > Max.z) return 0;
    return 1;
#endif
}

inline bool VxBbox::operator==(const VxBbox &iBox) const {
    return (Max == iBox.Max) && (Min == iBox.Min);
}

inline void VxBbox::TransformTo(VxVector *pts, const VxMatrix &Mat) const {
#if defined(VX_SIMD_SSE)
    VxSIMDBboxTransformTo(this, pts, &Mat);
#else
    Vx3DMultiplyMatrixVector(&pts[0], Mat, &Min);
    const float sizeX=Max.x-Min.x, sizeY=Max.y-Min.y, sizeZ=Max.z-Min.z;
    const VxVector xVec(sizeX*Mat[0][0], sizeX*Mat[0][1], sizeX*Mat[0][2]);
    const VxVector yVec(sizeY*Mat[1][0], sizeY*Mat[1][1], sizeY*Mat[1][2]);
    const VxVector zVec(sizeZ*Mat[2][0], sizeZ*Mat[2][1], sizeZ*Mat[2][2]);
    pts[1] = pts[0] + zVec;
    pts[2] = pts[0] + yVec;
    pts[3] = pts[2] + zVec;
    pts[4] = pts[0] + xVec;
    pts[5] = pts[4] + zVec;
    pts[6] = pts[4] + yVec;
    pts[7] = pts[6] + zVec;
#endif
}

inline void VxBbox::TransformFrom(const VxBbox &sbox, const VxMatrix &Mat) {
#if defined(VX_SIMD_SSE)
    VxSIMDBboxTransformFrom(this, &sbox, &Mat);
#else
    VxVector center((sbox.Min.x+sbox.Max.x)*0.5f, (sbox.Min.y+sbox.Max.y)*0.5f, (sbox.Min.z+sbox.Max.z)*0.5f);
    Vx3DMultiplyMatrixVector(&Min, Mat, &center);
    const float sizeX=sbox.Max.x-sbox.Min.x, sizeY=sbox.Max.y-sbox.Min.y, sizeZ=sbox.Max.z-sbox.Min.z;
    const VxVector xVec(sizeX*Mat[0][0], sizeX*Mat[0][1], sizeX*Mat[0][2]);
    const VxVector yVec(sizeY*Mat[1][0], sizeY*Mat[1][1], sizeY*Mat[1][2]);
    const VxVector zVec(sizeZ*Mat[2][0], sizeZ*Mat[2][1], sizeZ*Mat[2][2]);
    const float halfX = (XAbs(xVec.x)+XAbs(yVec.x)+XAbs(zVec.x))*0.5f;
    const float halfY = (XAbs(xVec.y)+XAbs(yVec.y)+XAbs(zVec.y))*0.5f;
    const float halfZ = (XAbs(xVec.z)+XAbs(yVec.z)+XAbs(zVec.z))*0.5f;
    Max.x = Min.x + halfX; Max.y = Min.y + halfY; Max.z = Min.z + halfZ;
    Min.x = Min.x - halfX; Min.y = Min.y - halfY; Min.z = Min.z - halfZ;
#endif
}

// =============================================================================
// Part 2 - SIMD-Accelerated BBox Operations
// =============================================================================

#if defined(VX_SIMD_SSE)

#include "VxSIMD.h"

VX_SIMD_INLINE int VxSIMDBboxClassify(const VxBbox *self, const VxBbox *other, const VxVector *point) noexcept {
    __m128 selfMin = VxSIMDLoadFloat3(&self->Min.x);
    __m128 selfMax = VxSIMDLoadFloat3(&self->Max.x);
    __m128 ptVec = VxSIMDLoadFloat3(&point->x);
    __m128 otherMin = VxSIMDLoadFloat3(&other->Min.x);
    __m128 otherMax = VxSIMDLoadFloat3(&other->Max.x);

    XDWORD ptFlags = 0;
    __m128 ptLessThanMin = _mm_cmplt_ps(ptVec, selfMin);
    __m128 ptGreaterThanMax = _mm_cmpgt_ps(ptVec, selfMax);
    int ptLessFlags = _mm_movemask_ps(ptLessThanMin);
    int ptGreaterFlags = _mm_movemask_ps(ptGreaterThanMax);

    if (ptLessFlags & 1) ptFlags |= VXCLIP_LEFT;
    if (ptGreaterFlags & 1) ptFlags |= VXCLIP_RIGHT;
    if (ptLessFlags & 2) ptFlags |= VXCLIP_BOTTOM;
    if (ptGreaterFlags & 2) ptFlags |= VXCLIP_TOP;
    if (ptLessFlags & 4) ptFlags |= VXCLIP_BACK;
    if (ptGreaterFlags & 4) ptFlags |= VXCLIP_FRONT;

    XDWORD box2Flags = 0;
    __m128 otherMaxLessThanMin = _mm_cmplt_ps(otherMax, selfMin);
    __m128 otherMinGreaterThanMax = _mm_cmpgt_ps(otherMin, selfMax);
    int box2LessFlags = _mm_movemask_ps(otherMaxLessThanMin);
    int box2GreaterFlags = _mm_movemask_ps(otherMinGreaterThanMax);

    if (box2LessFlags & 4) box2Flags |= VXCLIP_BACK;
    if (box2GreaterFlags & 4) box2Flags |= VXCLIP_FRONT;
    if (box2LessFlags & 1) box2Flags |= VXCLIP_LEFT;
    if (box2GreaterFlags & 1) box2Flags |= VXCLIP_RIGHT;
    if (box2LessFlags & 2) box2Flags |= VXCLIP_BOTTOM;
    if (box2GreaterFlags & 2) box2Flags |= VXCLIP_TOP;

    if (ptFlags) {
        if (!box2Flags) {
            __m128 box2MinGE = _mm_cmpge_ps(otherMin, selfMin);
            __m128 box2MaxLE = _mm_cmple_ps(otherMax, selfMax);
            __m128 box2Inside = _mm_and_ps(box2MinGE, box2MaxLE);

            if ((_mm_movemask_ps(box2Inside) & 0x7) == 0x7) {
                return -1;
            }

            __m128 selfMinGE = _mm_cmpge_ps(selfMin, otherMin);
            __m128 selfMaxLE = _mm_cmple_ps(selfMax, otherMax);
            __m128 selfInside = _mm_and_ps(selfMinGE, selfMaxLE);

            if ((_mm_movemask_ps(selfInside) & 0x7) == 0x7) {
                __m128 ptLessThanOtherMin = _mm_cmplt_ps(ptVec, otherMin);
                __m128 ptGreaterThanOtherMax = _mm_cmpgt_ps(ptVec, otherMax);
                __m128 ptOutside = _mm_or_ps(ptLessThanOtherMin, ptGreaterThanOtherMax);
                return (_mm_movemask_ps(ptOutside) & 0x7) ? 1 : -1;
            }
        }
    } else {
        if (box2Flags) return -1;
        __m128 selfMinGE = _mm_cmpge_ps(selfMin, otherMin);
        __m128 selfMaxLE = _mm_cmple_ps(selfMax, otherMax);
        __m128 selfInside = _mm_and_ps(selfMinGE, selfMaxLE);
        if ((_mm_movemask_ps(selfInside) & 0x7) == 0x7) {
            return 1;
        }
    }

    return 0;
}

VX_SIMD_INLINE void VxSIMDBboxClassifyVertices(const VxBbox *self, int count, const XBYTE *vertices, XDWORD stride, XDWORD *flags) noexcept {
    __m128 bboxMin = VxSIMDLoadFloat3(&self->Min.x);
    __m128 bboxMax = VxSIMDLoadFloat3(&self->Max.x);

    for (int i = 0; i < count; ++i) {
        const float *v = reinterpret_cast<const float *>(vertices + i * stride);
        __m128 vertex = VxSIMDLoadFloat3(v);

        __m128 lessThanMin = _mm_cmplt_ps(vertex, bboxMin);
        __m128 greaterThanMax = _mm_cmpgt_ps(vertex, bboxMax);
        int lessFlags = _mm_movemask_ps(lessThanMin);
        int greaterFlags = _mm_movemask_ps(greaterThanMax);

        XDWORD flag = 0;
        if (lessFlags & 4) flag |= VXCLIP_BACK;
        else if (greaterFlags & 4) flag |= VXCLIP_FRONT;
        if (lessFlags & 2) flag |= VXCLIP_BOTTOM;
        else if (greaterFlags & 2) flag |= VXCLIP_TOP;
        if (lessFlags & 1) flag |= VXCLIP_LEFT;
        else if (greaterFlags & 1) flag |= VXCLIP_RIGHT;

        flags[i] = flag;
    }
}

VX_SIMD_INLINE void VxSIMDBboxTransformTo(const VxBbox *self, VxVector *points, const VxMatrix *mat) noexcept {
    VxSIMDMultiplyMatrixVector(&points[0], mat, &self->Min);

    const float sizeX = self->Max.x - self->Min.x;
    const float sizeY = self->Max.y - self->Min.y;
    const float sizeZ = self->Max.z - self->Min.z;

    const VxVector xVec(sizeX * (*mat)[0][0], sizeX * (*mat)[0][1], sizeX * (*mat)[0][2]);
    const VxVector yVec(sizeY * (*mat)[1][0], sizeY * (*mat)[1][1], sizeY * (*mat)[1][2]);
    const VxVector zVec(sizeZ * (*mat)[2][0], sizeZ * (*mat)[2][1], sizeZ * (*mat)[2][2]);

    points[1] = points[0] + zVec;
    points[2] = points[0] + yVec;
    points[3] = points[2] + zVec;
    points[4] = points[0] + xVec;
    points[5] = points[4] + zVec;
    points[6] = points[4] + yVec;
    points[7] = points[6] + zVec;
}

VX_SIMD_INLINE void VxSIMDBboxTransformFrom(VxBbox *dest, const VxBbox *src, const VxMatrix *mat) noexcept {
    VxVector center((src->Min.x + src->Max.x) * 0.5f,
                    (src->Min.y + src->Max.y) * 0.5f,
                    (src->Min.z + src->Max.z) * 0.5f);

    VxSIMDMultiplyMatrixVector(&dest->Min, mat, &center);

    const float sizeX = src->Max.x - src->Min.x;
    const float sizeY = src->Max.y - src->Min.y;
    const float sizeZ = src->Max.z - src->Min.z;

    const VxVector xVec(sizeX * (*mat)[0][0], sizeX * (*mat)[0][1], sizeX * (*mat)[0][2]);
    const VxVector yVec(sizeY * (*mat)[1][0], sizeY * (*mat)[1][1], sizeY * (*mat)[1][2]);
    const VxVector zVec(sizeZ * (*mat)[2][0], sizeZ * (*mat)[2][1], sizeZ * (*mat)[2][2]);

    const float halfX = (fabsf(xVec.x) + fabsf(yVec.x) + fabsf(zVec.x)) * 0.5f;
    const float halfY = (fabsf(xVec.y) + fabsf(yVec.y) + fabsf(zVec.y)) * 0.5f;
    const float halfZ = (fabsf(xVec.z) + fabsf(yVec.z) + fabsf(zVec.z)) * 0.5f;

    dest->Max.x = dest->Min.x + halfX;
    dest->Max.y = dest->Min.y + halfY;
    dest->Max.z = dest->Min.z + halfZ;

    dest->Min.x = dest->Min.x - halfX;
    dest->Min.y = dest->Min.y - halfY;
    dest->Min.z = dest->Min.z - halfZ;
}

#endif // VX_SIMD_SSE
