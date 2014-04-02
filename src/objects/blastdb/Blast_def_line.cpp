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
 * Author:  Thomas W. Rackers (current versions of SetTaxIds/GetTaxIds)
 *
 * File Description:
 *   .......
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using the following specifications:
 *   'blastdb.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>

// generated includes
#include <objects/blastdb/Blast_def_line.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CBlast_def_line::~CBlast_def_line(void)
{
}

void
CBlast_def_line::SetTaxIds(const CBlast_def_line::TTaxIds& t)
{
    // Clear the 'links' field.  We may be setting new values there anyway.
    ResetLinks();

    // Next step depends on size of input set.
    if (t.empty()) {
        // If it's empty, clear the 'taxid' field too.
        ResetTaxid();
    } else if (t.size() == 1) {
        // Or if it has a single value, set 'taxid' to that value.
        SetTaxid(*t.begin());
    } else {
        // Otherwise, set the 'taxid' field to the FIRST value in the set,
        // UNLESS the following conditions are all met:
        // (1) 'taxid' has a value;
        // (2) that value is non-zero;
        // (3) it's already present in the input set.
        bool overwrite = true;
        if (CanGetTaxid()) {
            const TTaxid taxid = GetTaxid();
            if (taxid != 0) {
                TTaxIds::iterator it = t.find(taxid);
                if (it != t.end()) {
                    overwrite = false;
                }
            }
        }
        // If the above conditions were not met, overwrite the existing
        // 'taxid'.
        if (overwrite) {
            SetTaxid(*t.begin());
        }
        // Save all of the input set to the 'links' field.
        ITERATE(TTaxIds, itr, t) {
            SetLinks().push_back(*itr);
        }
    }
}

CBlast_def_line::TTaxIds
CBlast_def_line::GetTaxIds() const
{
    TTaxIds retval;                 // set<int>, initially empty

    // If there's a non-zero 'taxid' value, add it to the result set.
    if (CanGetTaxid()) {
        TTaxid taxid = GetTaxid();
        if (taxid != 0) {
            retval.insert(taxid);
        }
    }
    // If there are any non-zero 'links' values, add them to the result set.
    if (IsSetLinks()) {
        TLinks taxids = GetLinks();  // see ASN.1 spec comment
        ITERATE(TLinks, itr, taxids) {
            if (*itr != 0) {
                retval.insert(*itr);
            }
        }
    }
    // Remember, set containers guarantee that all members are unique,
    // so if the 'taxid' and 'links' fields share a value, it will not
    // be returned twice.

    // Return result set.
    return retval;
}

END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 57, chars: 1739, CRC32: eaa77e11 */
