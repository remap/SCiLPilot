//---------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Abstract:
//      Defines our media sink object in the MF topology
//
//---------------------------------------------------------------------------
#pragma once

#include <atomic>

namespace SVF
{
    //**********************************************************************
    // CSVFRefCount is the base class that manages the reference count on
    // all our SVF objects
    //**********************************************************************
    class CSVFRefCount
    {
    protected:
        std::atomic<uint32_t> m_RefCnt;
        std::atomic<bool> m_DestructionFlag;
    public:
        CSVFRefCount() : m_RefCnt(0), m_DestructionFlag(false)
        {
        }

        virtual ~CSVFRefCount()
        {
        }

        ULONG GetRefCount() const { return m_RefCnt; }

        virtual ULONG STDMETHODCALLTYPE AddRef()
        {
            _ASSERT(m_DestructionFlag == 0);
            return ++m_RefCnt;
        }

        virtual ULONG STDMETHODCALLTYPE Release()
        {
            uint32_t ref = --m_RefCnt;
            if (ref == 0)
            {
                bool expected = false;
                if (m_DestructionFlag.compare_exchange_strong(expected, true))
                {
                    delete this;
                }
            }
            return ref;
        }
    private:
    };
}
