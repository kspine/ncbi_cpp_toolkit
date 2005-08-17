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
 * Author:  Anatoliy Kuznetsov
 *
 * File Description:
 *   Implementation of NetSchedule client.
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>
#include <corelib/ncbitime.hpp>
#include <connect/ncbi_socket.hpp>
#include <connect/ncbi_conn_exception.hpp>
#include <connect/services/netschedule_client.hpp>
#include <util/request_control.hpp>
#include <memory>
#include <stdio.h>


BEGIN_NCBI_SCOPE


const string kNetSchedule_KeyPrefix = "JSID";

/// Request rate controller (one for all client instances)
/// Default limitation is 20000 requests per minute
///
/// @internal
///
static CRequestRateControl s_Throttler(20000, CTimeSpan(60,0));



unsigned CNetSchedule_GetJobId(const string&  key_str)
{
    unsigned job_id;
    const char* ch = key_str.c_str();

    if (isdigit((unsigned char)(*ch))) {
        job_id = (unsigned) atoi(ch);
        if (job_id) {
            return job_id;
        }        
    }

    for (;*ch != 0 && *ch != '_'; ++ch) {
    }
    ++ch;
    for (;*ch != 0 && *ch != '_'; ++ch) {
    }
    ++ch;
    if (*ch) {
        job_id = (unsigned) atoi(ch);
        if (job_id) {
            return job_id;
        }
    }
    NCBI_THROW(CNetScheduleException, eKeyFormatError, "Key syntax error.");
}

void CNetSchedule_ParseJobKey(CNetSchedule_Key* key, const string& key_str)
{
    _ASSERT(key);

    // JSID_01_1_MYHOST_9000

    const char* ch = key_str.c_str();
    key->hostname = key->prefix = kEmptyStr;

    // prefix

    for (;*ch && *ch != '_'; ++ch) {
        key->prefix += *ch;
    }
    if (*ch == 0) {
        NCBI_THROW(CNetScheduleException, eKeyFormatError, "Key syntax error.");
    }
    ++ch;

    if (key->prefix != kNetSchedule_KeyPrefix) {
        NCBI_THROW(CNetScheduleException, eKeyFormatError, 
                                       "Key syntax error. Invalid prefix.");
    }

    // version
    key->version = atoi(ch);
    while (*ch && *ch != '_') {
        ++ch;
    }
    if (*ch == 0) {
        NCBI_THROW(CNetScheduleException, eKeyFormatError, "Key syntax error.");
    }
    ++ch;

    // id
    key->id = (unsigned) atoi(ch);
    while (*ch && *ch != '_') {
        ++ch;
    }
    if (*ch == 0 || key->id == 0) {
        NCBI_THROW(CNetScheduleException, eKeyFormatError, "Key syntax error.");
    }
    ++ch;


    // hostname
    for (;*ch && *ch != '_'; ++ch) {
        key->hostname += *ch;
    }
    if (*ch == 0) {
        NCBI_THROW(CNetScheduleException, eKeyFormatError, "Key syntax error.");
    }
    ++ch;

    // port
    key->port = atoi(ch);
}

void CNetSchedule_GenerateJobKey(string*        key, 
                                  unsigned       id, 
                                  const string&  host, 
                                  unsigned short port)
{
    string tmp;
    *key = "JSID_01";  

    NStr::IntToString(tmp, id);
    *key += "_";
    *key += tmp;

    *key += "_";
    *key += host;    

    NStr::IntToString(tmp, port);
    *key += "_";
    *key += tmp;
}




CNetScheduleClient::CNetScheduleClient(const string& client_name,
                                       const string& queue_name)
    : CNetServiceClient(client_name),
      m_Queue(queue_name),
      m_RequestRateControl(true),
      m_ConnMode(eCloseConnection)
{
}

CNetScheduleClient::CNetScheduleClient(const string&  host,
                                       unsigned short port,
                                       const string&  client_name,
                                       const string&  queue_name)
    : CNetServiceClient(host, port, client_name),
      m_Queue(queue_name),
      m_RequestRateControl(true),
      m_ConnMode(eCloseConnection)
{
}

CNetScheduleClient::CNetScheduleClient(CSocket*      sock,
                                       const string& client_name,
                                       const string& queue_name)
    : CNetServiceClient(sock, client_name),
      m_Queue(queue_name),
      m_RequestRateControl(true),
      m_ConnMode(eCloseConnection)
{
    if (m_Sock) {
        m_Sock->DisableOSSendDelay();
        RestoreHostPort();
    }
}

CNetScheduleClient::~CNetScheduleClient()
{
}

string CNetScheduleClient::StatusToString(EJobStatus status)
{
    switch(status) {
    case eJobNotFound: return "NotFound";
    case ePending:     return "Pending";
    case eRunning:     return "Running";
    case eReturned:    return "Returned";
    case eCanceled:    return "Canceled";
    case eFailed:      return "Failed";
    case eDone:        return "Done";
    default: _ASSERT(0);
    }
    return kEmptyStr;
}

