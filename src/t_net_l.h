/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_l.h
 * \brief     functions interface for t_net*
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#define __USE_MISC
#define _DEFAULT_SOURCE 1
#include <stdint.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>         // O_NONBLOCK,...
#include <sys/select.h>    // fd_set
#include <netinet/tcp.h>   // TCP_NODELAY
#include <netinet/in.h>    // IPPROTO_*
#ifdef __linux
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/sockios.h> //SIOCINQ
#endif
#ifdef _WIN32
#include <WinSock2.h>
#endif

#include "t_net.h"
#include "t.h"             // t_typeerror,t_getLoadedValue

#ifndef T_NET_DEF_FAM_H
#define T_NET_DEF_FAM_H
// Always create sockets or addresses with this family unless specified
extern int _t_net_default_family;
#endif  // T_NET_DEF_FAM_H


// Socket option handling type declaration
enum t_net_sck_optionType {
	T_NET_SCK_OTP_BOOL,
	T_NET_SCK_OTP_INT,
	T_NET_SCK_OTP_LINGER,
	T_NET_SCK_OTP_TIME,
	T_NET_SCK_OTP_FCNTL,
	T_NET_SCK_OTP_IOCTL,
	// proprietary options
	T_NET_SCK_OTP_DSCR,    ///< retrieve Socket descriptor number
	T_NET_SCK_OTP_FMLY,    ///< retrieve Socket Family Name
	T_NET_SCK_OTP_PRTC,    ///< retrieve Socket Protocol Name
	T_NET_SCK_OTP_TYPE,    ///< retrieve Socket Type Name
};

struct t_net_sck_option
{
	const char *const                name;
	const int                        getlevel;
	const int                        setlevel;
	const int                        option;
	const enum t_net_sck_optionType  type;
	const int                        get;
	const int                        set;
};

// Constructors
// t_net_adr.c
int                      luaopen_t_net_adr     ( lua_State *L );
struct sockaddr_storage *t_net_adr_check_ud    ( lua_State *L, int pos, int check );
struct sockaddr_storage *t_net_adr_create_ud   ( lua_State *L );
#define t_net_adr_is( L, pos ) (NULL != t_net_adr_check_ud( L, pos, 0 ))


// ----------------------------- INTERFACES -----------------------------------
// t_netl.c
int    t_net_getStack          ( lua_State *L, const int pos, struct t_net_sck **sck,
                                 struct sockaddr_storage **adr );
// t_net_ifc.c
int    luaopen_t_net_ifc   ( lua_State *L );
void   t_net_ifc_check     ( lua_State *L, int pos );
int    t_net_ifc_create    ( lua_State *L, const char *name );
int    p_net_ifc_get       ( lua_State *L, const char *name );

// t_net_sck.c
int               luaopen_t_net_sck  ( lua_State *L );
struct t_net_sck *t_net_sck_create_ud( lua_State *L );

// t_net_sck platform specific implementation (interface)
int    p_net_sck_open( void );
int    p_net_sck_createHandle   (               struct t_net_sck *sck, int family, int type, int protocol );
int    p_net_sck_listen         (               struct t_net_sck *sck, const int bl );
int    p_net_sck_bind           (               struct t_net_sck *sck, struct sockaddr_storage *adr );
int    p_net_sck_connect        (               struct t_net_sck *sck, struct sockaddr_storage *adr );
int    p_net_sck_accept         (               struct t_net_sck *srv, struct t_net_sck *cli, struct sockaddr_storage *adr );
ssize_t p_net_sck_send          (               struct t_net_sck *sck, struct sockaddr_storage *adr, const char* buf, size_t len );
ssize_t p_net_sck_recv          (               struct t_net_sck *sck, struct sockaddr_storage *adr,       char *buf, size_t len );
int    p_net_sck_shutDown       (               struct t_net_sck *sck, int shutVal );
int    p_net_sck_close          (               struct t_net_sck *sck );
int    p_net_sck_setSocketOption( lua_State *L, struct t_net_sck *sck, struct t_net_sck_option *opt );
int    p_net_sck_getSocketOption( lua_State *L, struct t_net_sck *sck, struct t_net_sck_option *opt );
int    p_net_sck_getsockname    (               struct t_net_sck *sck, struct sockaddr_storage *adr );
int    p_net_sck_mkFdSet        ( lua_State *L, int pos, fd_set *set );


