#pragma once

#include <cstddef>
#include <cstdint>
#include "CKTypes.h"
#include "VxMatrix.h"

class CKObject;

// Native in-memory state. No pointers or object-layout assumptions cross the
// module boundary. Restore requires the saved hierarchy to be established.
struct CKEntityTransformState {
    std::uint32_t version = 1;
    CK_ID object = 0;
    CK_ID parent = 0;
    std::uint32_t inverseValid = 0;
    std::uint32_t hasLastFrame = 0;
    std::uint64_t inverseGeneration = 0;
    VxMatrix local;
    VxMatrix world;
    VxMatrix inverse;
    VxMatrix lastFrame;
};

extern "C" int CKCaptureEntityTransformState(
    CKObject *, CKEntityTransformState *, std::size_t);
extern "C" int CKRestoreEntityTransformState(
    CKObject *, const CKEntityTransformState *, std::size_t);
