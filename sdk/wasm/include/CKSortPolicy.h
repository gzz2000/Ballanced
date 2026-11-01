#ifndef CK_SORT_POLICY_H
#define CK_SORT_POLICY_H

#include "CKDefines.h"

// Reimplementation extension to CKCreateContext flags, not a retail SDK flag.
// Select before constructing the context: equal-priority permutations affect
// script loading and cannot be reconstructed by changing policy during play.
// Uses the Wine 11 msvcrt/UCRT qsort ordering validated against legacy TAS
// environments. This is not a promise of every Windows CRT's ordering.
// Stored in CKContext's existing start options; no object/layout/ABI change.
constexpr CKDWORD CK_CONFIG_WINE_MSVCRT_SORT = 0x10000000u;

#endif
