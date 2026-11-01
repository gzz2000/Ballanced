/**
 * @file VxMatrix.inl
 * @brief Inline implementations for VxMatrix and related matrix operations.
 *
 * Part 1: Scalar inline bodies (moved from VxMatrix.h).
 * Part 2: SIMD-accelerated high-level operations on VxMatrix.
 *
 * Included automatically at the bottom of VxMatrix.h - do not include directly.
 */

#pragma once

#include "VxSIMD.h"

#if defined(VX_SIMD_SSE)
VX_SIMD_INLINE void VxSIMDMultiplyMatrixM(VxMatrix *result, const VxMatrix *a, const VxMatrix *b) noexcept;
VX_SIMD_INLINE void VxSIMDMultiplyMatrixM4(VxMatrix *result, const VxMatrix *a, const VxMatrix *b) noexcept;
VX_SIMD_INLINE void VxSIMDTransposeMatrixM(VxMatrix *result, const VxMatrix *a) noexcept;
VX_SIMD_INLINE void VxSIMDMultiplyMatrixVector(VxVector *result, const VxMatrix *mat, const VxVector *v) noexcept;
VX_SIMD_INLINE void VxSIMDMultiplyMatrixVector4(VxVector4 *result, const VxMatrix *mat, const VxVector4 *v) noexcept;
VX_SIMD_INLINE void VxSIMDMultiplyMatrixVector4FromVector3(VxVector4 *result, const VxMatrix *mat, const VxVector *v) noexcept;
VX_SIMD_INLINE void VxSIMDMatrixIdentity(VxMatrix *mat) noexcept;
VX_SIMD_INLINE void VxSIMDRotateVectorM(VxVector *result, const VxMatrix *mat, const VxVector *v) noexcept;
VX_SIMD_INLINE void VxSIMDMultiplyMatrixVectorMany(VxVector *resultVectors, const VxMatrix *mat, const VxVector *vectors, int count, int stride) noexcept;
VX_SIMD_INLINE void VxSIMDRotateVectorMany(VxVector *resultVectors, const VxMatrix *mat, const VxVector *vectors, int count, int stride) noexcept;
VX_SIMD_INLINE void VxSIMDMultiplyMatrixVectorStrided(VxStridedData *dest, const VxStridedData *src, const VxMatrix *mat, int count) noexcept;
VX_SIMD_INLINE void VxSIMDMultiplyMatrixVector4Strided(VxStridedData *dest, const VxStridedData *src, const VxMatrix *mat, int count) noexcept;
VX_SIMD_INLINE void VxSIMDRotateVectorStrided(VxStridedData *dest, const VxStridedData *src, const VxMatrix *mat, int count) noexcept;
VX_SIMD_INLINE float VxSIMDDeterminant3x3(const VxMatrix *mat) noexcept;
VX_SIMD_INLINE void VxSIMDInverseAffineMatrix(VxMatrix *result, const VxMatrix *mat) noexcept;
#endif

// =============================================================================
// Part 1 - Scalar Inline Implementations
// =============================================================================

// ---------- VxMatrix member helpers ----------

inline VxMatrix::VxMatrix() : m_Data() {}

inline VxMatrix::VxMatrix(float m[4][4]) {
    memcpy(m_Data, m, sizeof(VxMatrix));
}

inline VxMatrix VxMatrix::Identity() {
   VxMatrix m;
    m.SetIdentity();
    return m;
}

inline void VxMatrix::Clear() {
    memset(m_Data, 0, sizeof(VxMatrix));
}

inline const VxVector4 &VxMatrix::operator[](int i) const {
    return (const VxVector4 &) (*(VxVector4 *) (m_Data + i));
}

inline VxVector4 &VxMatrix::operator[](int i) {
    return (VxVector4 &) (*(VxVector4 *) (m_Data + i));
}

inline VxMatrix::operator const void *() const {
    return &m_Data[0];
}

inline VxMatrix::operator void *() {
    return &m_Data[0];
}

inline XBOOL VxMatrix::operator==(const VxMatrix &mat) const {
#if defined(VX_SIMD_SSE)
    for (int i = 0; i < 4; ++i) {
        __m128 a = _mm_loadu_ps(&m_Data[i][0]);
        __m128 b = _mm_loadu_ps(&mat.m_Data[i][0]);
        if (_mm_movemask_ps(_mm_cmpeq_ps(a, b)) != 0xF) {
            return FALSE;
        }
    }
    return TRUE;
#else
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (m_Data[i][j] != mat.m_Data[i][j]) {
                return FALSE;
            }
        }
    }
    return TRUE;
#endif
}

inline XBOOL VxMatrix::operator!=(const VxMatrix &mat) const {
    return !(*this == mat);
}

inline VxMatrix &VxMatrix::operator*=(const VxMatrix &mat) {
    Vx3DMultiplyMatrix(*this, *this, mat);
    return *this;
}

inline VxMatrix VxMatrix::operator*(const VxMatrix &iMat) const {
    VxMatrix temp;
    Vx3DMultiplyMatrix(temp, *this, iMat);
    return temp;
}

inline void VxMatrix::SetIdentity() {
#if defined(VX_SIMD_SSE)
    VxSIMDMatrixIdentity(this);
#else
    m_Data[0][1] = m_Data[0][2] = m_Data[0][3] =
    m_Data[1][0] = m_Data[1][2] = m_Data[1][3] =
    m_Data[2][0] = m_Data[2][1] = m_Data[2][3] =
    m_Data[3][0] = m_Data[3][1] = m_Data[3][2] = 0;
    m_Data[0][0] = m_Data[1][1] = m_Data[2][2] = m_Data[3][3] = 1.0f;
#endif
}

// ---------- Matrix-vector operators ----------

inline VxVector operator*(const VxVector &v, const VxMatrix &m) {
    VxVector result;
    Vx3DMultiplyMatrixVector(&result, m, &v);
    return result;
}

inline VxVector operator*(const VxMatrix &m, const VxVector &v) {
    VxVector result;
    Vx3DMultiplyMatrixVector(&result, m, &v);
    return result;
}

inline VxVector4 operator*(const VxMatrix &m, const VxVector4 &v) {
    VxVector4 result;
    Vx3DMultiplyMatrixVector4(&result, m, &v);
    return result;
}

inline VxVector4 operator*(const VxVector4 &v, const VxMatrix &m) {
    VxVector4 result;
    Vx3DMultiplyMatrixVector4(&result, m, &v);
    return result;
}

// ---------- Projection matrices ----------

inline void VxMatrix::Perspective(float Fov, float Aspect, float Near_plane, float Far_plane) {
    Clear();
    m_Data[0][0] = cosf(Fov * 0.5f) / sinf(Fov * 0.5f);
    m_Data[1][1] = m_Data[0][0] * Aspect;
    m_Data[2][2] = Far_plane / (Far_plane - Near_plane);
    m_Data[3][2] = -m_Data[2][2] * Near_plane;
    m_Data[2][3] = 1;
}

inline void VxMatrix::PerspectiveRect(float Left, float Right, float Top, float Bottom, float Near_plane, float Far_plane) {
    Clear();
    float RL = 1.0f / (Right - Left);
    float TB = 1.0f / (Top - Bottom);
    m_Data[0][0] = 2.0f * Near_plane * RL;
    m_Data[1][1] = 2.0f * Near_plane * TB;
    m_Data[2][0] = -(Right + Left) * RL;
    m_Data[2][1] = -(Top + Bottom) * TB;
    m_Data[2][2] = Far_plane / (Far_plane - Near_plane);
    m_Data[3][2] = -m_Data[2][2] * Near_plane;
    m_Data[2][3] = 1;
}

inline void VxMatrix::Orthographic(float Zoom, float Aspect, float Near_plane, float Far_plane) {
    Clear();
    float iz = 1.0f / (Far_plane - Near_plane);
    m_Data[0][0] = Zoom;
    m_Data[1][1] = Zoom * Aspect;
    m_Data[2][2] = iz;
    m_Data[3][2] = -Near_plane * iz;
    m_Data[3][3] = 1.0f;
}

inline void VxMatrix::OrthographicRect(float Left, float Right, float Top, float Bottom, float Near_plane, float Far_plane) {
    Clear();
    float ix = 1.0f / (Right - Left);
    float iy = 1.0f / (Top - Bottom);
    float iz = 1.0f / (Far_plane - Near_plane);
    m_Data[0][0] = 2.0f * ix;
    m_Data[1][1] = -2.0f * iy;
    m_Data[2][2] = iz;
    m_Data[3][0] = -(Left + Right) * ix;
    m_Data[3][1] = (Top + Bottom) * iy;
    m_Data[3][2] = -Near_plane * iz;
    m_Data[3][3] = 1.0f;
}

// ---------- Basic matrix-vector transforms ----------

inline void Vx3DMultiplyMatrixVector(VxVector *ResultVector, const VxMatrix &Mat, const VxVector *Vector) {
#if defined(VX_SIMD_SSE)
    VxSIMDMultiplyMatrixVector(ResultVector, &Mat, Vector);
#else
    const float vx = Vector->x;
    const float vy = Vector->y;
    const float vz = Vector->z;

    const float *m0 = Mat[0];
    const float *m1 = Mat[1];
    const float *m2 = Mat[2];
    const float *m3 = Mat[3];

    ResultVector->x = vx * m0[0] + vy * m1[0] + vz * m2[0] + m3[0];
    ResultVector->y = vx * m0[1] + vy * m1[1] + vz * m2[1] + m3[1];
    ResultVector->z = vx * m0[2] + vy * m1[2] + vz * m2[2] + m3[2];
#endif
}

