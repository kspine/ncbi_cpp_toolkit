#ifndef DATA_SOURCE__HPP
#define DATA_SOURCE__HPP

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
* Author: Aleksey Grichenko, Michael Kimelman
*
* File Description:
*   Data source for object manager
*
*/

#include "tse_info.hpp"
#include "seq_id_mapper.hpp"
#include <objects/objmgr/object_manager.hpp>
#include <objects/objmgr/scope.hpp>
#include <objects/objmgr/seq_map.hpp>
#include <objects/objmgr/data_loader.hpp>
#include <objects/seq/Seq_data.hpp>
#include <objects/seq/Seq_annot.hpp>
#include <corelib/ncbithr.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)


class CDelta_seq;


class CDataSource : public CObject
{
public:
    /// 'ctors
    CDataSource(CDataLoader& loader, CObjectManager& objmgr);
    CDataSource(CSeq_entry& entry, CObjectManager& objmgr);
    virtual ~CDataSource(void);

    /// Register new TSE (Top Level Seq-entry)
    typedef set< CRef<CTSE_Info> > TTSESet;
    CTSE_Info* AddTSE(CSeq_entry& se, TTSESet* tse_set, bool dead = false);

    /// Add new sub-entry to "parent".
    /// Return FALSE and do nothing if "parent" is not a node in an
    /// existing TSE tree of this data-source.
    bool AttachEntry(const CSeq_entry& parent, CSeq_entry& entry);

    /// Add sequence map for the given Bioseq.
    /// Return FALSE and do nothing if "bioseq" is not a node in an
    /// existing TSE tree of this data-source, or if "bioseq" is not a Bioseq.
    bool AttachMap(const CSeq_entry& bioseq, CSeqMap& seqmap);

    /// Add seq-data to a Bioseq.
    /// Return FALSE and do nothing if "bioseq" is not a node in an
    /// existing TSE tree of this data-source, or if "bioseq" is not a Bioseq.
    /// If "start" is 0, "length" is equal to the sequence length (from
    /// the bioseq's seq-inst), and the segment is of type "literal", the data
    /// is added as a regular seq-data. Otherwise the bioseq is treated as a
    /// segmented sequence and the segment is added as a delta-ext segment.
    /// "seq_seg" must be allocated on the heap, since it will be referenced
    /// by the bioseq (except the case of a non-segmented bioseq, when only
    /// seq-data part of the seq_seg will be locked).
    bool AttachSeqData(const CSeq_entry& bioseq, CDelta_seq& seq_seg,
                       TSeqPosition start, TSeqLength length);

    /// Add annotations to a Seq-entry.
    /// Return FALSE and do nothing if "parent" is not a node in an
    /// existing TSE tree of this data-source.
    bool AttachAnnot(const CSeq_entry& entry, CSeq_annot& annot);

    /// Get Bioseq handle by Seq-Id.
    /// Return "NULL" handle if the Bioseq cannot be resolved.
    CBioseq_Handle GetBioseqHandle(CScope& scope, CSeqMatch_Info& info);

    // Filter set of CSeq_id (setSource)
    // Select from the setSource ones which are "owned" by this DataSource
    // and move them into setResult
    void FilterSeqid(TSeq_id_HandleSet& setResult,
                     TSeq_id_HandleSet& setSource) const;

    // Remove TSE from the datasource, update indexes
    bool DropTSE(const CSeq_entry& tse);

    // Contains (or can load) any entries?
    bool IsEmpty(void) const;

    CSeq_id_Mapper& GetIdMapper(void) const;

    /// Get the complete Bioseq (as loaded by now)
    const CBioseq& GetBioseq(const CBioseq_Handle& handle);

    /// Get top level Seq-Entry for a Bioseq
    const CSeq_entry& GetTSE(const CBioseq_Handle& handle);

    /// Get Bioseq core structure
    CBioseq_Handle::TBioseqCore GetBioseqCore(const CBioseq_Handle& handle);

