#ifndef VXVECTOR_H
#define VXVECTOR_H

#include "VxMathDefines.h"
#include "XUtil.h"

// Forward declarations
struct VxCompressedVector;
struct VxCompressedVectorOld;
class VxMatrix;

/** @name Trigonometry Utility Functions */
///@{

/**
 * @brief Converts radians to a custom angular unit (likely for table lookup).
 * @param val Angle in radians.
 * @return The angle in the custom unit.
 */
VX_EXPORT int radToAngle(float val);

/**
 * @brief Computes sine from a custom angular unit (likely using a lookup table).
 * @param angle Angle in the custom unit.
 * @return The sine of the angle.
 */
VX_EXPORT float Tsin(int angle);

/**
 * @brief Computes cosine from a custom angular unit (likely using a lookup table).
 * @param angle Angle in the custom unit.
 * @return The cosine of the angle.
 */
VX_EXPORT float Tcos(int angle);
///@}

/**
 * @brief Represents a point or direction in 3D space with x, y, and z components.
 *
 * @remarks
 * A VxVector is a fundamental class for 3D mathematics. Its elements can be
 * accessed either via named members (x, y, z) or through array-style indexing.
 *
 * @code
 * VxVector v(1.0f, 2.0f, 3.0f);
 * float x_val = v.x;    // Direct member access
 * float y_val = v[1];   // Array-style access
 * @endcode
 *
 * @see VxVector4, VxCompressedVector
 */
struct VxVector {
    union {
        struct {
            float x; ///< The x-component of the vector.
            float y; ///< The y-component of the vector.
            float z; ///< The z-component of the vector.
        };
        float v[3]; ///< Array access to components {x, y, z}.
    };

    /** @name Constructors */
    ///@{

    /// @brief Default constructor. Initializes the vector to (0, 0, 0).
    VxVector();

    /**
     * @brief Uniform constructor. Initializes all components to the same value.
     * @param f The value to assign to all three components.
     */
    VxVector(float f);

    /**
     * @brief Component-wise constructor.
     * @param _x The value for the x-component.
     * @param _y The value for the y-component.
     * @param _z The value for the z-component.
     */
    VxVector(float _x, float _y, float _z);

    /**
     * @brief Array constructor.
     * @param f A pointer to a 3-element float array [x, y, z].
     */
    VxVector(const float f[3]);
    ///@}

    /// @brief Converts each component of the vector to its absolute value.
    void Absolute();

    /**
     * @brief Returns the squared magnitude (length) of the vector.
     * @return The square of the vector's length (x*x + y*y + z*z).
     * @remarks This is more efficient than Magnitude() as it avoids a square root operation.
     * @see Magnitude()
     */
    float SquareMagnitude() const;

    /**
     * @brief Returns the magnitude (length) of the vector.
     * @return The length of the vector (sqrt(x*x + y*y + z*z)).
     * @warning Calling this on a null vector (0,0,0) will result in NaN (Not aNumber).
     * @see SquareMagnitude()
     */
    float Magnitude() const;

    /** @name Element Access */
    ///@{

    /**
     * @brief Provides const array-style access to the vector's components.
     * @param i The component index (0 for x, 1 for y, 2 for z).
     * @return A const reference to the specified component.
     */
    const float &operator[](int i) const;

    /**
     * @brief Provides array-style access to the vector's components.
     * @param i The component index (0 for x, 1 for y, 2 for z).
     * @return A reference to the specified component.
     */
    float &operator[](int i);

    /**
     * @brief Sets the three components of the vector.
     * @param X The new x-component.
     * @param Y The new y-component.
     * @param Z The new z-component.
     */
    void Set(float X, float Y, float Z);
    ///@}

    /** @name Assignment Operators */
    ///@{

    /// @brief Performs component-wise addition and assignment.
    VxVector &operator+=(const VxVector &v);
    /// @brief Performs component-wise subtraction and assignment.
    VxVector &operator-=(const VxVector &v);
    /// @brief Performs component-wise multiplication and assignment.
    VxVector &operator*=(const VxVector &v);
    /// @brief Performs component-wise division and assignment.
    VxVector &operator/=(const VxVector &v);
    /// @brief Performs scalar multiplication and assignment.
    VxVector &operator*=(float s);
    /// @brief Performs scalar division and assignment.
    VxVector &operator/=(float s);

    /**
     * @brief Assigns the value of a compressed vector to this vector.
     * @param v The compressed vector.
     * @return A reference to this modified vector.
     */
    VX_EXPORT VxVector &operator=(const VxCompressedVector &v);
    ///@}