inline void Vx3DRotateVector(VxVector *ResultVector, const VxMatrix &Mat, const VxVector *Vector) {
#if defined(VX_SIMD_SSE)
    VxSIMDRotateVectorM(ResultVector, &Mat, Vector);
#else
    const float vx = Vector->x;
    const float vy = Vector->y;
    const float vz = Vector->z;

    const float *m0 = Mat[0];
    const float *m1 = Mat[1];
    const float *m2 = Mat[2];

    ResultVector->x = vx * m0[0] + vy * m1[0] + vz * m2[0];
    ResultVector->y = vx * m0[1] + vy * m1[1] + vz * m2[1];
    ResultVector->z = vx * m0[2] + vy * m1[2] + vz * m2[2];
#endif
}

inline void VxVector::Rotate(const VxMatrix &M) {
#if defined(VX_SIMD_SSE)
    VxVector result;
    VxSIMDRotateVectorM(&result, &M, this);
    *this = result;
#else
    const float ox = x, oy = y, oz = z;

    const float m00 = M[0][0], m10 = M[1][0], m20 = M[2][0];
    const float m01 = M[0][1], m11 = M[1][1], m21 = M[2][1];
    const float m02 = M[0][2], m12 = M[1][2], m22 = M[2][2];

    x = m00 * ox + m10 * oy + m20 * oz;
    y = m01 * ox + m11 * oy + m21 * oz;
    z = m02 * ox + m12 * oy + m22 * oz;
#endif
}

inline const VxVector Rotate(const VxMatrix &mat, const VxVector &pt) {
#if defined(VX_SIMD_SSE)
    VxVector result;
    VxSIMDRotateVectorM(&result, &mat, &pt);
    return result;
#else
    const float m00 = mat[0][0], m10 = mat[1][0], m20 = mat[2][0];
    const float m01 = mat[0][1], m11 = mat[1][1], m21 = mat[2][1];
    const float m02 = mat[0][2], m12 = mat[1][2], m22 = mat[2][2];

    return VxVector(
        m00 * pt.x + m10 * pt.y + m20 * pt.z,
        m01 * pt.x + m11 * pt.y + m21 * pt.z,
        m02 * pt.x + m12 * pt.y + m22 * pt.z
    );
#endif
}

// ---------- Core matrix operations ----------

inline void Vx3DMatrixIdentity(VxMatrix &Mat) { Mat.SetIdentity(); }

inline void Vx3DMultiplyMatrixVector4(VxVector4 *ResultVector, const VxMatrix &Mat, const VxVector4 *Vector) {
#if defined(VX_SIMD_SSE)
    VxSIMDMultiplyMatrixVector4(ResultVector, &Mat, Vector);
#else
    const float vx = Vector->x, vy = Vector->y, vz = Vector->z, vw = Vector->w;
    const float *m0 = Mat[0], *m1 = Mat[1], *m2 = Mat[2], *m3 = Mat[3];
    ResultVector->x = vx*m0[0] + vy*m1[0] + vz*m2[0] + vw*m3[0];
    ResultVector->y = vx*m0[1] + vy*m1[1] + vz*m2[1] + vw*m3[1];
    ResultVector->z = vx*m0[2] + vy*m1[2] + vz*m2[2] + vw*m3[2];
    ResultVector->w = vx*m0[3] + vy*m1[3] + vz*m2[3] + vw*m3[3];
#endif
}

inline void Vx3DMultiplyMatrixVector4(VxVector4 *ResultVector, const VxMatrix &Mat, const VxVector *Vector) {
#if defined(VX_SIMD_SSE)
    VxSIMDMultiplyMatrixVector4FromVector3(ResultVector, &Mat, Vector);
#else
    const float vx = Vector->x, vy = Vector->y, vz = Vector->z;
    const float *m0 = Mat[0], *m1 = Mat[1], *m2 = Mat[2], *m3 = Mat[3];
    ResultVector->x = vx*m0[0] + vy*m1[0] + vz*m2[0] + m3[0];
    ResultVector->y = vx*m0[1] + vy*m1[1] + vz*m2[1] + m3[1];
    ResultVector->z = vx*m0[2] + vy*m1[2] + vz*m2[2] + m3[2];
    ResultVector->w = vx*m0[3] + vy*m1[3] + vz*m2[3] + m3[3];
#endif
}

inline void Vx3DMultiplyMatrix(VxMatrix &ResultMat, const VxMatrix &MatA, const VxMatrix &MatB) {
#if defined(VX_SIMD_SSE)
    VxSIMDMultiplyMatrixM(&ResultMat, &MatA, &MatB);
#else
    VxMatrix temp;
    for (int i = 0; i < 4; i++) {
        const float bi0=MatB[i][0], bi1=MatB[i][1], bi2=MatB[i][2], bi3=MatB[i][3];
        for (int j = 0; j < 4; j++)
            temp[i][j] = MatA[0][j]*bi0 + MatA[1][j]*bi1 + MatA[2][j]*bi2 + MatA[3][j]*bi3;
    }
    temp[0][3] = 0.0f; temp[1][3] = 0.0f; temp[2][3] = 0.0f; temp[3][3] = 1.0f;
    ResultMat = temp;
#endif
}

inline void Vx3DMultiplyMatrix4(VxMatrix &ResultMat, const VxMatrix &MatA, const VxMatrix &MatB) {
#if defined(VX_SIMD_SSE)
    VxSIMDMultiplyMatrixM4(&ResultMat, &MatA, &MatB);
#else
    VxMatrix temp;
    for (int i = 0; i < 4; i++) {
        const float bi0=MatB[i][0], bi1=MatB[i][1], bi2=MatB[i][2], bi3=MatB[i][3];
        for (int j = 0; j < 4; j++)
            temp[i][j] = MatA[0][j]*bi0 + MatA[1][j]*bi1 + MatA[2][j]*bi2 + MatA[3][j]*bi3;
    }
    ResultMat = temp;
#endif
}

inline void Vx3DInverseMatrix(VxMatrix &InverseMat, const VxMatrix &Mat) {
    if (VxTryLegacyInverse(&InverseMat[0][0], &Mat[0][0])) return;
#if defined(VX_SIMD_SSE)
    VxSIMDInverseAffineMatrix(&InverseMat, &Mat);
#else
    const float a00=Mat[0][0], a01=Mat[0][1], a02=Mat[0][2];
    const float a10=Mat[1][0], a11=Mat[1][1], a12=Mat[1][2];
    const float a20=Mat[2][0], a21=Mat[2][1], a22=Mat[2][2];
    const float minor1 = a11*a22 - a12*a21;
    const float minor2 = a12*a20 - a10*a22;
    const float minor3 = a10*a21 - a11*a20;
    const double det = a00*minor1 + a01*minor2 + a02*minor3;
    if (fabs(det) < EPSILON) { InverseMat.SetIdentity(); return; }
    const double id = 1.0 / det;
    InverseMat[0][0] = static_cast<float>((a11*a22 - a12*a21)*id);
    InverseMat[0][1] = static_cast<float>((a02*a21 - a01*a22)*id);
    InverseMat[0][2] = static_cast<float>((a01*a12 - a02*a11)*id);
    InverseMat[0][3] = 0.0f;
    InverseMat[1][0] = static_cast<float>((a12*a20 - a10*a22)*id);
    InverseMat[1][1] = static_cast<float>((a00*a22 - a02*a20)*id);
    InverseMat[1][2] = static_cast<float>((a02*a10 - a00*a12)*id);
    InverseMat[1][3] = 0.0f;
    InverseMat[2][0] = static_cast<float>((a10*a21 - a11*a20)*id);
    InverseMat[2][1] = static_cast<float>((a01*a20 - a00*a21)*id);
    InverseMat[2][2] = static_cast<float>((a00*a11 - a01*a10)*id);
    InverseMat[2][3] = 0.0f;
    const float tx=Mat[3][0], ty=Mat[3][1], tz=Mat[3][2];
    InverseMat[3][0] = -(InverseMat[0][0]*tx + InverseMat[1][0]*ty + InverseMat[2][0]*tz);
    InverseMat[3][1] = -(InverseMat[0][1]*tx + InverseMat[1][1]*ty + InverseMat[2][1]*tz);
    InverseMat[3][2] = -(InverseMat[0][2]*tx + InverseMat[1][2]*ty + InverseMat[2][2]*tz);
    InverseMat[3][3] = 1.0f;
#endif
}

inline float Vx3DMatrixDeterminant(const VxMatrix &Mat) {
#if defined(VX_SIMD_SSE)
    return VxSIMDDeterminant3x3(&Mat);
#else
    const float a00=Mat[0][0], a01=Mat[0][1], a02=Mat[0][2];
    const float a10=Mat[1][0], a11=Mat[1][1], a12=Mat[1][2];
    const float a20=Mat[2][0], a21=Mat[2][1], a22=Mat[2][2];
    return a00*(a11*a22-a12*a21) + a01*(a12*a20-a10*a22) + a02*(a10*a21-a11*a20);
#endif
}

inline void Vx3DTransposeMatrix(VxMatrix &Result, const VxMatrix &A) {
#if defined(VX_SIMD_SSE)
    VxSIMDTransposeMatrixM(&Result, &A);
#else
    VxMatrix temp;
    temp[0][0]=A[0][0]; temp[0][1]=A[1][0]; temp[0][2]=A[2][0]; temp[0][3]=A[3][0];
    temp[1][0]=A[0][1]; temp[1][1]=A[1][1]; temp[1][2]=A[2][1]; temp[1][3]=A[3][1];
    temp[2][0]=A[0][2]; temp[2][1]=A[1][2]; temp[2][2]=A[2][2]; temp[2][3]=A[3][2];
    temp[3][0]=A[0][3]; temp[3][1]=A[1][3]; temp[3][2]=A[2][3]; temp[3][3]=A[3][3];
    Result = temp;
#endif
}

// ---------- VxQuaternion methods needing VxMatrix ----------

inline VxQuaternion Vx3DQuaternionFromMatrix(const VxMatrix &Mat) {
    VxQuaternion quat;
    quat.FromMatrix(Mat, TRUE, TRUE);
    return quat;
}

