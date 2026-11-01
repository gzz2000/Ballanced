/**
 * @file VxRect.inl
 * @brief Inline implementations for VxRect.
 *
 * Part 1: Scalar inline bodies (moved from VxRect.h).
 * Part 2: SIMD-accelerated rect operations.
 *
 * Included automatically at the bottom of VxRect.h - do not include directly.
 */

#pragma once

#if defined(VX_SIMD_SSE)
VX_SIMD_INLINE void VxSIMDRectTransform(VxRect *rect, const VxRect *destScreen, const VxRect *srcScreen) noexcept;
VX_SIMD_INLINE void VxSIMDRectTransformBySize(VxRect *rect, const Vx2DVector *destScreenSize, const Vx2DVector *srcScreenSize) noexcept;
VX_SIMD_INLINE void VxSIMDRectTransformToHomogeneous(VxRect *rect, const VxRect *screen) noexcept;
VX_SIMD_INLINE void VxSIMDRectTransformFromHomogeneous(VxRect *rect, const VxRect *screen) noexcept;
#endif

// =============================================================================
// Part 1 - Scalar Inline Implementations
// =============================================================================

inline VxRect::VxRect() {}

#if !defined(_MSC_VER)
inline VxRect::VxRect(Vx2DVector &topleft, Vx2DVector &bottomright) : left(topleft.x), top(topleft.y), right(bottomright.x), bottom(bottomright.y) {}
inline VxRect::VxRect(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}
#else
inline VxRect::VxRect(Vx2DVector &topleft, Vx2DVector &bottomright) : m_TopLeft(topleft), m_BottomRight(bottomright) {}
inline VxRect::VxRect(float l, float t, float r, float b) : m_TopLeft(l, t), m_BottomRight(r, b) {}
#endif

inline void VxRect::SetWidth(float w) { right = left + w; }
inline float VxRect::GetWidth() const { return right - left; }
inline void VxRect::SetHeight(float h) { bottom = top + h; }
inline float VxRect::GetHeight() const { return bottom - top; }
inline float VxRect::GetHCenter() const { return left + 0.5f * GetWidth(); }
inline float VxRect::GetVCenter() const { return top + 0.5f * GetHeight(); }

inline void VxRect::SetSize(const Vx2DVector &v) {
    SetWidth(v.x);
    SetHeight(v.y);
}

inline Vx2DVector VxRect::GetSize() const { return Vx2DVector(GetWidth(), GetHeight()); }

inline void VxRect::SetHalfSize(const Vx2DVector &v) {
    Vx2DVector c = GetCenter();
    SetCenter(c, v);
}

inline Vx2DVector VxRect::GetHalfSize() const { return Vx2DVector(0.5f * GetWidth(), 0.5f * GetHeight()); }

inline void VxRect::SetCenter(const Vx2DVector &v) {
    Vx2DVector hs = GetHalfSize();
    SetCenter(v, hs);
}

inline Vx2DVector VxRect::GetCenter() const { return Vx2DVector(GetHCenter(), GetVCenter()); }

inline void VxRect::SetTopLeft(const Vx2DVector &v) {
    left = v.x;
    top = v.y;
}

#if defined(_MSC_VER)
inline const Vx2DVector &VxRect::GetTopLeft() const { return m_TopLeft; }
inline Vx2DVector &VxRect::GetTopLeft() { return m_TopLeft; }
#else
inline const Vx2DVector &VxRect::GetTopLeft() const { return (const Vx2DVector &)*(Vx2DVector *)&left; }
inline Vx2DVector &VxRect::GetTopLeft() { return (Vx2DVector &)*(Vx2DVector *)&left; }
#endif

inline void VxRect::SetBottomRight(const Vx2DVector &v) {
    right = v.x;
    bottom = v.y;
}

#if defined(_MSC_VER)
inline const Vx2DVector &VxRect::GetBottomRight() const { return m_BottomRight; }
inline Vx2DVector &VxRect::GetBottomRight() { return m_BottomRight; }
#else
inline const Vx2DVector &VxRect::GetBottomRight() const { return (const Vx2DVector &)*(Vx2DVector *)&right; }
inline Vx2DVector &VxRect::GetBottomRight() { return (Vx2DVector &)*(Vx2DVector *)&right; }
#endif