    /**
     * @brief Computes the dot product with another vector.
     * @param iV The other vector.
     * @return The scalar dot product.
     */
    float Dot(const VxVector &iV) const;

    /**
     * @brief Adds a scalar value to all components.
     * @param s The scalar value to add.
     * @return A new vector with the result.
     */
    VxVector operator+(float s) const;

    /**
     * @brief Subtracts a scalar value from all components.
     * @param s The scalar value to subtract.
     * @return A new vector with the result.
     */
    VxVector operator-(float s) const;

    /** @name Unary Operators */
    ///@{
    friend const VxVector operator+(const VxVector &v);
    friend const VxVector operator-(const VxVector &v);
    ///@}

    /** @name Binary Operators */
    ///@{
    friend const VxVector operator+(const VxVector &v1, const VxVector &v2);
    friend const VxVector operator-(const VxVector &v1, const VxVector &v2);
    friend const VxVector operator*(const VxVector &v, float s);
    friend const VxVector operator*(float s, const VxVector &v);
    friend const VxVector operator/(const VxVector &v, float s);
    friend const VxVector operator*(const VxVector &v1, const VxVector &v2);
    friend const VxVector operator/(const VxVector &v1, const VxVector &v2);
    friend int operator<(const VxVector &v1, const VxVector &v2);
    friend int operator<=(const VxVector &v1, const VxVector &v2);
    friend int operator==(const VxVector &v1, const VxVector &v2);
    friend int operator!=(const VxVector &v1, const VxVector &v2);
    friend float Min(const VxVector &v);
    friend float Max(const VxVector &v);
    friend const VxVector Minimize(const VxVector &v1, const VxVector &v2);
    friend const VxVector Maximize(const VxVector &v1, const VxVector &v2);
    friend const VxVector Interpolate(float step, const VxVector &v1, const VxVector &v2);
    ///@}

    /**
     * @brief Normalizes the vector, making its length equal to 1.
     * @warning The result is undefined for a null vector (0,0,0).
     */
    inline void Normalize();

    /**
     * @brief Rotates the vector by applying a transformation matrix.
     * @param M The rotation matrix to apply.
     */
    void Rotate(const VxMatrix &M);

    /** @name Predefined Vectors */
    ///@{
    VX_EXPORT static const VxVector &axisX(); ///< Returns a unit vector along the X-axis (1,0,0).
    VX_EXPORT static const VxVector &axisY(); ///< Returns a unit vector along the Y-axis (0,1,0).
    VX_EXPORT static const VxVector &axisZ(); ///< Returns a unit vector along the Z-axis (0,0,1).
    VX_EXPORT static const VxVector &axis0(); ///< Returns a zero vector (0,0,0).
    VX_EXPORT static const VxVector &axis1(); ///< Returns a unit vector (1,1,1).
    ///@}

private:
    /** @name Static Vector Constants */
    ///@{
    const static VxVector m_AxisX; ///< Unit vector along the X-axis.
    const static VxVector m_AxisY; ///< Unit vector along the Y-axis.
    const static VxVector m_AxisZ; ///< Unit vector along the Z-axis.
    const static VxVector m_Axis0; ///< Zero vector.
    const static VxVector m_Axis1; ///< Unit vector (1,1,1).
    ///@}
};

/** @name Vector Standalone Functions */
///@{

/**
 * @brief Returns the squared magnitude (length) of a vector.
 * @param v The vector.
 * @return The square of the vector's length (x*x + y*y + z*z).
 * @remarks This is more efficient than Magnitude() as it avoids a square root operation.
 * @see Magnitude()
 */
inline float SquareMagnitude(const VxVector &v);

/**
 * @brief Returns the magnitude (length) of a vector.
 * @param v The vector.
 * @return The length of the vector (sqrt(x*x + y*y + z*z)).
 * @warning Calling this on a null vector (0,0,0) will result in NaN (Not aNumber).
 * @see SquareMagnitude()
 */
inline float Magnitude(const VxVector &v);

/**
 * @brief Returns the inverse of the squared magnitude of a vector.
 * @param v The vector.
 * @return 1.0f / (x*x + y*y + z*z).
 * @warning The result is undefined for a null vector (0,0,0).
 * @see SquareMagnitude()
 */
inline float InvSquareMagnitude(const VxVector &v);

