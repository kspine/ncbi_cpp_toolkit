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
 * Authors: Sema
 *
 */

#include <ncbi_pch.hpp>
#include "discrepancy_core.hpp"
#include <sstream>
#include <objmgr/util/sequence.hpp>


BEGIN_NCBI_SCOPE;
BEGIN_SCOPE(NDiscrepancy)
USING_SCOPE(objects);

CSafeStatic<map<string, CDiscrepancyConstructor*> > CDiscrepancyConstructor::sm_Table;
CSafeStatic<map<string, string> > CDiscrepancyConstructor::sm_DescrTable;
CSafeStatic<map<string, TGroup> > CDiscrepancyConstructor::sm_GroupTable;
CSafeStatic<map<string, string> > CDiscrepancyConstructor::sm_AliasTable;
CSafeStatic<map<string, vector<string> > > CDiscrepancyConstructor::sm_AliasListTable;


string CDiscrepancyConstructor::GetDiscrepancyCaseName(const string& name)
{
    map<string, CDiscrepancyConstructor*>& Table = GetTable();
    map<string, string>& AliasTable = GetAliasTable();
    if (Table.find(name) != Table.end()) {
        return name;
    }
    if (AliasTable.find(name) != AliasTable.end()) {
        return AliasTable[name];
    }
    if (name.substr(0, 5) == "DISC_") {
        return GetDiscrepancyCaseName(name.substr(5));
    }
    return "";
}


const CDiscrepancyConstructor* CDiscrepancyConstructor::GetDiscrepancyConstructor(const string& name)
{
    string str = GetDiscrepancyCaseName(name);
    return str.empty() ? 0 : GetTable()[str];
}


string GetDiscrepancyCaseName(const string& name)
{
    return CDiscrepancyConstructor::GetDiscrepancyCaseName(name);
}


string GetDiscrepancyDescr(const string& name)
{
    string str = GetDiscrepancyCaseName(name);
    return str.empty() ? "" : CDiscrepancyConstructor::GetDescrTable()[str];
}


TGroup GetDiscrepancyGroup(const string& name)
{
    string str = GetDiscrepancyCaseName(name);
    return str.empty() ? 0 : CDiscrepancyConstructor::GetGroupTable()[str];
}


vector<string> GetDiscrepancyNames()
{
    typedef map<string, CDiscrepancyConstructor*> MyMap;
    map<string, CDiscrepancyConstructor*>& Table = CDiscrepancyConstructor::GetTable();
    vector<string> V;
    ITERATE (MyMap, J, Table) {
        V.push_back(J->first);
    }
    return V;
}


vector<string> GetDiscrepancyAliases(const string& name)
{
    map<string, vector<string> >& AliasListTable = CDiscrepancyConstructor::GetAliasListTable();
    return AliasListTable.find(name)!=AliasListTable.end() ? AliasListTable[name] : vector<string>();
}


CReportNode& CReportNode::operator[](const string& name)
{
    if (m_Map.find(name) == m_Map.end()) {
        m_Map[name] = CRef<CReportNode>(new CReportNode(name));
    }
    return *m_Map[name];
}


void CReportNode::Add(TReportObjectList& list, CReportObj& obj, bool unique)
{
    if (unique) {
        ITERATE(TReportObjectList, it, list) {
            if (static_cast<CDiscrepancyObject&>(obj).Equal(**it)) {
                return;
            }
        }
    }
    list.push_back(CRef<CReportObj>(&obj));
}


void CReportNode::Add(TReportObjectList& list, TReportObjectList& objs, bool unique)
{
    NON_CONST_ITERATE (TReportObjectList, it, objs) {
        Add(list, **it, unique);
    }
}


