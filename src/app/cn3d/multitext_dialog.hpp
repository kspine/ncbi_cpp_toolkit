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
* Authors:  Paul Thiessen
*
* File Description:
*       generic multi-line text entry dialog
*
* ===========================================================================
*/

#ifndef CN3D_MULTITEXT_DIALOG__HPP
#define CN3D_MULTITEXT_DIALOG__HPP

#include <corelib/ncbistd.hpp>
#include <corelib/ncbistl.hpp>

#ifdef __WXMSW__
#include <windows.h>
#include <wx/msw/winundef.h>
#endif
#include <wx/wx.h>

#include <map>
#include <vector>


BEGIN_SCOPE(Cn3D)

class MultiTextDialog;

// so the owner can receive notification that dialog text has changed, or dialog destroyed

class MultiTextDialogOwner
{
public:
    virtual void DialogTextChanged(const MultiTextDialog *changed) = 0;
    virtual void DialogDestroyed(const MultiTextDialog *destroyed) = 0;
};


// this is really intended to be used as a non-modal dialog; it calls its owner's DialogTextChanged()
// method every time the user types something. But it should function modally as well.

class MultiTextDialog : private wxDialog
{
public:
    typedef std::vector < std::string > TextLines;

    MultiTextDialog(MultiTextDialogOwner *owner, const TextLines& initialText,
        wxWindow* parent, wxWindowID id, const wxString& title);
    ~MultiTextDialog(void);

    bool GetLines(TextLines *lines) const;
    bool GetLine(std::string *singleString) const;  // collapse all lines to single string

    bool ShowDialog(bool);
    int ShowModalDialog(void);
    bool DestroyDialog(void) { return Destroy(); }

private:
    MultiTextDialogOwner *myOwner;

    // GUI elements
    wxTextCtrl *textCtrl;
    wxButton *bDone;

    // event callbacks
    void OnButton(wxCommandEvent& event);
    void OnTextChange(wxCommandEvent& event);
    void OnCloseWindow(wxCloseEvent& event);

    DECLARE_EVENT_TABLE()
};

END_SCOPE(Cn3D)

#endif // CN3D_MULTITEXT_DIALOG__HPP

/*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.9  2005/01/04 16:06:59  thiessen
* make MultiTextDialog remember its position+size
*
* Revision 1.8  2003/02/03 19:20:04  thiessen
* format changes: move CVS Log to bottom of file, remove std:: from .cpp files, and use new diagnostic macros
*
* Revision 1.7  2002/08/15 22:13:15  thiessen
* update for wx2.3.2+ only; add structure pick dialog; fix MultitextDialog bug
*
* Revision 1.6  2002/06/12 15:09:15  thiessen
* kludge to avoid initial selected-all state
*
* Revision 1.5  2001/10/18 14:48:46  thiessen
* remove class name from member function
*
* Revision 1.4  2001/10/16 21:48:28  thiessen
* restructure MultiTextDialog; allow virtual bonds for alpha-only PDB's
*
* Revision 1.3  2001/10/11 14:18:20  thiessen
* make MultiTextDialog non-modal
*
* Revision 1.2  2001/08/06 20:22:48  thiessen
* add preferences dialog ; make sure OnCloseWindow get wxCloseEvent
*
* Revision 1.1  2001/07/10 16:39:33  thiessen
* change selection control keys; add CDD name/notes dialogs
*
*/
