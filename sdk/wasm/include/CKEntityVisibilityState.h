#pragma once

#include <cstddef>
#include <cstdint>
#include "CKTypes.h"

class CKObject;

// Previous-render visibility is observable simulation input: TT Simple Shadow
// queries IsAllOutsideFrustrum before the next traversal updates it. Bounding
// boxes and traversal/sort caches are deliberately not part of this history.
struct CKEntityVisibilityState {
    std::uint32_t version = 1;
    CK_ID object = 0;
    std::uint32_t hasSceneGraphNode = 0;
    std::uint32_t frustumFlags = 0;
};

extern "C" int CKCaptureEntityVisibilityState(
    CKObject *, CKEntityVisibilityState *, std::size_t);
extern "C" int CKRestoreEntityVisibilityState(
    CKObject *, const CKEntityVisibilityState *, std::size_t);
