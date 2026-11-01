#ifndef VXMATRIX_H
#define VXMATRIX_H

#include "VxLegacyMatrix.h"
#include "VxQuaternion.h"
#include "VxVector.h"
#include "XUtil.h"

class VxMatrix;
struct Vx2DVector;
struct VxStridedData;

/// @name Matrix Utility Functions
/// @{

/**
 * @brief Sets a matrix to the identity matrix.
 * @param Mat The matrix to be set to identity.
 */
inline void Vx3DMatrixIdentity(VxMatrix &Mat);

/**
 * @brief Transforms a 3D vector by a matrix (assumes w=1 for input, performs perspective divide).
 * @param ResultVector Pointer to store the resulting transformed vector.
 * @param Mat The transformation matrix.
 * @param Vector Pointer to the input vector to transform.
 */
inline void Vx3DMultiplyMatrixVector(VxVector *ResultVector, const VxMatrix &Mat, const VxVector *Vector);

/**
 * @brief Transforms an array of 3D vectors by a matrix.
 * @param ResultVectors Pointer to the output array of transformed vectors.
 * @param Mat The transformation matrix.
 * @param Vectors Pointer to the input array of vectors.
 * @param count The number of vectors to transform.
 * @param stride The byte offset between consecutive vectors in the input and output arrays.
 */
inline void Vx3DMultiplyMatrixVectorMany(VxVector *ResultVectors, const VxMatrix &Mat, const VxVector *Vectors, int count, int stride);

/**
 * @brief Transforms a 4D vector by a matrix.
 * @param ResultVector Pointer to store the resulting transformed 4D vector.
 * @param Mat The transformation matrix.
 * @param Vector Pointer to the input 4D vector.
 */
inline void Vx3DMultiplyMatrixVector4(VxVector4 *ResultVector, const VxMatrix &Mat, const VxVector4 *Vector);

/**
 * @brief Transforms a 3D vector by a matrix, returning a 4D vector (w component is calculated).
 * @param ResultVector Pointer to store the resulting transformed 4D vector.
 * @param Mat The transformation matrix.
 * @param Vector Pointer to the input 3D vector (w is assumed to be 1).
 */
inline void Vx3DMultiplyMatrixVector4(VxVector4 *ResultVector, const VxMatrix &Mat, const VxVector *Vector);

/**
 * @brief Rotates a 3D vector by a matrix, ignoring the translation part of the matrix.
 * @param ResultVector Pointer to store the resulting rotated vector.
 * @param Mat The rotation matrix.
 * @param Vector Pointer to the input vector to rotate.
 */
inline void Vx3DRotateVector(VxVector *ResultVector, const VxMatrix &Mat, const VxVector *Vector);

/**
 * @brief Rotates an array of 3D vectors by a matrix.
 * @param ResultVector Pointer to the output array of rotated vectors.
 * @param Mat The rotation matrix.
 * @param Vector Pointer to the input array of vectors.
 * @param count The number of vectors to rotate.
 * @param stride The byte offset between consecutive vectors.
 */
inline void Vx3DRotateVectorMany(VxVector *ResultVector, const VxMatrix &Mat, const VxVector *Vector, int count, int stride);

/**
 * @brief Multiplies two affine 3D transformation matrices.
 * @param ResultMat The resulting matrix (MatA * MatB).
 * @param MatA The first matrix operand.
 * @param MatB The second matrix operand.
 * @remarks This function enforces an affine output matrix by forcing the last column to [0,0,0,1].
 * Use `Vx3DMultiplyMatrix4()` when projective terms must be preserved.
 */
inline void Vx3DMultiplyMatrix(VxMatrix &ResultMat, const VxMatrix &MatA, const VxMatrix &MatB);

/**
 * @brief Multiplies two full 4x4 matrices.
 * @param ResultMat The resulting matrix (MatA * MatB).
 * @param MatA The first matrix operand.
 * @param MatB The second matrix operand.
 */
inline void Vx3DMultiplyMatrix4(VxMatrix &ResultMat, const VxMatrix &MatA, const VxMatrix &MatB);