namespace VxQuaternionDetail {
inline void NormalizeRotationAxis(VxVector &axis) {
    // Finite float squares fit in double even when float arithmetic would
    // underflow or overflow. Zero/nonfinite axes retain their NaN behavior.
    const double inverse = 1.0/LengthWide(axis.x, axis.y, axis.z);
    axis.Set(static_cast<float>(axis.x*inverse),
             static_cast<float>(axis.y*inverse),
             static_cast<float>(axis.z*inverse));
}

inline float CrossComponentWide(float a, float b, float c, float d) {
    return static_cast<float>(static_cast<double>(a)*b - static_cast<double>(c)*d);
}
} // namespace VxQuaternionDetail

inline void Vx3DNormalizeRotationRows(const VxMatrix &in, VxMatrix &out) {
    out = in;
    VxVector row0(in[0][0], in[0][1], in[0][2]);
    VxVector row1(in[1][0], in[1][1], in[1][2]);
    VxQuaternionDetail::NormalizeRotationAxis(row0);
    VxQuaternionDetail::NormalizeRotationAxis(row1);
    // Keep tiny products until subtraction and the final float store.
    const VxVector row2(VxQuaternionDetail::CrossComponentWide(row1.z, row0.y, row0.z, row1.y),
                        VxQuaternionDetail::CrossComponentWide(row0.z, row1.x, row0.x, row1.z),
                        VxQuaternionDetail::CrossComponentWide(row0.x, row1.y, row1.x, row0.y));
    out[0][0] = row0.x; out[0][1] = row0.y; out[0][2] = row0.z;
    out[1][0] = row1.x; out[1][1] = row1.y; out[1][2] = row1.z;
    out[2][0] = row2.x; out[2][1] = row2.y; out[2][2] = row2.z;
}

inline void VxQuaternion::FromMatrix(const VxMatrix &Mat, XBOOL MatIsUnit, XBOOL RestoreMat) {
    if (VxTryLegacyQuaternionFromMatrix(&x, &Mat[0][0], MatIsUnit, RestoreMat)) return;
    const VxMatrix *matrix = &Mat;
    VxMatrix normalized;
    if (!MatIsUnit) {
        Vx3DNormalizeRotationRows(Mat, normalized);
        matrix = &normalized;
        // The legacy const-reference API leaves normalized rotation rows in
        // the caller's matrix when RestoreMat is false (0x2429C420).
        if (!RestoreMat) {
            VxMatrix &mutableMat = const_cast<VxMatrix &>(Mat);
            for (int row = 0; row < 3; ++row)
                for (int column = 0; column < 3; ++column)
                    mutableMat[row][column] = normalized[row][column];
        }
    }

    // 0x2429C420 sums yy,zz,xx and keeps the intermediates in x87 registers.
    // A finite matrix can overflow a float sum while its quaternion is finite.
    const double trace = static_cast<double>((*matrix)[1][1]) + (*matrix)[2][2] + (*matrix)[0][0];
    if (trace >= 0.0) {
        const double root = std::sqrt(trace + 1.0);
        w = static_cast<float>(root * 0.5);
        const double s = 0.5 / root;
        x = static_cast<float>((static_cast<double>((*matrix)[2][1]) - (*matrix)[1][2]) * s);
        y = static_cast<float>((static_cast<double>((*matrix)[0][2]) - (*matrix)[2][0]) * s);
        z = static_cast<float>((static_cast<double>((*matrix)[1][0]) - (*matrix)[0][1]) * s);
    } else {
        int i = 0;
        if ((*matrix)[1][1] > (*matrix)[0][0]) i = 1;
        if ((*matrix)[2][2] > (*matrix)[i][i]) i = 2;
        static const int next[3] = {1, 2, 0};
        int j = next[i];
        int k = next[j];
        const double other = static_cast<double>((*matrix)[k][k]) + (*matrix)[j][j];
        const double radicand = ((*matrix)[i][i] - other) + 1.0;
        const double root = std::sqrt(radicand);
        float *q[4] = {&x, &y, &z, &w};
        *q[i] = static_cast<float>(root * 0.5);
        const double s = 0.5 / root;
        *q[j] = static_cast<float>((static_cast<double>((*matrix)[j][i]) + (*matrix)[i][j]) * s);
        *q[k] = static_cast<float>((static_cast<double>((*matrix)[k][i]) + (*matrix)[i][k]) * s);
        *q[3] = static_cast<float>((static_cast<double>((*matrix)[k][j]) - (*matrix)[j][k]) * s);
    }
}

inline void VxQuaternion::ToMatrix(VxMatrix &Mat) const {
    // Keep both the norm and scaled products wide. This is invariant under
    // finite nonzero quaternion scaling across the entire float range.
    const double dx = x, dy = y, dz = z, dw = w;
    const double s = 2.0 / VxQuaternionDetail::SquaredLengthWide(x, y, z, w);
    const double xs = dx*s, ys = dy*s, zs = dz*s;
    const double wx = dw*xs, wy = dw*ys, wz = dw*zs;
    const double xx = dx*xs, xy = dx*ys, xz = dx*zs;
    const double yy = dy*ys, yz = dy*zs, zz = dz*zs;
    Mat[0][0] = static_cast<float>(1.0 - (yy+zz));
    Mat[0][1] = static_cast<float>(xy-wz);
    Mat[0][2] = static_cast<float>(xz+wy); Mat[0][3] = 0.0f;
    Mat[1][0] = static_cast<float>(xy+wz);
    Mat[1][1] = static_cast<float>(1.0 - (zz+xx));
    Mat[1][2] = static_cast<float>(yz-wx); Mat[1][3] = 0.0f;
    Mat[2][0] = static_cast<float>(xz-wy);
    Mat[2][1] = static_cast<float>(yz+wx);
    Mat[2][2] = static_cast<float>(1.0 - (yy+xx)); Mat[2][3] = 0.0f;
    Mat[3][0] = Mat[3][1] = Mat[3][2] = 0.0f; Mat[3][3] = 1.0f;
}

inline void VxQuaternion::FromRotation(const VxVector &Vector, float Angle) {
    VxMatrix Mat;
    Vx3DMatrixFromRotation(Mat, Vector, Angle);
    FromMatrix(Mat, TRUE, TRUE);
}

inline void VxQuaternion::FromEulerAngles(float eax, float eay, float eaz) {
    VxMatrix Mat;
    Vx3DMatrixFromEulerAngles(Mat, eax, eay, eaz);
    FromMatrix(Mat, TRUE, TRUE);
}

inline void VxQuaternion::ToEulerAngles(float *eax, float *eay, float *eaz) const {
    VxMatrix Mat;
    ToMatrix(Mat);
    Vx3DMatrixToEulerAngles(Mat, eax, eay, eaz);
}

// ---------- Batch matrix-vector transforms ----------

inline void Vx3DMultiplyMatrixVectorMany(VxVector *ResultVectors, const VxMatrix &Mat, const VxVector *Vectors, int count, int stride) {
#if defined(VX_SIMD_SSE)
    VxSIMDMultiplyMatrixVectorMany(ResultVectors, &Mat, Vectors, count, stride);
#else
    if (count <= 0) return;
    const float m00=Mat[0][0], m01=Mat[0][1], m02=Mat[0][2];
    const float m10=Mat[1][0], m11=Mat[1][1], m12=Mat[1][2];
    const float m20=Mat[2][0], m21=Mat[2][1], m22=Mat[2][2];
    const float m30=Mat[3][0], m31=Mat[3][1], m32=Mat[3][2];
    const char *srcPtr = reinterpret_cast<const char *>(Vectors);
    char *dstPtr = reinterpret_cast<char *>(ResultVectors);
    int blockCount = count / 4, remainder = count % 4;
    for (int block = 0; block < blockCount; ++block) {
        for (int i = 0; i < 4; ++i) {
            const VxVector *vec = reinterpret_cast<const VxVector *>(srcPtr + (block*4+i)*stride);
            VxVector *result   = reinterpret_cast<VxVector *>(dstPtr + (block*4+i)*stride);
            const float vx=vec->x, vy=vec->y, vz=vec->z;
            result->x = vx*m00 + vy*m10 + vz*m20 + m30;
            result->y = vx*m01 + vy*m11 + vz*m21 + m31;
            result->z = vx*m02 + vy*m12 + vz*m22 + m32;
        }
    }
    for (int i = 0; i < remainder; ++i) {
        const VxVector *vec = reinterpret_cast<const VxVector *>(srcPtr + (blockCount*4+i)*stride);
        VxVector *result   = reinterpret_cast<VxVector *>(dstPtr + (blockCount*4+i)*stride);
        const float vx=vec->x, vy=vec->y, vz=vec->z;
        result->x = vx*m00 + vy*m10 + vz*m20 + m30;
        result->y = vx*m01 + vy*m11 + vz*m21 + m31;
        result->z = vx*m02 + vy*m12 + vz*m22 + m32;
    }
#endif
}

inline void Vx3DRotateVectorMany(VxVector *ResultVector, const VxMatrix &Mat, const VxVector *Vector, int count, int stride) {
#if defined(VX_SIMD_SSE)
    VxSIMDRotateVectorMany(ResultVector, &Mat, Vector, count, stride);
#else
    if (count <= 0) return;
    const float m00=Mat[0][0], m01=Mat[0][1], m02=Mat[0][2];
    const float m10=Mat[1][0], m11=Mat[1][1], m12=Mat[1][2];
    const float m20=Mat[2][0], m21=Mat[2][1], m22=Mat[2][2];
    const char *srcPtr = reinterpret_cast<const char *>(Vector);
    char *dstPtr = reinterpret_cast<char *>(ResultVector);
    for (int i = 0; i < count; i += 4) {
        int remaining = (count - i < 4) ? count - i : 4;
        for (int j = 0; j < remaining; ++j) {
            const VxVector *vec = reinterpret_cast<const VxVector *>(srcPtr + (i+j)*stride);
            VxVector *result   = reinterpret_cast<VxVector *>(dstPtr + (i+j)*stride);
            const float vx=vec->x, vy=vec->y, vz=vec->z;
            result->x = vx*m00 + vy*m10 + vz*m20;
            result->y = vx*m01 + vy*m11 + vz*m21;
            result->z = vx*m02 + vy*m12 + vz*m22;
        }
    }
#endif
}

