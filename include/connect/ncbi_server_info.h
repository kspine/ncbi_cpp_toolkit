#ifndef NCBI_SERVER_INFO__H
#define NCBI_SERVER_INFO__H

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
 * Author:  Anton Lavrentiev, Denis Vakatov
 *
 * File Description:
 *   NCBI server meta-address info
 *   Note that all server meta-addresses are allocated as
 *   single contiguous pieces of memory, which can be copied in whole
 *   with the use of 'SERV_SizeOfInfo' call. Dynamically allocated
 *   server infos can be freed with a direct call to 'free'.
 *   Assumptions on the fields: all fields in the server info come in
 *   host byte order except 'host', which comes in network byte order.
 *
 * --------------------------------------------------------------------------
 * $Log$
 * Revision 6.19  2001/05/03 16:35:34  lavr
 * Local bonus coefficient modified: meaning of negative value changed
 *
 * Revision 6.18  2001/04/24 21:24:05  lavr
 * New server attributes added: locality and bonus coefficient
 *
 * Revision 6.17  2001/03/06 23:52:57  lavr
 * SERV_ReadInfo can now consume either hostname or IP address
 *
 * Revision 6.16  2001/03/05 23:09:20  lavr
 * SERV_WriteInfo & SERV_ReadInfo both take only one argument now
 *
 * Revision 6.15  2001/03/02 20:06:26  lavr
 * Typos fixed
 *
 * Revision 6.14  2001/03/01 18:47:45  lavr
 * Verbal representation of server info is wordly documented
 *
 * Revision 6.13  2000/12/29 17:39:42  lavr
 * Pretty printed
 *
 * Revision 6.12  2000/12/06 22:17:02  lavr
 * Binary host addresses are now explicitly stated to be in network byte
 * order, whereas binary port addresses now use native (host) representation
 *
 * Revision 6.11  2000/10/20 17:05:48  lavr
 * TServType made 'unsigned'
 *
 * Revision 6.10  2000/10/05 21:25:45  lavr
 * ncbiconf.h removed
 *
 * Revision 6.9  2000/10/05 21:10:11  lavr
 * ncbiconf.h included
 *
 * Revision 6.8  2000/05/31 23:12:14  lavr
 * First try to assemble things together to get working service mapper
 *
 * Revision 6.7  2000/05/23 19:02:47  lavr
 * Server-info now includes rate; verbal representation changed
 *
 * Revision 6.6  2000/05/22 16:53:07  lavr
 * Rename service_info -> server_info everywhere (including
 * file names) as the latter name is more relevant
 *
 * Revision 6.5  2000/05/15 19:06:05  lavr
 * Use home-made ANSI extensions (NCBI_***)
 *
 * Revision 6.4  2000/05/12 21:42:11  lavr
 * SSERV_Info::  use ESERV_Type, ESERV_Flags instead of TSERV_Type, TSERV_Flags
 *
 * Revision 6.3  2000/05/12 18:31:48  lavr
 * First working revision
 *
 * Revision 6.2  2000/05/09 15:31:29  lavr
 * Minor changes
 *
 * Revision 6.1  2000/05/05 20:24:00  lavr
 * Initial revision
 *
 * ==========================================================================
 */

#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Server types
 */
typedef enum {
    fSERV_Ncbid      = 0x1,
    fSERV_Standalone = 0x2,
    fSERV_HttpGet    = 0x4,
    fSERV_HttpPost   = 0x8,
    fSERV_Http       = fSERV_HttpGet | fSERV_HttpPost
} ESERV_Type;

#define fSERV_Any           0
#define fSERV_StatelessOnly 0x80
typedef unsigned TSERV_Type;  /* bit-wise OR of "ESERV_Type" flags */


/* Flags to specify the algorithm for selecting the most preferred
 * server from the set of available servers
 */
typedef enum {
    fSERV_Regular = 0x0,
    fSERV_Blast   = 0x1
} ESERV_Flags;
typedef int TSERV_Flags;

#define SERV_DEFAULT_FLAG       fSERV_Regular


/* Verbal representation of a server type (no internal spaces allowed)
 */
const char* SERV_TypeStr(ESERV_Type type);


/* Read server info type.
 * If successful, assign "type" and return pointer to the position
 * in the "str" immediately following the type tag.
 * On error, return NULL.
 */
const char* SERV_ReadType(const char* str, ESERV_Type* type);


/* Meta-addresses for various types of NCBI servers
 */
typedef struct {
    size_t         args;
#define SERV_NCBID_ARGS(ui)     ((char *)(ui) + (ui)->args)
} SSERV_NcbidInfo;

typedef struct {
    int            dummy;       /* placeholder, not used */
} SSERV_StandaloneInfo;

typedef struct {
    size_t         path;
    size_t         args;
#define SERV_HTTP_PATH(ui)      ((char *)(ui) + (ui)->path)
#define SERV_HTTP_ARGS(ui)      ((char *)(ui) + (ui)->args)
} SSERV_HttpInfo;


/* Generic NCBI server meta-address
 */
typedef union {
    SSERV_NcbidInfo      ncbid;
    SSERV_StandaloneInfo standalone;
    SSERV_HttpInfo       http;
} USERV_Info;

typedef struct {
    ESERV_Type            type; /* type of server                            */
    unsigned int          host; /* host the server running on, network b.o.  */
    unsigned short        port; /* port the server running on, host b.o.     */
    unsigned char/*bool*/ sful; /* true for stateful-only server (default=no)*/
    unsigned char/*bool*/ locl; /* true for local (LBSMD-only) server(def=no)*/
    ESERV_Flags           flag; /* algorithm flag for the server             */
    time_t                time; /* relaxation/expiration time/period         */
    double                coef; /* bonus coefficient for server run locally  */
    double                rate; /* rate of the server                        */
    USERV_Info            u;    /* server type-specific data/params          */
} SSERV_Info;


