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
 * Author:  .......
 *
 * File Description:
 *   .......
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using specifications from the ASN data definition file
 *   'seq.asn'.
 */

#ifndef OBJECTS_SEQ_SEQDESC_HPP
#define OBJECTS_SEQ_SEQDESC_HPP


// generated includes
#include <objects/seq/Seqdesc_.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

class CSeqdesc : public CSeqdesc_Base
{
    typedef CSeqdesc_Base Tparent;
public:
    // constructor
    CSeqdesc(void);
    // destructor
    ~CSeqdesc(void);
    
    enum ELabelType {
        eType,
        eContent,
        eBoth
    };
    
    // Appends a label for a CSeqdesc to label. Label may be based on just the 
    // type of CSeqdesc, just the content, or both. 
    void GetLabel(string* const label, ELabelType label_type) const;

private:
    // Prohibit copy constructor and assignment operator
    CSeqdesc(const CSeqdesc& value);
    CSeqdesc& operator=(const CSeqdesc& value);

};



/////////////////// CSeqdesc inline methods

// constructor
inline
CSeqdesc::CSeqdesc(void)
{
}


/////////////////// end of CSeqdesc inline methods


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.1  2002/10/03 16:52:55  clausen
* Added GetLabel()
*
*
* ===========================================================================
*/

#endif // OBJECTS_SEQ_SEQDESC_HPP
/* Original file checksum: lines: 93, chars: 2350, CRC32: a6859469 */