int    luaopen_t_net_fml        ( lua_State *L );
int    luaopen_t_net_sck_ptc    ( lua_State *L );
int    luaopen_t_net_sck_typ    ( lua_State *L );
// ---------------------------- MACRO HELPERS FOR IP ADDRESSES ----------------
//
#define SOCK_ADDR_PTR(ptr)        ((struct sockaddr *)(ptr))
#define SOCK_ADDR_FAMILY(ptr)     SOCK_ADDR_PTR(ptr)->sa_family

#define SOCK_ADDR_SS_PTR(ptr)     ((struct sockaddr_storage *)(ptr))
#define SOCK_ADDR_SS_FAMILY(ptr)  SOCK_ADDR_SS_PTR(ptr)->ss_family

#define SOCK_ADDR_SS_LEN(ss)                            \
   (NULL == ss                                          \
      ? sizeof( ss )                                    \
      : (SOCK_ADDR_SS_PTR(ss)->ss_family == AF_INET6    \
         ? sizeof(struct sockaddr_in6)                  \
         : sizeof(struct sockaddr_in) ) )

#define SOCK_ADDR_IN4_PTR(ss)      ((struct sockaddr_in *)(ss))
#define SOCK_ADDR_IN4_FAMILY(ss)   SOCK_ADDR_IN4_PTR(ss)->sin_family
#define SOCK_ADDR_IN4_PORT(ss)     SOCK_ADDR_IN4_PTR(ss)->sin_port
#define SOCK_ADDR_IN4_ADDR(ss)     SOCK_ADDR_IN4_PTR(ss)->sin_addr
#define SOCK_ADDR_IN4_ADDR_INT(ss) SOCK_ADDR_IN4_PTR(ss)->sin_addr.s_addr

#define SOCK_ADDR_IN6_PTR(ss)      ((struct sockaddr_in6 *)(ss))
#define SOCK_ADDR_IN6_FAMILY(ss)   SOCK_ADDR_IN6_PTR(ss)->sin6_family
#define SOCK_ADDR_IN6_PORT(ss)     SOCK_ADDR_IN6_PTR(ss)->sin6_port
#define SOCK_ADDR_IN6_ADDR(ss)     SOCK_ADDR_IN6_PTR(ss)->sin6_addr
#define SOCK_ADDR_IN6_ADDR_INT(ss) SOCK_ADDR_IN6_PTR(ss)->sin6_addr.s6_addr
#define SOCK_ADDR_IN6_SCOPE(ss)    SOCK_ADDR_IN6_PTR(ss)->sin6_scope_id
#define SOCK_ADDR_IN6_FLOW(ss)     SOCK_ADDR_IN6_PTR(ss)->sin6_flowinfo

#define SOCK_ADDR_UNX_PTR(ss)      ((struct sockaddr_un *)(ss))
#define SOCK_ADDR_UNX_FAMILY(ss)   SOCK_ADDR_UNX_PTR(ss)->sun_family
#define SOCK_ADDR_UNX_PATH(ss)     SOCK_ADDR_UNX_PTR(ss)->sun_path

#define SOCK_ADDR_SS_PORT(ss)                           \
   (SOCK_ADDR_SS_PTR(ss)->ss_family == AF_INET6         \
      ? SOCK_ADDR_IN6_PORT(ss)                          \
      : SOCK_ADDR_IN4_PORT(ss) )

#define SOCK_ADDR_SS_ADDR(ss)                           \
   (SOCK_ADDR_SS_PTR(ss)->ss_family == AF_INET6         \
      ? SOCK_ADDR_IN6_ADDR(ss)                          \
      : SOCK_ADDR_IN4_ADDR(ss) )

#define SOCK_ADDR_SS_ADDR_INT(ss)                       \
   (SOCK_ADDR_SS_PTR(ss)->ss_family == AF_INET6         \
      ? SOCK_ADDR_IN6_ADDR_INT(ss)                      \
      : SOCK_ADDR_IN4_ADDR_INT(ss) )

//     ---------------------------- inet_ntop, inet_pton ...
#ifdef _WIN32
#define INET_PTON InetPton
#define INET_NTOP InetNtop
#else
#define INET_PTON inet_pton
#define INET_NTOP inet_ntop
#endif