/**
 * @brief Computes the inverse of a given 4x4 matrix.
 * @param InverseMat The resulting inverse matrix.
 * @param Mat The matrix to invert.
 */
inline void Vx3DInverseMatrix(VxMatrix &InverseMat, const VxMatrix &Mat);

/**
 * @brief Computes the determinant of a 4x4 matrix.
 * @param Mat The input matrix.
 * @return The determinant of the matrix.
 */
inline float Vx3DMatrixDeterminant(const VxMatrix &Mat);

/**
 * @brief Creates a rotation matrix from an axis and an angle.
 * @param ResultMat The resulting rotation matrix.
 * @param Vector The axis of rotation (should be normalized).
 * @param Angle The angle of rotation in radians.
 */
inline void Vx3DMatrixFromRotation(VxMatrix &ResultMat, const VxVector &Vector, float Angle);

/**
 * @brief Creates a transformation matrix that rotates around a specific origin point.
 * @param ResultMat The resulting transformation matrix.
 * @param Vector The axis of rotation (should be normalized).
 * @param Origin The point to rotate around.
 * @param Angle The angle of rotation in radians.
 */
// As in the original runtime, an Origin referencing ResultMat[3] observes
// sequential translation writes. Pass a copy for a fixed pivot in that case.
inline void Vx3DMatrixFromRotationAndOrigin(VxMatrix &ResultMat, const VxVector &Vector, const VxVector &Origin, float Angle);

/**
 * @brief Creates a rotation matrix from Euler angles.
 * @param Mat The resulting rotation matrix.
 * @param eax The rotation around the X-axis (pitch) in radians.
 * @param eay The rotation around the Y-axis (yaw) in radians.
 * @param eaz The rotation around the Z-axis (roll) in radians.
 */
inline void Vx3DMatrixFromEulerAngles(VxMatrix &Mat, float eax, float eay, float eaz);

/**
 * @brief Extracts Euler angles from a rotation matrix.
 * @param Mat The input rotation matrix.
 * @param eax Pointer to store the X-axis rotation (pitch) in radians.
 * @param eay Pointer to store the Y-axis rotation (yaw) in radians.
 * @param eaz Pointer to store the Z-axis rotation (roll) in radians.
 */
inline void Vx3DMatrixToEulerAngles(const VxMatrix &Mat, float *eax, float *eay, float *eaz);

/**
 * @brief Interpolates decomposed translation and row scales linearly, and rotation with Slerp.
 * @param step The interpolation factor (0.0 for matrix A, 1.0 for matrix B).
 * @param Res The resulting interpolated matrix.
 * @param A The first matrix.
 * @param B The second matrix.
 */
inline void Vx3DInterpolateMatrix(float step, VxMatrix &Res, const VxMatrix &A, const VxMatrix &B);

/**
 * @brief Compatibility alias for Vx3DInterpolateMatrix, including scale interpolation.
 * @note Both names resolve to the same function in the original runtime.
 * @param step The interpolation factor (0.0 for matrix A, 1.0 for matrix B).
 * @param Res The resulting interpolated matrix.
 * @param A The first matrix.
 * @param B The second matrix.
 */
inline void Vx3DInterpolateMatrixNoScale(float step, VxMatrix &Res, const VxMatrix &A, const VxMatrix &B);

/**
 * @brief Transforms an array of vectors with a specific stride by a matrix.
 * @param Dest Pointer to the destination strided data structure.
 * @param Src Pointer to the source strided data structure.
 * @param Mat The transformation matrix.
 * @param count The number of vectors to transform.
 */
inline void Vx3DMultiplyMatrixVectorStrided(VxStridedData *Dest, VxStridedData *Src, const VxMatrix &Mat, int count);

/**
 * @brief Transforms an array of 4D vectors with a specific stride by a matrix.
 * @param Dest Pointer to the destination strided data structure.
 * @param Src Pointer to the source strided data structure.
 * @param Mat The transformation matrix.
 * @param count The number of vectors to transform.
 */
inline void Vx3DMultiplyMatrixVector4Strided(VxStridedData *Dest, VxStridedData *Src, const VxMatrix &Mat, int count);

