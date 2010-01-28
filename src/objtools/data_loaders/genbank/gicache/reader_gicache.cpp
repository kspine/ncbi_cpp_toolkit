/*  $Id$
 * ===========================================================================
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
 *  Author:  Eugene Vasilchenko
 *
 *  File Description: Data reader from gicache
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbi_param.hpp>

#include <objtools/data_loaders/genbank/gicache/reader_gicache.hpp>
#include <objtools/data_loaders/genbank/gicache/reader_gicache_entry.hpp>
#include <objtools/data_loaders/genbank/gicache/reader_gicache_params.h>
#include <objtools/data_loaders/genbank/readers.hpp> // for entry point
#include <objtools/data_loaders/genbank/dispatcher.hpp>
#include <objtools/data_loaders/genbank/request_result.hpp>
#include <objtools/error_codes.hpp>

#include <objmgr/objmgr_exception.hpp>

#include <corelib/plugin_manager_impl.hpp>
#include <corelib/plugin_manager_store.hpp>

#include "gicache.hpp"


#define NCBI_USE_ERRCODE_X   Objtools_Rd_GICache

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(objects)

#ifdef NCBI_OS_MSWIN
# define DEFAULT_PATH "/panfs/pan1.be-md.ncbi.nlm.nih.gov/id_dumps/gi_cache"
#else
# define DEFAULT_PATH "/net/ipubseq00/pubmed/sybase/PubSeq"
#endif

CGICacheReader::CGICacheReader(void)
    : m_Path(DEFAULT_PATH)
{
    SetMaximumConnections(1);
    x_Initialize();
}


CGICacheReader::CGICacheReader(const TPluginManagerParamTree* params,
                               const string& driver_name)
{
    CConfig conf(params);
    m_Path = conf.GetString(
        driver_name,
        NCBI_GBLOADER_READER_GICACHE_PARAM_PATH_NAME,
        CConfig::eErr_NoThrow,
        DEFAULT_PATH);
    x_Initialize();
}


CGICacheReader::~CGICacheReader()
{
}


void CGICacheReader::x_Initialize(void)
{
    string index = m_Path+"/gi2acc";
    LOG_POST("CGICacheReader: loading from "<<index);
    GICache_ReadData(index.c_str());
}


void CGICacheReader::x_AddConnectionSlot(TConn /*conn*/)
{
}


void CGICacheReader::x_RemoveConnectionSlot(TConn /*conn*/)
{
}


void CGICacheReader::x_DisconnectAtSlot(TConn /*conn*/, bool /*failed*/)
{
}


void CGICacheReader::x_ConnectAtSlot(TConn /*conn*/)
{
}


int CGICacheReader::GetRetryCount(void) const
{
    return 1;
}


bool CGICacheReader::MayBeSkippedOnErrors(void) const
{
    return true;
}


int CGICacheReader::GetMaximumConnectionsLimit(void) const
{
    return 1;
}


bool CGICacheReader::LoadSeq_idAccVer(CReaderRequestResult& result,
                                      const CSeq_id_Handle& seq_id)
{
    if ( seq_id.IsGi() ) {
        CLoadLockSeq_ids ids(result, seq_id);
        char buffer[256];
        if ( GICache_GetAccession(seq_id.GetGi(), buffer, sizeof(buffer)) ) {
            if ( buffer[0] ) {
                try {
                    ids->SetLoadedAccVer(CSeq_id_Handle::GetHandle(buffer));
                    return true;
                }
                catch ( CException& /*ignored*/ ) {
                    ERR_POST("Bad accver for gi "<<seq_id.GetGi()<<
                             ": \""<<buffer<<"\"");
                }
            }
        }
    }
    // if any problem occurs -> fall back to regular reader
    return false;
}


bool CGICacheReader::LoadStringSeq_ids(CReaderRequestResult& /*result*/,
                                       const string& /*seq_id*/)
{
    return false;
}


bool CGICacheReader::LoadSeq_idSeq_ids(CReaderRequestResult& /*result*/,
                                       const CSeq_id_Handle& /*seq_id*/)
{
    return false;
}


bool CGICacheReader::LoadBlobVersion(CReaderRequestResult& /*result*/,
                                     const TBlobId& /*blob_id*/)
{
    return false;
}


bool CGICacheReader::LoadBlob(CReaderRequestResult& /*result*/,
                              const CBlob_id& /*blob_id*/)
{
    return false;
}


END_SCOPE(objects)

void GenBankReaders_Register_GICache(void)
{
    RegisterEntryPoint<objects::CReader>(NCBI_EntryPoint_GICacheReader);
}


/// Class factory for ID1 reader
///
/// @internal
///
class CGICacheReaderCF : 
    public CSimpleClassFactoryImpl<objects::CReader, objects::CGICacheReader>
{
public:
    typedef CSimpleClassFactoryImpl<objects::CReader,
                                    objects::CGICacheReader> TParent;
public:
    CGICacheReaderCF()
        : TParent(NCBI_GBLOADER_READER_GICACHE_DRIVER_NAME, 0) {}
    ~CGICacheReaderCF() {}

    objects::CReader* 
    CreateInstance(const string& driver  = kEmptyStr,
                   CVersionInfo version =
                   NCBI_INTERFACE_VERSION(objects::CReader),
                   const TPluginManagerParamTree* params = 0) const
    {
        objects::CReader* drv = 0;
        if ( !driver.empty()  &&  driver != m_DriverName ) {
            return 0;
        }
        if (version.Match(NCBI_INTERFACE_VERSION(objects::CReader)) 
                            != CVersionInfo::eNonCompatible) {
            drv = new objects::CGICacheReader(params, driver);
        }
        return drv;
    }
};


void NCBI_EntryPoint_GICacheReader(
     CPluginManager<objects::CReader>::TDriverInfoList&   info_list,
     CPluginManager<objects::CReader>::EEntryPointRequest method)
{
    CHostEntryPointImpl<CGICacheReaderCF>::NCBI_EntryPointImpl(info_list,
                                                               method);
}


void NCBI_EntryPoint_xreader_gicache(
     CPluginManager<objects::CReader>::TDriverInfoList&   info_list,
     CPluginManager<objects::CReader>::EEntryPointRequest method)
{
    NCBI_EntryPoint_GICacheReader(info_list, method);
}


END_NCBI_SCOPE
