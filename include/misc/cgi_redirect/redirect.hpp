#ifndef MISC___CGI_REDIRECT__REDIRECT__HPP
#define MISC___CGI_REDIRECT__REDIRECT__HPP

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
 * Author:  Vladimir Ivanov
 *
 *
 */

/// @cgi_redirect.hpp
/// Define class CCgiRedirectApplication used to redirect CGI requests.

#include <cgi/cgiapp.hpp>
#include <html/page.hpp>

/** @addtogroup CGIBase
 *
 * @{
 */


BEGIN_NCBI_SCOPE


/////////////////////////////////////////////////////////////////////////////
///
/// CCgiRedirectApplication --
///
/// Defines class for CGI redirection.
///
/// CCgiRedirectApplication inherits its basic functionality from
/// CCgiApplication and defines additional method for remapping CGI entries.

class NCBI_XCGI_REDIRECT_EXPORT CCgiRedirectApplication:public CCgiApplication
{
    typedef CCgiApplication CParent;
public:
    virtual void Init(void);
    virtual int  ProcessRequest(CCgiContext& ctx);

public:
    /// Remap CGI entries for the redirection.
    ///
    /// This default implementation uses registry file to obtain rules for
    /// the entries' remapping (see details in this class's description)
    /// New entries will be placed into "new_entries" according to these rules.
    /// If there is no rule defined for an entry, then the entry will be copied
    /// to "new_entries" as is, unchanged.
    ///
    /// @param ctx
    ///   Current CGI context.
    ///   Can be used to get original entries and server context. 
    /// @param new_entries
    ///   Storage for the new, remapped CGI entries. Initially, it is empty,
    ///   and this method should fill it up. These entries will then be used
    ///   instead of original entries to generate request(URL) for the redirection.
    /// @return
    ///   Reference to "new_entries" parameter.
    /// @sa
    ///   ProcessRequest()
    virtual TCgiEntries& RemapEntries(CCgiContext& ctx, TCgiEntries& new_entries);

protected:
    const CHTMLPage& GetPage(void) const;
    CHTMLPage&       GetPage(void);

private:
    CHTMLPage m_Page;  ///< HTML page used to send back the redirect information.
};


END_NCBI_SCOPE


/* @} */


/*
 * ===========================================================================
 * $Log$
 * Revision 1.3  2004/02/09 19:37:45  ivanov
 * Moved from "cgi/redirect" to "misc/cgi_redirect" directory
 *
 * Revision 1.2  2004/02/09 17:58:13  ivanov
 * Added and changed comments (by Denis Vakatov)
 *
 * Revision 1.1  2004/02/09 17:10:19  ivanov
 * Initial revision
 *
 * ===========================================================================
 */

#endif // MISC___CGI_REDIRECT__REDIRECT__HPP
