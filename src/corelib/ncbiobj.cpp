/*  $Id$
* ===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
* Author: 
*   Eugene Vasilchenko
*
* File Description:
*   Standard CObject and CRef classes for reference counter based GC
*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.10  2000/11/01 20:37:16  vasilche
* Fixed detection of heap objects.
* Removed ECanDelete enum and related constructors.
* Disabled sync_with_stdio ad the beginning of AppMain.
*
* Revision 1.9  2000/10/17 17:59:08  vasilche
* Detected misuse of CObject constructors will be reported via ERR_POST and will
* not throw exception.
*
* Revision 1.8  2000/10/13 16:26:30  vasilche
* Added heuristic for detection of CObject allocation in heap.
*
* Revision 1.7  2000/08/15 19:41:41  vasilche
* Changed refernce counter to allow detection of more errors.
*
* Revision 1.6  2000/06/16 16:29:51  vasilche
* Added SetCanDelete() method to allow to change CObject 'in heap' status immediately after creation.
*
* Revision 1.5  2000/06/07 19:44:22  vasilche
* Removed unneeded THROWS declaration - they lead to encreased code size.
*
* Revision 1.4  2000/03/29 15:50:41  vasilche
* Added const version of CRef - CConstRef.
* CRef and CConstRef now accept classes inherited from CObject.
*
* Revision 1.3  2000/03/08 17:47:30  vasilche
* Simplified error check.
*
* Revision 1.2  2000/03/08 14:18:22  vasilche
* Fixed throws instructions.
*
* Revision 1.1  2000/03/07 14:03:15  vasilche
* Added CObject class as base for reference counted objects.
* Added CRef templace for reference to CObject descendant.
*
* 
* ===========================================================================
*/

#include <corelib/ncbiobj.hpp>

#define STACK_THRESHOLD (16*1024)

BEGIN_NCBI_SCOPE

CNullPointerError::CNullPointerError(void)
    THROWS_NONE
{
}

CNullPointerError::~CNullPointerError(void)
    THROWS_NONE
{
}

const char* CNullPointerError::what() const
    THROWS_NONE
{
    return "null pointer error";
}

// CObject local new operator to mark allocation in heap
void* CObject::operator new(size_t size)
{
    _ASSERT(size >= sizeof(CObject));
    void* ptr = ::operator new(size);
    memset(ptr, 0, size);
    static_cast<CObject*>(ptr)->m_Counter = eCounterNew;
    return ptr;
}

void* CObject::operator new[](size_t size)
{
    void* ptr = ::operator new[](size);
    memset(ptr, 0, size);
    return ptr;
}

#ifdef _DEBUG
void CObject::operator delete(void* ptr)
{
    CObject* objectPtr = static_cast<CObject*>(ptr);
    _ASSERT(objectPtr->m_Counter == TCounter(eCounterDeleted));
    ::operator delete(ptr);
}

void CObject::operator delete[](void* ptr)
{
    ::operator delete[](ptr);
}
#endif

// initialization in debug mode
void CObject::InitCounter(void)
{
    if ( m_Counter != eCounterNew ) {
        m_Counter = eCounterNotInHeap;
    }
    else {
        // m_Counter == eCounterNew -> possibly in heap
        char stackObject;
        const char* stackObjectPtr = &stackObject;
        const char* objectPtr = reinterpret_cast<const char*>(this);
        bool inStack =
            (objectPtr < stackObjectPtr + STACK_THRESHOLD) &&
            (objectPtr > stackObjectPtr - STACK_THRESHOLD);

        // surely not in heap
        if ( inStack )
            m_Counter = eCounterNotInHeap;
        else
            m_Counter = eCounterInHeap;
    }
}

CObject::CObject(void)
{
    InitCounter();
}

CObject::CObject(const CObject& /*src*/)
{
    InitCounter();
}

CObject::~CObject(void)
{
    TCounter counter = m_Counter;
    if ( counter == TCounter(eCounterInHeap) ||
         counter == TCounter(eCounterNotInHeap) ) {
        // reference counter is zero -> ok
    }
    else if ( ObjectStateValid(counter) ) {
        _ASSERT(ObjectStateReferenced(counter));
        // referenced object
        THROW1_TRACE(runtime_error,
                     "deletion of referenced CObject");
    }
    else if ( counter == TCounter(eCounterDeleted) ) {
        // deleted object
        THROW1_TRACE(runtime_error,
                     "double deletion of CObject");
    }
    else {
        // bad object
        THROW1_TRACE(runtime_error,
                     "deletion of corrupted CObject");
    }
    // mark object as deleted
    m_Counter = TCounter(eCounterDeleted);
}

void CObject::AddReferenceOverflow(void) const
{
    TCounter counter = m_Counter;
    if ( ObjectStateValid(counter) ) {
        // counter overflow
        THROW1_TRACE(runtime_error,
                     "AddReference: CObject reference counter overflow");
    }
    else {
        // bad object
        THROW1_TRACE(runtime_error,
                     "AddReference of invalid CObject");
    }
}

void CObject::RemoveLastReference(void) const
{
    TCounter counter = m_Counter;
    if ( counter == TCounter(eCounterInHeap + eCounterStep) ) {
        // last reference to heap object -> delete
        m_Counter = eCounterInHeap;
        delete this;
    }
    else if ( counter == TCounter(eCounterNotInHeap + eCounterStep) ) {
        // last reference to non heap object -> do nothing
        m_Counter = eCounterNotInHeap;
    }
    else {
        _ASSERT(!ObjectStateValid(counter));
        // bad object
        THROW1_TRACE(runtime_error,
                     "RemoveReference of unreferenced CObject");
    }
}

void CObject::ReleaseReference(void) const
{
    TCounter counter = m_Counter;
    if ( ObjectStateReferenced(counter) ) {
        // release reference to object in heap
        m_Counter = TCounter(counter - eCounterStep);
    }
    else if ( !ObjectStateValid(counter) ) {
        THROW1_TRACE(runtime_error,
                     "ReleaseReference of corrupted CObject");
    }
    else {
        THROW1_TRACE(runtime_error,
                     "ReleaseReference of unreferenced CObject");
    }
}

void CObject::DoNotDeleteThisObject(void)
{
    TCounter counter = m_Counter;
    if ( !ObjectStateValid(counter) ) {
        THROW1_TRACE(runtime_error,
                     "DoNotDeleteThisObject of corrupted CObject");
    }
    else if ( ObjectStateReferenced(counter) ) {
        THROW1_TRACE(runtime_error,
                     "DoNotDeleteThisObject of referenced CObject");
    }
    else {
        m_Counter = eCounterNotInHeap;
    }
}

END_NCBI_SCOPE
