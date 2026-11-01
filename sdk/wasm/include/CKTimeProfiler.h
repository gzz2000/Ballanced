#ifndef CKTIMEPROFILER_H
#define CKTIMEPROFILER_H

#include "CKContext.h"
#include "XArray.h"
#include "VxTimeProfiler.h"

#include <stdio.h>

/*************************************************
Name: CKTimeProfiler
Summary: Class for profiling purposes

See also:
*************************************************/
class CKTimeProfiler
{
public:
    /*************************************************
    Name: VxMultiTimeProfiler
    Summary: Starts profiling
    *************************************************/
    CKTimeProfiler(CKSTRING iTitle, CKContext *iContext, int iStartingCount = 4) : m_Title(iTitle), m_Context(iContext), m_Marks(iStartingCount) {}

    ~CKTimeProfiler()
    {
        char buffer[512];
        Dump(buffer);
        if (strlen(buffer))
            m_Context->OutputToConsoleEx("[%s] : %s", m_Title, buffer);
        else
            m_Context->OutputToConsoleEx("[%s] : %g", m_Title, m_Profiler.Current());
    }

    /*************************************************
    Summary: Restarts the timer
    *************************************************/
    void Reset()
    {
        m_Profiler.Reset();
        m_Marks.Resize(0);
    }

    /*************************************************
    Summary: add a split time with a name
    *************************************************/
    void operator()(CKSTRING iString)
    {
        Mark m;
        m.name = iString;
        m.time = m_Profiler.Current();
        m_Marks.PushBack(m);
        m_Profiler.Reset();
    }

    /*************************************************
    Summary: Restarts the timer
    *************************************************/
    void Dump(char *oBuffer, CKSTRING iSeparator = " | ")
    {
        char buffer[64];
        *oBuffer = '\0';

        float sum = 0.0f;

        for (XArray<Mark>::Iterator it = m_Marks.Begin(); it != m_Marks.End(); ++it)
        {

            sum += (*it).time;

            snprintf(buffer, sizeof(buffer), "%s = %.3g", (*it).name, (*it).time);
            strcat(oBuffer, buffer);

            if (it != (m_Marks.End() - 1)) // we don't add the separator for the last mark
                strcat(oBuffer, iSeparator);
        }

        if (sum != 0.0f)
        {
            snprintf(buffer, sizeof(buffer), "=> %g ms", sum);
            strcat(oBuffer, buffer);
        }
    }

protected:
    // types
    struct Mark
    {
        CKSTRING name;
        float time;
    };

    VxTimeProfiler m_Profiler;
    CKSTRING m_Title;
    CKContext *m_Context;
    XArray<Mark> m_Marks;
};

#endif // CKTIMEPROFILER_H