inline void VxRect::Clear() { SetCorners(0, 0, 0, 0); }

inline void VxRect::SetCorners(const Vx2DVector &topleft, const Vx2DVector &bottomright) {
    left = topleft.x;
    top = topleft.y;
    right = bottomright.x;
    bottom = bottomright.y;
}

inline void VxRect::SetCorners(float l, float t, float r, float b) {
    left = l;
    top = t;
    right = r;
    bottom = b;
}

inline void VxRect::SetDimension(const Vx2DVector &position, const Vx2DVector &size) {
    left = position.x;
    top = position.y;
    right = left + size.x;
    bottom = top + size.y;
}

inline void VxRect::SetDimension(float x, float y, float w, float h) {
    left = x;
    top = y;
    right = x + w;
    bottom = y + h;
}

inline void VxRect::SetCenter(const Vx2DVector &center, const Vx2DVector &halfsize) {
    left = center.x - halfsize.x;
    top = center.y - halfsize.y;
    right = center.x + halfsize.x;
    bottom = center.y + halfsize.y;
}

inline void VxRect::SetCenter(float cx, float cy, float hw, float hh) {
    left = cx - hw;
    top = cy - hh;
    right = cx + hw;
    bottom = cy + hh;
}

inline void VxRect::CopyFrom(const CKRECT &iRect) {
    left = (float) iRect.left;
    top = (float) iRect.top;
    right = (float) iRect.right;
    bottom = (float) iRect.bottom;
}

inline void VxRect::CopyTo(CKRECT *oRect) const {
    XASSERT(oRect);
    oRect->left = (int) left;
    oRect->top = (int) top;
    oRect->right = (int) right;
    oRect->bottom = (int) bottom;
}

inline void VxRect::Bounding(const Vx2DVector &p1, const Vx2DVector &p2) {
    if (p1.x < p2.x) { left = p1.x; right = p2.x; } else { left = p2.x; right = p1.x; }
    if (p1.y < p2.y) { top = p1.y; bottom = p2.y; } else { top = p2.y; bottom = p1.y; }
}

inline void VxRect::Normalize() {
    if (left > right) XSwap(right, left);
    if (top > bottom) XSwap(top, bottom);
}

inline void VxRect::Move(const Vx2DVector &pos) {
    right += (pos.x - left);
    bottom += (pos.y - top);
    left = pos.x;
    top = pos.y;
}

inline void VxRect::Translate(const Vx2DVector &t) {
    left += t.x;
    right += t.x;
    top += t.y;
    bottom += t.y;
}

inline void VxRect::HMove(float h) {
    right += (h - left);
    left = h;
}

inline void VxRect::VMove(float v) {
    bottom += (v - top);
    top = v;
}

inline void VxRect::HTranslate(float h) {
    right += h;
    left += h;
}

inline void VxRect::VTranslate(float v) {
    bottom += v;
    top += v;
}

inline void VxRect::TransformFromHomogeneous(Vx2DVector &dest, const Vx2DVector &srchom) const {
    dest.x = left + GetWidth() * srchom.x;
    dest.y = top + GetHeight() * srchom.y;
}

inline void VxRect::Scale(const Vx2DVector &s) {
    SetWidth(s.x * GetWidth());
    SetHeight(s.y * GetHeight());
}

inline void VxRect::Inflate(const Vx2DVector &pt) {
    left -= pt.x;
    right += pt.x;
    top -= pt.y;
    bottom += pt.y;
}

inline void VxRect::Interpolate(float value, const VxRect &a) {
    left += (a.left - left) * value;
    right += (a.right - right) * value;
    top += (a.top - top) * value;
    bottom += (a.bottom - bottom) * value;
}

inline void VxRect::Merge(const VxRect &a) {
    if (a.left < left) left = a.left;
    if (a.right > right) right = a.right;
    if (a.top < top) top = a.top;
    if (a.bottom > bottom) bottom = a.bottom;
}

