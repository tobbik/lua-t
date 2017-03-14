/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net.h
 * \brief      socket functions wrapped for lua
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>     // O_NONBLOCK,...

#define T_NET_SCK_NAME   "Socket"
#define T_NET_IP4_NAME   "IPv4"
#define T_NET_IFC_NAME   "Interface"

#define T_NET_SCK_TYPE   T_NET_TYPE"."T_NET_SCK_NAME
#define T_NET_IP4_TYPE   T_NET_TYPE"."T_NET_IP4_NAME
#define T_NET_IFC_TYPE   T_NET_TYPE"."T_NET_IFC_NAME

#define INT_TO_ADDR( _addr ) \
	(_addr &       0xFF), \
	(_addr >> 8  & 0xFF), \
	(_addr >> 16 & 0xFF), \
	(_addr >> 24 & 0xFF)

// Constructors
// t_net_ip4.c
int                 luaopen_t_net_ip4     ( lua_State *L );
struct sockaddr_in *t_net_ip4_check_ud    ( lua_State *L, int pos, int check );
struct sockaddr_in *t_net_ip4_create_ud   ( lua_State *L );
void                t_net_ip4_set         ( lua_State *L, int pos, struct sockaddr_in *ip );
int                lt_net_ip4_getIpAndPort( lua_State *L );
#define t_net_ip4_is( L, pos ) (NULL != t_net_ip4_check_ud( L, pos, 0 ))


// ############################# REWRITE ######################################

// ----------------------------- DATA TYPES -----------------------------------
struct t_net_sck {
	int   domain;
	int   protocol;
	int   type;
	int   fd;    ///< socket handle
};

// ----------------------------- CONSTANT VALUES -----------------------------------

static const char *const t_net_domainName[ ] = { "ip4"   , "ip6"    , "unix" , NULL };
static const int         t_net_domainType[ ] = { AF_INET , AF_INET6 , AF_UNIX };

static const int t_net_sck_options[ ] = {
	//fcntl
	  O_NONBLOCK

	//getsockopt/setsockopt with integer value
	, SO_RCVLOWAT
	, SO_RCVTIMEO
	, SO_SNDBUF
	, SO_SNDLOWAT
	, SO_SNDTIMEO
	, SO_ERROR
	, SO_TYPE
	, SO_RCVBUF

	//getsockopt/setsockopt with boolean value
	, SO_BROADCAST
	, SO_DEBUG
	, SO_DONTROUTE
	, SO_KEEPALIVE
	, SO_OOBINLINE
	, SO_REUSEADDR
#ifdef SO_USELOOPBACK
	, SO_USELOOPBACK
#endif
#ifdef SO_REUSEPORT
	, SO_REUSEPORT
#endif
};

static const char *const t_net_sck_optionNames[ ] = {
	//fcntl
	  "nonblock"

	//getsockopt/setsockopt with boolean value
	, "recvlow"
	, "recvtimeout"
	, "sendbuffer"
	, "sendlow"
	, "sendtimeout"
	, "error"
	, "type"
	, "recvbuffer"

	//getsockopt/setsockopt with boolean value
	, "broadcast"
	, "debug"
	, "dontroute"
	, "keepalive"
	, "oobinline"
	, "reuseaddr"
#ifdef SO_USELOOPBACK
	, "useloopback"
#endif
#ifdef SO_REUSEPORT
	, "reuseport"
#endif
	, NULL
};


// ----------------------------- INTERFACES -----------------------------------
// t_net.c
int               t_net_getProtocol  ( lua_State *L, const int pos );
void              t_net_getdef       ( lua_State *L, const int pos, struct t_net_sck **sock, struct sockaddr_in **addr );
int               t_net_testOption   ( lua_State *L, int pos, const char *const lst[] );

// t_net_ifc.c
int               luaopen_t_net_ifc   ( lua_State *L );
void              t_net_ifc_check_ud  ( lua_State *L, int pos );
int               t_net_ifc_create_ud ( lua_State *L, const char *name );


// t_net_sck.c
int               luaopen_t_net_sck  ( lua_State *L );
struct t_net_sck *t_net_sck_create_ud( lua_State *L, int domain, int type, int protocol, int create );
struct t_net_sck *t_net_sck_check_ud ( lua_State *L, int pos, int check );

int               t_net_sck_listen   ( lua_State *L, const int pos );
int               t_net_sck_bind     ( lua_State *L, const int pos );
int               t_net_sck_connect  ( lua_State *L, const int pos );
int               t_net_sck_accept   ( lua_State *L, const int pos );
int               t_net_sck_send     ( lua_State *L, struct t_net_sck *sck, struct sockaddr_in *addr, const char* buf, size_t len );
int               t_net_sck_recv     ( lua_State *L, struct t_net_sck *sck, struct sockaddr_in *addr,       char *buf, size_t len );
int               t_net_sck_close    ( lua_State *L, struct t_net_sck *sck );
int               t_net_sck_setSocketOption( lua_State *L, struct t_net_sck *sck , int sckOpt,
                                                           const char *sckOptName, int val );
int               t_net_sck_getSocketOption( lua_State *L, struct t_net_sck *sck, int sckOpt,
                                                           const char       *sckOptName );