string CNetScheduleClient::GetConnectionInfo() const
{
    return m_Host + ':' + NStr::UIntToString(m_Port);
}

void CNetScheduleClient::ActivateRequestRateControl(bool on_off)
{
    m_RequestRateControl = on_off;
}

string CNetScheduleClient::SubmitJob(const string& input,
                                     const string& progress_msg)
{
    if (input.length() > kNetScheduleMaxDataSize) {
        NCBI_THROW(CNetScheduleException, eDataTooLong, 
            "Input data too long.");
    }
    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }

    bool connected = CheckConnect(kEmptyStr);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    MakeCommandPacket(&m_Tmp, "SUBMIT \"", connected);
    m_Tmp.append(input);
    m_Tmp.append("\"");

    if (!progress_msg.empty()) {
        m_Tmp.append(" \"");
        m_Tmp.append(progress_msg);
        m_Tmp.append("\"");
    }

    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);
    WaitForServer();
    if (!ReadStr(*m_Sock, &m_Tmp)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }
    TrimPrefix(&m_Tmp);

    if (m_Tmp.empty()) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Invalid server response. Empty key.");
    }

    return m_Tmp;
}

void CNetScheduleClient::SubmitJobBatch(SJobBatch& subm)
{
    subm.host.erase();
    subm.port = 0;

    // veryfy the input data
    ITERATE(TBatchSubmitJobList, it, subm.job_list) {
        const string& input = it->input;

        if (input.length() > kNetScheduleMaxDataSize) {
            NCBI_THROW(CNetScheduleException, eDataTooLong, 
                "Input data too long.");
        }
    }
    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }

    bool connected = CheckConnect(kEmptyStr);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    m_Sock->DisableOSSendDelay(true);

    // batch command
    MakeCommandPacket(&m_Tmp, "BSUB", connected);
    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);

    // check if server is ready for the batch submit
    WaitForServer();
    if (!ReadStr(*m_Sock, &m_Tmp)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }
    TrimPrefix(&m_Tmp);

    m_Sock->DisableOSSendDelay(false);

    for (unsigned i = 0; i < subm.job_list.size(); ) {

        const unsigned kMax_Batch = 50000;

        unsigned batch_size = subm.job_list.size() - i;
        if (batch_size > kMax_Batch) {
            batch_size = kMax_Batch;
        }

        char buf[kNetScheduleMaxDataSize * 4];
        sprintf(buf, "BTCH %u", batch_size);

        WriteStr(buf, strlen(buf)+1);

        unsigned batch_start = i;
        for (unsigned j = 0; j < batch_size; ++j,++i) {
            const string& input = subm.job_list[i].input;
            sprintf(buf, "\"%s\"", input.c_str());
            WriteStr(buf, strlen(buf)+1);
        }

        WriteStr("ENDB", 5);

        WaitForServer();
        if (!ReadStr(*m_Sock, &m_Tmp)) {
            NCBI_THROW(CNetServiceException, eCommunicationError, 
                    "Communication error");
        }
        TrimPrefix(&m_Tmp);

        if (m_Tmp.empty()) {
            NCBI_THROW(CNetServiceException, eProtocolError, 
                    "Invalid server response. Empty key.");
        }

        // parse the batch answer
        // FORMAT:
        //  first_job_id host port

        {{
        const char* s = m_Tmp.c_str();
        unsigned first_job_id = ::atoi(s);

        if (subm.host.empty()) {
            for (; *s != ' '; ++s) {
                if (*s == 0) {
                    NCBI_THROW(CNetServiceException, eProtocolError, 
                            "Invalid server response. Batch answer format.");
                }
            }
            ++s;
            if (*s == 0) {
                NCBI_THROW(CNetServiceException, eProtocolError, 
                        "Invalid server response. Batch answer format.");
            }
            for (; *s != ' '; ++s) {
                if (*s == 0) {
                    NCBI_THROW(CNetServiceException, eProtocolError, 
                            "Invalid server response. Batch answer format.");
                }
                subm.host.push_back(*s);
            }
            ++s;
            if (*s == 0) {
                NCBI_THROW(CNetServiceException, eProtocolError, 
                        "Invalid server response. Batch answer format.");
            }

            subm.port = atoi(s); 
            if (subm.port == 0) {
                NCBI_THROW(CNetServiceException, eProtocolError, 
                        "Invalid server response. Port=0.");
            }
        }

        // assign job ids, protocol guarantees all jobs in batch will
        // receive sequential numbers, so server sends only first job id
        //
        for (unsigned j = 0; j < batch_size; ++j) {
            subm.job_list[batch_start].job_id = first_job_id;
            ++first_job_id;
            ++batch_start;
        }
        

        }}


    } // for

    m_Sock->DisableOSSendDelay(true);


    WriteStr("ENDS", 5);

}


