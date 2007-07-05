#ifndef ALGO_ALIGN_UTIL_COMPARTMENT_FINDER__HPP
#define ALGO_ALIGN_UTIL_COMPARTMENT_FINDER__HPP

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
* Author:  Yuri Kapustin, Alexander Souvorov
*
* File Description:  CCompartmentFinder- identification of genomic compartments
*                   
* ===========================================================================
*/


#include <corelib/ncbi_limits.hpp>

#include <algo/align/nw/align_exception.hpp>
#include <algo/align/util/hit_filter.hpp>

#include <algorithm>
#include <numeric>
#include <vector>


BEGIN_NCBI_SCOPE


// Detect all compartments over a set of hits
template<class THit>
class CCompartmentFinder {

public:

    typedef CRef<THit>            THitRef;
    typedef vector<THitRef>       THitRefs;
    typedef typename THit::TCoord TCoord;

    // hits must be in plus strand
    CCompartmentFinder(typename THitRefs::const_iterator start,
                       typename THitRefs::const_iterator finish);

    // setters and getters
    void SetMaxIntron (TCoord intr_max) {
        m_intron_max = intr_max;
    }

    static TCoord s_GetDefaultMaxIntron(void) {
        return 1048575;
    }
    
    void SetPenalty(TCoord penalty) {
        m_penalty = penalty;
    }
        
    void SetMinMatches(TCoord min_matches) {
        m_MinSingletonMatches = m_MinMatches = min_matches;
    }
        
    void SetMinSingletonMatches(TCoord min_matches) {
        m_MinSingletonMatches = min_matches;
    }

    static TCoord GetDefaultPenalty(void) {
        return 500;
    }
    
    static TCoord GetDefaultMinCoverage(void) {
        return 500;
    }


    size_t Run(bool cross_filter = false); // do the compartment search

    // order compartments by lower subj coordinate
    void OrderCompartments(void);

    // single compartment representation
    class CCompartment {

    public:
        
        CCompartment(void) {
            m_box[0] = m_box[2] = numeric_limits<TCoord>::max();
            m_box[1] = m_box[3] = 0;
        }

        const THitRefs& GetMembers(void) {
            return m_members;
        }
    
        void AddMember(const THitRef& hitref) {
            m_members.push_back(hitref);
        }

        THitRefs& SetMembers(void) {
            return m_members;
        }
    
        void UpdateMinMax(void);

        bool GetStrand(void) const;

        const THitRef GetFirst(void) const {
            m_iter = 0;
            return GetNext();
        }
        
        const THitRef GetNext(void) const {
            if(m_iter < m_members.size()) {
                return m_members[m_iter++];
            }
            return THitRef(NULL);
        }
        
        const TCoord* GetBox(void) const {
            return m_box;
        }
              
        static bool s_PLowerSubj(const CCompartment& c1,
                                 const CCompartment& c2) {

            return c1.m_box[2] < c2.m_box[2];
        }
        
    protected:
        
        THitRefs            m_members;
        TCoord              m_box[4];
        mutable size_t      m_iter;
    };

    // iterate through compartments
    CCompartment* GetFirst(void);
    CCompartment* GetNext(void);

private:

    TCoord                m_intron_max;    // max intron size
    TCoord                m_penalty;       // penalty per compartment
    TCoord                m_MinMatches;    // min approx matches to report
    TCoord                m_MinSingletonMatches; // min matches for singleton comps

    THitRefs              m_hitrefs;         // input hits
    vector<CCompartment>  m_compartments;    // final compartments
    int                   m_iter;            // GetFirst/Next index
    
    struct SHitStatus {
        
        enum EType {
            eNone, 
            eExtension,
            eOpening
        };
        
        EType  m_type;
        int    m_prev;
        double m_score;        

        SHitStatus(void): m_type(eNone), m_prev(-1), m_score(0) 
        {}
        SHitStatus(EType type, int prev, double score): 
            m_type(type), m_prev(prev), m_score(score) 
        {}
    };    


    static size_t sx_XFilter(THitRefs& hitrefs,
                             typename THitRefs::iterator ihr,
                             typename THitRefs::iterator ihr_e,
                             Uint1 w,
                             size_t min_compartment_hit_len);