CRef<CReportItem> CReportNode::Export(CDiscrepancyCase& test, bool unique)
{
    TReportObjectList objs = m_Objs;
    TReportItemList subs;
    bool autofix = false;
    NON_CONST_ITERATE (TNodeMap, it, m_Map) {
        CRef<CReportItem> sub = it->second->Export(test, unique);
        subs.push_back(sub);
        TReportObjectList details = sub->GetDetails();
        NON_CONST_ITERATE (TReportObjectList, ob, details) {
            Add(objs, **ob, unique);
        }
    }
    NON_CONST_ITERATE(TReportObjectList, ob, objs) {
        if ((*ob)->CanAutofix()) {
            autofix = true;
        }
    }
    string str = m_Name;
    NStr::TruncateSpacesInPlace(str);
    NStr::ReplaceInPlace(str, "[n]", NStr::Int8ToString(objs.size()));
    NStr::ReplaceInPlace(str, "[s]", objs.size() == 1 ? "" : "s");
    NStr::ReplaceInPlace(str, "[S]", objs.size() == 1 ? "s" : "");
    NStr::ReplaceInPlace(str, "[is]", objs.size() == 1 ? "is" : "are");
    NStr::ReplaceInPlace(str, "[does]", objs.size() == 1 ? "does" : "do");
    NStr::ReplaceInPlace(str, "[has]", objs.size() == 1 ? "has" : "have");
    CRef<CDiscrepancyItem> item(new CDiscrepancyItem(test, str));
    item->m_Autofix = autofix;
    item->m_Subs = subs;
    item->m_Objs = objs;
    return CRef<CReportItem>((CReportItem*)item);
}


template<typename T> void CDiscrepancyVisitor<T>::Call(const T& obj, CDiscrepancyContext& context)
{
    try {
        Visit(obj, context);
    }
    catch (CException& e) {
        string ss = "EXCEPTION caught: "; ss += e.what();
        m_Objs[ss].Add(*new CDiscrepancyObject(context.GetCurrentBioseq(), context.GetScope(), context.GetFile(), false));
    }
}


CRef<CDiscrepancySet> CDiscrepancySet::New(CScope& scope){ return CRef<CDiscrepancySet>(new CDiscrepancyContext(scope));}


bool CDiscrepancyContext::AddTest(const string& name)
{
    string str = GetDiscrepancyCaseName(name);
    if (str.empty()) {
        return false; // no such test
    }
    if (m_Names.find(str)!=m_Names.end()) {
        return false;  // already there
    }
    CRef<CDiscrepancyCase> test = CDiscrepancyConstructor::GetDiscrepancyConstructor(str)->Create();
    m_Names.insert(str);
    m_Tests.push_back(test);

#define REGISTER_DISCREPANCY_TYPE(type) \
    if (dynamic_cast<CDiscrepancyVisitor<type>* >(test.GetPointer())) {                         \
        m_All_##type.push_back(dynamic_cast<CDiscrepancyVisitor<type>* >(test.GetPointer()));   \
        m_Enable_##type = true;                                                                 \
        return true;                                                                            \
    }
    REGISTER_DISCREPANCY_TYPE(CSeq_inst)
    REGISTER_DISCREPANCY_TYPE(CSeqFeatData)
    REGISTER_DISCREPANCY_TYPE(CBioSource)
    REGISTER_DISCREPANCY_TYPE(COrgName)
    REGISTER_DISCREPANCY_TYPE(CRNA_ref)
    return false;
}


void CDiscrepancyContext::Parse(const CSeq_entry_Handle& handle)
{
    CTypesConstIterator i;
    CType<CBioseq>::AddTo(i);
    CType<CBioseq_set>::AddTo(i);
    CType<CSeq_feat>::AddTo(i);
#define ENABLE_DISCREPANCY_TYPE(type) if (m_Enable_##type) CType<type>::AddTo(i);
    ENABLE_DISCREPANCY_TYPE(CSeq_inst)
    ENABLE_DISCREPANCY_TYPE(CSeqFeatData)
    ENABLE_DISCREPANCY_TYPE(CBioSource)
    ENABLE_DISCREPANCY_TYPE(COrgName)
    ENABLE_DISCREPANCY_TYPE(CRNA_ref)
    
    for (i = Begin(*handle.GetCompleteSeq_entry()); i; ++i) {
        if (CType<CBioseq>::Match(i)) {
            m_Current_Bioseq.Reset(m_Scope->GetBioseqHandle(*CType<CBioseq>::Get(i)).GetCompleteBioseq());
            m_Count_Bioseq++;
            m_Current_Bioseq_set.Reset();
        }
        else if (CType<CBioseq_set>::Match(i)) {
            m_Current_Bioseq.Reset();
            m_Current_Bioseq_set.Reset(m_Scope->GetBioseq_setHandle(*CType<CBioseq_set>::Get(i)).GetCompleteBioseq_set());
        }
        else if (CType<CSeq_feat>::Match(i)) {
            m_Current_Seq_feat.Reset(m_Scope->GetSeq_featHandle(*CType<CSeq_feat>::Get(i)).GetSeq_feat());
            m_Count_Seq_feat++;
        }
#define HANDLE_DISCREPANCY_TYPE(type) \
        else if (m_Enable_##type && CType<type>::Match(i)) {                                    \
            const type& obj = *CType<type>::Get(i);                                             \
            NON_CONST_ITERATE (vector<CDiscrepancyVisitor<type>* >, it, m_All_##type) {         \
                Call(**it, obj);                                                                \
            }                                                                                   \
        }
        HANDLE_DISCREPANCY_TYPE(CSeq_inst)  // no semicolon!
        HANDLE_DISCREPANCY_TYPE(CSeqFeatData)
        HANDLE_DISCREPANCY_TYPE(CBioSource)
        HANDLE_DISCREPANCY_TYPE(COrgName)
        HANDLE_DISCREPANCY_TYPE(CRNA_ref)
    }
}