inline void Vx3DMultiplyMatrixVectorStrided(VxStridedData *Dest, VxStridedData *Src, const VxMatrix &Mat, int count) {
#if defined(VX_SIMD_SSE)
    VxSIMDMultiplyMatrixVectorStrided(Dest, Src, &Mat, count);
#else
    if (!Dest || !Src || !Dest->Ptr || !Src->Ptr || count <= 0) return;
    const float m00=Mat[0][0], m01=Mat[0][1], m02=Mat[0][2];
    const float m10=Mat[1][0], m11=Mat[1][1], m12=Mat[1][2];
    const float m20=Mat[2][0], m21=Mat[2][1], m22=Mat[2][2];
    const float m30=Mat[3][0], m31=Mat[3][1], m32=Mat[3][2];
    const char *srcPtr = static_cast<const char *>(Src->Ptr);
    char *destPtr = static_cast<char *>(Dest->Ptr);
    for (int i = 0; i < count; ++i) {
        const VxVector *srcVec = reinterpret_cast<const VxVector *>(srcPtr);
        VxVector *destVec = reinterpret_cast<VxVector *>(destPtr);
        const float vx=srcVec->x, vy=srcVec->y, vz=srcVec->z;
        destVec->x = vx*m00 + vy*m10 + vz*m20 + m30;
        destVec->y = vx*m01 + vy*m11 + vz*m21 + m31;
        destVec->z = vx*m02 + vy*m12 + vz*m22 + m32;
        srcPtr += Src->Stride;
        destPtr += Dest->Stride;
    }
#endif
}

inline void Vx3DMultiplyMatrixVector4Strided(VxStridedData *Dest, VxStridedData *Src, const VxMatrix &Mat, int count) {
#if defined(VX_SIMD_SSE)
    VxSIMDMultiplyMatrixVector4Strided(Dest, Src, &Mat, count);
#else
    if (!Dest || !Src || !Dest->Ptr || !Src->Ptr || count <= 0) return;
    const float *m0=Mat[0], *m1=Mat[1], *m2=Mat[2], *m3=Mat[3];
    const char *srcPtr = static_cast<const char *>(Src->Ptr);
    char *destPtr = static_cast<char *>(Dest->Ptr);
    for (int i = 0; i < count; ++i) {
        const VxVector4 *srcVec = reinterpret_cast<const VxVector4 *>(srcPtr);
        VxVector4 *destVec = reinterpret_cast<VxVector4 *>(destPtr);
        const float vx=srcVec->x, vy=srcVec->y, vz=srcVec->z, vw=srcVec->w;
        destVec->x = vx*m0[0] + vy*m1[0] + vz*m2[0] + vw*m3[0];
        destVec->y = vx*m0[1] + vy*m1[1] + vz*m2[1] + vw*m3[1];
        destVec->z = vx*m0[2] + vy*m1[2] + vz*m2[2] + vw*m3[2];
        destVec->w = vx*m0[3] + vy*m1[3] + vz*m2[3] + vw*m3[3];
        srcPtr += Src->Stride;
        destPtr += Dest->Stride;
    }
#endif
}

inline void Vx3DRotateVectorStrided(VxStridedData *Dest, VxStridedData *Src, const VxMatrix &Mat, int count) {
#if defined(VX_SIMD_SSE)
    VxSIMDRotateVectorStrided(Dest, Src, &Mat, count);
#else
    if (!Dest || !Src || !Dest->Ptr || !Src->Ptr || count <= 0) return;
    const float m00=Mat[0][0], m01=Mat[0][1], m02=Mat[0][2];
    const float m10=Mat[1][0], m11=Mat[1][1], m12=Mat[1][2];
    const float m20=Mat[2][0], m21=Mat[2][1], m22=Mat[2][2];
    const char *srcPtr = static_cast<const char *>(Src->Ptr);
    char *destPtr = static_cast<char *>(Dest->Ptr);
    for (int i = 0; i < count; ++i) {
        const VxVector *srcVec = reinterpret_cast<const VxVector *>(srcPtr);
        VxVector *destVec = reinterpret_cast<VxVector *>(destPtr);
        const float vx=srcVec->x, vy=srcVec->y, vz=srcVec->z;
        destVec->x = vx*m00 + vy*m10 + vz*m20;
        destVec->y = vx*m01 + vy*m11 + vz*m21;
        destVec->z = vx*m02 + vy*m12 + vz*m22;
        srcPtr += Src->Stride;
        destPtr += Dest->Stride;
    }
#endif
}

// ---------- Matrix rotation / Euler builders ----------

namespace VxMatrixRotationDetail {
inline float OriginCoordinate(float origin, float a, float x, float b, float y, float c, float z) {
    const float result = origin - ((a*x + b*y) + c*z);
    if (std::isfinite(result)) return result;
    // 0x2429B0E0 retains x87's exponent range until each coordinate write.
    // An aliased origin can grow enough for the next dot product to overflow
    // float even when the original sum has a well-defined sign or cancels.
    const double ax = static_cast<double>(a)*x;
    const double by = static_cast<double>(b)*y;
    const double cz = static_cast<double>(c)*z;
    const double sum = (ax+by)+cz;
    return static_cast<float>(origin-sum);
}
} // namespace VxMatrixRotationDetail

inline void Vx3DMatrixFromRotation(VxMatrix &ResultMat, const VxVector &Vector, float Angle) {
    float c, s;
    VxQuaternionDetail::RotationSinCos(Angle, s, c);
    const float t = 1.0f - c;
    VxVector axis = Vector;
    VxQuaternionDetail::NormalizeRotationAxis(axis);
    const float x = axis.x, y = axis.y, z = axis.z;
    const float xx=x*x, yy=y*y, zz=z*z;
    const float xy=x*y, xz=x*z, yz=y*z;
    const float xs=x*s, ys=y*s, zs=z*s;
    const float xyt=xy*t, xzt=xz*t, yzt=yz*t;
    ResultMat[0][0] = (1.0f-xx)*c+xx; ResultMat[0][1] = xyt-zs; ResultMat[0][2] = xzt+ys; ResultMat[0][3] = 0.0f;
    ResultMat[1][0] = xyt+zs; ResultMat[1][1] = (1.0f-yy)*c+yy; ResultMat[1][2] = yzt-xs; ResultMat[1][3] = 0.0f;
    ResultMat[2][0] = xzt-ys; ResultMat[2][1] = yzt+xs; ResultMat[2][2] = (1.0f-zz)*c+zz; ResultMat[2][3] = 0.0f;
    ResultMat[3][0] = 0.0f;    ResultMat[3][1] = 0.0f;    ResultMat[3][2] = 0.0f;    ResultMat[3][3] = 1.0f;
}

inline void Vx3DMatrixFromRotationAndOrigin(VxMatrix &ResultMat, const VxVector &Vector, const VxVector &Origin, float Angle) {
    VxMatrix rotation;
    Vx3DMatrixFromRotation(rotation, Vector, Angle);
    // The original leaves row 3 intact until the translation writes. Origin
    // can refer to that row, and each subsequent component sees prior writes.
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) ResultMat[i][j] = rotation[i][j];
    const float *r0=ResultMat[0], *r1=ResultMat[1], *r2=ResultMat[2];
    using VxMatrixRotationDetail::OriginCoordinate;
    ResultMat[3][0] = OriginCoordinate(Origin.x, r2[0], Origin.z, Origin.y, r1[0], r0[0], Origin.x);
    ResultMat[3][1] = OriginCoordinate(Origin.y, r0[1], Origin.x, Origin.z, r2[1], r1[1], Origin.y);
    ResultMat[3][2] = OriginCoordinate(Origin.z, Origin.y, r1[2], Origin.z, r2[2], r0[2], Origin.x);
    ResultMat[0][3] = ResultMat[1][3] = ResultMat[2][3] = 0.0f;
    ResultMat[3][3] = 1.0f;
}

inline void Vx3DMatrixFromEulerAngles(VxMatrix &Mat, float eax, float eay, float eaz) {
    float cx, sx, cy, sy, cz, sz;
    if (fabsf(eax) <= 1e-8f) { cx = 1.0f; sx = 0.0f; } else { VxQuaternionDetail::RotationSinCos(eax, sx, cx); }
    if (fabsf(eay) <= 1e-8f) { cy = 1.0f; sy = 0.0f; } else { VxQuaternionDetail::RotationSinCos(eay, sy, cy); }
    if (fabsf(eaz) <= 1e-8f) { cz = 1.0f; sz = 0.0f; } else { VxQuaternionDetail::RotationSinCos(eaz, sz, cz); }
    // 0x2429B280 stores the x/z products before multiplying by sin(y).
    // Reassociation changes overflow and zero products for large angles.
    const float cxcz=cx*cz, cxsz=cx*sz, sxcz=sx*cz, sxsz=sx*sz;
    Mat[0][0] = cz*cy;            Mat[0][1] = sz*cy;            Mat[0][2] = -sy;    Mat[0][3] = 0.0f;
    Mat[1][0] = sxcz*sy - cxsz;   Mat[1][1] = sxsz*sy + cxcz;   Mat[1][2] = sx*cy;  Mat[1][3] = 0.0f;
    Mat[2][0] = cxcz*sy + sxsz;   Mat[2][1] = cxsz*sy - sxcz;   Mat[2][2] = cx*cy;  Mat[2][3] = 0.0f;
    Mat[3][0] = 0.0f;              Mat[3][1] = 0.0f;              Mat[3][2] = 0.0f;   Mat[3][3] = 1.0f;
}