    // helper predicate
    static bool s_PNullRef(const THitRef& hr) {
        return hr.IsNull();
    }
};


// Facilitate access to compartments over a hit set
template<class THit>
class CCompartmentAccessor
{
public:

    typedef CCompartmentFinder<THit> TCompartmentFinder;
    typedef typename TCompartmentFinder::THitRefs THitRefs;
    typedef typename TCompartmentFinder::TCoord   TCoord;

    // [start,finish) are assumed to share same query and subj
    CCompartmentAccessor(typename THitRefs::iterator start, 
                         typename THitRefs::iterator finish,
                         TCoord comp_penalty_bps,
                         TCoord min_matches,
                         TCoord min_singleton_matches =numeric_limits<TCoord>::max(),
                         bool cross_filter = false);
    
    bool GetFirst(THitRefs& compartment);
    bool GetNext(THitRefs& compartment);
    
    pair<size_t,size_t> GetCounts(void) const {

        pair<size_t, size_t> rv (m_pending.size(), count(m_status.begin(), 
                                                         m_status.end(), true));
        return rv;
    }
    
    void Get(size_t i, THitRefs& compartment) const {
        compartment = m_pending[i];
    }
    
    const TCoord* GetBox(size_t i) const {
        return &m_ranges.front() + i*4;
    }
    
    bool GetStrand(size_t i) const {
        return m_strands[i];
    }

    bool GetStatus(size_t i) const {
        return m_status[i];
    }
        
private:
    
    vector<THitRefs>         m_pending;
    vector<TCoord>           m_ranges;
    vector<bool>             m_strands;
    vector<bool>             m_status;
    size_t                   m_iter;
        
    void x_Copy2Pending(TCompartmentFinder& finder);
};



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


const double kPenaltyPerIntronBase = -1e-7; // a small penalty to prefer
                                            // more compact models among equal

const double kPenaltyPerIntronPos  = -1e-9; // a small penalty to favor uniform
                                            // selection among equivalent chains

template<class THit>
void CCompartmentFinder<THit>::CCompartment::UpdateMinMax() 
{
    typedef CHitFilter<THit> THitFilter;
    THitFilter::s_GetSpan(m_members, m_box);
}


template<class THit>
bool CCompartmentFinder<THit>::CCompartment::GetStrand() const {

    if(m_members.size()) {
        return m_members.front()->GetSubjStrand();
    }
    else {
        NCBI_THROW( CAlgoAlignException,
                    eInternal,
                    "Strand requested on an empty compartment" );
    }
}


template<class THit>
CCompartmentFinder<THit>::CCompartmentFinder(
    typename THitRefs::const_iterator start,
    typename THitRefs::const_iterator finish ):

    m_intron_max(s_GetDefaultMaxIntron()),
    m_penalty(GetDefaultPenalty()),
    m_MinMatches(1),
    m_MinSingletonMatches(1),
    m_iter(-1)
{
    m_hitrefs.resize(finish - start);
    copy(start, finish, m_hitrefs.begin());
}

// accumulate matches on query
template<class THit>
class CQueryMatchAccumulator:
    public binary_function<double, CRef<THit>, double>
{
public:

    CQueryMatchAccumulator(void): m_Finish(-1.0)
    {}

    double operator() (double acm, CRef<THit> ph)
    {
        const typename THit::TCoord qmin (ph->GetQueryMin()), 
            qmax (ph->GetQueryMax());
        if(qmin > m_Finish)
            return acm + ph->GetIdentity() * ((m_Finish = qmax) - qmin + 1);
        else {
            if(qmax > m_Finish) {
                const double finish0 (m_Finish);
                return acm + ph->GetIdentity() * ((m_Finish = qmax) - finish0);
            }
            else {
                return acm;
            }
        }
    }

private:

    double m_Finish;
};


template<class THit>
double GetTotalMatches(
    const typename CCompartmentFinder<THit>::THitRefs& hitrefs0)
{
    typename CCompartmentFinder<THit>::THitRefs hitrefs (hitrefs0);   

    typedef CHitComparator<THit> THitComparator;
    THitComparator sorter (THitComparator::eQueryMin);
    stable_sort(hitrefs.begin(), hitrefs.end(), sorter);

    const double rv (accumulate(hitrefs.begin(), hitrefs.end(), 0.0, 
                                CQueryMatchAccumulator<THit>()));
    return rv;
}


