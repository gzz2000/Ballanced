#pragma once
#include <cstddef>
#include <cstdint>
#include "CKTypes.h"
#include "VxBbox.h"

class CKObject;

// A user-supplied local box is authored state, unlike a mesh-derived box.
// Capturing must not call GetBoundingBox(), which updates renderer caches.
struct CKEntityUserBoxState {
    std::uint32_t version = 1;
    CK_ID object = 0;
    std::uint32_t enabled = 0;
    VxBbox local;
};

extern "C" int CKCaptureEntityUserBoxState(CKObject *, CKEntityUserBoxState *, std::size_t);
extern "C" int CKRestoreEntityUserBoxState(CKObject *, const CKEntityUserBoxState *, std::size_t);