/**
 * @brief Returns the inverse of the magnitude of a vector.
 * @param v The vector.
 * @return 1.0f / sqrt(x*x + y*y + z*z).
 * @warning Calling this on a null vector (0,0,0) will result in NaN (Not aNumber).
 * @see Magnitude()
 */
inline float InvMagnitude(const VxVector &v);

/**
 * @brief Returns a normalized copy of a vector.
 * @param v The vector to normalize.
 * @return A new unit vector pointing in the same direction as v.
 * @warning Returns a vector with NaN components for a null input vector (0,0,0).
 * @remarks This free function may be more precise than the member function `VxVector::Normalize()`.
 * @see VxVector::Normalize(), Magnitude()
 */
inline const VxVector Normalize(const VxVector &v);
inline const VxVector Normalize(const VxVector *vect);

/**
 * @brief Calculates the dot product of two vectors.
 * @param v1 The first vector.
 * @param v2 The second vector.
 * @return The scalar dot product.
 * @see VxVector::Dot()
 */
inline float DotProduct(const VxVector &v1, const VxVector &v2);

/**
 * @brief Calculates the cross product of two vectors.
 * @param Vect1 The first vector.
 * @param Vect2 The second vector.
 * @return A new vector perpendicular to both input vectors (Vect1 x Vect2).
 */
inline const VxVector CrossProduct(const VxVector &Vect1, const VxVector &Vect2);

/**
 * @brief Reflects an incident vector about a surface normal.
 * @param v1 The incident vector (should point towards the surface).
 * @param Norm The normal of the reflection plane (should be a unit vector).
 * @return The reflected vector, calculated as R = v1 - 2 * (v1 . N) * N.
 * @remarks For a correct reflection, the incident vector should point towards the surface.
 * @see DotProduct()
 */
inline const VxVector Reflect(const VxVector &v1, const VxVector &Norm);

/// @brief Rotates a vector using a transformation matrix.
inline const VxVector Rotate(const VxMatrix &mat, const VxVector &pt);
/// @brief Rotates a vector around an axis by a specified angle.
inline const VxVector Rotate(const VxVector &v1, const VxVector &v2, float angle);
/// @brief Rotates a vector around an axis by a specified angle (pointer version).
inline const VxVector Rotate(const VxVector *v1, const VxVector *v2, float angle);

///@}

/**
 * @struct VxCompressedVector
 * @brief A memory-efficient representation of a unit vector using two 16-bit polar angles.
 *
 * @remarks
 * This structure reduces the memory usage for a unit vector from 12 bytes (three floats)
 * to 4 bytes (two shorts). It is primarily used for storing normal vectors where precision
 * is less critical than memory footprint.
 *
 * @see VxVector, VxCompressedVectorOld
 */
typedef struct VxCompressedVector {
    short int xa; ///< The polar angle for the x-axis rotation.
    short int ya; ///< The polar angle for the y-axis rotation.

    /// @brief Default constructor. Initializes angles to 0.
    VxCompressedVector();
    /// @brief Constructs and sets the compressed vector from three float components.
    VxCompressedVector(float _x, float _y, float _z);

    /// @brief Sets the compressed vector from a 3D vector's components.
    void Set(float X, float Y, float Z);

    /// @brief Performs spherical linear interpolation between two compressed vectors.
    void Slerp(float step, VxCompressedVector &v1, VxCompressedVector &v2);

    /// @brief Assigns a VxVector to this compressed vector, performing the conversion.
    VxCompressedVector &operator=(const VxVector &v);
    /// @brief Assigns an old format compressed vector to this one.
    VxCompressedVector &operator=(const VxCompressedVectorOld &v);
} VxCompressedVector;

/**
 * @struct VxCompressedVectorOld
 * @brief A legacy version of VxCompressedVector using 32-bit integers for angles.
 * @deprecated Prefer `VxCompressedVector` for new development.
 */
typedef struct VxCompressedVectorOld {
    int xa; ///< The polar angle for the x-axis rotation.
    int ya; ///< The polar angle for the y-axis rotation.

    /// @brief Default constructor. Initializes angles to 0.
    VxCompressedVectorOld();
    /// @brief Constructs and sets the compressed vector from three float components.
    VxCompressedVectorOld(float _x, float _y, float _z);

    /// @brief Sets the compressed vector from a 3D vector's components.
    void Set(float X, float Y, float Z);
    /// @brief Performs spherical linear interpolation between two old compressed vectors.
    void Slerp(float step, VxCompressedVectorOld &v1, VxCompressedVectorOld &v2);

    /// @brief Assigns a VxVector to this compressed vector.
    VxCompressedVectorOld &operator=(const VxVector &v);
    /// @brief Assigns a modern compressed vector to this old format one.
    VxCompressedVectorOld &operator=(const VxCompressedVector &v);
} VxCompressedVectorOld;

