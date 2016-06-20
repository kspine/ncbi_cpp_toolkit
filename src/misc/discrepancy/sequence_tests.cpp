/*  $Id$
 * =========================================================================
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
 * =========================================================================
 *
 * Authors: Colleen Bollin, based on similar discrepancy tests
 *
 */

#include <ncbi_pch.hpp>
#include <objects/general/User_object.hpp>
#include <objects/seq/MolInfo.hpp>
#include <objects/valid/Comment_rule.hpp>
#include <objmgr/seqdesc_ci.hpp>
#include <objmgr/seq_vector.hpp>
#include <objects/seq/Seq_ext.hpp>
#include <objects/seq/Delta_ext.hpp>
#include <objects/seq/Delta_seq.hpp>
#include <objects/seq/Seq_literal.hpp>
#include <objects/general/Object_id.hpp>

#include "discrepancy_core.hpp"

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(NDiscrepancy)
USING_SCOPE(objects);

DISCREPANCY_MODULE(sequence_tests);


// MISSING_GENOMEASSEMBLY_COMMENTS

const string kMissingGenomeAssemblyComments = "[n] bioseq[s] [is] missing GenomeAssembly structured comments";

//  ----------------------------------------------------------------------------
DISCREPANCY_CASE(MISSING_GENOMEASSEMBLY_COMMENTS, CSeq_inst, eDisc, "Bioseqs should have GenomeAssembly structured comments")
//  ----------------------------------------------------------------------------
{
    if (obj.IsAa()) {
        return;
    }
    CConstRef<CBioseq> seq = context.GetCurrentBioseq();
    if (!seq) {
        return;
    }
    CBioseq_Handle b = context.GetScope().GetBioseqHandle(*seq);

    CSeqdesc_CI d(b, CSeqdesc::e_User);
    bool found = false;
    while (d && !found) {
        if (d->GetUser().GetObjectType() == CUser_object::eObjectType_StructuredComment &&
            NStr::Equal(CComment_rule::GetStructuredCommentPrefix(d->GetUser()), "Genome-Assembly-Data")) {
            found = true;
        }
        ++d;
    }

    if (!found) {
        m_Objs[kMissingGenomeAssemblyComments].Add(*context.NewDiscObj(seq),
                    false).Fatal();
    }
}


//  ----------------------------------------------------------------------------
DISCREPANCY_SUMMARIZE(MISSING_GENOMEASSEMBLY_COMMENTS)
//  ----------------------------------------------------------------------------
{
    if (m_Objs.empty()) {
        return;
    }
    m_ReportItems = m_Objs.Export(*this)->GetSubitems();
}


//  ----------------------------------------------------------------------------
DISCREPANCY_CASE(DUP_DEFLINE, CSeq_inst, eOncaller, "Definition lines should be unique")
//  ----------------------------------------------------------------------------
{
    CConstRef<CBioseq> seq = context.GetCurrentBioseq();
    if (!seq || !seq->IsSetDescr() || obj.IsAa()) return;

    ITERATE(CBioseq::TDescr::Tdata, it, seq->GetDescr().Get()) {
        if ((*it)->IsTitle()) {
            CRef<CDiscrepancyObject> this_disc_obj(context.NewDiscObj((*it), eKeepRef));
            m_Objs["titles"][(*it)->GetTitle()].Add(*this_disc_obj, false);
        }
    }
}


const string kIdenticalDeflines = "[n] definition line[s] [is] identical:";
const string kAllUniqueDeflines = "All deflines are unique";
const string kUniqueDeflines = "[n] definition line[s] [is] unique";