inline int VxRect::IsInside(const VxRect &cliprect) const {
    if (left >= cliprect.right) return ALLOUTSIDE;
    if (right < cliprect.left) return ALLOUTSIDE;
    if (top >= cliprect.bottom) return ALLOUTSIDE;
    if (bottom < cliprect.top) return ALLOUTSIDE;
    if (left < cliprect.left) return PARTINSIDE;
    if (right > cliprect.right) return PARTINSIDE;
    if (top < cliprect.top) return PARTINSIDE;
    if (bottom > cliprect.bottom) return PARTINSIDE;
    return ALLINSIDE;
}

inline XBOOL VxRect::IsOutside(const VxRect &cliprect) const {
    if (left >= cliprect.right) return TRUE;
    if (right < cliprect.left) return TRUE;
    if (top >= cliprect.bottom) return TRUE;
    if (bottom < cliprect.top) return TRUE;
    return FALSE;
}

inline XBOOL VxRect::IsInside(const Vx2DVector &pt) const {
    if (pt.x < left) return FALSE;
    if (pt.x > right) return FALSE;
    if (pt.y < top) return FALSE;
    if (pt.y > bottom) return FALSE;
    return TRUE;
}

inline XBOOL VxRect::IsNull() const { return (left == 0 && right == 0 && bottom == 0 && top == 0); }
inline XBOOL VxRect::IsEmpty() const { return ((left == right) || (bottom == top)); }

inline XBOOL VxRect::Clip(const VxRect &cliprect) {
    if (IsOutside(cliprect)) return FALSE;
    if (left < cliprect.left) left = cliprect.left;
    if (right > cliprect.right) right = cliprect.right;
    if (top < cliprect.top) top = cliprect.top;
    if (bottom > cliprect.bottom) bottom = cliprect.bottom;
    return TRUE;
}

inline void VxRect::Clip(Vx2DVector &pt, XBOOL excluderightbottom) const {
    if (pt.x < left) pt.x = left;
    else if (pt.x >= right) {
        if (excluderightbottom) pt.x = right - 1;
        else pt.x = right;
    }
    if (pt.y < top) pt.y = top;
    else if (pt.y >= bottom) {
        if (excluderightbottom) pt.y = bottom - 1;
        else pt.y = bottom;
    }
}

inline void VxRect::Transform(const VxRect &destScreen, const VxRect &srcScreen) {
#if defined(VX_SIMD_SSE)
    VxSIMDRectTransform(this, &destScreen, &srcScreen);
#else
    float srcInvW = 1.0f / (srcScreen.right - srcScreen.left);
    float srcInvH = 1.0f / (srcScreen.bottom - srcScreen.top);
    VxRect r((left - srcScreen.left) * srcInvW,
             (top - srcScreen.top) * srcInvH,
             (right - srcScreen.right) * srcInvW,
             (bottom - srcScreen.bottom) * srcInvH);
    float dstW = destScreen.right - destScreen.left;
    float dstH = destScreen.bottom - destScreen.top;
    left = r.left * dstW + destScreen.left;
    top = r.top * dstH + destScreen.top;
    right = r.right * dstW + destScreen.right;
    bottom = r.bottom * dstH + destScreen.bottom;
#endif
}

inline void VxRect::Transform(const Vx2DVector &destScreenSize, const Vx2DVector &srcScreenSize) {
#if defined(VX_SIMD_SSE)
    VxSIMDRectTransformBySize(this, &destScreenSize, &srcScreenSize);
#else
    float invW = 1.0f / srcScreenSize.x;
    float invH = 1.0f / srcScreenSize.y;
    left *= invW * destScreenSize.x;
    top *= invH * destScreenSize.y;
    right *= invW * destScreenSize.x;
    bottom *= invH * destScreenSize.y;
#endif
}