inline void Vx3DMatrixToEulerAngles(const VxMatrix &Mat, float *eax, float *eay, float *eaz) {
    const float m00=Mat[0][0], m01=Mat[0][1], m02=Mat[0][2];
    const float m12=Mat[1][2], m22=Mat[2][2], m21=Mat[2][1], m11=Mat[1][1];
    const float magnitude = sqrtf(m00*m00 + m01*m01);
    if (!(magnitude > 16.0f * EPSILON)) {
        if (eay) *eay = atan2f(-m02, magnitude);
        if (eax) *eax = atan2f(-m21, m11);
        if (eaz) *eaz = 0.0f;
    } else {
        if (eay) *eay = atan2f(-m02, magnitude);
        if (eax) *eax = atan2f(m12, m22);
        if (eaz) *eaz = atan2f(m01, m00);
    }
}

// ---------- Internal matrix decomposition helpers ----------

inline void Vx3DMatrixAdjoint(const VxMatrix &in, VxMatrix &out) {
    const float m00=in[0][0], m01=in[0][1], m02=in[0][2];
    const float m10=in[1][0], m11=in[1][1], m12=in[1][2];
    const float m20=in[2][0], m21=in[2][1], m22=in[2][2];
    out[0][0] = m11*m22 - m12*m21; out[1][0] = m12*m20 - m10*m22; out[2][0] = m10*m21 - m11*m20;
    out[0][1] = m02*m21 - m01*m22; out[1][1] = m00*m22 - m02*m20; out[2][1] = m01*m20 - m00*m21;
    out[0][2] = m01*m12 - m02*m11; out[1][2] = m02*m10 - m00*m12; out[2][2] = m00*m11 - m01*m10;
}

inline float Vx3DMatrixNorm(const VxMatrix &M, bool isOneNorm) {
    float maxNorm = 0.0f;
    for (int j = 0; j < 3; ++j) {
        float sum = 0.0f;
        for (int i = 0; i < 3; ++i)
            sum += fabsf(isOneNorm ? M[i][j] : M[j][i]);
        if (sum > maxNorm) maxNorm = sum;
    }
    return maxNorm;
}

namespace VxMatrixDecompositionDetail {
inline double Dot3(const float *a, const float *b) {
    return static_cast<double>(a[2])*b[2] + static_cast<double>(a[1])*b[1] + static_cast<double>(a[0])*b[0];
}

inline int MaxColumn(const VxMatrix &m) {
    float maximum = 0.0f;
    int column = -1;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            if (std::fabs(m[i][j]) > maximum) { maximum = std::fabs(m[i][j]); column = j; }
    return column;
}

inline void MakeReflector(float (&v)[3]) {
    double length = std::sqrt(Dot3(v, v));
    if (v[2] < 0.0f) length = -length;
    v[2] = static_cast<float>(v[2] + length);
    const double scale = std::sqrt(2.0 / Dot3(v, v));
    for (int i = 0; i < 3; ++i) v[i] = static_cast<float>(v[i] * scale);
}

inline void ReflectColumns(VxMatrix &m, const float (&u)[3]) {
    for (int j = 0; j < 3; ++j) {
        const double scale = static_cast<double>(m[2][j])*u[2] + static_cast<double>(m[1][j])*u[1] + static_cast<double>(m[0][j])*u[0];
        for (int i = 0; i < 3; ++i) m[i][j] = static_cast<float>(m[i][j] - scale*u[i]);
    }
}

inline void ReflectRows(VxMatrix &m, const float (&u)[3]) {
    for (int i = 0; i < 3; ++i) {
        const double scale = Dot3(u, m[i]);
        for (int j = 0; j < 3; ++j) m[i][j] = static_cast<float>(m[i][j] - scale*u[j]);
    }
}

inline void OrthogonalRank1(VxMatrix &m, VxMatrix &q) {
    // Preserve initialization order even when m and q alias, as they do in
    // original polar decomposition's singular fallback (0x2429B730).
    q.SetIdentity();
    const int column = MaxColumn(m);
    if (column < 0) return;
    float u[3] = {m[0][column], m[1][column], m[2][column]};
    MakeReflector(u);
    ReflectColumns(m, u);
    float v[3] = {m[2][0], m[2][1], m[2][2]};
    MakeReflector(v);
    ReflectRows(m, v);
    if (m[2][2] < 0.0f) q[2][2] = -1.0f;
    ReflectColumns(q, u);
    ReflectRows(q, v);
}

inline void OrthogonalRank2(VxMatrix &m, const VxMatrix &adjointTranspose, VxMatrix &q) {
    const int column = MaxColumn(adjointTranspose);
    if (column < 0) { OrthogonalRank1(m, q); return; }
    float u[3] = {adjointTranspose[0][column], adjointTranspose[1][column], adjointTranspose[2][column]};
    MakeReflector(u);
    ReflectColumns(m, u);
    float v[3] = {
        static_cast<float>(static_cast<double>(m[0][1])*m[1][2] - static_cast<double>(m[0][2])*m[1][1]),
        static_cast<float>(static_cast<double>(m[0][2])*m[1][0] - static_cast<double>(m[0][0])*m[1][2]),
        static_cast<float>(static_cast<double>(m[0][0])*m[1][1] - static_cast<double>(m[0][1])*m[1][0])};
    MakeReflector(v);
    ReflectRows(m, v);
    const double w = m[0][0], z = m[1][1];
    const float x = m[1][0], y = m[0][1];
    if (static_cast<double>(x)*y >= w*z) {
        const double c = z - w;
        const float s = x + y;
        const float inverse = static_cast<float>(1.0 / std::sqrt(static_cast<double>(s)*s + c*c));
        q[1][1] = static_cast<float>(c*inverse); q[0][0] = -q[1][1];
        q[1][0] = q[0][1] = s*inverse;
    } else {
        const double c = w + z;
        const float s = x - y;
        const float inverse = static_cast<float>(1.0 / std::sqrt(static_cast<double>(s)*s + c*c));
        q[0][0] = q[1][1] = static_cast<float>(c*inverse);
        q[1][0] = s*inverse; q[0][1] = -q[1][0];
    }
    q[0][2] = q[1][2] = q[2][0] = q[2][1] = 0.0f; q[2][2] = 1.0f;
    ReflectColumns(q, u);
    ReflectRows(q, v);
}
} // namespace VxMatrixDecompositionDetail

inline float Vx3DMatrixPolarDecomposition(const VxMatrix &M_in, VxMatrix &Q, VxMatrix &S) {
    using namespace VxMatrixDecompositionDetail;
    VxMatrix e;
    Vx3DTransposeMatrix(e, M_in);
    float oneNorm = Vx3DMatrixNorm(e, true), infNorm = Vx3DMatrixNorm(e, false);
    float determinant = 0.0f;
    for (;;) {
        VxMatrix adjointTranspose;
        for (int i = 0; i < 3; ++i) {
            const int j = (i+1)%3, k = (i+2)%3;
            for (int c = 0; c < 3; ++c) {
                const int a = (c+1)%3, b = (c+2)%3;
                adjointTranspose[i][c] = static_cast<float>(static_cast<double>(e[j][a])*e[k][b] - static_cast<double>(e[j][b])*e[k][a]);
            }
        }
        const double det = Dot3(e[0], adjointTranspose[0]);
        determinant = static_cast<float>(det);
        if (det == 0.0) { OrthogonalRank2(e, adjointTranspose, e); break; }
        const float adjOne = Vx3DMatrixNorm(adjointTranspose, true);
        const double adjInf = Vx3DMatrixNorm(adjointTranspose, false);
        const double gamma = std::sqrt(std::sqrt(adjInf*adjOne / (static_cast<double>(infNorm)*oneNorm)) / std::fabs(determinant));
        const float c1 = static_cast<float>(0.5*gamma);
        const double c2 = 0.5 / (gamma*determinant);
        VxMatrix difference;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) {
                const float previous = e[i][j];
                e[i][j] = static_cast<float>(static_cast<double>(c1)*previous + c2*adjointTranspose[i][j]);
                difference[i][j] = previous - e[i][j];
            }
        oneNorm = Vx3DMatrixNorm(e, true);
        infNorm = Vx3DMatrixNorm(e, false);
        if (Vx3DMatrixNorm(difference, true) <= oneNorm*1e-6) break;
        // No finite orthogonal factor exists for nonfinite intermediate state.
        if (!std::isfinite(oneNorm) || !std::isfinite(infNorm)) break;
    }
    Q.SetIdentity(); S.SetIdentity();
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            Q[i][j] = e[j][i];
            // The internal original MatrixMultiply is A*B, unlike the public
            // Vx3DMultiplyMatrix argument convention (B*A).
            S[i][j] = static_cast<float>(static_cast<double>(e[i][2])*M_in[2][j] + static_cast<double>(e[i][0])*M_in[0][j] + static_cast<double>(e[i][1])*M_in[1][j]);
        }
    for (int i = 0; i < 3; ++i)
        for (int j = i; j < 3; ++j) {
            const float symmetric = static_cast<float>((static_cast<double>(S[i][j]) + S[j][i])*0.5);
            S[i][j] = S[j][i] = symmetric;
        }
    return determinant;
}

