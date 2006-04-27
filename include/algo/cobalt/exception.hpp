#ifndef _ALGO_COBALT_EXCEPTION_HPP
#define _ALGO_COBALT_EXCEPTION_HPP

/* $Id$
* ===========================================================================
*
*                            public DOMAIN NOTICE                          
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
* Filename: exception.hpp
*
* Author: Jason Papadopoulos
*
* Contents: Multiple aligner exceptions
*/

/// @file exception.hpp
/// Multiple aligner exceptions


#include <corelib/ncbiexpt.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(cobalt)

class CMultiAlignerException : public CException
{
public:
    enum EErrCode {
        eInvalidScoreMatrix,
        eInvalidInput
    };

    NCBI_EXCEPTION_DEFAULT(CMultiAlignerException, CException);
};

END_SCOPE(cobalt)
END_NCBI_SCOPE


/* ======================================================================
 *   $Log$
 *   Revision 1.3  2006/04/27 17:19:05  dicuccio
 *   Drop hanging comma in enum list
 *
 *   Revision 1.2  2006/03/22 19:23:17  dicuccio
 *   Cosmetic changes: adjusted include guards; formatted CVS logs; added export
 *   specifiers
 *
 *   Revision 1.1  2006/03/08 15:43:01  papadopo
 *   multiple aligner exception class
 *
 * ======================================================================
 */

#endif  /* ALGO_COBALT___EXCEPTION__HPP */