template<class THit>
size_t CCompartmentFinder<THit>::Run(bool cross_filter)
{
    const double kMinusInf (-1e12);

    m_compartments.clear();
    const int dimhits = m_hitrefs.size();
    if(dimhits == 0) {
        return 0;
    }

    // sort the hits to make sure that each hit is placed after:
    // - hits from which to continue a compartment
    // - hits from which to open a new compartment

    typedef CHitComparator<THit> THitComparator;
    stable_sort(m_hitrefs.begin(), m_hitrefs.end(),
                THitComparator(THitComparator::eSubjMaxQueryMax));

    // For every hit:
    // - evaluate its best extension potential
    // - evaluate its best potential to start a new compartment
    // - select the best variant
    // - update the hit status array
    
    typedef vector<SHitStatus> THitData;
    THitData hitstatus (dimhits);

    const TCoord subj_min_global (m_hitrefs.front()->GetSubjMin());

    int i_bestsofar (0);
    for(int i (0); i < dimhits; ++i) {
        
        const THitRef& h (m_hitrefs[i]);
        const double identity (h->GetIdentity());
        const typename THit::TCoord hbox [4] = {
            h->GetQueryMin(),  h->GetQueryMax(),
            h->GetSubjMin(),   h->GetSubjMax()
        };

//#define CF_DBG_TRACE
#ifdef CF_DBG_TRACE
        cerr << endl << *h << endl;
#endif

        double best_ext_score (kMinusInf);
        int    i_best_ext (-1);

        double best_open_score (identity*(hbox[1] - hbox[0] + 1) - m_penalty);
        int    i_best_open (-1); // each can be a start of the very first compartment

        if(hbox[2] > m_hitrefs[i_bestsofar]->GetSubjMax()) {

            const double score_open (identity*(hbox[1] - hbox[0] + 1) 
                                     + hitstatus[i_bestsofar].m_score
                                     - m_penalty);

            if(score_open > best_open_score) {
                best_open_score = score_open;
                i_best_open = i_bestsofar;
            }
        }
        else {
            // try every prior hit
            for(int j (i - 1); j >= 0; --j) {

                const double score_open (identity * (hbox[1] - hbox[0] + 1)
                                         + hitstatus[j].m_score
                                         - m_penalty);

                if(score_open > best_open_score 
                   && hbox[2] > m_hitrefs[j]->GetSubjMax())
                {
                    best_open_score = score_open;
                    i_best_open = j;
                }
            }
        }

        for(int j = i; j >= 0; --j) {
            
            THitRef phc;
            typename THit::TCoord phcbox[4];
            if(j != i) {

                phc = m_hitrefs[j];
                phcbox[0] = phc->GetQueryMin();
                phcbox[1] = phc->GetQueryMax();
                phcbox[2] = phc->GetSubjMin();
                phcbox[3] = phc->GetSubjMax();

#ifdef CF_DBG_TRACE
                cerr << '\t' << *phc << endl;
#endif
            }
#ifdef CF_DBG_TRACE
            else {
                cerr << "\t[Dummy]" << endl;
            }
#endif
            
            // check if good for extension
            {{
                typename THit::TCoord q0, s0; // possible continuation
                bool good (false);
                int subj_space;
                TCoord intron_start (0);

                if(i != j) {

                    if(phcbox[1] < hbox[1] && phcbox[0] < hbox[0]) {

                        if(hbox[0] <= phcbox[1] && 
                           hbox[2] + phcbox[1] - hbox[0] >= phcbox[3]) {

                            q0 = phcbox[1] + 1;
                            s0 = hbox[2] + phcbox[1] - hbox[0];
                        }
                        else if(phcbox[3] >= hbox[2]) {

                            s0 = phcbox[3] + 1;
                            q0 = hbox[1] - (hbox[3] - s0);
                        }
                        else {
                            
                            q0 = hbox[0];
                            s0 = hbox[2];
                        }
                        subj_space = s0 - phcbox[3] - 1;

                        const TCoord max_gap = 50; // max run of spaces inside an exon
                        good = (subj_space <= int(m_intron_max))
                            && (subj_space + max_gap >= q0 - phcbox[1] - 1)
                            && (s0 < hbox[3] && q0 < hbox[1]);

                        if(good) {
                            intron_start = phcbox[3];
                        }
                    }
                }
                
                if(good) {
                    
                    const double jscore (hitstatus[j].m_score);
                    const double intron_penalty ((subj_space > 0)?
                         (kPenaltyPerIntronPos * (intron_start - subj_min_global) 
                         + subj_space * kPenaltyPerIntronBase): 0.0);

                    const double ext_score (jscore +
                        identity * (hbox[1] - q0 + 1) + intron_penalty);

                    if(ext_score > best_ext_score) {
                        best_ext_score = ext_score;
                        i_best_ext = j;
                    }
#ifdef CF_DBG_TRACE
                    cerr << "\tGood for extension with score = " << ext_score << endl;
#endif
                }
            }}          
        }
        
        typename SHitStatus::EType hit_type;
        int prev_hit;
        double best_score;
        if(best_ext_score > best_open_score) {

            hit_type = SHitStatus::eExtension;
            prev_hit = i_best_ext;
            best_score = best_ext_score;
        }
        else {

            hit_type = SHitStatus::eOpening;
            prev_hit = i_best_open;
            best_score = best_open_score;
        }
                
        hitstatus[i].m_type  = hit_type;
        hitstatus[i].m_prev  = prev_hit;
        hitstatus[i].m_score = best_score;

        if(best_score > hitstatus[i_bestsofar].m_score) {
            i_bestsofar = i;
        }

#ifdef CF_DBG_TRACE
        cerr << "Status = " << ((hit_type == SHitStatus::eOpening)? "Open": "Extend")
             << '\t' << best_score << endl;
        if(prev_hit == -1) {
            cerr << "[Dummy]" << endl;
        }
        else {
            cerr << *(m_hitrefs[prev_hit]) << endl;
        }
#endif
    }
    
#ifdef CF_DBG_TRACE
    cerr << "\n\n--- BACKTRACE ---\n";
#endif

    // *** backtrace ***
    // -  trace back the chain with the highest score
    const double score_best    (hitstatus[i_bestsofar].m_score);    
    const double min_matches   (m_MinSingletonMatches < m_MinMatches? 
        m_MinSingletonMatches: m_MinMatches);

    vector<bool> comp_status;
    if(score_best + m_penalty >= min_matches) {

        int i (i_bestsofar);
        bool new_compartment (true);
        THitRefs hitrefs;
        while(i != -1) {

            if(new_compartment) {

                const double mp (GetTotalMatches<THit>(hitrefs));
                if(mp > 0) {
                    // save the current compartment
                    m_compartments.push_back(CCompartment());
                    m_compartments.back().SetMembers() = hitrefs;
                    comp_status.push_back(mp >= m_MinMatches);
                }

                hitrefs.resize(0);
                new_compartment = false;
            }
            hitrefs.push_back(m_hitrefs[i]);

#ifdef CF_DBG_TRACE
            cerr << *(m_hitrefs[i]) << endl;
#endif            
            
            if(hitstatus[i].m_type == SHitStatus::eOpening) {
                new_compartment = true;
            }
            i = hitstatus[i].m_prev;
        }

        const double mp (GetTotalMatches<THit>(hitrefs));
        if(mp > 0) {
            bool status (m_compartments.size() == 0 && mp >= m_MinSingletonMatches 
                         || mp >= m_MinMatches);
            m_compartments.push_back(CCompartment());
            m_compartments.back().SetMembers() = hitrefs;
            comp_status.push_back(status);
        }
    }

    if(cross_filter && m_compartments.size()) {

        const TCoord kMinCompartmentHitLength (8);

        // partial x-filtering using compartment hits only
        for(size_t icn (m_compartments.size()), ic (0); ic < icn; ++ic) {

            CCompartment & comp (m_compartments[ic]);
            THitRefs& hitrefs (comp.SetMembers());
            size_t nullified (0);
            for(int in (hitrefs.size()), i (in - 1); i > 0; --i) {

                THitRef& h1 (hitrefs[i]);
                THitRef& h2 (hitrefs[i-1]);

                if(h1.IsNull()) continue;

                const TCoord * box1o (h1->GetBox());
                TCoord box1 [4] = {box1o[0], box1o[1], box1o[2], box1o[3]};
                const TCoord * box2o (h2->GetBox());
                TCoord box2 [4] = {box2o[0], box2o[1], box2o[2], box2o[3]};

                const int qd (box1[1] - box2[0] + 1);
                const int sd (box1[3] - box2[2] + 1);
                if(qd > sd && qd > 0) {

                    if(box2[0] <= box1[0] + kMinCompartmentHitLength) {

#ifdef ALGOALIGNUTIL_COMPARTMENT_FINDER_KEEP_TERMINII
                        if(i + 1 == in) {
                            TCoord new_coord (box1[0] + (box1[1] - box1[0])/2);
                            if(box1[0] + 1 <= new_coord) {
                                --new_coord;
                            }
                            else {
                                new_coord = box1[0];
                            }
                            h1->Modify(1, new_coord);
                        }
                        else 
#endif
                        {
                            h1.Reset(0);
                            ++nullified;
                        }
                    }
                    else {
                        h1->Modify(1, box2[0] - 1);
                    }
                    
                    if(box2[1] <= box1[1] + kMinCompartmentHitLength) {

#ifdef ALGOALIGNUTIL_COMPARTMENT_FINDER_KEEP_TERMINII
                        if(i == 1) {
                            TCoord new_coord (box2[0] + (box2[1] - box2[0])/2);
                            if(box2[1] >= new_coord + 1) {
                                ++new_coord;
                            }
                            else {
                                new_coord = box2[1];
                            }
                            h2->Modify(0, new_coord);
                        }
                        else
#endif
                        {
                            h2.Reset(0);
                            ++nullified;
                        }
                    }
                    else {
                        h2->Modify(0, box1[1] + 1);
                    }
                }
                else if (sd > 0) {

                    if(box2[2] <= box1[2] + kMinCompartmentHitLength) {

                        if(i + 1 == in) {
                            TCoord new_coord (box1[2] + (box1[3] - box1[2])/2);
                            if(box1[2] + 1 <= new_coord) {
                                --new_coord;
                            }
                            else {
                                new_coord = box1[2];
                            }
                            h1->Modify(3, new_coord);
                        }
                        else {
                            h1.Reset(0);
                            ++nullified;
                        }
                    }
                    else {
                        h1->Modify(3, box2[2] - 1);
                    }
                    
                    if(box2[3] <= box1[3] + kMinCompartmentHitLength) {

                        if(i == 1) {
                            TCoord new_coord (box2[2] + (box2[3] - box2[2])/2);
                            if(box2[3] >= new_coord + 1) {
                                ++new_coord;
                            }
                            else {
                                new_coord = box2[3];
                            }

                            h2->Modify(2, new_coord);
                        }
                        else {
                            h2.Reset(0);
                            ++nullified;
                        }
                    }
                    else {
                        h2->Modify(2, box1[3] + 1);
                    }
                }
            }

            if(nullified > 0) {
                hitrefs.erase(remove_if(hitrefs.begin(), hitrefs.end(),
                                        CCompartmentFinder<THit>::s_PNullRef),
                              hitrefs.end());
            }
        }

#define ALGO_ALIGN_COMPARTMENT_FINDER_USE_FULL_XFILTERING
#ifdef  ALGO_ALIGN_COMPARTMENT_FINDER_USE_FULL_XFILTERING

        typename THitRefs::iterator ihr_b (m_hitrefs.begin()),
            ihr_e(m_hitrefs.end()), ihr (ihr_b);

        stable_sort(ihr_b, ihr_e, THitComparator(THitComparator::eSubjMinSubjMax));

        // complete x-filtering using the full set of hits
        for(int icn (m_compartments.size()), ic (icn - 1); ic >= 0; --ic) {

            CCompartment & comp (m_compartments[ic]);
            THitRefs& hitrefs (comp.SetMembers());

            if(hitrefs.size() < 3) {
                continue;
            }
            else {
                NON_CONST_ITERATE(typename THitRefs, ii, hitrefs) {
                    (*ii)->SetScore(- (*ii)->GetScore());
                }
                hitrefs.front()->SetEValue(-1);
                hitrefs.back()->SetEValue(-1);
            }
            
            const TCoord comp_subj_min (hitrefs.back()->GetSubjStart());
            const TCoord comp_subj_max (hitrefs.front()->GetSubjStop());
            while(ihr != ihr_e && ((*ihr)->GetSubjStart() < comp_subj_min
                                   || (*ihr)->GetScore() < 0))
            {
                ++ihr;
            }

            if(ihr == ihr_e) break;
            if((*ihr)->GetSubjStart() > comp_subj_max) continue;
            typename THitRefs::iterator ihrc_b (ihr);

            typename THitRefs::iterator ihrc_e (ihr);
            while(ihrc_e != ihr_e && ((*ihrc_e)->GetSubjStart() < comp_subj_max
                                      || (*ihrc_e)->GetScore() < 0))
            {
                ++ihrc_e;
            }

            size_t nullified (sx_XFilter(hitrefs, ihrc_b, ihrc_e, 1, 
                                         kMinCompartmentHitLength));
            sort(ihrc_b, ihrc_e, THitComparator (THitComparator::eQueryMinQueryMax));

            nullified += sx_XFilter(hitrefs, ihrc_b, ihrc_e, 0,
                                    kMinCompartmentHitLength);

            ihr = ihrc_e;

            if(nullified > 0) {
                hitrefs.erase(remove_if(hitrefs.begin(), hitrefs.end(),
                                        CCompartmentFinder<THit>::s_PNullRef),
                              hitrefs.end());
            }
        }

        for(int icn (m_compartments.size()), ic (icn - 1); ic >= 0; --ic) {

            CCompartment & comp (m_compartments[ic]);
            THitRefs& hitrefs (comp.SetMembers());
            NON_CONST_ITERATE(typename THitRefs, ii, hitrefs) {
                const double score ((*ii)->GetScore());
                if(score < 0) {
                    (*ii)->SetScore(-score);
                }
                const double eval ((*ii)->GetEValue());
                if(eval < 0) {
                    (*ii)->SetEValue(0);
                }
            }
        }
#endif
    }

    // mask out compartments with low identity
    for(size_t i (0), in (m_compartments.size()); i < in; ++i) {
        if(comp_status[i] == false) {

            THitRefs & hitrefs (m_compartments[i].SetMembers());
            NON_CONST_ITERATE(typename THitRefs, ii, hitrefs) {
                THitRef & hr (*ii);
                hr->SetScore(-hr->GetScore());
            }
        }
    }

    return m_compartments.size();
}


