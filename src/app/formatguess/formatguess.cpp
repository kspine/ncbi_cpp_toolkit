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
* Author:  Frank Ludwig, NCBI
*
* File Description:
*   Test application for the CFormatGuess component
*
* ===========================================================================
*/

#include <ncbi_pch.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbienv.hpp>
#include <corelib/ncbiargs.hpp>

#include <util/format_guess.hpp>

typedef std::map<ncbi::CFormatGuess::EFormat, std::string> FormatMap;
typedef FormatMap::iterator FormatIter;

USING_NCBI_SCOPE;
//USING_SCOPE(objects);

//  ============================================================================
class CFormatGuessApp
//  ============================================================================
     : public CNcbiApplication
{
private:
    virtual void Init(void);
    virtual int  Run(void);
    virtual void Exit(void);
};

//  ============================================================================
void CFormatGuessApp::Init(void)
//  ============================================================================
{
    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);

    arg_desc->SetUsageContext
        (GetArguments().GetProgramBasename(),
         "CFormatGuess front end: Guess various file formats");

    //
    //  shared flags and parameters:
    //        
    arg_desc->AddDefaultKey
        ("i", "InputFile",
         "Input File Name or '-' for stdin.",
         CArgDescriptions::eInputFile,
         "-");
         
    arg_desc->AddFlag(
        "canonical-name",
        "Use the canonical name which is the name of the format as "
        "given by the underlying C++ format guesser.");
        
    arg_desc->AddFlag(
        "omit-file-name",
        "When sending output, do not include the name of the input file. "
        "This has no effect when reading from stdin.");

    SetupArgDescriptions(arg_desc.release());
}

//  ============================================================================
int 
CFormatGuessApp::Run(void)
//  ============================================================================
{
    const CArgs& args = GetArgs();
    CNcbiIstream & input_stream = args["i"].AsInputFile();
    const string name_of_input_stream = args["i"].AsString();

    CFormatGuess guesser( input_stream );
    CFormatGuess::EFormat uFormat = guesser.GuessFormat();
    
    // include file name in output if it's not stdin
    if( ! args["omit-file-name"] &&
            ! name_of_input_stream.empty() && name_of_input_stream != "-" ) 
    {
        cout << name_of_input_stream << " :   ";
    }
    
    if( args["canonical-name"] ) {
        // caller wants to always use the format-guesser's name
         cout << CFormatGuess::GetFormatName(uFormat);
    } else {
        // caller wants special names for some types
        FormatMap FormatStrings;
        FormatStrings[ CFormatGuess::eUnknown ] = "Format not recognized";
        FormatStrings[ CFormatGuess::eBinaryASN ] = "Binary ASN.1";
        FormatStrings[ CFormatGuess::eTextASN ] = "Text ASN.1";
        FormatStrings[ CFormatGuess::eFasta ] = "FASTA sequence record";
        FormatStrings[ CFormatGuess::eXml ] = "XML";
        FormatStrings[ CFormatGuess::eRmo ] = "RepeatMasker Out";
        FormatStrings[ CFormatGuess::eGlimmer3 ] = "Glimmer3 prediction";
        FormatStrings[ CFormatGuess::ePhrapAce ] = "Phrap ACE assembly file";
        FormatStrings[ CFormatGuess::eGtf ] = "GFF/GTF style annotation";
        FormatStrings[ CFormatGuess::eAgp ] = "AGP format assembly";
        FormatStrings[ CFormatGuess::eNewick ] = "Newick tree";
        FormatStrings[ CFormatGuess::eDistanceMatrix ] = "Distance matrix";
        FormatStrings[ CFormatGuess::eFiveColFeatureTable ] = 
            "Five column feature table";
        FormatStrings[ CFormatGuess::eTaxplot ] = "Tax plot";
        FormatStrings[ CFormatGuess::eTable ] = "Generic table";
        FormatStrings[ CFormatGuess::eAlignment ] = "Text alignment";
        FormatStrings[ CFormatGuess::eFlatFileSequence ] = 
            "Flat file sequence portion";
        FormatStrings[ CFormatGuess::eSnpMarkers ] = "SNP marker flat file";
        FormatStrings[ CFormatGuess::eWiggle ] = "UCSC Wiggle file";
        FormatStrings[ CFormatGuess::eBed ] = "UCSC BED file";
        FormatStrings[ CFormatGuess::eBed15 ] = "UCSC microarray file";
        FormatStrings[ CFormatGuess::eHgvs ] = "HGVS Variation file";
        FormatStrings[ CFormatGuess::eGff2 ] = "GFF2 feature table";
        FormatStrings[ CFormatGuess::eGff3 ] = "GFF3 feature table";
        FormatStrings[ CFormatGuess::eGvf ] = "GVF gene variation data";
        FormatStrings[ CFormatGuess::eVcf ] = "VCF Variant Call Format";
            
        FormatIter it = FormatStrings.find( uFormat );
        if ( it == FormatStrings.end() ) {
            // cout << "Unmapped format [" << uFormat << "]";
            cout << CFormatGuess::GetFormatName(uFormat);
        }
        else {
            cout << it->second;
        }
    }

    // end the one line we're printing
    cout << endl;
            
    return 0;
}

//  ============================================================================
void CFormatGuessApp::Exit(void)
//  ============================================================================
{
    SetDiagStream(0);
}

//  ============================================================================
int main(int argc, const char* argv[])
//  ============================================================================
{
    // Execute main application function
    return CFormatGuessApp().AppMain(argc, argv, 0, eDS_Default, 0);
}