//  ----------------------------------------------------------------------------
DISCREPANCY_SUMMARIZE(DUP_DEFLINE)
//  ----------------------------------------------------------------------------
{
    if (m_Objs.empty()) {
        return;
    }
    CReportNode::TNodeMap::iterator it = m_Objs["titles"].GetMap().begin();
    bool all_unique = true;
    while (it != m_Objs["titles"].GetMap().end() && all_unique) {
        if (it->second->GetObjects().size() > 1) {
            all_unique = false;
        }
        ++it;
    }
    it = m_Objs["titles"].GetMap().begin();
    while (it != m_Objs["titles"].GetMap().end()) {            
        NON_CONST_ITERATE(TReportObjectList, robj, m_Objs["titles"][it->first].GetObjects())
        {
            const CDiscrepancyObject* other_disc_obj = dynamic_cast<CDiscrepancyObject*>(robj->GetNCPointer());
            CConstRef<CSeqdesc> title_desc(dynamic_cast<const CSeqdesc*>(other_disc_obj->GetObject().GetPointer()));
            if (it->second->GetObjects().size() > 1) {
                //non-unique definition line
                m_Objs[kIdenticalDeflines + it->first].Add(*context.NewDiscObj(title_desc), false);
            } else {
                //unique definition line
                if (all_unique) {
                    m_Objs[kAllUniqueDeflines].Add(*context.NewDiscObj(title_desc), false);
                } else {
                    m_Objs[kUniqueDeflines].Add(*context.NewDiscObj(title_desc), false);
                }
            }
        }  
        ++it;
    }
    m_Objs.GetMap().erase("titles");
    if (all_unique) {
        m_Objs[kAllUniqueDeflines].clearObjs();
    }

    m_ReportItems = m_Objs.Export(*this)->GetSubitems();
}


// TERMINAL_NS

const string kTerminalNs = "[n] sequence[s] [has] terminal Ns";

//  ----------------------------------------------------------------------------
DISCREPANCY_CASE(TERMINAL_NS, CSeq_inst, eDisc, "Ns at end of sequences")
//  ----------------------------------------------------------------------------
{
    CConstRef<CBioseq> seq = context.GetCurrentBioseq();
    if (!seq || seq->IsAa()) {
        return;
    }

    bool has_terminal_n_or_gap = false;

    CBioseq_Handle b = context.GetScope().GetBioseqHandle(*seq);
    CRef<CSeq_loc> start(new CSeq_loc());
    start->SetInt().SetId().Assign(*(seq->GetId().front()));
    start->SetInt().SetFrom(0);
    start->SetInt().SetTo(0);

    CSeqVector start_vec(*start, context.GetScope(), CBioseq_Handle::eCoding_Iupac);
    CSeqVector::const_iterator vi_start = start_vec.begin();

    if (*vi_start == 'N' || vi_start.IsInGap()) {
        has_terminal_n_or_gap = true;
    } else {
        CRef<CSeq_loc> stop(new CSeq_loc());
        stop->SetInt().SetId().Assign(*(seq->GetId().front()));
        stop->SetInt().SetFrom(seq->GetLength() - 1);
        stop->SetInt().SetTo(seq->GetLength() - 1);

        CSeqVector stop_vec(*stop, context.GetScope(), CBioseq_Handle::eCoding_Iupac);
        CSeqVector::const_iterator vi_stop = stop_vec.begin();
        if (*vi_stop == 'N' || vi_stop.IsInGap()) {
            has_terminal_n_or_gap = true;
        }
    }

    if (has_terminal_n_or_gap) {
        m_Objs[kTerminalNs].Add(*context.NewDiscObj(seq),
            false).Fatal();
    }
}


//  ----------------------------------------------------------------------------
DISCREPANCY_SUMMARIZE(TERMINAL_NS)
//  ----------------------------------------------------------------------------
{
    if (m_Objs.empty()) {
        return;
    }
    m_ReportItems = m_Objs.Export(*this)->GetSubitems();
}


// SHORT_PROT_SEQUENCES
const string kShortProtSeqs = "[n] protein sequences are shorter than 50 aa.";

//  ----------------------------------------------------------------------------
DISCREPANCY_CASE(SHORT_PROT_SEQUENCES, CSeq_inst, eDisc | eOncaller, "Protein sequences should be at least 50 aa, unless they are partial")
//  ----------------------------------------------------------------------------
{
    if (!obj.IsAa() || !obj.IsSetLength() || obj.GetLength() >= 50) {
        return;
    }

    CConstRef<CBioseq> seq = context.GetCurrentBioseq();
    if (!seq) {
        return;
    }

    CBioseq_Handle bsh = context.GetScope().GetBioseqHandle(*seq);
    if (!bsh) {
        return;
    }
    CSeqdesc_CI mi(bsh, CSeqdesc::e_Molinfo);
    if (mi && mi->GetMolinfo().IsSetCompleteness() &&
        mi->GetMolinfo().GetCompleteness() != CMolInfo::eCompleteness_unknown &&
        mi->GetMolinfo().GetCompleteness() != CMolInfo::eCompleteness_complete) {
        return;
    }

    m_Objs[kShortProtSeqs].Add(*context.NewDiscObj(seq), false);
}