inline void VxRect::TransformToHomogeneous(const VxRect &screen) {
#if defined(VX_SIMD_SSE)
    VxSIMDRectTransformToHomogeneous(this, &screen);
#else
    float invW = 1.0f / (screen.right - screen.left);
    float invH = 1.0f / (screen.bottom - screen.top);
    float w = right - left;
    float h = bottom - top;
    left = (left - screen.left) * invW;
    top = (top - screen.top) * invH;
    right = left + w * invW;
    bottom = top + h * invH;
#endif
}

inline void VxRect::TransformFromHomogeneous(const VxRect &screen) {
#if defined(VX_SIMD_SSE)
    VxSIMDRectTransformFromHomogeneous(this, &screen);
#else
    float sW = screen.right - screen.left;
    float sH = screen.bottom - screen.top;
    float w = right - left;
    float h = bottom - top;
    left = screen.left + left * sW;
    top = screen.top + top * sH;
    right = left + w * sW;
    bottom = top + h * sH;
#endif
}

inline VxRect &VxRect::operator+=(const Vx2DVector &t) {
    left += t.x;
    right += t.x;
    top += t.y;
    bottom += t.y;
    return *this;
}

inline VxRect &VxRect::operator-=(const Vx2DVector &t) {
    left -= t.x;
    right -= t.x;
    top -= t.y;
    bottom -= t.y;
    return *this;
}

inline VxRect &VxRect::operator*=(const Vx2DVector &t) {
    left *= t.x;
    right *= t.x;
    top *= t.y;
    bottom *= t.y;
    return *this;
}

inline VxRect &VxRect::operator/=(const Vx2DVector &t) {
    float x = 1.0f / t.x;
    float y = 1.0f / t.y;
    left *= x;
    right *= x;
    top *= y;
    bottom *= y;
    return *this;
}

inline int operator==(const VxRect &r1, const VxRect &r2) {
    return (r1.left == r2.left && r1.right == r2.right && r1.top == r2.top && r1.bottom == r2.bottom);
}

inline int operator!=(const VxRect &r1, const VxRect &r2) {
    return !(r1 == r2);
}

// =============================================================================
// Part 2 - SIMD-Accelerated Rect Operations
// =============================================================================

#if defined(VX_SIMD_SSE)

#include "VxSIMD.h"

VX_SIMD_INLINE void VxSIMDRectTransform(VxRect *rect, const VxRect *destScreen, const VxRect *srcScreen) noexcept {
    const __m128 r = _mm_loadu_ps(&rect->left);
    const __m128 s = _mm_loadu_ps(&srcScreen->left);
    const __m128 d = _mm_loadu_ps(&destScreen->left);

    const __m128 s_right = _mm_shuffle_ps(s, s, _MM_SHUFFLE(2, 2, 2, 2));
    const __m128 s_left = _mm_shuffle_ps(s, s, _MM_SHUFFLE(0, 0, 0, 0));
    const __m128 s_bottom = _mm_shuffle_ps(s, s, _MM_SHUFFLE(3, 3, 3, 3));
    const __m128 s_top = _mm_shuffle_ps(s, s, _MM_SHUFFLE(1, 1, 1, 1));
    const __m128 w = _mm_sub_ps(s_right, s_left);
    const __m128 h = _mm_sub_ps(s_bottom, s_top);
    const __m128 srcSize = _mm_unpacklo_ps(w, h);
    const __m128 srcInvSize = _mm_div_ps(_mm_set1_ps(1.0f), srcSize);

    const __m128 normalized = _mm_mul_ps(_mm_sub_ps(r, s), srcInvSize);

    const __m128 d_right = _mm_shuffle_ps(d, d, _MM_SHUFFLE(2, 2, 2, 2));
    const __m128 d_left = _mm_shuffle_ps(d, d, _MM_SHUFFLE(0, 0, 0, 0));
    const __m128 d_bottom = _mm_shuffle_ps(d, d, _MM_SHUFFLE(3, 3, 3, 3));
    const __m128 d_top = _mm_shuffle_ps(d, d, _MM_SHUFFLE(1, 1, 1, 1));
    const __m128 dw = _mm_sub_ps(d_right, d_left);
    const __m128 dh = _mm_sub_ps(d_bottom, d_top);
    const __m128 destSize = _mm_unpacklo_ps(dw, dh);

    const __m128 result = _mm_add_ps(_mm_mul_ps(normalized, destSize), d);
    _mm_storeu_ps(&rect->left, result);
}