    /// Get sequence map
    const CSeqMap& GetSeqMap(const CBioseq_Handle& handle);
    /// Get sequence map with resolved multi-level references
    const CSeqMap& GetResolvedSeqMap(CBioseq_Handle handle);

    /// Get a piece of seq. data ("seq_piece"), which the "point" belongs to:
    ///   "length"     -- length   of the data piece the "point" is found in;
    ///   "dest_start" -- position of the data piece in the dest. Bioseq;
    ///   "src_start"  -- position of the data piece in the source Bioseq;
    ///   "src_data"   -- Seq-data of the last Bioseq found in the chain;
    /// Return FALSE if the final source Bioseq (i.e. the Bioseq that actually
    /// describe the piece of sequence data containing the "point") cannot be
    /// found.
    bool GetSequence(const CBioseq_Handle& handle,
                     TSeqPosition point,
                     SSeqData* seq_piece,
                     CScope& scope);

    CDataLoader* GetDataLoader(void);
    CSeq_entry* GetTopEntry(void);

    // Internal typedefs
    typedef CTSE_Info::TRange                       TRange;
    typedef CTSE_Info::TRangeMap                    TRangeMap;
    typedef CTSE_Info::TAnnotMap                    TAnnotMap;
    typedef map<CRef<CSeq_entry>, CRef<CTSE_Info> > TEntries;
    typedef CTSE_Info::TBioseqMap                   TBioseqMap;
    typedef map<CSeq_id_Handle, TTSESet>            TTSEMap;
    typedef map<const CBioseq*, CRef<CSeqMap> >     TSeqMaps;

    // Get TSEs containing annotations for the given location
    void PopulateTSESet(CHandleRangeMap& loc,
                        TTSESet& tse_set,
                        CSeq_annot::C_Data::E_Choice sel,
                        CScope& scope);

    // Fill the set with bioseq handles for all sequences from a given TSE.
    // Return false if the entry was not found or is not a TSE. "filter" may
    // be used to select a particular sequence type.
    // Used to initialize bioseq iterators.
    bool GetTSEHandles(CScope& scope,
                       const CSeq_entry& entry,
                       set<CBioseq_Handle>& handles,
                       CSeq_inst::EMol filter);

    CSeqMatch_Info BestResolve(const CSeq_id& id, CScope& scope);

    bool IsSynonym(const CSeq_id& id1, CSeq_id& id2) const;

    string GetName(void) const;

    virtual void DebugDump(CDebugDumpContext ddc, unsigned int depth) const;

private:
    // Process seq-entry recursively
    // Return TSE info for the indexed entry. For new TSEs the info is locked.
    CTSE_Info* x_IndexEntry (CSeq_entry& entry,
                             CSeq_entry& tse,
                             bool dead,
                             TTSESet* tse_set);
    CTSE_Info* x_AddToBioseqMap (CSeq_entry& entry,
                                  bool dead,
                                  TTSESet* tse_set);
    void x_AddToAnnotMap  (CSeq_entry& entry);

    // Find the seq-entry with best bioseq for the seq-id handle.
    // The best bioseq is the bioseq from the live TSE or from the
    // only one TSE containing the ID (no matter live or dead).
    // If no matches were found, return 0.
    CTSE_Info* x_FindBestTSE(CSeq_id_Handle handle,
        const CScope::TRequestHistory& history) const;

    // Create CSeqMap for a bioseq
    void x_CreateSeqMap(const CBioseq& seq);
    void x_LocToSeqMap(const CSeq_loc& loc, TSeqPos& pos, CSeqMap& seqmap);
    void x_DataToSeqMap(const CSeq_data& data,
                        TSeqPos& pos, TSeqPos len,
                        CSeqMap& seqmap);
    // Non-const version of GetSeqMap()
    CSeqMap& x_GetSeqMap(const CBioseq_Handle& handle);

    // Process a single data element
    void x_MapFeature(const CSeq_feat& feat,
                      const CSeq_annot& annot,
                      CTSE_Info& tse);
    void x_MapAlign(const CSeq_align& align,
                    const CSeq_annot& annot,
                    CTSE_Info& tse);
    void x_MapGraph(const CSeq_graph& graph,
                    const CSeq_annot& annot,
                    CTSE_Info& tse);