inline VxVector Vx3DMatrixSpectralDecomposition(const VxMatrix &S_in, VxMatrix &U_out) {
    // 0x2429BC30 uses cyclic off-diagonals [yz,zx,xy] and sweeps 2,1,0.
    // The original DLL selects 24-bit x87 precision (control word 0x007f).
    // Round each operation to float: extra precision changes the basis chosen
    // for repeated eigenvalues, which is observable through Snuggle.
    U_out.SetIdentity();
    float diagonal[3] = {S_in[0][0], S_in[1][1], S_in[2][2]};
    float off[3] = {S_in[1][2], S_in[2][0], S_in[0][1]};
    for (int sweep = 0; sweep < 20; ++sweep) {
        if (std::fabs(off[2]) + std::fabs(off[1]) + std::fabs(off[0]) == 0.0f) break;
        for (int k = 2; k >= 0; --k) {
            const int p = (k+1)%3, q = (p+1)%3;
            const float magnitude = std::fabs(off[k]);
            if (!(magnitude > 0.0f)) continue;
            const float difference = diagonal[q] - diagonal[p];
            float tangent;
            if (magnitude*100.0f + std::fabs(difference) == std::fabs(difference)) tangent = off[k] / difference;
            else {
                const float theta = 0.5f / off[k] * difference;
                tangent = 1.0f / (std::sqrt(theta*theta + 1.0f) + std::fabs(theta));
                if (theta < 0.0f) tangent = -tangent;
            }
            const float cosine = 1.0f / std::sqrt(tangent*tangent + 1.0f);
            const float sine = cosine*tangent;
            const float tau = sine / (cosine+1.0f);
            const float shift = tangent*off[k];
            off[k] = 0.0f;
            diagonal[p] -= shift;
            diagonal[q] += shift;
            const float oldQ = off[q];
            off[q] = oldQ - (tau*oldQ + off[p])*sine;
            off[p] = (oldQ - tau*off[p])*sine + off[p];
            for (int row = 2; row >= 0; --row) {
                const float a = U_out[row][p], b = U_out[row][q];
                U_out[row][p] = a - (a*tau+b)*sine;
                U_out[row][q] = (a-b*tau)*sine+b;
            }
        }
    }
    return VxVector(diagonal[0], diagonal[1], diagonal[2]);
}

// ---------- Matrix decomposition functions ----------

inline void Vx3DDecomposeMatrix(const VxMatrix &A, VxQuaternion &Quat, VxVector &Pos, VxVector &Scale) {
    Pos = VxVector(A[3][0], A[3][1], A[3][2]);
    VxMatrix unitRows;
    Vx3DNormalizeRotationRows(A, unitRows);
    Quat.FromMatrix(unitRows, TRUE, TRUE);
    Scale.x = unitRows[0][0]*A[0][0] + unitRows[0][1]*A[0][1] + unitRows[0][2]*A[0][2];
    Scale.y = unitRows[1][0]*A[1][0] + unitRows[1][1]*A[1][1] + unitRows[1][2]*A[1][2];
    Scale.z = unitRows[2][0]*A[2][0] + unitRows[2][1]*A[2][1] + unitRows[2][2]*A[2][2];
}

inline float Vx3DDecomposeMatrixTotal(const VxMatrix &A, VxQuaternion &Quat, VxVector &Pos, VxVector &Scale, VxQuaternion &URot) {
    Pos = VxVector(A[3][0], A[3][1], A[3][2]);
    VxMatrix Q, S;
    float det = Vx3DMatrixPolarDecomposition(A, Q, S);
    if (det < 0.0f) {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                Q[i][j] = -Q[i][j];
        det = -1.0f;
    } else {
        det = 1.0f;
    }
    Quat = Vx3DQuaternionFromMatrix(Q);
    VxMatrix U;
    Scale = Vx3DMatrixSpectralDecomposition(S, U);
    URot = Vx3DQuaternionFromMatrix(U);
    // Snuggle rewrites its quaternion argument when exactly two scales are
    // equal. The original multiplies that rewritten value, so the stretch
    // axis no longer matches Scale; compose with the unmodified rotation.
    VxQuaternion snuggleInput = URot;
    VxQuaternion snuggleQuat = Vx3DQuaternionSnuggle(&snuggleInput, &Scale);
    URot = Vx3DQuaternionMultiply(URot, snuggleQuat);
    return det;
}

inline float Vx3DDecomposeMatrixTotalPtr(const VxMatrix &A, VxQuaternion *Quat, VxVector *Pos, VxVector *Scale, VxQuaternion *URot) {
    if (Pos) *Pos = VxVector(A[3][0], A[3][1], A[3][2]);
    if (!Quat && !Scale && !URot) return 1.0f;
    VxMatrix Q, S;
    float det = Vx3DMatrixPolarDecomposition(A, Q, S);
    if (det < 0.0f) {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                Q[i][j] = -Q[i][j];
        det = -1.0f;
    } else {
        det = 1.0f;
    }
    if (Quat) *Quat = Vx3DQuaternionFromMatrix(Q);
    if (Scale || URot) {
        VxMatrix U;
        VxVector tempScale = Vx3DMatrixSpectralDecomposition(S, U);
        VxQuaternion tempURot = Vx3DQuaternionFromMatrix(U);
        // Compose with the unmodified rotation; see Vx3DDecomposeMatrixTotal.
        VxQuaternion snuggleInput = tempURot;
        VxQuaternion snuggleQuat = Vx3DQuaternionSnuggle(&snuggleInput, &tempScale);
        if (URot) *URot = Vx3DQuaternionMultiply(tempURot, snuggleQuat);
        if (Scale) *Scale = tempScale;
    }
    return det;
}

// ---------- Matrix interpolation ----------

inline void Vx3DInterpolateMatrixNoScale(float step, VxMatrix &Res, const VxMatrix &A, const VxMatrix &B) {
    // Preserve the selected endpoint, including shear and singular matrices,
    // without letting the unused endpoint's decomposition contaminate it.
    if (step == 0.0f) { Res = A; return; }
    if (step == 1.0f) { Res = B; return; }
    VxQuaternion quatA, quatB;
    VxVector posA, posB, scaleA, scaleB;
    Vx3DDecomposeMatrix(A, quatA, posA, scaleA);
    Vx3DDecomposeMatrix(B, quatB, posB, scaleB);
    VxQuaternion rotation = Slerp(step, quatA, quatB);
    const VxVector position = Interpolate(step, posA, posB);
    const VxVector scale = Interpolate(step, scaleA, scaleB);
    rotation.Normalize();
    const double x=rotation.x, y=rotation.y, z=rotation.z, w=rotation.w;
    const double xx=x*x, yy=y*y, zz=z*z, ww=w*w, xy=y*x, wz=w*z;
    const float xz=static_cast<float>(z*x), wx=static_cast<float>(w*x);
    const float yz=static_cast<float>(z*y), wy=static_cast<float>(w*y);
    Res.SetIdentity();
    Res[0][0]=static_cast<float>(ww+xx-yy-zz); Res[0][1]=static_cast<float>(2*xy-2*wz); Res[0][2]=2*wy+2*xz;
    Res[1][0]=static_cast<float>(2*wz+2*xy); Res[1][1]=static_cast<float>(ww-xx+yy-zz); Res[1][2]=2*yz-2*wx;
    Res[2][0]=2*xz-2*wy; Res[2][1]=2*yz+2*wx; Res[2][2]=static_cast<float>(ww-xx-yy+zz);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) Res[i][j] *= scale[i];
    Res[3][0]=position.x; Res[3][1]=position.y; Res[3][2]=position.z;
}

inline void Vx3DInterpolateMatrix(float step, VxMatrix &Res, const VxMatrix &A, const VxMatrix &B) {
    // Both original exports alias 0x2429A9B0, including scale interpolation.
    Vx3DInterpolateMatrixNoScale(step, Res, A, B);
}

// =============================================================================
// Part 2 - SIMD-Accelerated Matrix Operations
// =============================================================================

#if defined(VX_SIMD_SSE)

#include "VxSIMD.h"

VX_SIMD_INLINE __m128 VxSIMDTransformVec3Rows(__m128 m0, __m128 m1, __m128 m2, __m128 m3, const VxVector *src) noexcept {
    const __m128 v = _mm_set_ps(0.0f, src->z, src->y, src->x);
    const __m128 v_x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
    const __m128 v_y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
    const __m128 v_z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));

    __m128 res = VX_FMADD_PS(m2, v_z, m3);
    res = VX_FMADD_PS(m1, v_y, res);
    res = VX_FMADD_PS(m0, v_x, res);
    return res;
}

VX_SIMD_INLINE __m128 VxSIMDRotateVec3Rows(__m128 m0, __m128 m1, __m128 m2, const VxVector *src) noexcept {
    const __m128 v = _mm_set_ps(0.0f, src->z, src->y, src->x);
    const __m128 v_x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
    const __m128 v_y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
    const __m128 v_z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));

    __m128 res = _mm_mul_ps(m2, v_z);
    res = VX_FMADD_PS(m1, v_y, res);
    res = VX_FMADD_PS(m0, v_x, res);
    return res;
}

VX_SIMD_INLINE XBOOL VxSIMDIsPureTranslationMatrix(const VxMatrix *mat) noexcept {
    const __m128 row0 = _mm_loadu_ps(&(*mat)[0][0]);
    const __m128 row1 = _mm_loadu_ps(&(*mat)[1][0]);
    const __m128 row2 = _mm_loadu_ps(&(*mat)[2][0]);
    const __m128 row3 = _mm_loadu_ps(&(*mat)[3][0]);

    const __m128 eq0 = _mm_cmpeq_ps(row0, VX_SIMD_IDENTITY_R0);
    const __m128 eq1 = _mm_cmpeq_ps(row1, VX_SIMD_IDENTITY_R1);
    const __m128 eq2 = _mm_cmpeq_ps(row2, VX_SIMD_IDENTITY_R2);
    const __m128 wEq = _mm_cmpeq_ps(
        _mm_shuffle_ps(row3, row3, _MM_SHUFFLE(3, 3, 3, 3)),
        _mm_set1_ps(1.0f));

    return (_mm_movemask_ps(eq0) == 0xF) &&
           (_mm_movemask_ps(eq1) == 0xF) &&
           (_mm_movemask_ps(eq2) == 0xF) &&
           ((_mm_movemask_ps(wEq) & 0x1) != 0);
}

