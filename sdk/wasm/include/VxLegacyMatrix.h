#ifndef VX_LEGACY_MATRIX_H
#define VX_LEGACY_MATRIX_H
#include "VxMathDefines.h"
#include <cstdint>

// Optional arithmetic provider. The default path keeps host-native arithmetic.
// Select/unselect on the engine thread between simulation steps. Providers must
// unregister before unloading. Arrays contain 16 row-major binary32 components.
struct VxLegacyMatrixBackend {
    void (*inverse)(float *out, const float *matrix);
    void (*updateInverseTranslation)(float *inverse, const float *world);
};
VX_EXPORT void VxSetLegacyMatrixBackend(const VxLegacyMatrixBackend *backend);
VX_EXPORT std::uint64_t VxGetLegacyMatrixGeneration();
VX_EXPORT bool VxTryLegacyInverse(float *out, const float *matrix);
VX_EXPORT bool VxTryLegacyInverseTranslation(float *inverse, const float *world);
// Separate registration preserves the existing matrix-provider struct ABI.
// Uses the same engine-thread/lifetime contract; no normalization result cache.
using VxLegacyOrientationNormalizer = void (*)(const float *input, float *output);
VX_EXPORT void VxSetLegacyOrientationNormalizer(VxLegacyOrientationNormalizer normalize);
VX_EXPORT bool VxTryLegacyNormalizeOrientation(const float *input, float *output);
using VxLegacyVectorMagnitude = float (*)(const float *input);
VX_EXPORT void VxSetLegacyVectorMagnitude(VxLegacyVectorMagnitude magnitude);
VX_EXPORT bool VxTryLegacyVectorMagnitude(const float *input,float *output);
using VxLegacyQuaternionFromMatrix = void (*)(float *quaternion, const float *matrix,
                                             int matrixIsUnit, int restoreMatrix);
VX_EXPORT void VxSetLegacyQuaternionFromMatrix(VxLegacyQuaternionFromMatrix convert);
VX_EXPORT bool VxTryLegacyQuaternionFromMatrix(float *quaternion, const float *matrix,
                                               int matrixIsUnit, int restoreMatrix);
// TT Toolbox spring-follow update. All vectors are three binary32 components
// in the caller's coordinate system; only target is modified. Registration is
// separate so existing provider layouts and savestate structures stay intact.
using VxLegacyDynamicPosition = void (*)(float *target, const float *object,
    const float *previous, const float *force, const float *damping,
    const float *offset, float deltaTime, float maximumDistance);
VX_EXPORT void VxSetLegacyDynamicPosition(VxLegacyDynamicPosition update);
VX_EXPORT bool VxTryLegacyDynamicPosition(float *target, const float *object,
    const float *previous, const float *force, const float *damping,
    const float *offset, float deltaTime, float maximumDistance);
// Original entity transform multiplication uses binary32 SSE operation order,
// even when the physics solver uses x87. Inputs/output must be disjoint.
// Separate registration keeps the existing backend struct ABI unchanged.
using VxLegacyEntityMatrixMultiply = void (*)(float *out, const float *a, const float *b);
VX_EXPORT void VxSetLegacyEntityMatrixMultiply(VxLegacyEntityMatrixMultiply multiply);
// Returns false without touching out if no provider is selected or out aliases
// an input. Alias handling belongs to the caller's existing matrix path.
VX_EXPORT bool VxTryLegacyEntityMatrixMultiply(float *out, const float *a, const float *b);
// TT Extra has x87 arithmetic inside the behavior, separate from VxMath's
// vector API. Keep float storage and existing savestate layouts unchanged.
struct VxLegacyExtraBackend {
    void (*normalizeOrbit)(float *position);
    void (*rotationMatrix)(float *matrix, const float *axis, float angle);
    void (*rotateVector)(float *out, const float *matrix, const float *position);
    void (*move)(float *out, const float *position, const float *previous,
                 const float *target, float damping, float force,
                 float timeValue, int flyaway);
    int (*withinDistance)(const float *a, const float *b, float squaredThreshold);
};
VX_EXPORT void VxSetLegacyExtraBackend(const VxLegacyExtraBackend *backend);
VX_EXPORT bool VxTryLegacyExtraNormalize(float *position);
VX_EXPORT bool VxTryLegacyExtraRotation(float *matrix, const float *axis, float angle);
VX_EXPORT bool VxTryLegacyExtraRotateVector(float *out, const float *matrix, const float *position);
VX_EXPORT bool VxTryLegacyExtraMove(float *out, const float *position,
    const float *previous, const float *target, float damping, float force,
    float timeValue, int flyaway);
VX_EXPORT bool VxTryLegacyExtraWithinDistance(const float *a, const float *b,
    float squaredThreshold, bool *within);
#endif