/* Constructors for the various types of NCBI server meta-addresses
 */
SSERV_Info* SERV_CreateNcbidInfo
(unsigned int   host,           /* network byte order */
 unsigned short port,           /* host byte order    */
 const char*    args
 );

SSERV_Info* SERV_CreateStandaloneInfo
(unsigned int   host,           /* network byte order */
 unsigned short port            /* host byte order    */
 );

SSERV_Info* SERV_CreateHttpInfo
(ESERV_Type     type,           /* verified, must be one of fSERV_Http* */
 unsigned int   host,           /* network byte order */
 unsigned short port,           /* host byte order    */
 const char*    path,
 const char*    args
 );


/* Dump server info to a string.
 * The server type goes first, and it is followed by a single space.
 * The returned string is '\0'-terminated, and must be deallocated by 'free()'.
 */
char* SERV_WriteInfo(const SSERV_Info* info);


/* Server specification consists of the following:
 * TYPE [host][:port] [server-specific_parameters] [tags]
 *
 * TYPE := { STANDALONE | NCBID | HTTP | HTTP_GET | HTTP_POST }
 *
 * Host should be specified as either an IP address (in dotted notation),
 * or as a host name (using domain notation if necessary).
 * Port number must be preceded by a colon.
 * Both host and port get their default values if not specified.
 *
 * Server-specific parameters:
 *
 *    Standalone servers: None
 *                        Servers of this type do not take any arguments.
 *
 *    NCBID servers: Arguments to CGI in addition to specified by application.
 *                   Empty additional arguments denoted as '' (double quotes).
 *                   Note that arguments must not contain space characters.
 *
 *    HTTP* servers: Path (required) and args (optional) in the form
 *                   path[?args] (here brackets denote the optional part).
 *                   Note that no spaces allowed within this parameter.
 *
 * Tags may follow in no specific order but no more than one instance
 * of each flag is allowed:
 *
 *    Load average calculation for the server:
 *       Regular (default)
 *       Blast
 *
 *    Local server:
 *       L=no    (default)
 *       L=yes
 *           Local servers are accessible only by direct clients of LBSMD,
 *           that is such servers cannot be accessed by means of DISPD.
 *
 *    Stateful server:
 *       S=no    (default)
 *       S=yes
 *           Indication of stateful server, which allows only dedicated socket
 *           (stateful) connections (HTTP mode - aka stateless - not allowed).
 *
 *    Validity period:
 *       T=integer    [0 = default]
 *           specifies the time in seconds this server entry is valid
 *           without update. (If equal to 0 then defaulted by
 *           the LBSM Daemon to some reasonable value.)
 *
 *    Reachability coefficient:
 *       R=double     [0.0 = default]
 *           specifies availability ratio for the server, expressed as
 *           a floating point number with 0.0 meaning the server is down
 *           (unavailable) and 1000.0 meaning the server is up and running.
 *           Intermediate values can be used to make the server less or more
 *           favorable for choosing by LBSM Daemon, as this coefficient is
 *           directly used as a multiplier in the load-average calculation
 *           for the entire family of servers for the same service.
 *           (If equal to 0.0 then defaulted by the LBSM Daemon to 1000.0.)
 *           Normally, LBSMD keeps track of server reachability, and
 *           dynamically switches this rate to be maximal specified when
 *           the server is up, and to be zero when the server is down.
 *           Note that negative values are reserved for LBSMD private use.
 *
 *    Bonus coefficient:
 *       B=double     [0.0 = default]
 *           specifies a multiplicative bonus given to a server run locally,
 *           when calculating reachability rate.
 *           Special rules apply to negitive/zero values:
 *           0.0 means not to use the described rate increase at all (default
 *           rate calculation is used, which only slightly increases rates
 *           of locally run servers).
 *           Negative value denotes that locally run server should
 *           be taken in first place, regardless of its rate, if this rate
 *           is larger than percent of expressed by the absolute value
 *           of this coefficient of average rate coefficient of other
 *           servers for the same service. That is -5 instructs to
 *           ignore locally run server if its status is less than 5% of
 *           average status of remaining servers for the same service.
 *
 *
 * Note that optional arguments can be omitted along with all preceding
 * optional arguments, that is the following 2 server specifications are
 * both valid:
 *
 * NCBID ''
 * and
 * NCBID
 *
 * but they are not equal to the following specification:
 *
 * NCBID Regular
 *
 * because here 'Regular' is treated as an argument, not as a tag.
 * To make the latter specification equivalent to the former two, one has
 * to use the following form:
 *
 * NCBID '' Regular
 */


/* Read full server info (including type) from string "str"
 * (e.g. composed by SERV_WriteInfo). Result can be later freed by 'free()'.
 * If host is not found in the server specification, info->host is
 * set to 0; if port is not found, type-specific default value is used.
 */
SSERV_Info* SERV_ReadInfo(const char* info_str);


/* Return an actual size (in bytes) the server info occupies
 * (to be used for copying info structures in whole).
 */
size_t SERV_SizeOfInfo(const SSERV_Info* info);


/* Return 'true' if two server infos are equal.
 */
int/*bool*/ SERV_EqualInfo(const SSERV_Info* info1, const SSERV_Info* info2);


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* NCBI_SERVER_INFO__H */
