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
 *   'general.asn'.
 *
 * ---------------------------------------------------------------------------
 * $Log$
 * Revision 6.4  2000/12/15 19:22:10  ostell
 * made AsString do Upcase, and switched to using PNocase().Equals()
 *
 * Revision 6.3  2000/12/08 21:49:29  ostell
 * changed MakeString to AsString and to use ostream instead of string
 *
 * Revision 6.2  2000/12/08 19:53:15  ostell
 * added MakeString()
 *
 * Revision 6.1  2000/11/21 18:58:21  vasilche
 * Added Match() methods for CSeq_id, CObject_id and CDbtag.
 *
 *
 * ===========================================================================
 */

// standard includes

// generated includes
#include <objects/general/Object_id.hpp>
#include <corelib/ncbistd.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CObject_id::~CObject_id(void)
{
}

// match for identity
bool CObject_id::Match(const CObject_id& oid2) const
{
	E_Choice type1 = Which();
	E_Choice type2 = oid2.Which();

	if (type1 != type2)
		return false;

	switch (type1)
	{
		case e_Id:
			if ((GetId()) == (oid2.GetId()))
				return true;
			else
				return false;
		case e_Str:
			return PNocase().Equals(GetStr(), oid2.GetStr());
		default:
			break;
	}
	return false;
}

    // format contents into a stream
ostream& CObject_id::AsString(ostream &s) const
{
	switch (Which())
	{
		case e_Id:
			s << GetId();
			break;
		case e_Str:
			s << Upcase(GetStr());
			break;
		default:
			break;
	}
	return s;
}

END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

