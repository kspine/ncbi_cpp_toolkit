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
 *   using the following specifications:
 *   'omssa.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>

// generated includes
#include <objects/omssa/MSHits.hpp>

#include <objects/omssa/MSMod.hpp>
#include <objects/omssa/MSModHit.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CMSHits::~CMSHits(void)
{
}


///
///  Makes a string of mods, positions for display purposes
///  
void CMSHits::MakeModString(string& StringOut, CRef <CMSModSpecSet> Modset) const
{
    StringOut.erase();
    if(CanGetMods()) {
        ITERATE(TMods, i, GetMods()) {
            if(!StringOut.empty()) StringOut += " ,";
            if((*i)->GetModtype() < eMSMod_max) StringOut += Modset->GetModName((*i)->GetModtype());
            else StringOut += NStr::IntToString((*i)->GetModtype());
            StringOut += ":" + NStr::IntToString((*i)->GetSite()+1);
        }
    }
}


///
///  Makes a peptide AA string, lower case for mods
///  
void CMSHits::MakePepString(string& StringOut) const
{    
    StringOut.erase();
    if(CanGetPepstring()) {
        StringOut = GetPepstring();
        NStr::ToUpper(StringOut);
        ITERATE(TMods, i, GetMods()) {
            if((*i)->GetSite() < StringOut.size())
                StringOut[(*i)->GetSite()] = tolower(StringOut[(*i)->GetSite()]);
        }
    }
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE


/*
* ===========================================================================
*
* $Log$
* Revision 1.5  2005/03/14 22:29:54  lewisg
* add mod file input
*
* Revision 1.4  2004/12/08 19:21:12  lewisg
* start sequence position by 1
*
* Revision 1.3  2004/12/08 19:12:43  lewisg
* const
*
* Revision 1.2  2004/12/08 02:00:08  ucko
* Use string::erase() rather than string::clear(), which is not 100%
* portable.  (GCC 2.95.x continues not to support it....)
*
* Revision 1.1  2004/12/07 23:38:22  lewisg
* add modification handling code
*
*
* ===========================================================================
*/
/* Original file checksum: lines: 65, chars: 1878, CRC32: e17e37ca */