/**
 * @brief Rotates an array of vectors with a specific stride by a matrix.
 * @param Dest Pointer to the destination strided data structure.
 * @param Src Pointer to the source strided data structure.
 * @param Mat The rotation matrix.
 * @param count The number of vectors to rotate.
 */
inline void Vx3DRotateVectorStrided(VxStridedData *Dest, VxStridedData *Src, const VxMatrix &Mat, int count);

/**
 * @brief Computes the transpose of a 4x4 matrix.
 * @param Result The resulting transposed matrix.
 * @param A The input matrix.
 */
inline void Vx3DTransposeMatrix(VxMatrix &Result, const VxMatrix &A);

/**
 * @brief Decomposes a transformation matrix into its scale, rotation, and translation components.
 * @param A The input transformation matrix.
 * @param Quat The output rotation as a quaternion.
 * @param Pos The output translation vector.
 * @param Scale The output scaling vector.
 */
inline void Vx3DDecomposeMatrix(const VxMatrix &A, VxQuaternion &Quat, VxVector &Pos, VxVector &Scale);

/**
 * @brief Decomposes a transformation matrix, including an additional rotation for handling shear or complex transforms.
 * @param A The input transformation matrix.
 * @param Quat The output primary rotation quaternion.
 * @param Pos The output translation vector.
 * @param Scale The output scaling vector.
 * @param URot The output secondary rotation quaternion.
 * @return A metric representing the overall scaling factor.
 */
inline float Vx3DDecomposeMatrixTotal(const VxMatrix &A, VxQuaternion &Quat, VxVector &Pos, VxVector &Scale, VxQuaternion &URot);

/**
 * @brief Decomposes a transformation matrix into components, using pointers for optional outputs.
 * @param A The input transformation matrix.
 * @param Quat Pointer to store the output rotation quaternion (can be NULL).
 * @param Pos Pointer to store the output translation vector (can be NULL).
 * @param Scale Pointer to store the output scaling vector (can be NULL).
 * @param URot Pointer to store the output secondary rotation quaternion (can be NULL).
 * @return A metric representing the overall scaling factor.
 */
inline float Vx3DDecomposeMatrixTotalPtr(const VxMatrix &A, VxQuaternion *Quat, VxVector *Pos, VxVector *Scale, VxQuaternion *URot);
/// @}

/**
 * @brief A class representing a 4x4 matrix.
 */
class VxMatrix {
public:
    /// @brief Default constructor. Does not initialize the matrix.
    VxMatrix();

    /**
     * @brief Constructs a matrix from a 2D float array.
     * @param m A 4x4 array of floats to initialize the matrix with.
     */
    VxMatrix(float m[4][4]);

    /**
     * @brief Returns a reference to a static identity matrix.
     * @return A const reference to the identity matrix.
     */
    static VxMatrix Identity();

    // Matrix construction
    /// @brief Sets all elements of the matrix to zero.
    void Clear();
    /// @brief Sets this matrix to the identity matrix.
    void SetIdentity();

    /**
     * @brief Constructs an orthographic projection matrix.
     * @param Zoom The zoom factor for both X and Y axes.
     * @param Aspect The aspect ratio (width / height).
     * @param Near_plane The distance to the near clipping plane.
     * @param Far_plane The distance to the far clipping plane.
     * @remarks The resulting matrix is:
     * @code
     *   F = Far_plane, N = Near_plane
     *
     *   [ Zoom      0           0         0 ]
     *   [ 0         Zoom*Aspect 0         0 ]
     *   [ 0         0           1/(F-N)   0 ]
     *   [ 0         0           -N/(F-N)  1 ]
     * @endcode
     * @see Perspective, OrthographicRect
     */
    void Orthographic(float Zoom, float Aspect, float Near_plane, float Far_plane);