VX_SIMD_INLINE void VxSIMDRectTransformBySize(VxRect *rect, const Vx2DVector *destScreenSize, const Vx2DVector *srcScreenSize) noexcept {
    __m128 rectVec = _mm_loadu_ps(&rect->left);

    __m128 srcInvSize = _mm_setr_ps(
        1.0f / srcScreenSize->x,
        1.0f / srcScreenSize->y,
        1.0f / srcScreenSize->x,
        1.0f / srcScreenSize->y
    );

    __m128 destSize = _mm_setr_ps(
        destScreenSize->x,
        destScreenSize->y,
        destScreenSize->x,
        destScreenSize->y
    );

    __m128 result = _mm_mul_ps(_mm_mul_ps(rectVec, srcInvSize), destSize);
    _mm_storeu_ps(&rect->left, result);
}

VX_SIMD_INLINE void VxSIMDRectTransformToHomogeneous(VxRect *rect, const VxRect *screen) noexcept {
    float width = rect->right - rect->left;
    float height = rect->bottom - rect->top;

    __m128 rectVec = _mm_loadu_ps(&rect->left);
    __m128 screenVec = _mm_loadu_ps(&screen->left);

    __m128 screenRightBottom = _mm_shuffle_ps(screenVec, screenVec, _MM_SHUFFLE(3, 2, 3, 2));
    __m128 screenSize = _mm_sub_ps(screenRightBottom, screenVec);
    __m128 screenInvSize = _mm_div_ps(_mm_set1_ps(1.0f), screenSize);

    __m128 minTransformed = _mm_mul_ps(_mm_sub_ps(rectVec, screenVec), screenInvSize);

    float leftNorm, topNorm;
    _mm_store_ss(&leftNorm, minTransformed);
    _mm_store_ss(&topNorm, _mm_shuffle_ps(minTransformed, minTransformed, _MM_SHUFFLE(1, 1, 1, 1)));

    float screenInvWidth, screenInvHeight;
    _mm_store_ss(&screenInvWidth, screenInvSize);
    _mm_store_ss(&screenInvHeight, _mm_shuffle_ps(screenInvSize, screenInvSize, _MM_SHUFFLE(1, 1, 1, 1)));

    rect->left = leftNorm;
    rect->top = topNorm;
    rect->right = leftNorm + width * screenInvWidth;
    rect->bottom = topNorm + height * screenInvHeight;
}

VX_SIMD_INLINE void VxSIMDRectTransformFromHomogeneous(VxRect *rect, const VxRect *screen) noexcept {
    float width = rect->right - rect->left;
    float height = rect->bottom - rect->top;

    __m128 rectVec = _mm_loadu_ps(&rect->left);
    __m128 screenVec = _mm_loadu_ps(&screen->left);

    __m128 screenRightBottom = _mm_shuffle_ps(screenVec, screenVec, _MM_SHUFFLE(3, 2, 3, 2));
    __m128 screenSize = _mm_sub_ps(screenRightBottom, screenVec);

    __m128 minTransformed = _mm_add_ps(screenVec, _mm_mul_ps(rectVec, screenSize));

    float leftTrans, topTrans;
    _mm_store_ss(&leftTrans, minTransformed);
    _mm_store_ss(&topTrans, _mm_shuffle_ps(minTransformed, minTransformed, _MM_SHUFFLE(1, 1, 1, 1)));

    float screenWidth, screenHeight;
    _mm_store_ss(&screenWidth, screenSize);
    _mm_store_ss(&screenHeight, _mm_shuffle_ps(screenSize, screenSize, _MM_SHUFFLE(1, 1, 1, 1)));

    rect->left = leftTrans;
    rect->top = topTrans;
    rect->right = leftTrans + width * screenWidth;
    rect->bottom = topTrans + height * screenHeight;
}

#endif // VX_SIMD_SSE