    // Check if the Seq-entry is handled by this data-source
    CSeq_entry* x_FindEntry(const CSeq_entry& entry);

    // Remove entry from the datasource, update indexes
    void x_DropEntry(CSeq_entry& entry);
    void x_DropAnnotMap(CSeq_entry& entry);
    // Process a single data element
    void x_DropFeature(const CSeq_feat& feat, const CSeq_annot& annot);
    void x_DropAlign(const CSeq_align& align, const CSeq_annot& annot);
    void x_DropGraph(const CSeq_graph& graph, const CSeq_annot& annot);

    // Global cleanup -- search for unlocked TSEs and drop them.
    void x_CleanupUnusedEntries(void);

    // Change live/dead status of a TSE
    void x_UpdateTSEStatus(CSeq_entry& tse, bool dead);

    // Replace initial handle range map with the new one, containing duplicate
    // range sets for each synonym of each handle
    void x_ResolveLocationHandles(CHandleRangeMap& loc,
        const CScope::TRequestHistory& history) const;

    // Resolve the reference to rh, add the resolved interval(s) to dmap.
    // "start" is referenced region position on the "rh" sequence.
    // "dpos" is the starting point on the master sequence.
    void x_ResolveMapSegment(CSeq_id_Handle rh,
                             TSeqPos start, TSeqPos len,
                             CSeqMap& dmap, TSeqPos& dpos, TSeqPos dstop,
                             CScope& scope);

    // Used to lock: m_Entries, m_TSE_seq, m_TSE_ref, m_SeqMaps
    mutable CMutex m_DataSource_Mtx;

    CDataLoader          *m_Loader;
    CRef<CSeq_entry>      m_pTopEntry;
    CObjectManager*       m_ObjMgr;

    TEntries              m_Entries;   // All known seq-entries and their TSEs
    TTSEMap               m_TSE_seq;   // id -> TSEs with bioseq
    TTSEMap               m_TSE_ref;   // id -> TSEs with references to id
    TSeqMaps              m_SeqMaps;   // Sequence maps for bioseqs
    // "true" if annotations need to be indexed immediately, "false" while
    // delayed indexing is allowed.
    bool                  m_IndexedAnnot;

    friend class CAnnot_CI;
    friend class CAnnotTypes_CI; // using mutex etc.
    friend class CBioseq_Handle; // using mutex
    friend class CGBDataLoader;  //
};

inline
CDataSource::CDataSource(CDataLoader& loader, CObjectManager& objmgr)
    : m_Loader(&loader), m_pTopEntry(0), m_ObjMgr(&objmgr)
{
    m_Loader->SetTargetDataSource(*this);
}

inline
CDataSource::CDataSource(CSeq_entry& entry, CObjectManager& objmgr)
    : m_Loader(0), m_pTopEntry(&entry), m_ObjMgr(&objmgr),
      m_IndexedAnnot(false)
{
    x_AddToBioseqMap(entry, false, 0);
}

inline
CDataSource::~CDataSource(void)
{
    // Find and drop each TSE
    while (m_Entries.size() > 0) {
        _ASSERT( !m_Entries.begin()->second->CounterLocked() );
        DropTSE(*(m_Entries.begin()->second->m_TSE));
    }
    if(m_Loader) delete m_Loader;
}

inline
CDataLoader* CDataSource::GetDataLoader(void)
{
    return m_Loader;
}

inline
CSeq_entry* CDataSource::GetTopEntry(void)
{
    return m_pTopEntry;
}

inline
bool CDataSource::IsEmpty(void) const
{
    return m_Loader == 0  &&  m_Entries.empty();
}

inline
CSeq_id_Mapper& CDataSource::GetIdMapper(void) const
{
    return *m_ObjMgr->m_IdMapper;
}


END_SCOPE(objects)
END_NCBI_SCOPE

