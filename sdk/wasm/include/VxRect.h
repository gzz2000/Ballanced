#ifndef VXRECT_H
#define VXRECT_H

#include "VxMathDefines.h"
#include "Vx2dVector.h"
#include "VxSIMD.h"

/**
 * @enum VXRECT_INTERSECTION
 * @brief Describes the result of an intersection test between two rectangles.
 */
typedef enum VXRECT_INTERSECTION {
    ALLOUTSIDE = 0, ///< The rectangle is completely outside the clipping rectangle.
    ALLINSIDE  = 1, ///< The rectangle is completely inside the clipping rectangle.
    PARTINSIDE = 2  ///< The rectangle is partially inside (intersecting) the clipping rectangle.
} VXRECT_INTERSECTION;

/**
 * @brief Represents a 2D rectangle defined by four float values.
 *
 * @remarks
 * A VxRect is defined by its left, top, right, and bottom coordinates.
 * It is used to represent a 2D region.
 *
 * The class is defined as:
 * @code
 * class VxRect {
 * public:
 *     float left;
 *     float top;
 *     float right;
 *     float bottom;
 * };
 * @endcode
 *
 * For convenience, especially under MSVC, a union allows accessing the corners
 * as Vx2DVector objects (`m_TopLeft`, `m_BottomRight`). Elements can be accessed
 * directly or through various accessor methods for more sophisticated operations.
 */
class VxRect {
public:
#if !defined(_MSC_VER)
    float left;   ///< The x-coordinate of the left edge.
    float top;    ///< The y-coordinate of the top edge.
    float right;  ///< The x-coordinate of the right edge.
    float bottom; ///< The y-coordinate of the bottom edge.
#else
    union {
        struct {
            float left;   ///< The x-coordinate of the left edge.
            float top;    ///< The y-coordinate of the top edge.
            float right;  ///< The x-coordinate of the right edge.
            float bottom; ///< The y-coordinate of the bottom edge.
        };
        struct {
            Vx2DVector m_TopLeft;       ///< The top-left corner as a 2D vector.
            Vx2DVector m_BottomRight;   ///< The bottom-right corner as a 2D vector.
        };
    };
#endif

    /** @name Constructors */
    ///@{
    /// @brief Default constructor. Coordinates are left uninitialized.
    VxRect();

#if !defined(_MSC_VER)
    /**
     * @brief Builds a rectangle from top-left and bottom-right corners.
     * @param topleft Top-left corner.
     * @param bottomright Bottom-right corner.
     */
    VxRect(Vx2DVector &topleft, Vx2DVector &bottomright);
    /**
     * @brief Builds a rectangle from explicit coordinates.
     * @param l Left coordinate.
     * @param t Top coordinate.
     * @param r Right coordinate.
     * @param b Bottom coordinate.
     */
    VxRect(float l, float t, float r, float b);
#else
    /**
     * @brief Builds a rectangle from top-left and bottom-right corners.
     * @param topleft Top-left corner.
     * @param bottomright Bottom-right corner.
     */
    VxRect(Vx2DVector &topleft, Vx2DVector &bottomright);
    /**
     * @brief Builds a rectangle from explicit coordinates.
     * @param l Left coordinate.
     * @param t Top coordinate.
     * @param r Right coordinate.
     * @param b Bottom coordinate.
     */
    VxRect(float l, float t, float r, float b);
#endif
    ///@}

    /** @name Size And Center */
    ///@{
    /// @brief Sets width by updating `right = left + w`.
    void SetWidth(float w);
    /// @brief Returns rectangle width (`right - left`).
    float GetWidth() const;
    /// @brief Sets height by updating `bottom = top + h`.
    void SetHeight(float h);
    /// @brief Returns rectangle height (`bottom - top`).
    float GetHeight() const;
    /// @brief Returns horizontal center coordinate.
    float GetHCenter() const;
    /// @brief Returns vertical center coordinate.
    float GetVCenter() const;
    /// @brief Sets size using width/height from a vector.
    void SetSize(const Vx2DVector &v);
    /// @brief Returns current size as `(width, height)`.
    Vx2DVector GetSize() const;
    /// @brief Sets half-size while preserving current center.
    void SetHalfSize(const Vx2DVector &v);
    /// @brief Returns half-size as `(width*0.5, height*0.5)`.
    Vx2DVector GetHalfSize() const;
    /// @brief Moves rectangle to a new center, preserving current size.
    void SetCenter(const Vx2DVector &v);
    /// @brief Returns current center point.
    Vx2DVector GetCenter() const;
    ///@}

    /** @name Corner Accessors */
    ///@{
    /// @brief Sets top-left corner coordinates.
    void SetTopLeft(const Vx2DVector &v);

#if defined(_MSC_VER)
    const Vx2DVector &GetTopLeft() const;
    Vx2DVector &GetTopLeft();
#else
    const Vx2DVector &GetTopLeft() const;
    Vx2DVector &GetTopLeft();
#endif

    /// @brief Sets bottom-right corner coordinates.
    void SetBottomRight(const Vx2DVector &v);

#if defined(_MSC_VER)
    const Vx2DVector &GetBottomRight() const;
    Vx2DVector &GetBottomRight();
#else
    const Vx2DVector &GetBottomRight() const;
    Vx2DVector &GetBottomRight();
#endif
    ///@}

