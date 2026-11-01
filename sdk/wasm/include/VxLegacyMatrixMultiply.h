#pragma once

// Original nonaliased VxMath.dll!Vx3DMultiplyMatrix SSE path (RVA 0x40c0).
// E381/E382 validate operation order and signed zeros against the original DLL.
// Inputs/output must be disjoint. This is affine multiplication, not general 4x4.
inline void LegacyNonAliasedSSEMatrixMultiply(float *out, const float *a,
                                             const float *b) noexcept {
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            // Explicit stores also exclude fused operations and excess precision.
            volatile float p0 = a[col] * b[row * 4];
            volatile float p1 = a[4 + col] * b[row * 4 + 1];
            volatile float p2 = a[8 + col] * b[row * 4 + 2];
            volatile float s01 = row == 2 ? p1 + p0 : p0 + p1;
            if (row == 3) {
                volatile float s2t = p2 + a[12 + col];
                out[row * 4 + col] = s2t + s01;
            } else {
                out[row * 4 + col] = s01 + p2;
            }
        }
    }
    out[15] = 1.0f;
}