#define SOCK_ADDR_IN6_PTON(ss, ips)                      \
   INET_PTON( AF_INET6, ips, &(SOCK_ADDR_IN6_ADDR( ss )))

#define SOCK_ADDR_IN4_PTON(ss, ips)                      \
   INET_PTON( AF_INET, ips, &(SOCK_ADDR_IN4_ADDR( ss )))

#define SOCK_ADDR_IN6_NTOP( ss, dst )                    \
   INET_NTOP( AF_INET6, &(SOCK_ADDR_IN6_ADDR( ss )), dst, INET6_ADDRSTRLEN )

#define SOCK_ADDR_IN4_NTOP( ss, dst )                    \
   INET_NTOP( AF_INET,  &(SOCK_ADDR_IN4_ADDR( ss )), dst, INET_ADDRSTRLEN )

#define SOCK_ADDR_INET_PTON(ss, ips)                     \
   (SOCK_ADDR_SS_PTR(ss)->ss_family == AF_INET6          \
      ? SOCK_ADDR_IN6_PTON( ss, ips )                    \
      : SOCK_ADDR_IN4_PTON( ss, ips ) )

#define SOCK_ADDR_INET_NTOP(ss, dst)                     \
   (SOCK_ADDR_SS_PTR(ss)->ss_family == AF_INET6          \
      ? SOCK_ADDR_IN6_NTOP( ss, dst )                    \
      : SOCK_ADDR_IN4_NTOP( ss, dst ) )

//    -------------------------- address equality
#define SOCK_ADDR_EQ_FAMILY(sa, sb)                                                  \
   (SOCK_ADDR_SS_FAMILY(sa) == SOCK_ADDR_SS_FAMILY(sb) )

#define SOCK_ADDR_EQ_ADDR(sa, sb)                                                    \
   ((SOCK_ADDR_SS_FAMILY(sa) == AF_INET  && SOCK_ADDR_SS_FAMILY(sb) == AF_INET       \
       && SOCK_ADDR_IN4_ADDR(sa).s_addr == SOCK_ADDR_IN4_ADDR(sb).s_addr)            \
    || (SOCK_ADDR_SS_FAMILY(sa) == AF_INET6 && SOCK_ADDR_SS_FAMILY(sb) == AF_INET6   \
       && memcmp((char *) &(SOCK_ADDR_IN6_ADDR(sa)),                                 \
                 (char *) &(SOCK_ADDR_IN6_ADDR(sb)),                                 \
                  sizeof(SOCK_ADDR_IN6_ADDR(sa))) == 0)   )

#define SOCK_ADDR_EQ_PORT(sa, sb) \
    ((SOCK_ADDR_SS_FAMILY(sa) == AF_INET  && SOCK_ADDR_SS_FAMILY(sb) == AF_INET      \
        && SOCK_ADDR_IN4_PORT(sa) == SOCK_ADDR_IN4_PORT(sb))                         \
  || (SOCK_ADDR_SS_FAMILY(sa) == AF_INET6 && SOCK_ADDR_SS_FAMILY(sb) == AF_INET6     \
        && SOCK_ADDR_IN6_PORT(sa) == SOCK_ADDR_IN6_PORT(sb))  )


// ----------------------------- CONSTANT VALUES -----------------------------------

/* ############################################################################
 * Handling of socket, descriptor, protocoll options etc.  After trying a big
 * old switch statement for *ALL* options lua-conman's approach is much more
 * flexible and readable.  Adapting and extending.
 * ############################################################################ */