    /**
     * @brief Constructs a perspective projection matrix.
     * @param Fov The vertical field of view, in radians.
     * @param Aspect The aspect ratio (width / height).
     * @param Near_plane The distance to the near clipping plane.
     * @param Far_plane The distance to the far clipping plane.
     * @remarks The resulting matrix is:
     * @code
     *   A = cot(Fov/2), F = Far_plane, N = Near_plane
     *
     *   [ A         0           0         0 ]
     *   [ 0         A*Aspect    0         0 ]
     *   [ 0         0           F/(F-N)   1 ]
     *   [ 0         0           -F*N/(F-N) 0 ]
     * @endcode
     * @see Orthographic, PerspectiveRect
     */
    void Perspective(float Fov, float Aspect, float Near_plane, float Far_plane);

    /**
     * @brief Constructs an orthographic projection matrix from a view rectangle.
     * @param Left The coordinate for the left vertical clipping plane.
     * @param Right The coordinate for the right vertical clipping plane.
     * @param Top The coordinate for the top horizontal clipping plane.
     * @param Bottom The coordinate for the bottom horizontal clipping plane.
     * @param Near_plane The distance to the near clipping plane.
     * @param Far_plane The distance to the far clipping plane.
     * @see PerspectiveRect, Orthographic
     */
    void OrthographicRect(float Left, float Right, float Top, float Bottom, float Near_plane, float Far_plane);

    /**
     * @brief Constructs a perspective projection matrix from a view rectangle.
     * @param Left The coordinate for the left vertical clipping plane on the near plane.
     * @param Right The coordinate for the right vertical clipping plane on the near plane.
     * @param Top The coordinate for the top horizontal clipping plane on the near plane.
     * @param Bottom The coordinate for the bottom horizontal clipping plane on the near plane.
     * @param Near_plane The distance to the near clipping plane.
     * @param Far_plane The distance to the far clipping plane.
     * @see Perspective, OrthographicRect
     */
    void PerspectiveRect(float Left, float Right, float Top, float Bottom, float Near_plane, float Far_plane);

    // operators
    /// @brief Provides const access to a row of the matrix.
    const VxVector4 &operator[](int i) const;
    /// @brief Provides mutable access to a row of the matrix.
    VxVector4 &operator[](int i);

    /// @brief Implicitly converts the matrix to a const void pointer to its data.
    operator const void *() const;
    /// @brief Implicitly converts the matrix to a void pointer to its data.
    operator void *();

    /**
     * @brief Compares two matrices for equality.
     * @remarks This operator compares the contents of the matrices.
     * @return TRUE if the matrices have the same contents, FALSE otherwise.
     */
    XBOOL operator==(const VxMatrix &mat) const;

    /**
     * @brief Compares two matrices for inequality.
     * @remarks This operator compares the contents of the matrices.
     * @return TRUE if the matrices have different contents, FALSE otherwise.
     */
    XBOOL operator!=(const VxMatrix &mat) const;

    /**
     * @brief Multiplies this matrix by another matrix and assigns the result to this matrix.
     * @param mat The matrix to multiply with.
     * @return A reference to this modified matrix.
     * @remarks Uses affine multiplication (`Vx3DMultiplyMatrix`).
     */
    VxMatrix &operator*=(const VxMatrix &mat);

    /**
     * @brief Multiplies this matrix by another matrix.
     * @param iMat The matrix to multiply with.
     * @return The resulting new matrix.
     * @remarks Uses affine multiplication (`Vx3DMultiplyMatrix`).
     */
    VxMatrix operator*(const VxMatrix &iMat) const;

    /// @brief Transforms a 3D vector by this matrix.
    friend VxVector operator*(const VxMatrix &m, const VxVector &v);
    /// @brief Transforms a 4D vector by this matrix.
    friend VxVector4 operator*(const VxMatrix &m, const VxVector4 &v);
    /// @brief Transforms a 3D vector by this matrix.
    friend VxVector operator*(const VxVector &v, const VxMatrix &m);
    /// @brief Transforms a 4D vector by this matrix.
    friend VxVector4 operator*(const VxVector4 &v, const VxMatrix &m);

protected:
    float m_Data[4][4]; ///< The 16 floating-point values of the matrix.
};

#include "VxMatrix.inl"

#endif // VXMATRIX_H