VX_SIMD_INLINE void VxSIMDMultiplyMatrixM(VxMatrix *result, const VxMatrix *a, const VxMatrix *b) noexcept {
    if (VxSIMDIsPureTranslationMatrix(a) && VxSIMDIsPureTranslationMatrix(b)) {
        const float tx = (*a)[3][0] + (*b)[3][0];
        const float ty = (*a)[3][1] + (*b)[3][1];
        const float tz = (*a)[3][2] + (*b)[3][2];

        _mm_storeu_ps(&(*result)[0][0], VX_SIMD_IDENTITY_R0);
        _mm_storeu_ps(&(*result)[1][0], VX_SIMD_IDENTITY_R1);
        _mm_storeu_ps(&(*result)[2][0], VX_SIMD_IDENTITY_R2);
        _mm_storeu_ps(&(*result)[3][0], _mm_set_ps(1.0f, tz, ty, tx));
        return;
    }

    const float *ap = static_cast<const float *>(static_cast<const void *>(*a));
    const float *bp = static_cast<const float *>(static_cast<const void *>(*b));

    const __m128 a0 = _mm_loadu_ps(ap + 0);
    const __m128 a1 = _mm_loadu_ps(ap + 4);
    const __m128 a2 = _mm_loadu_ps(ap + 8);
    const __m128 a3 = _mm_loadu_ps(ap + 12);
    VxMatrix temp;
    VxMatrix *out = ((result == a) || (result == b)) ? &temp : result;

    for (int i = 0; i < 4; ++i) {
        const __m128 bRow = _mm_loadu_ps(bp + i * 4);
        const __m128 b_x = _mm_shuffle_ps(bRow, bRow, _MM_SHUFFLE(0, 0, 0, 0));
        const __m128 b_y = _mm_shuffle_ps(bRow, bRow, _MM_SHUFFLE(1, 1, 1, 1));
        const __m128 b_z = _mm_shuffle_ps(bRow, bRow, _MM_SHUFFLE(2, 2, 2, 2));
        const __m128 b_w = _mm_shuffle_ps(bRow, bRow, _MM_SHUFFLE(3, 3, 3, 3));

        __m128 res = VX_FMADD_PS(a2, b_z, _mm_mul_ps(a3, b_w));
        res = VX_FMADD_PS(a1, b_y, res);
        res = VX_FMADD_PS(a0, b_x, res);
        _mm_storeu_ps(&(*out)[i][0], res);
    }

    // Enforce 3D transformation constraints
    (*out)[0][3] = 0.0f;
    (*out)[1][3] = 0.0f;
    (*out)[2][3] = 0.0f;
    (*out)[3][3] = 1.0f;

    if (out != result) {
        *result = *out;
    }
}

VX_SIMD_INLINE float VxSIMDDeterminant3x3(const VxMatrix *mat) noexcept {
    const __m128 row0 = VxSIMDLoadFloat3(&(*mat)[0][0]);
    const __m128 row1 = VxSIMDLoadFloat3(&(*mat)[1][0]);
    const __m128 row2 = VxSIMDLoadFloat3(&(*mat)[2][0]);
    return _mm_cvtss_f32(VxSIMDDotProduct3(row0, VxSIMDCrossProduct3(row1, row2)));
}

VX_SIMD_INLINE void VxSIMDInverseAffineMatrix(VxMatrix *result, const VxMatrix *mat) noexcept {
    const __m128 row0 = VxSIMDLoadFloat3(&(*mat)[0][0]);
    const __m128 row1 = VxSIMDLoadFloat3(&(*mat)[1][0]);
    const __m128 row2 = VxSIMDLoadFloat3(&(*mat)[2][0]);

    __m128 col0 = VxSIMDCrossProduct3(row1, row2);
    __m128 col1 = VxSIMDCrossProduct3(row2, row0);
    __m128 col2 = VxSIMDCrossProduct3(row0, row1);

    const double det = static_cast<double>(_mm_cvtss_f32(VxSIMDDotProduct3(row0, col0)));
    if (fabs(det) < EPSILON) {
        VxSIMDMatrixIdentity(result);
        return;
    }

    const __m128 invDet = _mm_set1_ps(static_cast<float>(1.0 / det));
    col0 = _mm_mul_ps(col0, invDet);
    col1 = _mm_mul_ps(col1, invDet);
    col2 = _mm_mul_ps(col2, invDet);

    alignas(16) float c0[4];
    alignas(16) float c1[4];
    alignas(16) float c2[4];
    _mm_store_ps(c0, col0);
    _mm_store_ps(c1, col1);
    _mm_store_ps(c2, col2);

    (*result)[0][0] = c0[0]; (*result)[0][1] = c1[0]; (*result)[0][2] = c2[0]; (*result)[0][3] = 0.0f;
    (*result)[1][0] = c0[1]; (*result)[1][1] = c1[1]; (*result)[1][2] = c2[1]; (*result)[1][3] = 0.0f;
    (*result)[2][0] = c0[2]; (*result)[2][1] = c1[2]; (*result)[2][2] = c2[2]; (*result)[2][3] = 0.0f;

    const __m128 translation = VxSIMDLoadFloat3(&(*mat)[3][0]);
    (*result)[3][0] = -_mm_cvtss_f32(VxSIMDDotProduct3(col0, translation));
    (*result)[3][1] = -_mm_cvtss_f32(VxSIMDDotProduct3(col1, translation));
    (*result)[3][2] = -_mm_cvtss_f32(VxSIMDDotProduct3(col2, translation));
    (*result)[3][3] = 1.0f;
}

VX_SIMD_INLINE void VxSIMDMultiplyMatrixM4(VxMatrix *result, const VxMatrix *a, const VxMatrix *b) noexcept {
    const float *ap = static_cast<const float *>(static_cast<const void *>(*a));
    const float *bp = static_cast<const float *>(static_cast<const void *>(*b));

    const __m128 a0 = _mm_loadu_ps(ap + 0);
    const __m128 a1 = _mm_loadu_ps(ap + 4);
    const __m128 a2 = _mm_loadu_ps(ap + 8);
    const __m128 a3 = _mm_loadu_ps(ap + 12);

    VxMatrix temp;
    VxMatrix *out = ((result == a) || (result == b)) ? &temp : result;
    for (int i = 0; i < 4; ++i) {
        const __m128 bRow = _mm_loadu_ps(bp + i * 4);
        const __m128 b_x = _mm_shuffle_ps(bRow, bRow, _MM_SHUFFLE(0, 0, 0, 0));
        const __m128 b_y = _mm_shuffle_ps(bRow, bRow, _MM_SHUFFLE(1, 1, 1, 1));
        const __m128 b_z = _mm_shuffle_ps(bRow, bRow, _MM_SHUFFLE(2, 2, 2, 2));
        const __m128 b_w = _mm_shuffle_ps(bRow, bRow, _MM_SHUFFLE(3, 3, 3, 3));

        __m128 res = VX_FMADD_PS(a2, b_z, _mm_mul_ps(a3, b_w));
        res = VX_FMADD_PS(a1, b_y, res);
        res = VX_FMADD_PS(a0, b_x, res);
        _mm_storeu_ps(&(*out)[i][0], res);
    }

    if (out != result) {
        *result = *out;
    }
}

VX_SIMD_INLINE void VxSIMDTransposeMatrixM(VxMatrix *result, const VxMatrix *a) noexcept {
    __m128 r0 = _mm_loadu_ps((const float *) &(*a)[0][0]);
    __m128 r1 = _mm_loadu_ps((const float *) &(*a)[1][0]);
    __m128 r2 = _mm_loadu_ps((const float *) &(*a)[2][0]);
    __m128 r3 = _mm_loadu_ps((const float *) &(*a)[3][0]);

    _MM_TRANSPOSE4_PS(r0, r1, r2, r3);

    VxMatrix temp;
    _mm_storeu_ps((float *) &temp[0][0], r0);
    _mm_storeu_ps((float *) &temp[1][0], r1);
    _mm_storeu_ps((float *) &temp[2][0], r2);
    _mm_storeu_ps((float *) &temp[3][0], r3);

    *result = temp;
}

VX_SIMD_INLINE void VxSIMDMultiplyMatrixVector(VxVector *result, const VxMatrix *mat, const VxVector *v) noexcept {
    __m128 vec = VxSIMDLoadFloat3(&v->x);
    __m128 res = VxSIMDMatrixMultiplyVector3((const float *) &(*mat)[0][0], vec);
    VxSIMDStoreFloat3(&result->x, res);
}

VX_SIMD_INLINE void VxSIMDMultiplyMatrixVector4(VxVector4 *result, const VxMatrix *mat, const VxVector4 *v) noexcept {
    __m128 vec = VxSIMDLoadFloat4((const float *) v);
    __m128 res = VxSIMDMatrixMultiplyVector4((const float *) &(*mat)[0][0], vec);
    VxSIMDStoreFloat4((float *) result, res);
}

VX_SIMD_INLINE void VxSIMDMultiplyMatrixVector4FromVector3(VxVector4 *result, const VxMatrix *mat, const VxVector *v) noexcept {
    __m128 vec = _mm_setr_ps(v->x, v->y, v->z, 1.0f);
    __m128 res = VxSIMDMatrixMultiplyVector4((const float *) &(*mat)[0][0], vec);
    VxSIMDStoreFloat4((float *) result, res);
}

VX_SIMD_INLINE void VxSIMDMatrixIdentity(VxMatrix *mat) noexcept {
    _mm_storeu_ps(&(*mat)[0][0], VX_SIMD_IDENTITY_R0);
    _mm_storeu_ps(&(*mat)[1][0], VX_SIMD_IDENTITY_R1);
    _mm_storeu_ps(&(*mat)[2][0], VX_SIMD_IDENTITY_R2);
    _mm_storeu_ps(&(*mat)[3][0], VX_SIMD_IDENTITY_R3);
}

