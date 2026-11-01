#ifndef VXFRUSTUM_H
#define VXFRUSTUM_H

#include "VxMatrix.h"
#include "VxPlane.h"
#include "VxVector.h"

/**
 * @brief Represents a viewing frustum, typically used for visibility culling.
 *
 * @remarks
 * This class defines a 3D frustum volume, which is essential for determining
 * which objects are within a camera's field of view. It is defined by an origin,
 * orientation vectors, and near/far plane distances, along with field of view and aspect ratio.
 * It also pre-computes the six bounding planes of the frustum for efficient intersection tests.
 *
 * @see VxVector, VxRay, VxIntersect
 */
class VxFrustum {
public:
    /**
     * @brief Default constructor.
     */
    inline VxFrustum();

    /**
     * @brief Constructs a frustum from a set of parameters.
     * @param origin The origin point of the frustum (camera position).
     * @param right The right direction vector.
     * @param up The up direction vector.
     * @param dir The viewing direction vector.
     * @param nearplane The distance to the near clipping plane.
     * @param farplane The distance to the far clipping plane.
     * @param fov The vertical field of view, in radians.
     * @param aspectratio The aspect ratio (width / height).
     */
    inline VxFrustum(const VxVector &origin, const VxVector &right, const VxVector &up, const VxVector &dir, float nearplane, float farplane, float fov, float aspectratio);

    /// @brief Gets a mutable reference to the frustum's origin.
    VxVector &GetOrigin();
    /// @brief Gets a const reference to the frustum's origin.
    const VxVector &GetOrigin() const;

    /// @brief Gets a mutable reference to the frustum's right vector.
    VxVector &GetRight();
    /// @brief Gets a const reference to the frustum's right vector.
    const VxVector &GetRight() const;

    /// @brief Gets a mutable reference to the frustum's up vector.
    VxVector &GetUp();
    /// @brief Gets a const reference to the frustum's up vector.
    const VxVector &GetUp() const;

    /// @brief Gets a mutable reference to the frustum's direction vector.
    VxVector &GetDir();
    /// @brief Gets a const reference to the frustum's direction vector.
    const VxVector &GetDir() const;

    /// @brief Gets a mutable reference to the horizontal bound size at the near plane.
    float &GetRBound();
    /// @brief Gets a const reference to the horizontal bound size at the near plane.
    const float &GetRBound() const;

    /// @brief Gets a mutable reference to the vertical bound size at the near plane.
    float &GetUBound();
    /// @brief Gets a const reference to the vertical bound size at the near plane.
    const float &GetUBound() const;

    /// @brief Gets a mutable reference to the near plane distance.
    float &GetDMin();
    /// @brief Gets a const reference to the near plane distance.
    const float &GetDMin() const;

    /// @brief Gets a mutable reference to the far plane distance.
    float &GetDMax();
    /// @brief Gets a const reference to the far plane distance.
    const float &GetDMax() const;

    /// @brief Gets the ratio of far plane distance to near plane distance.
    float GetDRatio() const;
    /// @brief Gets the horizontal field of view factor.
    float GetRF() const;
    /// @brief Gets the vertical field of view factor.
    float GetUF() const;

    /// @brief Gets a const reference to the near clipping plane.
    const VxPlane &GetNearPlane() const;
    /// @brief Gets a const reference to the far clipping plane.
    const VxPlane &GetFarPlane() const;
    /// @brief Gets a const reference to the left clipping plane.
    const VxPlane &GetLeftPlane() const;
    /// @brief Gets a const reference to the right clipping plane.
    const VxPlane &GetRightPlane() const;
    /// @brief Gets a const reference to the top clipping plane.
    const VxPlane &GetUpPlane() const;
    /// @brief Gets a const reference to the bottom clipping plane.
    const VxPlane &GetBottomPlane() const;

    /**
     * @brief Classifies a point against the frustum's planes.
     * @param v The point to classify.
     * @return A bitmask of VXCLIP flags indicating which planes the point is outside of. Returns 0 if inside.
     */
    XDWORD Classify(const VxVector &v) const;

    /**
     * @brief Classifies an axis-aligned bounding box (AABB) against the frustum.
     * @param v The AABB to classify.
     * @return A negative value if intersecting, a positive value if completely outside, and 0 if on a plane.
     */
    float Classify(const VxBbox &v) const;

    /**
     * @brief Classifies an oriented bounding box (OBB) against the frustum.
     * @param b The original AABB of the box.
     * @param mat The transformation matrix applied to the box.
     * @return A negative value if intersecting, a positive value if completely outside, and 0 if on a plane.
     */
    float Classify(const VxBbox &b, const VxMatrix &mat) const;

    /**
     * @brief Checks if a point is inside the frustum.
     * @param v The point to check.
     * @return TRUE if the point is inside or on the boundary of the frustum, FALSE otherwise.
     */
    XBOOL IsInside(const VxVector &v) const;

    /**
     * @brief Transforms the frustum by an inverse world matrix.
     * @param invworldmat The inverse of the world transformation matrix to apply.
     */
    inline void Transform(const VxMatrix &invworldmat);

    /**
     * @brief Computes the 8 corner vertices of the frustum.
     * @param vertices An array of 8 VxVector to store the computed vertices.
     */
    inline void ComputeVertices(VxVector vertices[8]) const;

    /**
     * @brief Updates the frustum's derived properties (e.g., planes) after modification of its base parameters.
     */
    inline void Update();

    /**
     * @brief Compares this frustum with another for equality.
     * @param iFrustum The frustum to compare against.
     * @return True if all base parameters are equal, false otherwise.
     */
    bool operator==(const VxFrustum &iFrustum) const;

protected:
    /**
     * @brief Classifies an OBB against a single plane.
     * @param axis The transformed axes and center of the OBB.
     * @param plane The plane to classify against.
     * @return A negative value if intersecting, a positive value if completely outside, and 0 if on the plane.
     */
    static float XClassify(const VxVector axis[4], const VxPlane &plane);

    VxVector m_Origin;  ///< The origin point of the frustum.
    VxVector m_Right;   ///< The right vector of the frustum's orientation.
    VxVector m_Up;      ///< The up vector of the frustum's orientation.
    VxVector m_Dir;     ///< The direction vector of the frustum.

    float m_RBound; ///< Horizontal bound at the near plane.
    float m_UBound; ///< Vertical bound at the near plane.
    float m_DMin;   ///< Near plane distance.
    float m_DMax;   ///< Far plane distance.

    // derived quantities
    float m_DRatio; ///< Ratio of m_DMax / m_DMin.
    float m_RF;     ///< Right factor for plane calculation.
    float m_UF;     ///< Up factor for plane calculation.
    VxPlane m_LeftPlane;    ///< The left clipping plane.
    VxPlane m_RightPlane;   ///< The right clipping plane.
    VxPlane m_UpPlane;      ///< The top clipping plane.
    VxPlane m_BottomPlane;  ///< The bottom clipping plane.
    VxPlane m_NearPlane;    ///< The near clipping plane.
    VxPlane m_FarPlane;     ///< The far clipping plane.
};

#include "VxFrustum.inl"

#endif // VXFRUSTUM_H