CNetScheduleClient::EJobStatus 
CNetScheduleClient::SubmitJobAndWait(const string&  input,
                                     string*        job_key,
                                     int*           ret_code,
                                     string*        output, 
                                     string*        err_msg,
                                     unsigned       wait_time,
                                     unsigned short udp_port)
{
    _ASSERT(job_key);
    _ASSERT(ret_code);
    _ASSERT(output);
    _ASSERT(wait_time);
    _ASSERT(udp_port);

    if (input.length() > kNetScheduleMaxDataSize) {
        NCBI_THROW(CNetScheduleException, eDataTooLong, 
            "Input data too long.");
    }
    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }
    *output = kEmptyStr;

    {{
    bool connected = CheckConnect(kEmptyStr);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    MakeCommandPacket(&m_Tmp, "SUBMIT \"", connected);
    m_Tmp.append(input);
    m_Tmp.append("\" ");
    m_Tmp.append(NStr::UIntToString(udp_port));
    m_Tmp.append(" ");
    m_Tmp.append(NStr::UIntToString(wait_time));
    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);
    WaitForServer();
    if (!ReadStr(*m_Sock, &m_Tmp)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }

    }}
    TrimPrefix(&m_Tmp);

    if (m_Tmp.empty()) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Invalid server response. Empty key.");
    }

    *job_key = m_Tmp;

    m_JobKey.id = 0;
    CNetSchedule_ParseJobKey(&m_JobKey, *job_key);

    WaitJobNotification(wait_time, udp_port, m_JobKey.id);

    return GetStatus(*job_key, ret_code, output, err_msg);
}

void CNetScheduleClient::WaitJobNotification(unsigned       wait_time,
                                             unsigned short udp_port,
                                             unsigned       job_id)
{
    _ASSERT(wait_time);

    EIO_Status status;
    int    sig_buf[4];
    const char* sig = "JNTF";
    memcpy(sig_buf, sig, 4);

    int    buf[1024/sizeof(int)];
    char*  chr_buf = (char*) buf;

    STimeout to;
    to.sec = wait_time;
    to.usec = 0;

    CDatagramSocket  udp_socket;
    udp_socket.SetReuseAddress(eOn);
    STimeout rto;
    rto.sec = rto.usec = 0;
    udp_socket.SetTimeout(eIO_Read, &rto);

    status = udp_socket.Bind(udp_port);
    if (eIO_Success != status) {
        return;
    }
    time_t curr_time, start_time, end_time;

    start_time = time(0);
    end_time = start_time + wait_time;

    for (;true;) {

        curr_time = time(0);
        to.sec = end_time - curr_time;  // remaining
        if (to.sec <= 0) {
            break;
        }
        status = udp_socket.Wait(&to);
        if (eIO_Success != status) {
            continue;
        }
        size_t msg_len;
        status = udp_socket.Recv(buf, sizeof(buf), &msg_len, &m_Tmp);
        _ASSERT(status != eIO_Timeout); // because we Wait()-ed
        if (eIO_Success == status) {
            if (buf[0] != sig_buf[0]) {
                continue;
            }

            const char* job = chr_buf + 5;
            unsigned notif_job_id = (unsigned)::atoi(job);
            if (notif_job_id == job_id) {
                return;
            }
        } 
    } // for

}



void CNetScheduleClient::CancelJob(const string& job_key)
{
    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }

    bool connected = CheckConnect(job_key);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    CommandInitiate("CANCEL ", job_key, &m_Tmp, connected);

    CheckOK(&m_Tmp);
}

void CNetScheduleClient::DropJob(const string& job_key)
{
    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }

    bool connected = CheckConnect(job_key);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    CommandInitiate("DROJ ", job_key, &m_Tmp, connected);

    CheckOK(&m_Tmp);
}

void CNetScheduleClient::Logging(bool on_off)
{
    bool connected = CheckConnect(kEmptyStr);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    CommandInitiate("LOG ", on_off ? "ON" : "OFF", &m_Tmp, connected);
    CheckOK(&m_Tmp);
}

void CNetScheduleClient::SetRunTimeout(const string& job_key, 
                                       unsigned      time_to_run)
{
    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }
    bool connected = CheckConnect(job_key);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    MakeCommandPacket(&m_Tmp, "JRTO ", connected);

    m_Tmp.append(job_key);
    m_Tmp.append(" ");
    m_Tmp.append(NStr::IntToString(time_to_run));

    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);
    WaitForServer();
    if (!ReadStr(*m_Sock, &m_Tmp)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }

    CheckOK(&m_Tmp);
}