// bsearch() is used on this array.  Order keys (name) alphabetically!
static const struct t_net_sck_option t_net_sck_options[ ] =
{
	{ "broadcast"   , SOL_SOCKET  , 0       , SO_BROADCAST   , T_NET_SCK_OTP_BOOL   , 1 , 1 } ,
#ifdef FD_CLOEXEC
	{ "closeexec"   , F_GETFD     , F_SETFD , FD_CLOEXEC     , T_NET_SCK_OTP_FCNTL  , 1 , 1 } ,
#endif
	{ "debug"       , SOL_SOCKET  , 0       , SO_DEBUG       , T_NET_SCK_OTP_BOOL   , 1 , 1 } ,
	{ "descriptor"  , 0           , 0       , 0              , T_NET_SCK_OTP_DSCR   , 1 , 0 } ,
	{ "dontroute"   , SOL_SOCKET  , 0       , SO_DONTROUTE   , T_NET_SCK_OTP_BOOL   , 1 , 1 } ,
	{ "error"       , SOL_SOCKET  , 0       , SO_ERROR       , T_NET_SCK_OTP_INT    , 1 , 0 } ,
	{ "family"      , 0           , 0       , 0              , T_NET_SCK_OTP_FMLY   , 1 , 0 } ,
	{ "keepalive"   , SOL_SOCKET  , 0       , SO_KEEPALIVE   , T_NET_SCK_OTP_BOOL   , 1 , 1 } ,
	{ "linger"      , SOL_SOCKET  , 0       , SO_LINGER      , T_NET_SCK_OTP_LINGER , 1 , 1 } ,
	{ "maxsegment"  , IPPROTO_TCP , 0       , TCP_MAXSEG     , T_NET_SCK_OTP_INT    , 1 , 1 } ,
	{ "nodelay"     , IPPROTO_TCP , 0       , TCP_NODELAY    , T_NET_SCK_OTP_BOOL   , 1 , 1 } ,
	{ "nonblock"    , F_GETFL     , F_SETFL , O_NONBLOCK     , T_NET_SCK_OTP_FCNTL  , 1 , 1 } ,
#ifdef SO_NOSIGPIPE
	{ "nosigpipe"   , SOL_SOCKET  , 0       , SO_NOSIGPIPE   , T_NET_SCK_OTP_BOOL   , 1 , 1 } ,
#endif
	{ "oobinline"   , SOL_SOCKET  , 0       , SO_OOBINLINE   , T_NET_SCK_OTP_BOOL   , 1 , 1 } ,
#ifdef SO_PROTOCOL
	{ "protocol"    , SOL_SOCKET  , 0       , SO_PROTOCOL    , T_NET_SCK_OTP_PRTC   , 1 , 0 } ,
#endif
	{ "recvbuffer"  , SOL_SOCKET  , 0       , SO_RCVBUF      , T_NET_SCK_OTP_INT    , 1 , 1 } ,
	{ "recvlow"     , SOL_SOCKET  , 0       , SO_RCVLOWAT    , T_NET_SCK_OTP_INT    , 1 , 1 } ,
#ifdef __linux
	{ "recvqueue"   , SIOCINQ     , 0       , 0              , T_NET_SCK_OTP_IOCTL  , 1 , 0 } ,
#endif
	{ "recvtimeout" , SOL_SOCKET  , 0       , SO_RCVTIMEO    , T_NET_SCK_OTP_TIME   , 1 , 1 } ,
	{ "reuseaddr"   , SOL_SOCKET  , 0       , SO_REUSEADDR   , T_NET_SCK_OTP_BOOL   , 1 , 1 } ,
#ifdef SO_REUSEPORT
	{ "reuseport"   , SOL_SOCKET  , 0       , SO_REUSEPORT   , T_NET_SCK_OTP_BOOL   , 1 , 1 } ,
#endif
	{ "sendbuffer"  , SOL_SOCKET  , 0       , SO_SNDBUF      , T_NET_SCK_OTP_INT    , 1 , 1 } ,
	{ "sendlow"     , SOL_SOCKET  , 0       , SO_SNDLOWAT    , T_NET_SCK_OTP_INT    , 1 , 1 } ,
#ifdef __linux
	{ "sendqueue"   , SIOCOUTQ    , 0       , 0              , T_NET_SCK_OTP_IOCTL  , 1 , 0 } ,
#endif
	{ "sendtimeout" , SOL_SOCKET  , 0       , SO_SNDTIMEO    , T_NET_SCK_OTP_TIME   , 1 , 1 } ,
	{ "type"        , SOL_SOCKET  , 0       , SO_TYPE        , T_NET_SCK_OTP_TYPE   , 1 , 0 } ,
#ifdef SO_USELOOPBACK
	{ "useloopback" , SOL_SOCKET  , 0       , SO_USELOOPBACK , T_NET_SCK_OTP_BOOL   , 1 , 1 } ,
#endif
};

#define T_NET_SCK_OPTS_MAX       (sizeof(t_net_sck_options) / sizeof(struct t_net_sck_option))