CConstRef<CBioseq> CDiscrepancyContext::GetCurrentBioseq(void) const 
{ 
    if (m_Current_Bioseq)
    {
        return m_Current_Bioseq;
    }
    else if (m_Current_Bioseq_set && m_Current_Bioseq_set->IsSetClass())
    {
        CConstRef<CBioseq> bioseq;
        CScope &scope = GetScope();
        if (m_Current_Bioseq_set->GetClass() == CBioseq_set::eClass_nuc_prot)
        {
            bioseq.Reset(scope.GetBioseqHandle(m_Current_Bioseq_set->GetNucFromNucProtSet()).GetCompleteBioseq());
            return bioseq;
        }
        if (m_Current_Bioseq_set->GetClass() == CBioseq_set::eClass_gen_prod_set)
        {
            bioseq.Reset(scope.GetBioseqHandle(m_Current_Bioseq_set->GetGenomicFromGenProdSet()).GetCompleteBioseq());
            return bioseq;
        }
        if (m_Current_Bioseq_set->GetClass() == CBioseq_set::eClass_segset)
        {
            bioseq.Reset(scope.GetBioseqHandle(m_Current_Bioseq_set->GetMasterFromSegSet()).GetCompleteBioseq());
            return bioseq;
        }

    }
    return m_Current_Bioseq;
}


void CDiscrepancyContext::Summarize()
{
    NON_CONST_ITERATE (vector<CRef<CDiscrepancyCase> >, it, m_Tests) {
        (*it)->Summarize();
    }
}


void CDiscrepancyContext::SetSuspectRules(const string& name)
{
    m_SuspectRules = name;
    if (!m_ProductRules) {
        m_ProductRules = CSuspect_rule_set::GetProductRules(m_SuspectRules);
    }
}


CConstRef<CSuspect_rule_set> CDiscrepancyContext::GetProductRules()
{
    if (!m_ProductRules) {
        m_ProductRules = CSuspect_rule_set::GetProductRules(m_SuspectRules);
    }
    return m_ProductRules;
}


CConstRef<CSuspect_rule_set> CDiscrepancyContext::GetOrganelleProductRules()
{
    if (!m_OrganelleProductRules) {
        m_OrganelleProductRules = CSuspect_rule_set::GetOrganelleProductRules();
    }
    return m_OrganelleProductRules;
}


const CBioSource* CDiscrepancyContext::GetCurrentBiosource()
{
    static const CBioSource* biosrc;
    static size_t count = 0;
    if (count != m_Count_Bioseq) {
        count = m_Count_Bioseq;
        biosrc = sequence::GetBioSource(m_Scope->GetBioseqHandle(*GetCurrentBioseq()));
    }
    return biosrc;
}


CBioSource::TGenome CDiscrepancyContext::GetCurrentGenome()
{
    static CBioSource::TGenome genome;
    static size_t count = 0;
    if (count != m_Count_Bioseq) {
        count = m_Count_Bioseq;
        const CBioSource* biosrc = GetCurrentBiosource();
        genome = biosrc ? biosrc->GetGenome() : CBioSource::eGenome_unknown;
    }
    return genome;
}


DISCREPANCY_LINK_MODULE(discrepancy_case);
DISCREPANCY_LINK_MODULE(suspect_product_names);
DISCREPANCY_LINK_MODULE(division_code_conflicts);
DISCREPANCY_LINK_MODULE(duplicate_gene_locus);

END_SCOPE(NDiscrepancy)
END_NCBI_SCOPE
