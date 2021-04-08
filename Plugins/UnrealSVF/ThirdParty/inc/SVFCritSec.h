//---------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------
#pragma once

//**********************************************************************
// SVFCriticalSection defines a smart object for managing critical sections
//**********************************************************************
#pragma prefast(push)
#pragma prefast(disable:25071) // Stop prefast from complaining about m_cs in ctor body
class SVFCriticalSection
{
public:
    SVFCriticalSection()
    {
        InitializeCriticalSection(&m_cs);
    }
    ~SVFCriticalSection()
    {
        DeleteCriticalSection(&m_cs);
    }

    _Acquires_lock_(this->m_cs)
        void Lock()
    {
        EnterCriticalSection(&m_cs);
    }

    _Releases_lock_(this->m_cs)
        void Unlock()
    {
        LeaveCriticalSection(&m_cs);
    }

    SVFCriticalSection(const SVFCriticalSection&) = delete;
    SVFCriticalSection &operator=(const SVFCriticalSection&) = delete;

private:
    friend class SVFAutoLock;
    CRITICAL_SECTION m_cs;
};
#pragma prefast(pop)

// SVFAutoLock manages a critical section such that it locks/free on scope entry/exit
class SVFAutoLock
{
public:
    _Acquires_lock_(m_lock.m_cs)
        SVFAutoLock(SVFCriticalSection& lock) :
        m_lock(lock)
    {
        m_lock.Lock();
    }

    _Releases_lock_(m_lock.m_cs)
        ~SVFAutoLock()
    {
        m_lock.Unlock();
    }

private:
    SVFAutoLock(const SVFAutoLock &);
    SVFAutoLock &operator=(const SVFAutoLock &);
    SVFCriticalSection &m_lock;
};
