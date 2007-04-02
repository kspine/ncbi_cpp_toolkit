/* $Id$
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
 */

/// @file soap_fault.hpp
/// User-defined methods of the data storage class.
///
/// This file was originally generated by application DATATOOL
/// using the following specifications:
/// 'soap_11.xsd'.
///
/// New methods or data members can be added to it if needed.
/// See also: soap_fault_.hpp


#ifndef SOAP_FAULT_HPP
#define SOAP_FAULT_HPP


// generated includes
#include <serial/soap/soap_fault_.hpp>

BEGIN_NCBI_SCOPE
// generated classes

/////////////////////////////////////////////////////////////////////////////
class CSoapFault : public CSoapFault_Base
{
    typedef CSoapFault_Base Tparent;
public:
    // constructor
    CSoapFault(void);
    // destructor
    ~CSoapFault(void);

    enum ESoap_FaultcodeEnum {
        e_not_set = 0,
        eDataEncodingUnknown = 1,
        eMustUnderstand,
        eReceiver,
        eSender,
        eVersionMismatch
    };

    ESoap_FaultcodeEnum GetFaultcodeEnum(void) const;
    void SetFaultcodeEnum(ESoap_FaultcodeEnum value);

private:
    // Prohibit copy constructor and assignment operator
    CSoapFault(const CSoapFault& value);
    CSoapFault& operator=(const CSoapFault& value);

    static TFaultcode x_FaultcodeEnumToCode(ESoap_FaultcodeEnum code);
    static ESoap_FaultcodeEnum x_CodeToFaultcodeEnum(const TFaultcode& value);
};

/////////////////// CSoapFault inline methods

// constructor
inline
CSoapFault::CSoapFault(void)
{
}


/////////////////// end of CSoapFault inline methods

END_NCBI_SCOPE


#endif // SOAP_FAULT_HPP