CNetScheduleClient::EJobStatus 
CNetScheduleClient::GetStatus(const string& job_key,
                              int*          ret_code,
                              string*       output,
                              string*       err_msg,
                              string*       input)
{
    _ASSERT(ret_code);
    _ASSERT(output);

    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }

    EJobStatus status;

    bool connected = CheckConnect(job_key);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);
/*
    if (m_JobKey.id) {
        char command[2048];
        sprintf(command, "%s\r\n%s\r\nSTATUS %u",
                m_ClientName.c_str(), 
                m_Queue.c_str(),
                m_JobKey.id);
        WriteStr(command, strlen(command)+1);
        
        WaitForServer();
        if (!ReadStr(*m_Sock, &m_Tmp)) {
            NCBI_THROW(CNetServiceException, eCommunicationError, 
                    "Communication error");
        }
    } else {
        CommandInitiate("STATUS ", job_key, &m_Tmp);
    }
*/
    CommandInitiate("STATUS ", job_key, &m_Tmp, connected);

    TrimPrefix(&m_Tmp);

    if (m_Tmp.empty()) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Invalid server response. Empty key.");
    }

    const char* str = m_Tmp.c_str();

    int st = atoi(str);
    status = (EJobStatus) st;

    if ((status == eDone || status == eFailed)) {
        for ( ;*str && isdigit((unsigned char)(*str)); ++str) {
        }

        for ( ; *str && isspace((unsigned char)(*str)); ++str) {
        }

        *ret_code = atoi(str);

        for ( ;*str && isdigit((unsigned char)(*str)); ++str) {
        }

        output->erase();

        for ( ; *str && isspace((unsigned char)(*str)); ++str) {
        }

        if (*str && *str == '"') {
            ++str;
            for( ;*str && *str != '"'; ++str) {
                output->push_back(*str);
            }
        }

        if (err_msg || input) {
            err_msg->erase();
            if (!*str)
                return status;

            for (++str; *str && isspace((unsigned char)(*str)); ++str) {
            }
            
            if (!*str)
                return status;

            if (*str && *str == '"') {
                ++str;
                for( ;*str && *str; ++str) {
                    if (*str == '"' && *(str-1) != '\\') break;
                    if (err_msg)
                        err_msg->push_back(*str);
                }
            }

            *err_msg = NStr::ParseEscapes(*err_msg);
        }

        if (input) {
            input->erase();
            if (!*str)
                return status;

            for (++str; *str && isspace((unsigned char)(*str)); ++str) {
            }
            
            if (!*str)
                return status;

            if (*str && *str == '"') {
                ++str;
                for( ;*str && *str; ++str) {
                    if (*str == '"' && *(str-1) != '\\') break;
                    input->push_back(*str);
                }
            }
        }

    } // if status done or failed

    return status;
}

bool CNetScheduleClient::GetJob(string*        job_key, 
                                string*        input,
                                unsigned short udp_port)
{
    _ASSERT(job_key);
    _ASSERT(input);

    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }

    bool connected = CheckConnect(kEmptyStr);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    if (udp_port == 0) {
        CommandInitiate("GET ", kEmptyStr, &m_Tmp, connected);
    } else {
        MakeCommandPacket(&m_Tmp, "GET ", connected);
        m_Tmp.append(NStr::IntToString(udp_port));

        WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);
        WaitForServer();

        if (!ReadStr(*m_Sock, &m_Tmp)) {
            NCBI_THROW(CNetServiceException, eCommunicationError, 
                    "Communication error");
        }
    }

    TrimPrefix(&m_Tmp);

    if (m_Tmp.empty()) {
        return false;
    }


    ParseGetJobResponse(job_key, input, m_Tmp);

    _ASSERT(!job_key->empty());
    _ASSERT(!input->empty());

    return true;
}


bool CNetScheduleClient::GetJobWaitNotify(string*    job_key, 
                                          string*    input, 
                                          unsigned   wait_time,
                                          unsigned short udp_port)
{
    _ASSERT(job_key);
    _ASSERT(input);

    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }

    {{
    bool connected = CheckConnect(kEmptyStr);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    MakeCommandPacket(&m_Tmp, "WGET ", connected);
    m_Tmp.append(NStr::IntToString(udp_port));
    m_Tmp.append(" ");
    m_Tmp.append(NStr::IntToString(wait_time));

    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);
    WaitForServer();

    if (!ReadStr(*m_Sock, &m_Tmp)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }

    }}

    TrimPrefix(&m_Tmp);
    if (!m_Tmp.empty()) {
        ParseGetJobResponse(job_key, input, m_Tmp);

        _ASSERT(!job_key->empty());
        _ASSERT(!input->empty());

        return true;
    }
    return false;
}