//  ----------------------------------------------------------------------------
DISCREPANCY_SUMMARIZE(SHORT_PROT_SEQUENCES)
//  ----------------------------------------------------------------------------
{
    if (m_Objs.empty()) {
        return;
    }
    m_ReportItems = m_Objs.Export(*this)->GetSubitems();
}


// COMMENT_PRESENT

//  ----------------------------------------------------------------------------
DISCREPANCY_CASE(COMMENT_PRESENT, CSeqdesc, eOncaller, "Comment descriptor present")
//  ----------------------------------------------------------------------------
{
    if (obj.IsComment()) {
        CRef<CDiscrepancyObject> this_disc_obj(context.NewDiscObj(CConstRef<CSeqdesc>(&obj), eKeepRef));
        m_Objs["comment"][obj.GetComment()].Add(*this_disc_obj, false);
    }
}


//  ----------------------------------------------------------------------------
DISCREPANCY_SUMMARIZE(COMMENT_PRESENT)
//  ----------------------------------------------------------------------------
{
    if (m_Objs.empty()) {
        return;
    }

    bool all_same = false;
    if (m_Objs["comment"].GetMap().size() == 1) {
        all_same = true;
    }
    string label = all_same ? "[n] comment descriptor[s] were found (all same)" : "[n] comment descriptor[s] were found (some different)";

    CReportNode::TNodeMap::iterator it = m_Objs["comment"].GetMap().begin();
    while (it != m_Objs["comment"].GetMap().end()) {
        NON_CONST_ITERATE(TReportObjectList, robj, m_Objs["comment"][it->first].GetObjects())
        {
            const CDiscrepancyObject* other_disc_obj = dynamic_cast<CDiscrepancyObject*>(robj->GetNCPointer());
            CConstRef<CSeqdesc> comment_desc(dynamic_cast<const CSeqdesc*>(other_disc_obj->GetObject().GetPointer()));
            m_Objs[label].Add(*context.NewDiscObj(comment_desc), false);
        }
        ++it;
    }
    m_Objs.GetMap().erase("comment");

    m_ReportItems = m_Objs.Export(*this)->GetSubitems();
}


// mRNA_ON_WRONG_SEQUENCE_TYPE

const string kmRNAOnWrongSequenceType = "[n] mRNA[s] [is] located on eukaryotic sequence[s] that [does] not have genomic or plasmid source[s]";
//  ----------------------------------------------------------------------------
DISCREPANCY_CASE(mRNA_ON_WRONG_SEQUENCE_TYPE, CSeq_feat_BY_BIOSEQ, eOncaller | eDisc, "Eukaryotic sequences that are not genomic or macronuclear should not have mRNA features")
//  ----------------------------------------------------------------------------
{
    if (!obj.IsSetData() || obj.GetData().GetSubtype() != CSeqFeatData::eSubtype_mRNA) {
        return;
    }
    if (!context.IsEukaryotic()) {
        return;
    }
    if (!context.GetCurrentBioseq() || !context.GetCurrentBioseq()->IsSetInst() ||
        !context.GetCurrentBioseq()->GetInst().IsSetMol() ||
        context.GetCurrentBioseq()->GetInst().GetMol() != CSeq_inst::eMol_dna) {
        return;
    }
    if (!context.GetCurrentMolInfo() || !context.GetCurrentMolInfo()->IsSetBiomol() ||
        context.GetCurrentMolInfo()->GetBiomol() != CMolInfo::eBiomol_genomic) {
        return;
    }
    const CBioSource* src = context.GetCurrentBiosource();
    if (!src || !src->IsSetGenome() ||
        src->GetGenome() == CBioSource::eGenome_macronuclear ||
        src->GetGenome() == CBioSource::eGenome_unknown || 
        src->GetGenome() == CBioSource::eGenome_genomic ||
        src->GetGenome() == CBioSource::eGenome_chromosome) {
        return;
    }
    m_Objs[kmRNAOnWrongSequenceType].Add(*context.NewDiscObj(CConstRef<CSeq_feat>(&obj)), false);

}


