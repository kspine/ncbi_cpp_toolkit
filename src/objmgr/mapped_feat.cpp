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
*   class CMappedFeat to represent feature with mapped locations
*
*/

#include <ncbi_pch.hpp>
#include <objmgr/mapped_feat.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

class CScope;

CMappedFeat::CMappedFeat(void)
{
    m_MappingInfoPtr = &m_MappingInfoObj;
}


CMappedFeat::CMappedFeat(const CSeq_feat_Handle& feat)
    : CSeq_feat_Handle(feat)
{
    m_MappingInfoPtr = &m_MappingInfoObj;
}


CMappedFeat::CMappedFeat(const CMappedFeat& feat)
{
    *this = feat;
}


CMappedFeat& CMappedFeat::operator=(const CMappedFeat& feat)
{
    if ( this != &feat ) {
        CSeq_feat_Handle::operator=(feat);
        m_MappingInfoObj = *feat.m_MappingInfoPtr;
        m_MappingInfoPtr = &m_MappingInfoObj;
        m_MappedFeat = feat.m_MappedFeat;
        m_OriginalSeq_feat_Lock.Reset();
    }
    return *this;
}


CMappedFeat::~CMappedFeat(void)
{
}


void CMappedFeat::Reset(void)
{
    CSeq_feat_Handle::Reset();
    m_MappingInfoObj.Reset();
    m_MappingInfoPtr = &m_MappingInfoObj;
    m_MappedFeat.ResetRefs();
    m_OriginalSeq_feat_Lock.Reset();
}


CMappedFeat& CMappedFeat::Set(CAnnot_Collector& collector,
                              const TIterator& annot)
{
    m_OriginalSeq_feat_Lock.Reset();

    const CAnnotObject_Ref& feat_ref = *annot;
    _ASSERT(feat_ref.IsFeat());

    if ( feat_ref.IsSNPFeat() ) {
        m_FeatIndex = feat_ref.GetAnnotIndex() | kSNPTableBit;
        if ( !collector.m_CreatedOriginal ) {
            collector.m_CreatedOriginal.Reset(new CCreatedFeat_Ref);
        }
        m_CreatedFeat = collector.m_CreatedOriginal;
        _ASSERT(IsTableSNP());
    }
    else if ( feat_ref.GetAnnotObject_Info().IsRegular() ) {
        m_FeatIndex = feat_ref.GetAnnotIndex();
        _ASSERT(!IsTableSNP());
    }
    else {
        m_FeatIndex = feat_ref.GetAnnotIndex();
        if ( !collector.m_CreatedOriginal ) {
            collector.m_CreatedOriginal.Reset(new CCreatedFeat_Ref);
        }
        m_CreatedFeat = collector.m_CreatedOriginal;
        _ASSERT(!IsTableSNP());
    }
    m_Seq_annot = annot->GetSeq_annot_Handle();

    m_MappingInfoPtr = &feat_ref.GetMappingInfo();
    m_MappedFeat.ResetRefs();
    return *this;
}


const CSeq_loc& CMappedFeat::GetMappedLocation(void) const
{
    return *m_MappedFeat.MakeMappedLocation(*m_MappingInfoPtr);
}


const CSeq_feat& CMappedFeat::GetMappedFeature(void) const
{
    if ( m_MappingInfoPtr->IsMapped() ) {
        CRef<CSeq_loc> mapped_location
            (&const_cast<CSeq_loc&>(GetMappedLocation()));
        return *m_MappedFeat.MakeMappedFeature(*this,
                                               *m_MappingInfoPtr,
                                               *mapped_location);
    }
    else {
        return GetOriginalFeature();
    }
}


const CSeq_feat& CMappedFeat::GetOriginalFeature(void) const
{
    if ( !m_OriginalSeq_feat_Lock ) {
        m_OriginalSeq_feat_Lock = GetOriginalSeq_feat();
    }
    return *m_OriginalSeq_feat_Lock;
}


CConstRef<CSeq_feat> CMappedFeat::GetSeq_feat(void) const
{
    return ConstRef(&GetMappedFeature());
}


bool CMappedFeat::IsSetPartial(void) const
{
    return m_MappingInfoPtr->IsPartial();
}


bool CMappedFeat::GetPartial(void) const
{
    return m_MappingInfoPtr->IsPartial();
}


const CSeq_loc& CMappedFeat::GetProduct(void) const
{
    return m_MappingInfoPtr->IsMappedProduct()?
        GetMappedLocation(): GetSeq_feat()->GetProduct();
}


const CSeq_loc& CMappedFeat::GetLocation(void) const
{
    return m_MappingInfoPtr->IsMappedLocation()?
        GetMappedLocation(): GetSeq_feat()->GetLocation();
}


CMappedFeat::TRange CMappedFeat::GetRange(void) const
{
    return m_MappingInfoPtr->IsMappedLocation()?
        m_MappingInfoPtr->GetTotalRange():
        GetSeq_feat()->GetLocation().GetTotalRange();
}


END_SCOPE(objects)
END_NCBI_SCOPE