bool CNetScheduleClient::WaitJob(string*    job_key, 
                                 string*    input, 
                                 unsigned   wait_time,
                                 unsigned short udp_port)
{
    bool job_received = 
        GetJobWaitNotify(job_key, input, wait_time, udp_port);
    if (job_received) {
        return job_received;
    }

    WaitQueueNotification(wait_time, udp_port);

    // no matter is WaitResult we re-try the request
    // using reliable comm.level and notify server that
    // we no longer on the UDP socket

    return GetJob(job_key, input, udp_port);
}


void CNetScheduleClient::WaitQueueNotification(unsigned       wait_time,
                                               unsigned short udp_port)
{
    _ASSERT(wait_time);

    EIO_Status status;
    int    sig_buf[4];
    const char* sig = "NCBI_JSQ_";
    memcpy(sig_buf, sig, 8);

    int    buf[1024/sizeof(int)];
    char*  chr_buf = (char*) buf;

    STimeout to;
    to.sec = wait_time;
    to.usec = 0;


    CDatagramSocket  udp_socket;
    udp_socket.SetReuseAddress(eOn);
    STimeout rto;
    rto.sec = rto.usec = 0;
    udp_socket.SetTimeout(eIO_Read, &rto);

    status = udp_socket.Bind(udp_port);
    if (eIO_Success != status) {
        return;
    }

    time_t curr_time, start_time, end_time;

    start_time = time(0);
    end_time = start_time + wait_time;

    // minilal length is prefix "NCBI_JSQ_" + queue length
    size_t min_msg_len = m_Queue.length() + 9;

    for (;true;) {

        curr_time = time(0);
        if (curr_time >= end_time) {
            break;
        }
        to.sec = end_time - curr_time;  // remaining

        status = udp_socket.Wait(&to);
        if (eIO_Success != status) {
            continue;
        }

        size_t msg_len;
        status = udp_socket.Recv(buf, sizeof(buf), &msg_len);
        _ASSERT(status != eIO_Timeout); // because we Wait()-ed
        if (eIO_Success == status) {

            // Analyse the message content
            //
            // this is a performance critical code, if this is not
            // our message we need to get back to the datagram wait ASAP
            // (int arithmetic XOR-OR comparison here...)
            if ((msg_len < min_msg_len) ||
                ((buf[0] ^ sig_buf[0]) | (buf[1] ^ sig_buf[1]))
                ) {
                continue;
            }

            const char* queue = chr_buf + 9;

            if (strncmp(m_Queue.c_str(), queue, m_Queue.length()) == 0) {
                // Message from our queue 
                return;
            }

        } 

    } // for

}



void CNetScheduleClient::ParseGetJobResponse(string*        job_key, 
                                             string*        input, 
                                             const string&  response)
{
    _ASSERT(!response.empty());

    input->erase();
    job_key->erase();

    const char* str = response.c_str();
    while (*str && isspace((unsigned char)(*str)))
        ++str;

    for(;*str && !isspace((unsigned char)(*str)); ++str) {
        job_key->push_back(*str);
    }

    while (*str && isspace((unsigned char)(*str)))
        ++str;

    if (*str == '"')
        ++str;
    for (;*str && *str != '"'; ++str) {
        input->push_back(*str);
    }
}






void CNetScheduleClient::PutResult(const string& job_key, 
                                   int           ret_code, 
                                   const string& output)
{
    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }

    bool connected = CheckConnect(job_key);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    MakeCommandPacket(&m_Tmp, "PUT ", connected);

    m_Tmp.append(job_key);
    m_Tmp.append(" ");
    m_Tmp.append(NStr::IntToString(ret_code));
    m_Tmp.append(" \"");
    m_Tmp.append(output);
    m_Tmp.append("\"");

    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);
    WaitForServer();
    if (!ReadStr(*m_Sock, &m_Tmp)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }
    CheckOK(&m_Tmp);
}

void CNetScheduleClient::PutProgressMsg(const string& job_key, 
                                        const string& progress_msg)
{
    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }
    if (progress_msg.length() >= kNetScheduleMaxDataSize) {
        NCBI_THROW(CNetScheduleException, eDataTooLong, 
                   "Progress message too long");
    }

    bool connected = CheckConnect(job_key);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    MakeCommandPacket(&m_Tmp, "MPUT ", connected);

    m_Tmp.append(job_key);
    m_Tmp.append(" \"");
    m_Tmp.append(NStr::PrintableString(progress_msg));
    m_Tmp.append("\"");

    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);
    WaitForServer();
    if (!ReadStr(*m_Sock, &m_Tmp)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }
    CheckOK(&m_Tmp);
}

string CNetScheduleClient::GetProgressMsg(const string& job_key)
{
    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }

    bool connected = CheckConnect(job_key);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    MakeCommandPacket(&m_Tmp, "MGET ", connected);

    m_Tmp.append(job_key);

    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);
    WaitForServer();
    if (!ReadStr(*m_Sock, &m_Tmp)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }
    TrimPrefix(&m_Tmp);

    return NStr::ParseEscapes(m_Tmp);
}