template<class THit>
size_t CCompartmentFinder<THit>::sx_XFilter(
    THitRefs& hitrefs,
    typename THitRefs::iterator ihr,
    typename THitRefs::iterator ihr_e,
    Uint1 w,
    size_t min_compartment_hit_len)
{
    size_t nullified (0);
    for(int in (hitrefs.size()), i (in - 2); i > 0 && ihr != ihr_e; --i) {

        THitRef& h1 (hitrefs[i]);
        if(h1.IsNull()) continue;
                      
        const TCoord * box1o (h1->GetBox());
        TCoord box1 [4] = {box1o[0], box1o[1], box1o[2], box1o[3]};

        // locate the first overlap
        for(; ihr != ihr_e; ++ihr) {

            THitRef hr (*ihr);
            if(hr.IsNull()) continue;
            if(hr->GetScore() > 0 && hr->GetStop(w) >= box1[2*w]) {
                break;
            }
        }

        if(ihr == ihr_e) {
            break;
        }

        // find the smallest not overlapped hit coord and its interval
        TCoord ls0 (box1[2*w+1] + 1), cursegmax (0);
        TCoord segmax_start(box1[2*w]), segmax_stop(box1[2*w+1]);

        if(box1[2*w] < (*ihr)->GetStart(w)) {

            segmax_start = ls0 = box1[2*w];
            segmax_stop = (*ihr)->GetStart(w) - 1;
            cursegmax = segmax_stop - segmax_start + 1;
        }
        else {

            TCoord shrsmax ((*ihr)->GetStop(w));
            for(++ihr; ihr != ihr_e; ++ihr) {

                THitRef hr (*ihr);
                if(hr.IsNull() || hr->GetScore() < 0) {
                    continue;
                }

                const TCoord hrs0 (hr->GetStart(w));
                if(hrs0 > box1[2*w+1]) {
                    segmax_stop = box1[2*w+1];
                    break;
                }

                if(hrs0 > shrsmax) {
                    segmax_stop = hrs0 - 1;
                    break;
                }

                const TCoord hrs1 (hr->GetStop(w));
                if(hrs1 > shrsmax) {
                    shrsmax = hrs1;
                }
            }

            if(shrsmax < box1[2*w+1]) {
                segmax_start = ls0 = shrsmax + 1;
                cursegmax = segmax_stop - segmax_start + 1;
            }
        }
                
        if(ls0 > box1[2*w+1]) {
            h1.Reset(0);
            ++nullified;
            continue;
        }

        // find the longest surviving segment       
        for(; ihr != ihr_e; ++ihr) {

            THitRef hr (*ihr);
            if(hr.IsNull() || hr->GetScore() < 0) {
                continue;
            }

            const TCoord hrs0 (hr->GetStart(w));
            if(hrs0 > box1[2*w+1]) {
                if(ls0 <= box1[2*w+1] && box1[2*w+1] + 1 - ls0 > cursegmax) {
                    segmax_start = ls0;
                    segmax_stop = box1[2*w+1];
                    cursegmax = segmax_stop - segmax_start + 1;
                }
                break;
            }

            if(hrs0 > ls0) {
                if(hrs0 - ls0 > cursegmax) {
                    segmax_start = ls0;
                    segmax_stop = hrs0 - 1;
                    cursegmax = segmax_stop - segmax_start + 1;
                }
                
                ls0 = hr->GetStop(w) + 1;
            }
            else if(hr->GetStop(w) + 1 > ls0) {
                ls0 = hr->GetStop(w) + 1;
            }
        }

        if(box1[2*w+1] > ls0 + cursegmax) {
            segmax_start = ls0;
            segmax_stop  = box1[2*w+1];
            cursegmax    = segmax_stop - segmax_start + 1;
        }
                
        if(cursegmax < min_compartment_hit_len) {
            h1.Reset(0);
            ++nullified;
            continue;
        }
        else {

            if(box1[2*w] < segmax_start) {
                h1->Modify(2 * w, segmax_start);
            }
            
            if(segmax_stop < box1[2*w+1]) {
                h1->Modify(2 * w + 1, segmax_stop);
            }
        }
        
        if(ihr == ihr_e) break;
    }

    return nullified;
}


