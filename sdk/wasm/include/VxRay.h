#ifndef VXRAY_H
#define VXRAY_H

#include "VxMatrix.h"
#include "VxSIMD.h"

/**
 * @brief Represents a ray in 3D space, defined by an origin and a direction.
 *
 * @remarks
 * A VxRay is defined by two VxVector instances and is used to represent a ray
 * for purposes such as intersection testing. The direction vector is not
 * necessarily normalized.
 *
 * A VxRay is defined as:
 * @code
 * class VxRay {
 * public:
 *     VxVector m_Origin;
 *     VxVector m_Direction;
 * };
 * @endcode
 *
 * @see VxMatrix, VxVector
 */
class VxRay {
public:
    /// @brief Default constructor.
    VxRay();

    /**
     * @brief Constructs a ray from a start point and an end point.
     * @param start The origin of the ray.
     * @param end The point that defines the direction from the origin.
     */
    VxRay(const VxVector &start, const VxVector &end);

    /**
     * @brief Constructs a ray from a starting point and a direction vector.
     * @param start The origin of the ray.
     * @param dir The direction vector of the ray.
     * @param dummy A dummy parameter to differentiate this constructor.
     */
    VxRay(const VxVector &start, const VxVector &dir, int *dummy);

    /**
     * @brief Transforms the ray by a matrix.
     * @param dest Output parameter for the transformed ray.
     * @param mat The transformation matrix to apply.
     */
    inline void Transform(VxRay &dest, const VxMatrix &mat);

    /**
     * @brief Interpolates a point along the ray.
     * @param p Output parameter for the interpolated point (m_Origin + t * m_Direction).
     * @param t The interpolation parameter.
     */
    void Interpolate(VxVector &p, float t) const;

    /**
     * @brief Returns the squared distance from a point to the infinite line defined by this ray.
     * @param p A point in space.
     * @return The squared distance from the point to the line.
     */
    float SquareDistance(const VxVector &p) const;

    /**
     * @brief Returns the distance from a point to the infinite line defined by this ray.
     * @param p A point in space.
     * @return The minimum distance between the point and the line.
     */
    float Distance(const VxVector &p) const;

    /// @brief Equality operator.
    bool operator==(const VxRay &iRay) const;

    /// @brief Gets a const reference to the ray's origin.
    const VxVector &GetOrigin() const;
    /// @brief Gets a mutable reference to the ray's origin.
    VxVector &GetOrigin();

    /// @brief Gets a const reference to the ray's direction vector.
    const VxVector &GetDirection() const;
    /// @brief Gets a mutable reference to the ray's direction vector.
    VxVector &GetDirection();

    VxVector m_Origin; ///< The origin point of the ray.
    VxVector m_Direction; ///< The direction vector of the ray (not necessarily normalized).
};

#include "VxRay.inl"

#endif // VXRAY_H