void CNetScheduleClient::PutFailure(const string& job_key, 
                                    const string& err_msg)
{
    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }

    if (err_msg.length() >= kNetScheduleMaxErrSize) {
        NCBI_THROW(CNetScheduleException, eDataTooLong, 
                   "Error message too long");
    }
    
    bool connected = CheckConnect(job_key);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    MakeCommandPacket(&m_Tmp, "FPUT ", connected);

    m_Tmp.append(job_key);
    m_Tmp.append(" ");
    m_Tmp.append(" \"");
    m_Tmp.append(NStr::PrintableString(err_msg));
    m_Tmp.append("\"");

    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);
    WaitForServer();
    if (!ReadStr(*m_Sock, &m_Tmp)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }
    CheckOK(&m_Tmp);
}


void CNetScheduleClient::ReturnJob(const string& job_key)
{
    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }

    bool connected = CheckConnect(job_key);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    MakeCommandPacket(&m_Tmp, "RETURN ", connected);
    m_Tmp.append(job_key);

    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);
    WaitForServer();
    if (!ReadStr(*m_Sock, &m_Tmp)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }
    CheckOK(&m_Tmp);
}


void CNetScheduleClient::ShutdownServer()
{
    bool connected = CheckConnect(kEmptyStr);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    MakeCommandPacket(&m_Tmp, "SHUTDOWN ", connected);
    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);
    WaitForServer();
    if (!ReadStr(*m_Sock, &m_Tmp)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }
    CheckOK(&m_Tmp);
}

void CNetScheduleClient::ReloadServerConfig()
{
    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }

    bool connected = CheckConnect(kEmptyStr);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    MakeCommandPacket(&m_Tmp, "RECO ", connected);
    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);
    WaitForServer();
    if (!ReadStr(*m_Sock, &m_Tmp)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }
    TrimPrefix(&m_Tmp);
}

string CNetScheduleClient::ServerVersion()
{
    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }

    bool connected = CheckConnect(kEmptyStr);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    MakeCommandPacket(&m_Tmp, "VERSION ", connected);
    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);
    WaitForServer();

    if (!ReadStr(*m_Sock, &m_Tmp)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }
    TrimPrefix(&m_Tmp);

    if (m_Tmp.empty()) {
        NCBI_THROW(CNetServiceException, 
                   eCommunicationError, 
                   "Invalid server response. Empty version.");
    }
    return m_Tmp;
}

void CNetScheduleClient::DumpQueue(CNcbiOstream& out,
                                   const string& job_key)
{
    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }

    bool connected = CheckConnect(kEmptyStr);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    MakeCommandPacket(&m_Tmp, "DUMP ", connected);
    m_Tmp += job_key;
    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);

    WaitForServer();
    PrintServerOut(out);
}

void CNetScheduleClient::PrintStatistics(CNcbiOstream & out)
{
    if (m_RequestRateControl) {
        s_Throttler.Approve(CRequestRateControl::eSleep);
    }

    bool connected = CheckConnect(kEmptyStr);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    MakeCommandPacket(&m_Tmp, "STAT ", connected);
    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);

    WaitForServer();
    PrintServerOut(out);
}

void CNetScheduleClient::Monitor(CNcbiOstream & out)
{
    bool connected = CheckConnect(kEmptyStr);
    CSockGuard sg(*m_Sock);

    MakeCommandPacket(&m_Tmp, "MONI ", connected);
    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);
    m_Tmp = "QUIT";
    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);

    STimeout rto;
    rto.sec = 1;
    rto.usec = 0;
    m_Sock->SetTimeout(eIO_Read, &rto);

    string line;
    while (1) {

        EIO_Status st = m_Sock->ReadLine(line);       
        if (st == eIO_Success) {
            if (m_Tmp == "END")
                break;
            out << line << "\n" << flush;
        } else {
            EIO_Status st = m_Sock->GetStatus(eIO_Open);
            if (st != eIO_Success) {
                break;
            }
        }
    }

}


string CNetScheduleClient::GetQueueList()
{
    bool connected = CheckConnect(kEmptyStr);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    MakeCommandPacket(&m_Tmp, "QLST ", connected);
    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);

    WaitForServer();

    if (!ReadStr(*m_Sock, &m_Tmp)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }
    TrimPrefix(&m_Tmp);

    return m_Tmp;
}


void CNetScheduleClient::DropQueue()
{
    bool connected = CheckConnect(kEmptyStr);
    CSockGuard sg(GetConnMode() == eKeepConnection ? 0 : m_Sock);

    MakeCommandPacket(&m_Tmp, "DROPQ ", connected);
    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);

    WaitForServer();

    if (!ReadStr(*m_Sock, &m_Tmp)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }
    TrimPrefix(&m_Tmp);
}