//  ----------------------------------------------------------------------------
DISCREPANCY_SUMMARIZE(mRNA_ON_WRONG_SEQUENCE_TYPE)
//  ----------------------------------------------------------------------------
{
    if (m_Objs.empty()) {
        return;
    }
    m_ReportItems = m_Objs.Export(*this)->GetSubitems();
}


// DISC_GAPS

const string kSequencesWithGaps = "[n] sequence[s] contain[s] gaps";

//  ----------------------------------------------------------------------------
DISCREPANCY_CASE(DISC_GAPS, CSeq_inst, eDisc, "Sequences with gaps")
//  ----------------------------------------------------------------------------
{
    if (obj.IsSetRepr() && obj.GetRepr() == CSeq_inst::eRepr_delta) {

        bool has_gaps = false;
        if (obj.IsSetExt() && obj.GetExt().IsDelta()) {

            ITERATE(CDelta_ext::Tdata, it, obj.GetExt().GetDelta().Get()) {

                if ((*it)->IsLiteral() && (*it)->GetLiteral().IsSetSeq_data() && (*it)->GetLiteral().GetSeq_data().IsGap()) {

                    has_gaps = true;
                    break;
                }
            }
        }

        if (!has_gaps) {

            CConstRef<CBioseq> bioseq = context.GetCurrentBioseq();
            if (!bioseq || !bioseq->IsSetAnnot()) {
                return;
            }

            const CSeq_annot* annot = nullptr;
            ITERATE(CBioseq::TAnnot, annot_it, bioseq->GetAnnot()) {
                if ((*annot_it)->IsFtable()) {
                    annot = *annot_it;
                    break;
                }
            }

            if (annot) {

                ITERATE(CSeq_annot::TData::TFtable, feat, annot->GetData().GetFtable()) {

                    if ((*feat)->IsSetData() && (*feat)->GetData().GetSubtype() == CSeqFeatData::eSubtype_gap) {
                        has_gaps = true;
                        break;
                    }
                }
            }
        }

        if (has_gaps) {
            m_Objs[kSequencesWithGaps].Add(*context.NewDiscObj(context.GetCurrentBioseq()), false);
        }
    }
}


//  ----------------------------------------------------------------------------
DISCREPANCY_SUMMARIZE(DISC_GAPS)
//  ----------------------------------------------------------------------------
{
    if (m_Objs.empty()) {
        return;
    }
    m_ReportItems = m_Objs.Export(*this)->GetSubitems();
}


// ONCALLER_BIOPROJECT_ID

const string kSequencesWithBioProjectIDs = "[n] sequence[s] contain[s] BioProject IDs";

//  ----------------------------------------------------------------------------
DISCREPANCY_CASE(ONCALLER_BIOPROJECT_ID, CSeq_inst, eOncaller, "Sequences with BioProject IDs")
//  ----------------------------------------------------------------------------
{
    CBioseq_Handle bioseq = context.GetScope().GetBioseqHandle(*context.GetCurrentBioseq());
    CSeqdesc_CI user_desc_it(bioseq, CSeqdesc::E_Choice::e_User);
    for (; user_desc_it; ++user_desc_it) {

        const CUser_object& user = user_desc_it->GetUser();
        if (user.IsSetData() && user.IsSetType() && user.GetType().IsStr() && user.GetType().GetStr() == "DBLink") {

            ITERATE(CUser_object::TData, user_field, user.GetData()) {

                if ((*user_field)->IsSetLabel() && (*user_field)->GetLabel().IsStr() && (*user_field)->GetLabel().GetStr() == "BioProject" &&
                    (*user_field)->IsSetData() && (*user_field)->GetData().IsStrs()) {

                    const CUser_field::C_Data::TStrs& strs = (*user_field)->GetData().GetStrs();
                    if (!strs.empty() && !strs[0].empty()) {

                        m_Objs[kSequencesWithBioProjectIDs].Add(*context.NewDiscObj(context.GetCurrentBioseq()), false);
                        return;
                    }
                }
            }
        }
    }
}


//  ----------------------------------------------------------------------------
DISCREPANCY_SUMMARIZE(ONCALLER_BIOPROJECT_ID)
//  ----------------------------------------------------------------------------
{
    if (m_Objs.empty()) {
        return;
    }
    m_ReportItems = m_Objs.Export(*this)->GetSubitems();
}

END_SCOPE(NDiscrepancy)
END_NCBI_SCOPE
