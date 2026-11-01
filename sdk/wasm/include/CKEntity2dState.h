#pragma once

#include <cstddef>
#include <cstdint>
#include "CKTypes.h"
#include "VxRect.h"

class CKObject;
class CKContext;

// In-memory layout, not a disk format. Screen and homogeneous coordinates are
// independent stored values: converting either back to the other loses bits.
struct CKEntity2dState {
    std::uint32_t version = 1;
    CK_ID object = 0;
    CK_ID parent = 0;
    CK_ID material = 0;
    CKDWORD flags = 0;
    int zOrder = 0;
    std::uint32_t hasHomogeneous = 0;
    VxRect rect{};
    VxRect homogeneous{};
    VxRect source{};
    VxRect vertices{};
    VxRect sourceCache{};
};

// Borrowed only for this call. Root children must be included even though
// their GetParent() returns null. Every participating entity has an entry.
struct CKEntity2dRestoreEntry {
    const CKEntity2dState *state;
    const CK_ID *children;
    std::size_t childCount;
};

extern "C" int CKCaptureEntity2dState(CKObject *, CKEntity2dState *, std::size_t);
extern "C" int CKRestoreEntity2dStates(CKContext *, const CKEntity2dRestoreEntry *,
                                       std::size_t count, std::size_t entryBytes);