void CNetScheduleClient::CommandInitiate(const string& command, 
                                         const string& job_key,
                                         string*       answer,
                                         bool          add_prefix)
{
    _ASSERT(answer);
    MakeCommandPacket(&m_Tmp, command, add_prefix);
    m_Tmp.append(job_key);
    WriteStr(m_Tmp.c_str(), m_Tmp.length() + 1);
    WaitForServer();
    if (!ReadStr(*m_Sock, answer)) {
        NCBI_THROW(CNetServiceException, eCommunicationError, 
                   "Communication error");
    }
}


void CNetScheduleClient::TrimPrefix(string* str)
{
    CheckOK(str);
    str->erase(0, 3); // "OK:"
}

void CNetScheduleClient::CheckOK(string* str)
{
    if (str->find("OK:") != 0) {
        // Answer is not in "OK:....." format
        string msg = "Server error:";
        TrimErr(str);

        if (*str == "CLIENT_VERSION") {
            NCBI_THROW(CNetScheduleException, eInvalidClientOrVersion, 
                       "Client name or version incompatibility");
        } else 
        if (*str == "QUEUE_NOT_FOUND") {
            NCBI_THROW(CNetScheduleException, eUnknownQueue, 
                       "Queue not found.");
        } else
        if (*str == "OPERATION_ACCESS_DENIED") {
            NCBI_THROW(CNetScheduleException, eOperationAccessDenied, 
                       "Access to operation denied.");
        }

        msg += *str;
        NCBI_THROW(CNetServiceException, eCommunicationError, msg);
    }
}

void 
CNetScheduleClient::MakeCommandPacket(string*       out_str,
                                      const string& cmd_str,
                                      bool          add_prefix)
{
    if (!add_prefix) {
        *out_str = cmd_str;
        return;
    }

    // command with full connection establishment

    unsigned client_len = m_ClientName.length();
    if (client_len < 3) {
        NCBI_THROW(CNetScheduleException, 
                   eAuthenticationError, "Client name too small or empty");
    }
    if (m_Queue.empty()) {
        NCBI_THROW(CNetScheduleException, 
                   eAuthenticationError, "Empty queue name");
    }

    *out_str = m_ClientName;

    if (!m_ProgramVersion.empty()) {
        out_str->append(" prog='");
        out_str->append(m_ProgramVersion);
        out_str->append("'");
    }
/*
    const string& client_name_comment = GetClientNameComment();
    if (!client_name_comment.empty()) {
        out_str->append(" ");
        out_str->append(client_name_comment);
    }
*/
    out_str->append("\r\n");
    out_str->append(m_Queue);
    out_str->append("\r\n");
    out_str->append(cmd_str);
}


bool CNetScheduleClient::CheckConnect(const string& key)
{
    m_JobKey.id = 0;
    if (m_Sock && (eIO_Success == m_Sock->GetStatus(eIO_Open))) {
        bool expired = CheckConnExpired();
        if (!expired) {
            if (key.empty()) {
                return false; // we are connected, nothing to do
            }
            // key is not empty, check if we are at the correct server
            CNetSchedule_ParseJobKey(&m_JobKey, key);
            if (m_JobKey.port != m_Port ||
                NStr::CompareNocase(m_JobKey.hostname, m_Host) != 0) {
                    m_Sock->Close();
                    CreateSocket(m_JobKey.hostname, m_JobKey.port);
                    return true;
            } else {
                return false;
            }
            
        } else {
            m_Sock->Close();
        }
    }

    if (!key.empty()) {
        CNetSchedule_ParseJobKey(&m_JobKey, key);
        CreateSocket(m_JobKey.hostname, m_JobKey.port);
        return true;
    }

    // not connected

    if (!m_Host.empty()) { // we can restore connection
        CreateSocket(m_Host, m_Port);
        return true;
    }

    NCBI_THROW(CNetServiceException, eCommunicationError,
        "Cannot establish connection with a server. Unknown host-port.");

    return false;
}

bool CNetScheduleClient::CheckConnExpired()
{
    _ASSERT(m_Sock);
    STimeout to = {0, 0};
    EIO_Status io_st = m_Sock->Wait(eIO_Read, &to);
    switch (io_st) {
    case eIO_Success:
        {
            string msg;
            io_st = m_Sock->ReadLine(msg);
            if (io_st == eIO_Closed) {
                return true;
            }
            if (!msg.empty()) {
                // TODO:
                // check the message
                return true;
            }
        }
        break;
    case eIO_Closed:
        return true;
    default:
        break;
    }
    return false;
}

void CNetScheduleClient::CloseConnection()
{
    if (m_Sock) {
        m_Sock->Close();
    }
}

bool CNetScheduleClient::IsError(const char* str)
{
    int cmp = NStr::strncasecmp(str, "ERR:", 4);
    return cmp == 0;
}