/*
* ---------------------------------------------------------------------------
* $Log$
* Revision 1.29  2002/10/18 19:12:40  grichenk
* Removed mutex pools, converted most static mutexes to non-static.
* Protected CSeqMap::x_Resolve() with mutex. Modified code to prevent
* dead-locks.
*
* Revision 1.28  2002/10/02 17:58:23  grichenk
* Added sequence type filter to CBioseq_CI
*
* Revision 1.27  2002/09/30 20:01:19  grichenk
* Added methods to support CBioseq_CI
*
* Revision 1.26  2002/07/08 20:51:01  grichenk
* Moved log to the end of file
* Replaced static mutex (in CScope, CDataSource) with the mutex
* pool. Redesigned CDataSource data locking.
*
* Revision 1.25  2002/06/04 17:18:33  kimelman
* memory cleanup :  new/delete/Cref rearrangements
*
* Revision 1.24  2002/05/31 17:53:00  grichenk
* Optimized for better performance (CTSE_Info uses atomic counter,
* delayed annotations indexing, no location convertions in
* CAnnot_Types_CI if no references resolution is required etc.)
*
* Revision 1.23  2002/05/28 18:00:43  gouriano
* DebugDump added
*
* Revision 1.22  2002/05/14 20:06:26  grichenk
* Improved CTSE_Info locking by CDataSource and CDataLoader
*
* Revision 1.21  2002/05/06 03:28:47  vakatov
* OM/OM1 renaming
*
* Revision 1.20  2002/05/03 21:28:09  ucko
* Introduce T(Signed)SeqPos.
*
* Revision 1.19  2002/04/17 21:09:14  grichenk
* Fixed annotations loading
* +IsSynonym()
*
* Revision 1.18  2002/04/11 12:08:21  grichenk
* Fixed GetResolvedSeqMap() implementation
*
* Revision 1.17  2002/03/28 14:02:31  grichenk
* Added scope history checks to CDataSource::x_FindBestTSE()
*
* Revision 1.16  2002/03/20 04:50:13  kimelman
* GB loader added
*
* Revision 1.15  2002/03/07 21:25:34  grichenk
* +GetSeq_annot() in annotation iterators
*
* Revision 1.14  2002/03/05 18:44:55  grichenk
* +x_UpdateTSEStatus()
*
* Revision 1.13  2002/03/05 16:09:10  grichenk
* Added x_CleanupUnusedEntries()
*
* Revision 1.12  2002/03/04 15:09:27  grichenk
* Improved MT-safety. Added live/dead flag to CDataSource methods.
*
* Revision 1.11  2002/03/01 19:41:34  gouriano
* *** empty log message ***
*
* Revision 1.10  2002/02/28 20:53:32  grichenk
* Implemented attaching segmented sequence data. Fixed minor bugs.
*
* Revision 1.9  2002/02/21 19:27:05  grichenk
* Rearranged includes. Added scope history. Added searching for the
* best seq-id match in data sources and scopes. Updated tests.
*
* Revision 1.8  2002/02/07 21:27:35  grichenk
* Redesigned CDataSource indexing: seq-id handle -> TSE -> seq/annot
*
* Revision 1.7  2002/02/06 21:46:11  gouriano
* *** empty log message ***
*
* Revision 1.6  2002/02/05 21:46:28  gouriano
* added FindSeqid function, minor tuneup in CSeq_id_mapper
*
* Revision 1.5  2002/01/29 17:45:00  grichenk
* Added seq-id handles locking
*
* Revision 1.4  2002/01/28 19:44:49  gouriano
* changed the interface of BioseqHandle: two functions moved from Scope
*
* Revision 1.3  2002/01/23 21:59:31  grichenk
* Redesigned seq-id handles and mapper
*
* Revision 1.2  2002/01/18 15:56:24  gouriano
* changed TSeqMaps definition
*
* Revision 1.1  2002/01/16 16:25:55  gouriano
* restructured objmgr
*
* Revision 1.1  2002/01/11 19:04:01  gouriano
* restructured objmgr
*
*
* ===========================================================================
*/

#endif  // DATA_SOURCE__HPP