VX_SIMD_INLINE void VxSIMDRotateVectorM(VxVector *result, const VxMatrix *mat, const VxVector *v) noexcept {
    __m128 vec = VxSIMDLoadFloat3(&v->x);
    __m128 res = VxSIMDMatrixRotateVector3((const float *) &(*mat)[0][0], vec);
    VxSIMDStoreFloat3(&result->x, res);
}

VX_SIMD_INLINE void VxSIMDMultiplyMatrixVectorMany(VxVector *resultVectors, const VxMatrix *mat, const VxVector *vectors, int count, int stride) noexcept {
    if (!resultVectors || !vectors || count <= 0) return;
    const __m128 m0 = _mm_loadu_ps(&(*mat)[0][0]);
    const __m128 m1 = _mm_loadu_ps(&(*mat)[1][0]);
    const __m128 m2 = _mm_loadu_ps(&(*mat)[2][0]);
    const __m128 m3 = _mm_loadu_ps(&(*mat)[3][0]);

    if (stride == static_cast<int>(sizeof(VxVector))) {
        int i = 0;
        for (; i + 3 < count; i += 4) {
            __m128 r0 = VxSIMDTransformVec3Rows(m0, m1, m2, m3, vectors + i + 0);
            __m128 r1 = VxSIMDTransformVec3Rows(m0, m1, m2, m3, vectors + i + 1);
            __m128 r2 = VxSIMDTransformVec3Rows(m0, m1, m2, m3, vectors + i + 2);
            __m128 r3 = VxSIMDTransformVec3Rows(m0, m1, m2, m3, vectors + i + 3);
            VxSIMDStoreFloat3(&resultVectors[i + 0].x, r0);
            VxSIMDStoreFloat3(&resultVectors[i + 1].x, r1);
            VxSIMDStoreFloat3(&resultVectors[i + 2].x, r2);
            VxSIMDStoreFloat3(&resultVectors[i + 3].x, r3);
        }
        for (; i < count; ++i) {
            __m128 r = VxSIMDTransformVec3Rows(m0, m1, m2, m3, vectors + i);
            VxSIMDStoreFloat3(&resultVectors[i].x, r);
        }
        return;
    }

    const char *srcPtr = reinterpret_cast<const char *>(vectors);
    char *dstPtr = reinterpret_cast<char *>(resultVectors);
    for (int i = 0; i < count; ++i) {
        const VxVector *srcVec = reinterpret_cast<const VxVector *>(srcPtr);
        VxVector *dstVec = reinterpret_cast<VxVector *>(dstPtr);
        const __m128 r = VxSIMDTransformVec3Rows(m0, m1, m2, m3, srcVec);
        VxSIMDStoreFloat3(&dstVec->x, r);

        srcPtr += stride;
        dstPtr += stride;
    }
}

VX_SIMD_INLINE void VxSIMDRotateVectorMany(VxVector *resultVectors, const VxMatrix *mat, const VxVector *vectors, int count, int stride) noexcept {
    if (!resultVectors || !vectors || count <= 0) return;
    const __m128 m0 = _mm_loadu_ps(&(*mat)[0][0]);
    const __m128 m1 = _mm_loadu_ps(&(*mat)[1][0]);
    const __m128 m2 = _mm_loadu_ps(&(*mat)[2][0]);

    if (stride == static_cast<int>(sizeof(VxVector))) {
        int i = 0;
        for (; i + 3 < count; i += 4) {
            __m128 r0 = VxSIMDRotateVec3Rows(m0, m1, m2, vectors + i + 0);
            __m128 r1 = VxSIMDRotateVec3Rows(m0, m1, m2, vectors + i + 1);
            __m128 r2 = VxSIMDRotateVec3Rows(m0, m1, m2, vectors + i + 2);
            __m128 r3 = VxSIMDRotateVec3Rows(m0, m1, m2, vectors + i + 3);
            VxSIMDStoreFloat3(&resultVectors[i + 0].x, r0);
            VxSIMDStoreFloat3(&resultVectors[i + 1].x, r1);
            VxSIMDStoreFloat3(&resultVectors[i + 2].x, r2);
            VxSIMDStoreFloat3(&resultVectors[i + 3].x, r3);
        }
        for (; i < count; ++i) {
            __m128 r = VxSIMDRotateVec3Rows(m0, m1, m2, vectors + i);
            VxSIMDStoreFloat3(&resultVectors[i].x, r);
        }
        return;
    }

    const char *srcPtr = reinterpret_cast<const char *>(vectors);
    char *dstPtr = reinterpret_cast<char *>(resultVectors);
    for (int i = 0; i < count; ++i) {
        const VxVector *srcVec = reinterpret_cast<const VxVector *>(srcPtr);
        VxVector *dstVec = reinterpret_cast<VxVector *>(dstPtr);
        const __m128 r = VxSIMDRotateVec3Rows(m0, m1, m2, srcVec);
        VxSIMDStoreFloat3(&dstVec->x, r);

        srcPtr += stride;
        dstPtr += stride;
    }
}

VX_SIMD_INLINE void VxSIMDMultiplyMatrixVectorStrided(VxStridedData *dest, const VxStridedData *src, const VxMatrix *mat, int count) noexcept {
    if (!dest || !src || !dest->Ptr || !src->Ptr || count <= 0) return;
    if (src->Stride == sizeof(VxVector) && dest->Stride == sizeof(VxVector)) {
        VxSIMDMultiplyMatrixVectorMany(
            reinterpret_cast<VxVector *>(dest->Ptr),
            mat,
            reinterpret_cast<const VxVector *>(src->Ptr),
            count,
            static_cast<int>(sizeof(VxVector)));
        return;
    }

    const __m128 m0 = _mm_loadu_ps(&(*mat)[0][0]);
    const __m128 m1 = _mm_loadu_ps(&(*mat)[1][0]);
    const __m128 m2 = _mm_loadu_ps(&(*mat)[2][0]);
    const __m128 m3 = _mm_loadu_ps(&(*mat)[3][0]);

    const char *srcPtr = static_cast<const char *>(src->Ptr);
    char *destPtr = static_cast<char *>(dest->Ptr);
    for (int i = 0; i < count; ++i) {
        const VxVector *srcVec = reinterpret_cast<const VxVector *>(srcPtr);
        VxVector *dstVec = reinterpret_cast<VxVector *>(destPtr);
        const __m128 r = VxSIMDTransformVec3Rows(m0, m1, m2, m3, srcVec);
        VxSIMDStoreFloat3(&dstVec->x, r);

        srcPtr += src->Stride;
        destPtr += dest->Stride;
    }
}

VX_SIMD_INLINE void VxSIMDMultiplyMatrixVector4Strided(VxStridedData *dest, const VxStridedData *src, const VxMatrix *mat, int count) noexcept {
    if (!dest || !src || !dest->Ptr || !src->Ptr || count <= 0) return;
    const __m128 m0 = _mm_loadu_ps(&(*mat)[0][0]);
    const __m128 m1 = _mm_loadu_ps(&(*mat)[1][0]);
    const __m128 m2 = _mm_loadu_ps(&(*mat)[2][0]);
    const __m128 m3 = _mm_loadu_ps(&(*mat)[3][0]);

    const char *srcPtr = static_cast<const char *>(src->Ptr);
    char *destPtr = static_cast<char *>(dest->Ptr);
    for (int i = 0; i < count; ++i) {
        const VxVector4 *srcVec = reinterpret_cast<const VxVector4 *>(srcPtr);
        VxVector4 *dstVec = reinterpret_cast<VxVector4 *>(destPtr);

        const __m128 v = VxSIMDLoadFloat4(reinterpret_cast<const float *>(srcVec));
        const __m128 v_x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
        const __m128 v_y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
        const __m128 v_z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
        const __m128 v_w = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3));

        __m128 res = VX_FMADD_PS(m2, v_z, _mm_mul_ps(m3, v_w));
        res = VX_FMADD_PS(m1, v_y, res);
        res = VX_FMADD_PS(m0, v_x, res);
        VxSIMDStoreFloat4(reinterpret_cast<float *>(dstVec), res);

        srcPtr += src->Stride;
        destPtr += dest->Stride;
    }
}

VX_SIMD_INLINE void VxSIMDRotateVectorStrided(VxStridedData *dest, const VxStridedData *src, const VxMatrix *mat, int count) noexcept {
    if (!dest || !src || !dest->Ptr || !src->Ptr || count <= 0) return;
    if (src->Stride == sizeof(VxVector) && dest->Stride == sizeof(VxVector)) {
        VxSIMDRotateVectorMany(
            reinterpret_cast<VxVector *>(dest->Ptr),
            mat,
            reinterpret_cast<const VxVector *>(src->Ptr),
            count,
            static_cast<int>(sizeof(VxVector)));
        return;
    }

    const __m128 m0 = _mm_loadu_ps(&(*mat)[0][0]);
    const __m128 m1 = _mm_loadu_ps(&(*mat)[1][0]);
    const __m128 m2 = _mm_loadu_ps(&(*mat)[2][0]);

    const char *srcPtr = static_cast<const char *>(src->Ptr);
    char *destPtr = static_cast<char *>(dest->Ptr);
    for (int i = 0; i < count; ++i) {
        const VxVector *srcVec = reinterpret_cast<const VxVector *>(srcPtr);
        VxVector *dstVec = reinterpret_cast<VxVector *>(destPtr);
        const __m128 r = VxSIMDRotateVec3Rows(m0, m1, m2, srcVec);
        VxSIMDStoreFloat3(&dstVec->x, r);

        srcPtr += src->Stride;
        destPtr += dest->Stride;
    }
}

#endif // VX_SIMD_SSE