template<class THit>
typename CCompartmentFinder<THit>::CCompartment* 
CCompartmentFinder<THit>::GetFirst()
{
    if(m_compartments.size()) {
        m_iter = 0;
        return &m_compartments[m_iter++];
    }
    else {
        m_iter = -1;
        return 0;
    }
}

template<class THit>
void CCompartmentFinder<THit>::OrderCompartments(void) {
  
    for(size_t i = 0, dim = m_compartments.size(); i < dim; ++i) {
        m_compartments[i].UpdateMinMax();
    }
    
    stable_sort(m_compartments.begin(), m_compartments.end(), 
                CCompartmentFinder<THit>::CCompartment::s_PLowerSubj);
}

template<class THit>
typename CCompartmentFinder<THit>::CCompartment* 
CCompartmentFinder<THit>::GetNext()
{
    const size_t dim = m_compartments.size();
    if(m_iter == -1 || m_iter >= int(dim)) {
        m_iter = -1;
        return 0;
    }
    else {
        return &m_compartments[m_iter++];
    }
}


template<class THit>
CCompartmentAccessor<THit>::CCompartmentAccessor(
     typename THitRefs::iterator istart,
     typename THitRefs::iterator ifinish,
     TCoord comp_penalty,
     TCoord min_matches,
     TCoord min_singleton_matches,
     bool cross_filter)
{
    const TCoord kMax_TCoord (numeric_limits<TCoord>::max());

    const TCoord max_intron (CCompartmentFinder<THit>::s_GetDefaultMaxIntron());

    // separate strands for CompartmentFinder
    typename THitRefs::iterator ib = istart, ie = ifinish, ii = ib, 
        iplus_beg = ie;
    typedef CHitComparator<THit> THitComparator;
    THitComparator sorter (THitComparator::eSubjStrand);
    stable_sort(ib, ie, sorter);

    TCoord minus_subj_min = kMax_TCoord, minus_subj_max = 0;
    for(ii = ib; ii != ie; ++ii) {
        if((*ii)->GetSubjStrand()) {
            iplus_beg = ii;
            break;
        }
        else {
            if((*ii)->GetSubjMin() < minus_subj_min) {
                minus_subj_min = (*ii)->GetSubjMin();
            }
            if((*ii)->GetSubjMax() > minus_subj_max) {
                minus_subj_max = (*ii)->GetSubjMax();
            }
        }
    }

    // minus
    {{
        // flip
        for(ii = ib; ii != iplus_beg; ++ii) {

            const typename THit::TCoord s0 = minus_subj_min + minus_subj_max 
                - (*ii)->GetSubjMax();
            const typename THit::TCoord s1 = minus_subj_min + minus_subj_max 
                - (*ii)->GetSubjMin();
            (*ii)->SetSubjStart(s0);
            (*ii)->SetSubjStop(s1);
        }
        
        CCompartmentFinder<THit> finder (ib, iplus_beg);
        finder.SetPenalty(comp_penalty);
        finder.SetMinMatches(min_matches);
        finder.SetMinSingletonMatches(min_singleton_matches);
        finder.SetMaxIntron(max_intron);
        finder.Run(cross_filter);
        
        // un-flip
        for(ii = ib; ii != iplus_beg; ++ii) {

            const typename THit::TCoord s0 = minus_subj_min + minus_subj_max 
                - (*ii)->GetSubjMax();
            const typename THit::TCoord s1 = minus_subj_min + minus_subj_max 
                - (*ii)->GetSubjMin();
            (*ii)->SetSubjStart(s1);
            (*ii)->SetSubjStop(s0);
        }        

        x_Copy2Pending(finder);
    }}

    // plus
    {{
        CCompartmentFinder<THit> finder (iplus_beg, ie);
        finder.SetPenalty(comp_penalty);
        finder.SetMinMatches(min_matches);
        finder.SetMinSingletonMatches(min_singleton_matches);
        finder.SetMaxIntron(max_intron);
        finder.Run(cross_filter);
        x_Copy2Pending(finder);
    }}
}