/**
 * @class VxVector4
 * @brief A 4D vector class, extending VxVector with a 'w' component.
 *
 * @remarks
 * VxVector4 is suitable for homogeneous coordinates used in 3D transformations,
 * particularly with projection matrices where the 'w' component is essential
 * for perspective division.
 *
 * @see VxVector
 */
class VxVector4 : public VxVector {
public:
    float w; ///< The w-component of the vector.

    /// @brief Default constructor. Initializes to (0,0,0,0).
    VxVector4();
    /// @brief Uniform constructor. Initializes all four components to the same value.
    VxVector4(float f);
    /// @brief Component-wise constructor.
    VxVector4(float _x, float _y, float _z, float _w);
    /// @brief Array constructor.
    VxVector4(const float f[4]);

    /**
     * @brief Assignment from a 3D vector. The 'w' component is not modified.
     * @param v The 3D vector to assign from.
     * @return A reference to this modified vector.
     */
    VxVector4 &operator=(const VxVector &v);

    // =====================================
    // Access grants
    // =====================================
    /// @brief Provides const array-style access to components.
    const float &operator[](int i) const;
    /// @brief Provides mutable array-style access to components.
    float &operator[](int i);

    /**
     * @brief Implicit conversion to a float pointer for direct memory access.
     * @return A pointer to the first component (x).
     */
#if !defined(_MSC_VER)
    operator float *() const;
#else
    operator float *() const;
#endif

    // Initialization
    /// @brief Sets all four components of the vector.
    void Set(float X, float Y, float Z, float W);
    /// @brief Sets the x, y, and z components, leaving w unmodified.
    void Set(float X, float Y, float Z);
    /// @brief Computes the 3D dot product (ignoring the w component).
    float Dot(const VxVector4 &iV) const;

    /// @brief Component-wise addition and assignment.
    VxVector4 &operator+=(const VxVector4 &v);
    /// @brief Component-wise subtraction and assignment.
    VxVector4 &operator-=(const VxVector4 &v);
    /// @brief Component-wise multiplication and assignment.
    VxVector4 &operator*=(const VxVector4 &v);
    /// @brief Component-wise division and assignment.
    VxVector4 &operator/=(const VxVector4 &v);

    /// @brief Component-wise addition and assignment with a 3D vector.
    VxVector4 &operator+=(const VxVector &v);
    /// @brief Component-wise subtraction and assignment with a 3D vector.
    VxVector4 &operator-=(const VxVector &v);
    /// @brief Component-wise multiplication and assignment with a 3D vector.
    VxVector4 &operator*=(const VxVector &v);
    /// @brief Component-wise division and assignment with a 3D vector.
    VxVector4 &operator/=(const VxVector &v);

    /// @brief Scalar multiplication and assignment.
    VxVector4 &operator*=(float s);
    /// @brief Scalar division and assignment.
    VxVector4 &operator/=(float s);
    /// @brief Adds a scalar to all components.
    VxVector4 operator+(float s) const;
    /// @brief Subtracts a scalar from all components.
    VxVector4 operator-(float s) const;

    // =====================================
    // Unary operators
    // =====================================
    friend const VxVector4 operator+(const VxVector4 &v);
    friend const VxVector4 operator-(const VxVector4 &v);

    // =====================================
    // Binary operators
    // =====================================
    friend const VxVector4 operator+(const VxVector4 &v1, const VxVector4 &v2);
    friend const VxVector4 operator-(const VxVector4 &v1, const VxVector4 &v2);
    friend const VxVector4 operator*(const VxVector4 &v, float s);
    friend const VxVector4 operator*(float s, const VxVector4 &v);
    friend const VxVector4 operator/(const VxVector4 &v, float s);
    friend const VxVector4 operator*(const VxVector4 &v1, const VxVector4 &v2);
    friend const VxVector4 operator/(const VxVector4 &v1, const VxVector4 &v2);

    // Bitwise equality
    friend int operator==(const VxVector4 &v1, const VxVector4 &v2);
    friend int operator!=(const VxVector4 &v1, const VxVector4 &v2);
};

#include "VxVector.inl"

#endif // VXVECTOR_H
