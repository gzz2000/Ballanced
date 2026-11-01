#ifndef VXBBOX_H
#define VXBBOX_H

#include "VxVector.h"

// Forward declarations
class VxMatrix;

/**
 * @struct VxBbox
 * @brief Represents an axis-aligned bounding box (AABB) in 3D space.
 *
 * @remarks
 * A VxBbox is defined by its minimum and maximum corner points. It provides
 * methods for merging, intersection, containment testing, and other spatial
 * operations commonly used in 3D graphics and physics.
 *
 * @see VxVector
 */
typedef struct VxBbox {
#if !defined(_MSC_VER)
    VxVector Max; ///< The maximum corner of the box (highest x, y, and z coordinates).
    VxVector Min; ///< The minimum corner of the box (lowest x, y, and z coordinates).
#else
    union {
        struct {
            VxVector Max; ///< The maximum corner of the box (highest x, y, and z coordinates).
            VxVector Min; ///< The minimum corner of the box (lowest x, y, and z coordinates).
        };
        float v[6]; ///< Array access to components {Max.x, Max.y, Max.z, Min.x, Min.y, Min.z}.
    };
#endif

    /**
     * @brief Default constructor. Creates an invalid box ready for merging.
     * @remarks The box is initialized with inverted bounds (Min > Max) so that the
     * first call to `Merge` will correctly initialize its extents.
     */
    VxBbox();

    /**
     * @brief Constructs a box from minimum and maximum corner points.
     * @param iMin The minimum corner point.
     * @param iMax The maximum corner point.
     */
    VxBbox(const VxVector &iMin, const VxVector &iMax);

    /**
     * @brief Constructs a box centered at the origin with a given radius.
     * @param value The half-size of the box in all dimensions.
     */
    explicit VxBbox(float value);

    /**
     * @brief Checks if the bounding box has valid extents.
     * @return TRUE if Min.x <= Max.x, Min.y <= Max.y, and Min.z <= Max.z.
     */
    XBOOL IsValid() const;

    /// @brief Returns the size of the box (width, height, depth).
    VxVector GetSize() const;
    /// @brief Returns the half-size of the box (half-width, half-height, half-depth).
    VxVector GetHalfSize() const;
    /// @brief Returns the center point of the box.
    VxVector GetCenter() const;

    /// @brief Sets the box's corners from min and max points.
    void SetCorners(const VxVector &min, const VxVector &max);
    /// @brief Sets the box from a position (min corner) and size.
    void SetDimension(const VxVector &position, const VxVector &size);
    /// @brief Sets the box from its center and half-size.
    void SetCenter(const VxVector &center, const VxVector &halfsize);

    /**
     * @brief Resets the box to an invalid state, ready for merging.
     * @remarks This sets Min to a large positive value and Max to a large negative
     * value, ensuring any subsequent `Merge` operation will correctly define the box.
     */
    void Reset();

    /**
     * @brief Expands this box to enclose another box.
     * @param v The other VxBbox to merge with this one.
     */
    void Merge(const VxBbox &v);

    /**
     * @brief Expands this box to enclose a point.
     * @param v The VxVector point to merge with this one.
     */
    void Merge(const VxVector &v);

    /**
     * @brief Classifies a point's position relative to the box's planes.
     * @param iPoint The point to classify.
     * @return A bitmask of `VXCLIP_FLAGS` indicating which planes the point is outside of.
     */
    XDWORD Classify(const VxVector &iPoint) const;

    /**
     * @brief Classifies a box's position relative to this box.
     * @param iBox The box to classify.
     * @return A bitmask of `VXCLIP_FLAGS` indicating full exclusion on one or more axes.
     */
    XDWORD Classify(const VxBbox &iBox) const;

    /**
     * @brief Classifies the relative position of two boxes from a viewpoint.
     * @param box2 The other box.
     * @param pt The viewpoint.
     * @return 2 if `box2` is on the opposite side of this box from `pt`, 1 if `box2` is inside, 0 otherwise.
     */
    inline int Classify(const VxBbox &box2, const VxVector &pt) const;

    /**
     * @brief Classifies an array of vertices against the box planes.
     * @param iVcount The number of vertices to classify.
     * @param iVertices Pointer to the start of the vertex data.
     * @param iStride The byte offset between consecutive vertices.
     * @param oFlags An output array to be filled with `VXCLIP_FLAGS` for each vertex.
     */
    inline void ClassifyVertices(const int iVcount, XBYTE *iVertices, XDWORD iStride, XDWORD *oFlags) const;

    /**
     * @brief Classifies an array of vertices against a single axis of the box.
     * @param iVcount The number of vertices.
     * @param iVertices Pointer to the vertex data.
     * @param iStride The byte offset between vertices.
     * @param iAxis The axis to test against (0=x, 1=y, 2=z).
     * @param oFlags An output array to be filled with flags (0x01 if < min, 0x10 if > max).
     */
    inline void ClassifyVerticesOneAxis(const int iVcount, XBYTE *iVertices, XDWORD iStride, const int iAxis, XDWORD *oFlags) const;

    /**
     * @brief Calculates the intersection of this box with another box.
     * @param v The VxBbox to intersect with this one.
     * @remarks The result modifies this box to become the intersection volume.
     */
    void Intersect(const VxBbox &v);

    /**
     * @brief Tests if a point is inside the box.
     * @param v The point to test.
     * @return TRUE if the point is inside or on the boundary of this box, FALSE otherwise.
     */
    XBOOL VectorIn(const VxVector &v) const;

    /**
     * @brief Tests if another box is totally inside this box.
     * @param b The box to test.
     * @return TRUE if box b is completely contained within this box, FALSE otherwise.
     */
    XBOOL IsBoxInside(const VxBbox &b) const;

    /// @brief Equality operator.
    bool operator==(const VxBbox &iBox) const;

    /**
     * @brief Transforms the eight corner points of this box by a matrix.
     * @param pts A pointer to an array to store the eight resulting points.
     * @param Mat The transformation matrix.
     */
    inline void TransformTo(VxVector *pts, const VxMatrix &Mat) const;

    /**
     * @brief Creates this box by transforming another box by a matrix.
     * @param sbox The source box to transform.
     * @param Mat The transformation matrix.
     */
    inline void TransformFrom(const VxBbox &sbox, const VxMatrix &Mat);
} VxBbox;

#include "VxBbox.inl"

#endif // VXBBOX_H