template<class THit>
void CCompartmentAccessor<THit>::x_Copy2Pending(
    CCompartmentFinder<THit>& finder)
{
    finder.OrderCompartments();

    typedef typename CCompartmentFinder<THit>::THitRef THitRef;

    // copy to pending
    for(typename CCompartmentFinder<THit>::CCompartment* compartment =
            finder.GetFirst(); compartment; compartment = finder.GetNext()) {
        
        if(compartment->GetMembers().size() > 0) {

            m_pending.push_back(THitRefs(0));
            THitRefs& vh = m_pending.back();
        
            for(THitRef ph (compartment->GetFirst()); ph; ph = compartment->GetNext())
            {
                vh.push_back(ph);
            }
        
            const TCoord* box = compartment->GetBox();
            m_ranges.push_back(box[0] - 1);
            m_ranges.push_back(box[1] - 1);
            m_ranges.push_back(box[2] - 1);
            m_ranges.push_back(box[3] - 1);
        
            m_strands.push_back(compartment->GetStrand());
            m_status.push_back(compartment->GetFirst()->GetScore() > 0);
        }
    }
}


template<class THit>
bool CCompartmentAccessor<THit>::GetFirst(THitRefs& compartment) {

    m_iter = 0;
    return GetNext(compartment);
}


template<class THit>
bool CCompartmentAccessor<THit>::GetNext(THitRefs& compartment) {

    compartment.clear();
    if(m_iter < m_pending.size()) {
        compartment = m_pending[m_iter++];
        return true;
    }
    else {
        return false;
    }
}



END_NCBI_SCOPE


#endif