void CNetScheduleClient::MakeJobKey(string*       job_key, 
                                    unsigned      job_id, 
                                    const string& host,
                                    unsigned      port)
{
    _ASSERT(job_key);
    char buf[2048];
    sprintf(buf, NETSCHEDULE_JOBMASK,
            job_id, host.c_str(), port);
    *job_key = buf;
}


END_NCBI_SCOPE

/*
 * ===========================================================================
 * $Log$
 * Revision 1.38  2005/08/17 14:27:31  kuznets
 * Added permanent connection mode
 *
 * Revision 1.37  2005/08/15 17:25:11  ucko
 * +<stdio.h> for sprintf()
 *
 * Revision 1.36  2005/08/15 13:28:33  kuznets
 * Implemented batch job submission
 *
 * Revision 1.35  2005/07/27 18:13:37  kuznets
 * PrintServerOut() moved to base class
 *
 * Revision 1.34  2005/06/20 13:32:00  kuznets
 * Added access denied error
 *
 * Revision 1.33  2005/05/17 13:50:50  kuznets
 * Added error checking for shutdown request
 *
 * Revision 1.32  2005/05/16 16:18:21  kuznets
 * + GetQueueList()
 *
 * Revision 1.31  2005/05/16 14:00:29  didenko
 * Added CetConnectionInfo() virtual method
 *
 * Revision 1.30  2005/05/12 18:36:57  kuznets
 * +ReloadServerConfig()
 *
 * Revision 1.29  2005/05/12 15:11:31  lavr
 * Use explicit (unsigned char) conversion in <ctype.h>'s macros
 *
 * Revision 1.28  2005/05/05 16:06:33  kuznets
 * DumpQueue() added job_key as an optional parameter
 *
 * Revision 1.27  2005/05/04 19:08:01  kuznets
 * +DumpQueue()
 *
 * Revision 1.26  2005/05/03 18:06:17  kuznets
 * Added strem flush to make monitoring immediate
 *
 * Revision 1.25  2005/05/02 14:41:15  kuznets
 * +Monitor()
 *
 * Revision 1.24  2005/04/20 15:42:29  kuznets
 * Added progress message to SubmitJob()
 *
 * Revision 1.23  2005/04/19 19:33:23  kuznets
 * Added methods to submit and receive progress messages
 *
 * Revision 1.22  2005/04/11 13:51:07  kuznets
 * Added logging
 *
 * Revision 1.21  2005/04/06 12:37:31  kuznets
 * Added support of client version control
 *
 * Revision 1.20  2005/04/01 16:42:59  didenko
 * Added reading services_list parameter from ini file when LB client is
 * created by PluginManager
 *
 * Revision 1.19  2005/03/28 14:21:30  didenko
 * Fixed bug that caused an infinit loop in WaitForJob method
 *
 * Revision 1.18  2005/03/24 16:43:14  didenko
 * Fixed error parsing
 *
 * Revision 1.17  2005/03/22 18:54:07  kuznets
 * Changed project tree layout
 *
 * Revision 1.16  2005/03/22 16:14:12  kuznets
 * Minor improvement in PrintStatistics
 *
 * Revision 1.15  2005/03/21 16:46:35  didenko
 * + creating from PluginManager
 *
 * Revision 1.14  2005/03/21 13:05:58  kuznets
 * + PrintStatistics
 *
 * Revision 1.13  2005/03/17 20:36:22  kuznets
 * +PutFailure()
 *
 * Revision 1.12  2005/03/17 17:18:25  kuznets
 * Cosmetics
 *
 * Revision 1.11  2005/03/16 14:25:46  kuznets
 * Fixed connection establishment when job key is known
 *
 * Revision 1.10  2005/03/15 20:13:07  kuznets
 * +SubmitJobAndWait()
 *
 * Revision 1.9  2005/03/15 14:48:56  kuznets
 * +DropJob()
 *
 * Revision 1.8  2005/03/10 14:17:58  kuznets
 * +SetRunTimeout()
 *
 * Revision 1.7  2005/03/07 17:30:49  kuznets
 * Job key parsing changed
 *
 * Revision 1.6  2005/03/04 12:05:46  kuznets
 * Implemented WaitJob() method
 *
 * Revision 1.5  2005/02/28 18:39:43  kuznets
 * +ReturnJob()
 *
 * Revision 1.4  2005/02/28 12:19:40  kuznets
 * +DropQueue()
 *
 * Revision 1.3  2005/02/10 20:01:19  kuznets
 * +GetJob(), +PutResult()
 *
 * Revision 1.2  2005/02/09 18:59:08  kuznets
 * Implemented job submission part of the API
 *
 * Revision 1.1  2005/02/07 13:02:47  kuznets
 * Initial revision
 *
 *
 * ===========================================================================
 */
