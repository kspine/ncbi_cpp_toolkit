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

/// @MSMod.hpp
/// User-defined methods of the data storage class.
///
/// This file was originally generated by application DATATOOL
/// using the following specifications:
/// 'omssa.asn'.
///
/// New methods or data members can be added to it if needed.
/// See also: MSMod_.hpp


#ifndef OBJECTS_OMSSA_MSMOD_HPP
#define OBJECTS_OMSSA_MSMOD_HPP


// generated includes
#include <objects/omssa/MSMod_.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::


///
/// Modification types
/// there are five kinds of mods:
/// 1. specific to an AA
/// 2. N terminus protein, not specific to an AA
/// 3. N terminus protein, specific to an AA
/// 4. C terminus protein, not specific to an AA
/// 5. C terminus protein, specific to an AA
/// 6. N terminus peptide, not specific
/// 7. N terminus peptide, specific AA
/// 8. C terminus peptide, not specific
/// 9. C terminus peptide, specific AA
///

const int kNumModType = 9;

enum EMSModType {
    eModAA = 0,
    eModN,
    eModNAA,
    eModC,
    eModCAA,
    eModNP,
    eModNPAA,
    eModCP,
    eModCPAA
};


/////////////////////////////////////////////////////////////////////////////
//
//  Informational arrays for mods
//
//  These are separate arrays for speed considerations
//

///
/// categorizes existing mods as the types listed above
///
const EMSModType ModTypes[eMSMod_max] = {
    eModAA,
    eModAA,
    eModAA,
    eModAA,
    eModAA,
    eModAA,
    eModAA,
    eModAA,
    eModAA,
    eModNAA,
    eModN,
    eModN,
    eModN,
    eModAA,
    eModAA,
    eModAA,
    eModAA,
    eModAA,
    eModCP,
    eModAA,
    eModAA,
    eModCP
};

///
/// the names of the various modifications codified in the asn.1
///
char const * const kModNames[eMSMod_max] = {
    "methylation of K",
    "oxidation of M",
    "carboxymethyl C",
    "carbamidomethyl C",
    "deamidation of N and Q",
    "propionamide C",
    "phosphorylation of S",
    "phosphorylation of T",
    "phosphorylation of Y",
    "N-term M removal",
    "N-term acetylation",
    "N-term methylation",
    "N-term trimethylation",
    "beta methythiolation of D",
    "methylation of Q",
    "trimethylation of K",
    "methylation of D",
    "methylation of E",
    "C-term peptide methylation",
    "trideuteromethylation of D",
    "trideuteromethylation of E",
    "C-term peptide trideuteromethylation"
};	   
 
///
/// the characters to compare
/// rows are indexed by mod
/// column are the AA's modified (if any)
///
const char ModChar [3][eMSMod_max] = {
    {'\x0a','\x0c','\x03','\x03','\x0d','\x03','\x11','\x12','\x16','\x0c','\x00','\x00','\x00','\x04','\x0f','\x0a','\x04','\x05','\x00','\x04','\x05','\x00' },
    {'\x00','\x00','\x00','\x00','\x0f','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00' },
    {'\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00','\x00' }
};

///
/// the number of characters to compare
///
const int NumModChars[eMSMod_max] = { 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0 };

///
/// the modification masses
///
const int ModMass[eMSMod_max] = { 1402, 1600, 5801, 5702, 98, 7104, 7997, 7997, 7997, -13104, 4201, 1402, 4205, 4599, 1402, 4205, 1402, 1402, 1402, 1704, 1704, 1704};



END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.1  2004/12/07 23:38:22  lewisg
* add modification handling code
*
*
* ===========================================================================
*/

#endif // OBJECTS_OMSSA_MSMOD_HPP
/* Original file checksum: lines: 65, chars: 1980, CRC32: a05bf8ae */