    /** @name Geometry Setup */
    ///@{
    /// @brief Clears rectangle to all zeros.
    void Clear();
    /// @brief Sets corners from vector arguments.
    void SetCorners(const Vx2DVector &topleft, const Vx2DVector &bottomright);
    /// @brief Sets corners from scalar arguments.
    void SetCorners(float l, float t, float r, float b);
    /// @brief Sets from top-left position plus size.
    void SetDimension(const Vx2DVector &position, const Vx2DVector &size);
    /// @brief Sets from top-left position plus width/height.
    void SetDimension(float x, float y, float w, float h);
    /// @brief Sets from center and half-size.
    void SetCenter(const Vx2DVector &center, const Vx2DVector &halfsize);
    /// @brief Sets from center and half-size scalar components.
    void SetCenter(float cx, float cy, float hw, float hh);
    /// @brief Copies coordinates from an integer CK rectangle.
    void CopyFrom(const CKRECT &iRect);
    /// @brief Copies coordinates to an integer CK rectangle.
    void CopyTo(CKRECT *oRect) const;
    /// @brief Sets this rectangle to the bounding box of two points.
    void Bounding(const Vx2DVector &p1, const Vx2DVector &p2);
    /// @brief Swaps bounds if needed so `left<=right` and `top<=bottom`.
    void Normalize();
    ///@}

    /** @name Translation And Scaling */
    ///@{
    /// @brief Moves top-left corner to `pos`, preserving width/height.
    void Move(const Vx2DVector &pos);
    /// @brief Translates by vector offset.
    void Translate(const Vx2DVector &t);
    /// @brief Moves horizontally to a new left coordinate, preserving width.
    void HMove(float h);
    /// @brief Moves vertically to a new top coordinate, preserving height.
    void VMove(float v);
    /// @brief Horizontal translation.
    void HTranslate(float h);
    /// @brief Vertical translation.
    void VTranslate(float v);
    /**
     * @brief Maps a homogeneous point in this rect to local coordinates.
     * @note Preserves legacy behavior from original implementation.
     */
    void TransformFromHomogeneous(Vx2DVector &dest, const Vx2DVector &srchom) const;
    /// @brief Scales rectangle size relative to top-left corner.
    void Scale(const Vx2DVector &s);
    /// @brief Inflates or deflates by extending each edge.
    void Inflate(const Vx2DVector &pt);
    /// @brief Linear interpolation toward another rectangle.
    void Interpolate(float value, const VxRect &a);
    /// @brief Expands this rectangle to include rectangle `a`.
    void Merge(const VxRect &a);
    ///@}

    /** @name Classification And Clipping */
    ///@{
    /// @brief Returns `ALLOUTSIDE`, `PARTINSIDE`, or `ALLINSIDE` against `cliprect`.
    int IsInside(const VxRect &cliprect) const;
    /// @brief Returns TRUE if completely outside `cliprect`.
    XBOOL IsOutside(const VxRect &cliprect) const;
    /// @brief Returns TRUE if a point is inside or on border.
    XBOOL IsInside(const Vx2DVector &pt) const;
    /// @brief Returns TRUE if all coordinates are zero.
    XBOOL IsNull() const;
    /// @brief Returns TRUE if width or height is zero.
    XBOOL IsEmpty() const;
    /// @brief Clips this rectangle against `cliprect`.
    XBOOL Clip(const VxRect &cliprect);
    /// @brief Clamps a point into rectangle bounds.
    void Clip(Vx2DVector &pt, XBOOL excluderightbottom = TRUE) const;
    ///@}

    /** @name Screen Space Conversion */
    ///@{
    /// @brief Converts from `srcScreen` space into `destScreen` space.
    void Transform(const VxRect &destScreen, const VxRect &srcScreen);
    /// @brief Converts from source screen size to destination screen size.
    void Transform(const Vx2DVector &destScreenSize, const Vx2DVector &srcScreenSize);
    /// @brief Converts from screen coordinates to normalized [0,1] space.
    void TransformToHomogeneous(const VxRect &screen);
    /// @brief Converts from normalized [0,1] space to screen coordinates.
    void TransformFromHomogeneous(const VxRect &screen);
    ///@}

    /** @name Arithmetic Operators */
    ///@{
    /// @brief Translates rectangle by adding vector offset.
    VxRect &operator+=(const Vx2DVector &t);
    /// @brief Translates rectangle by subtracting vector offset.
    VxRect &operator-=(const Vx2DVector &t);
    /// @brief Component-wise multiply (`x` on horizontal, `y` on vertical bounds).
    VxRect &operator*=(const Vx2DVector &t);
    /// @brief Component-wise divide (`x` on horizontal, `y` on vertical bounds).
    VxRect &operator/=(const Vx2DVector &t);

    /// @brief Equality comparison of all four coordinates.
    friend int operator==(const VxRect &r1, const VxRect &r2);
    /// @brief Inequality comparison of all four coordinates.
    friend int operator!=(const VxRect &r1, const VxRect &r2);
    ///@}
};

#include "VxRect.inl"

#endif // VXRECT_H
