#ifndef CKMEMORYPOOL_H
#define CKMEMORYPOOL_H

#include "CKContext.h"
#include "VxMemoryPool.h"

class DLL_EXPORT CKMemoryPool
{
public:
    explicit CKMemoryPool(CKContext *Context, int ByteCount = 0);
    ~CKMemoryPool();

    void *Mem() const;

protected:
    CKContext *m_Context;
    void *m_Memory;
    int m_Index;
};

#endif // CKMEMORYPOOL_H