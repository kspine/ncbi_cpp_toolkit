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
* Author: Eugene Vasilchenko
*
* File Description:
*   Definition CGI application class and its context class.
*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.6  1999/05/06 20:33:43  pubmed
* CNcbiResource -> CNcbiDbResource; utils from query; few more context methods
*
* Revision 1.5  1999/05/04 16:14:44  vasilche
* Fixed problems with program environment.
* Added class CNcbiEnvironment for cached access to C environment.
*
* Revision 1.4  1999/05/04 00:03:11  vakatov
* Removed the redundant severity arg from macro ERR_POST()
*
* Revision 1.3  1999/04/30 19:21:02  vakatov
* Added more details and more control on the diagnostics
* See #ERR_POST, EDiagPostFlag, and ***DiagPostFlag()
*
* Revision 1.2  1999/04/28 16:54:41  vasilche
* Implemented stream input processing for FastCGI applications.
* Fixed POST request parsing
*
* Revision 1.1  1999/04/27 14:50:04  vasilche
* Added FastCGI interface.
* CNcbiContext renamed to CCgiContext.
*
* ===========================================================================
*/

#include <corelib/ncbistd.hpp>
#include <corelib/ncbireg.hpp>
#include <corelib/ncbires.hpp>
#include <corelib/cgictx.hpp>
#include <corelib/cgiapp.hpp>

BEGIN_NCBI_SCOPE

//
// class CCgiContext
//

CCgiContext::CCgiContext(CCgiApplication& app, CNcbiEnvironment* env,
                         CNcbiIstream* in, CNcbiOstream* out)
    : m_app(app), m_request(0, 0, env, in), m_response(out)
{
}

CCgiContext::CCgiContext(int argc, char** argv,
                         CCgiApplication& app, CNcbiEnvironment* env,
                         CNcbiIstream* in, CNcbiOstream* out)
    : m_app(app), m_request(argc, argv, env, in), m_response(out)
{
}

CNcbiRegistry& CCgiContext::x_GetConfig(void) const
{
    return m_app.x_GetConfig();
}

CNcbiResource& CCgiContext::x_GetResource(void) const
{
    return m_app.x_GetResource();
}

CCgiServerContext& CCgiContext::x_GetServCtx( void ) const
{ 
    CCgiServerContext* context = m_srvCtx.get();
    if ( !context ) {
        context = m_app.LoadServerContext(const_cast<CCgiContext&>(*this));
        if ( !context ) {
            ERR_POST("CCg13iContext::GetServCtx: no server context set");
            throw runtime_error("no server context set");
        }
        const_cast<CCgiContext&>(*this).m_srvCtx.reset(context);
    }
    return *context;
}

string CCgiContext::GetRequestValue(const string& name) const
{
    TCgiEntries& entries = const_cast<TCgiEntries&>( 
                                                GetRequest().GetEntries() );
    pair<TCgiEntriesI, TCgiEntriesI> range = entries.equal_range(name);
    if ( range.second == range.first )
        return NcbiEmptyString;
    const string& value = range.first->second;
    while ( ++range.first != range.second ) {
        if ( range.first->second != value ) {
            THROW1_TRACE(runtime_error,
                         "duplicated entries in request with name: " +
                         name + ": " + value + "!=" + range.first->second);
        }
    }
    return value;
}

void CCgiContext::RemoveRequestValues(const string& name)
{
    const_cast<TCgiEntries&>(GetRequest().GetEntries()).erase(name);
}

void CCgiContext::AddRequestValue(const string& name, const string& value)
{
    const_cast<TCgiEntries&>( GetRequest().GetEntries()).insert(
                                     TCgiEntries::value_type(name, value));
}

void CCgiContext::ReplaceRequestValue(const string& name, 
                                      const string& value)
{
    RemoveRequestValues(name);
    AddRequestValue(name, value);
}


END_NCBI_SCOPE
