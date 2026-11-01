#ifndef CKOBJECTLIFETIME_H
#define CKOBJECTLIFETIME_H

#include "CKDefines.h"
#include <cstdint>

class CKContext;

// Native extension; does not change retail object/manager layouts or CK_ID
// allocation. Call on the context's engine thread, like GetObject(). A token
// identifies one registration in this loaded CK2 instance, including across
// context teardown/recreation. Zero means absent, pending deletion or exhausted
// token space. Tokens are not serialized, restored, or used as simulation order.
extern "C" DLL_EXPORT std::uint64_t CKGetObjectLifetimeToken(CKContext *context, CK_ID id);

#endif
