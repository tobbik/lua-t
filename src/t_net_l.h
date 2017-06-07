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
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>       // O_NONBLOCK,...
#include <sys/select.h>  // fd_set

#include "t_net.h"
#include "t.h"           // t_getType*


#define INT_TO_ADDR( _addr ) \
	(_addr &       0xFF), \
	(_addr >> 8  & 0xFF), \
	(_addr >> 16 & 0xFF), \
	(_addr >> 24 & 0xFF)

#ifndef T_NET_DEF_FAM_H
#define T_NET_DEF_FAM_H
// Always create sockets or addresses with this family unless specified
int _t_net_default_family;
#endif  // T_NET_DEF_FAM_H

// Constructors
// t_net_adr.c
int                      luaopen_t_net_adr     ( lua_State *L );
struct sockaddr_storage *t_net_adr_check_ud    ( lua_State *L, int pos, int check );
struct sockaddr_storage *t_net_adr_create_ud   ( lua_State *L );
void                     t_net_adr_setAddr     ( lua_State *L, struct sockaddr_storage *adr, const char* ips );
void                     t_net_adr_setPort     ( lua_State *L, struct sockaddr_storage *adr, const int port, const int pos );
int                     lt_net_adr_getIpAndPort( lua_State *L );
struct sockaddr_storage *t_net_adr_getFromStack( lua_State *L, int pos, int *returnables );
#define t_net_adr_is( L, pos ) (NULL != t_net_adr_check_ud( L, pos, 0 ))


// ----------------------------- INTERFACES -----------------------------------
// t_netl.c
void   t_net_getProtocolByName ( lua_State *L, int pos, const char *dft );
void   t_net_getProtocolByValue( lua_State *L, int pos, const int val );
int    t_net_getStack          ( lua_State *L, const int pos, struct t_net_sck **sck,
                                 struct sockaddr_storage **adr );

// t_net_ifc.c
int    luaopen_t_net_ifc   ( lua_State *L );
void   t_net_ifc_check     ( lua_State *L, int pos );
int    t_net_ifc_create    ( lua_State *L, const char *name );
int    p_net_ifc_get       ( lua_State *L, const char *name );

// t_net_sck.c
int               luaopen_t_net_sck  ( lua_State *L );
struct t_net_sck *t_net_sck_create_ud( lua_State *L, int family, int type, int protocol, int create );


// t_net_sck_implementation...
void   p_net_sck_createHandle   ( lua_State *L, struct t_net_sck *sck, int family, int type, int protocol );
int    p_net_sck_listen         ( lua_State *L, struct t_net_sck *sck, struct sockaddr_storage *adr, const int bl );
int    p_net_sck_bind           ( lua_State *L, struct t_net_sck *sck, struct sockaddr_storage *adr );
int    p_net_sck_connect        ( lua_State *L, struct t_net_sck *sck, struct sockaddr_storage *adr );
int    p_net_sck_accept         ( lua_State *L, struct t_net_sck *srv, struct t_net_sck *cli, struct sockaddr_storage *adr );
int    p_net_sck_send           ( lua_State *L, struct t_net_sck *sck, struct sockaddr_storage *adr, const char* buf, size_t len );
int    p_net_sck_recv           ( lua_State *L, struct t_net_sck *sck, struct sockaddr_storage *adr,       char *buf, size_t len );
int    p_net_sck_shutDown       ( lua_State *L, struct t_net_sck *sck, int shutVal );
int    p_net_sck_close          ( lua_State *L, struct t_net_sck *sck );
int    p_net_sck_setSocketOption( lua_State *L, struct t_net_sck *sck, int sckOpt, const char *sckOptName, int val );
int    p_net_sck_getSocketOption( lua_State *L, struct t_net_sck *sck, int sckOpt, const char *sckOptName );
int    p_net_sck_getsockname    ( struct t_net_sck *sck, struct sockaddr_storage *adr );
int    p_net_sck_mkFdSet        ( lua_State *L, int pos, fd_set *set );


// ---------------------------- MACRO HELPERS FOR IP ADDRESSES ----------------
//
#define SOCK_ADDR_PTR(ptr)        ((struct sockaddr *)(ptr))
#define SOCK_ADDR_FAMILY(ptr)     SOCK_ADDR_PTR(ptr)->sa_family

#define SOCK_ADDR_SS_PTR(ptr)     ((struct sockaddr_storage *)(ptr))
#define SOCK_ADDR_SS_FAMILY(ptr)  SOCK_ADDR_SS_PTR(ptr)->ss_family

#define SOCK_ADDR_SS_LEN(ss) \
   (NULL == ss \
      ? sizeof( ss ) \
      : (SOCK_ADDR_SS_PTR(ss)->ss_family == AF_INET6 \
         ? sizeof(struct sockaddr_in6) \
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

#define SOCK_ADDR_UNX_PTR(ss)      ((struct sockaddr_un *)(ss))
#define SOCK_ADDR_UNX_FAMILY(ss)   SOCK_ADDR_UNX_PTR(ss)->sun_family
#define SOCK_ADDR_UNX_PATH(ss)     SOCK_ADDR_UNX_PTR(ss)->sun_path

#define SOCK_ADDR_SS_PORT(ss) \
   (SOCK_ADDR_SS_PTR(ss)->ss_family == AF_INET6 \
      ? SOCK_ADDR_IN6_PORT(ss) \
      : SOCK_ADDR_IN4_PORT(ss) )

#define SOCK_ADDR_SS_ADDR(ss) \
   (SOCK_ADDR_SS_PTR(ss)->ss_family == AF_INET6 \
      ? SOCK_ADDR_IN6_ADDR(ss) \
      : SOCK_ADDR_IN4_ADDR(ss) )

#define SOCK_ADDR_SS_ADDR_INT(ss) \
   (SOCK_ADDR_SS_PTR(ss)->ss_family == AF_INET6 \
      ? SOCK_ADDR_IN6_ADDR_INT(ss) \
      : SOCK_ADDR_IN4_ADDR_INT(ss) )

//     ---------------------------- inet_ntop, inet_pton ...
#ifdef _WIN32
#define INET_PTON InetPton
#define INET_NTOP InetNtop
#else
#define INET_PTON inet_pton
#define INET_NTOP inet_ntop
#endif

#define SOCK_ADDR_IN6_PTON(ss, ips)                     \
   INET_PTON( AF_INET6, ips, &(SOCK_ADDR_IN6_ADDR( ss )))

#define SOCK_ADDR_IN4_PTON(ss, ips)                     \
   INET_PTON( AF_INET, ips, &(SOCK_ADDR_IN4_ADDR( ss )))

#define SOCK_ADDR_IN6_NTOP( ss, dst )                       \
   INET_NTOP( AF_INET6, &(SOCK_ADDR_IN6_ADDR( ss )), dst, SOCK_ADDR_SS_LEN( adr ))

#define SOCK_ADDR_IN4_NTOP( ss, dst )                       \
   INET_NTOP( AF_INET,  &(SOCK_ADDR_IN4_ADDR( ss )), dst, SOCK_ADDR_SS_LEN( adr ))

#define SOCK_ADDR_INET_PTON(ss, ips)                     \
   (SOCK_ADDR_SS_PTR(ss)->ss_family == AF_INET6              \
      ? SOCK_ADDR_IN6_PTON( ss, ips )                    \
      : SOCK_ADDR_IN4_PTON( ss, ips ) )

#define SOCK_ADDR_INET_NTOP(ss, dst)                     \
   (SOCK_ADDR_SS_PTR(ss)->ss_family == AF_INET6              \
      ? SOCK_ADDR_IN6_NTOP( ss, dst )                    \
      : SOCK_ADDR_IN4_NTOP( ss, dst ) )

//    -------------------------- address equality
#define SOCK_ADDR_EQ_FAMILY(sa, sb) \
   (SOCK_ADDR_SS_FAMILY(sa) == SOCK_ADDR_SS_FAMILY(sb) )

#define SOCK_ADDR_EQ_ADDR(sa, sb) \
   ((SOCK_ADDR_SS_FAMILY(sa) == AF_INET  && SOCK_ADDR_SS_FAMILY(sb) == AF_INET \
       && SOCK_ADDR_IN4_ADDR(sa).s_addr == SOCK_ADDR_IN4_ADDR(sb).s_addr) \
    || (SOCK_ADDR_SS_FAMILY(sa) == AF_INET6 && SOCK_ADDR_SS_FAMILY(sb) == AF_INET6 \
       && memcmp((char *) &(SOCK_ADDR_IN6_ADDR(sa)), \
                 (char *) &(SOCK_ADDR_IN6_ADDR(sb)), \
                  sizeof(SOCK_ADDR_IN6_ADDR(sa))) == 0)   )

#define SOCK_ADDR_EQ_PORT(sa, sb) \
    ((SOCK_ADDR_SS_FAMILY(sa) == AF_INET  && SOCK_ADDR_SS_FAMILY(sb) == AF_INET \
        && SOCK_ADDR_IN4_PORT(sa) == SOCK_ADDR_IN4_PORT(sb)) \
  || (SOCK_ADDR_SS_FAMILY(sa) == AF_INET6 && SOCK_ADDR_SS_FAMILY(sb) == AF_INET6 \
        && SOCK_ADDR_IN6_PORT(sa) == SOCK_ADDR_IN6_PORT(sb))  )


// ----------------------------- CONSTANT VALUES -----------------------------------

static const struct t_typ t_net_familyList[ ] = {
#ifdef AF_UNSPEC
	{ "AF_UNSPEC"         , AF_UNSPEC        },  //  0 Unspecified.
	{ "any"               , AF_UNSPEC        },
#endif
#ifdef AF_LOCAL
	{ "AF_LOCAL"          , AF_LOCAL         },  //  1 Local to host (pipes and file-domain).
#endif
#ifdef AF_UNIX
	{ "AF_UNIX"           , AF_UNIX          },  //  AF_LOCAL POSIX name for AF_LOCAL.
	{ "unix"              , AF_UNIX          },
#endif
#ifdef AF_FILE
	{ "AF_FILE"           , AF_FILE          },  //  AF_LOCAL Another non-standard name for AF_LOCAL.
#endif
#ifdef AF_INET
	{ "AF_INET"           , AF_INET          },  //  2 IP protocol family.
	{ "ip4"               , AF_INET          },
	{ "Ip4"               , AF_INET          },
	{ "IP4"               , AF_INET          },
	{ "IPv4"              , AF_INET          },
#endif
#ifdef AF_AX25
	{ "AF_AX25"           , AF_AX25          },  //  3 Amateur Radio AX.25.
#endif
#ifdef AF_IPX
	{ "AF_IPX"            , AF_IPX           },  //  4 Novell Internet Protocol.
#endif
#ifdef AF_APPLETALK
	{ "AF_APPLETALK"      , AF_APPLETALK     },  //  5 Appletalk DDP.
#endif
#ifdef AF_NETROM
	{ "AF_NETROM"         , AF_NETROM        },  //  6 Amateur radio NetROM.
#endif
#ifdef AF_BRIDGE
	{ "AF_BRIDGE"         , AF_BRIDGE        },  //  7 Multiprotocol bridge.
#endif
#ifdef AF_ATMPVC
	{ "AF_ATMPVC"         , AF_ATMPVC        },  //  8 ATM PVCs.
#endif
#ifdef AF_X25
	{ "AF_X25"            , AF_X25           },  //  9 Reserved for X.25 project.
#endif
#ifdef AF_INET6
	{ "AF_INET6"          , AF_INET6         },  // 10 IP version 6.
	{ "ip6"               , AF_INET6         },
	{ "Ip6"               , AF_INET6         },
	{ "IP6"               , AF_INET6         },
	{ "IPv6"              , AF_INET6         },
#endif
#ifdef AF_ROSE
	{ "AF_ROSE"           , AF_ROSE          },  // 11 Amateur Radio X.25 PLP.
#endif
#ifdef AF_DECnet
	{ "AF_DECnet"         , AF_DECnet        },  // 12 Reserved for DECnet project.
#endif
#ifdef AF_NETBEUI
	{ "AF_NETBEUI"        , AF_NETBEUI       },  // 13 Reserved for 802.2LLC project.
#endif
#ifdef AF_SECURITY
	{ "AF_SECURITY"       , AF_SECURITY      },  // 14 Security callback pseudo AF.
#endif
#ifdef AF_KEY
	{ "AF_KEY"            , AF_KEY           },  // 15 PF_KEY key management API.
#endif
#ifdef AF_NETLINK
	{ "AF_NETLINK"        , AF_NETLINK       },  // 16
#endif
#ifdef AF_ROUTE
	{ "AF_ROUTE"          , AF_ROUTE         },  // 1F_NETLINK Alias to emulate 4.4BSD.
#endif
#ifdef AF_PACKET
	{ "AF_PACKET"         , AF_PACKET        },  // 17 Packet family.
#endif
#ifdef AF_ASH
	{ "AF_ASH"            , AF_ASH           },  // 18 Ash.
#endif
#ifdef AF_ECONET
	{ "AF_ECONET"         , AF_ECONET        },  // 19 Acorn Econet.
#endif
#ifdef AF_ATMSVC
	{ "AF_ATMSVC"         , AF_ATMSVC        },  // 20 ATM SVCs.
#endif
#ifdef AF_RDS
	{ "AF_RDS"            , AF_RDS           },  // 21 RDS sockets.
#endif
#ifdef AF_SNA
	{ "AF_SNA"            , AF_SNA           },  // 22 Linux SNA Project
#endif
#ifdef AF_IRDA
	{ "AF_IRDA"           , AF_IRDA          },  // 23 IRDA sockets.
#endif
#ifdef AF_PPPOX
	{ "AF_PPPOX"          , AF_PPPOX         },  // 24 PPPoX sockets.
#endif
#ifdef AF_WANPIPE
	{ "AF_WANPIPE"        , AF_WANPIPE       },  // 25 Wanpipe API sockets.
#endif
#ifdef AF_LLC
	{ "AF_LLC"            , AF_LLC           },  // 26 Linux LLC.
#endif
#ifdef AF_IB
	{ "AF_IB"             , AF_IB            },  // 27 Native InfiniBand address.
#endif
#ifdef AF_MPLS
	{ "AF_MPLS"           , AF_MPLS          },  // 28 MPLS.
#endif
#ifdef AF_CAN
	{ "AF_CAN"            , AF_CAN           },  // 29 Controller Area Network.
#endif
#ifdef AF_TIPC
	{ "AF_TIPC"           , AF_TIPC          },  // 30 TIPC sockets.
#endif
#ifdef AF_BLUETOOTH
	{ "AF_BLUETOOTH"      , AF_BLUETOOTH     },  // 31 Bluetooth sockets.
#endif
#ifdef AF_IUCV
	{ "AF_IUCV"           , AF_IUCV          },  // 32 IUCV sockets.
#endif
#ifdef AF_RXRPC
	{ "AF_RXRPC"          , AF_RXRPC         },  // 33 RxRPC sockets.
#endif
#ifdef AF_ISDN
	{ "AF_ISDN"           , AF_ISDN          },  // 34 mISDN sockets.
#endif
#ifdef AF_PHONET
	{ "AF_PHONET"         , AF_PHONET        },  // 35 Phonet sockets.
#endif
#ifdef AF_IEEE802154
	{ "AF_IEEE802154"     , AF_IEEE802154    },  // 36 IEEE 802.15.4 sockets.
#endif
#ifdef AF_CAIF
	{ "AF_CAIF"           , AF_CAIF          },  // 37 CAIF sockets.
#endif
#ifdef AF_ALG
	{ "AF_ALG"            , AF_ALG           },  // 38 Algorithm sockets.
#endif
#ifdef AF_NFC
	{ "AF_NFC"            , AF_NFC           },  // 39 NFC sockets.
#endif
#ifdef AF_VSOCK
	{ "AF_VSOCK"          , AF_VSOCK         },  // 40 vSockets.
#endif
#ifdef AF_KCM
	{ "AF_KCM"            , AF_KCM           },  // 41 Kernel Connection Multiplexor.
#endif
#ifdef AF_QIPCRTR
	{ "AF_QIPCRTR"        , AF_QIPCRTR       },  // 42 Qualcomm IPC Router.
#endif
#ifdef AF_MAX
	{ "AF_MAX"            , AF_MAX           },  // 43 For now..
#endif
	{ NULL                , 0                }   // Sentinel
};


static const struct t_typ t_net_typeList[ ] = {
#ifdef SOCK_STREAM
	{ "SOCK_STREAM"       , SOCK_STREAM      },  // 1  Sequenced, reliable, connection-based byte streams.
	{ "stream"            , SOCK_STREAM      },
	{ "Stream"            , SOCK_STREAM      },
	{ "STREAM"            , SOCK_STREAM      },
#endif
#ifdef SOCK_DGRAM
	{ "SOCK_DGRAM"        , SOCK_DGRAM       },  // 2  Connectionless, unreliable datagrams of fixed maximum length.
	{ "dgram"             , SOCK_DGRAM       },
	{ "Dgram"             , SOCK_DGRAM       },
	{ "DGRAM"             , SOCK_DGRAM       },
	{ "datagram"          , SOCK_DGRAM       },
	{ "Datagram"          , SOCK_DGRAM       },
	{ "DATAGRAM"          , SOCK_DGRAM       },
#endif
#ifdef SOCK_RAW
	{ "SOCK_RAW"          , SOCK_RAW         },  // 3  Raw protocol interface.
	{ "raw"               , SOCK_RAW         },
	{ "Raw"               , SOCK_RAW         },
	{ "RAW"               , SOCK_RAW         },
#endif
#ifdef SOCK_RDM
	{ "SOCK_RDM"          , SOCK_RDM         },  // 4  Reliably-delivered messages
#endif
#ifdef SOCK_SEQPACKET
	{ "SOCK_SEQPACKET"    , SOCK_SEQPACKET   },  // 5  Sequenced, reliable, connection-based, datagrams of fixed maximum length.
#endif
#ifdef SOCK_DCCP
	{ "SOCK_DCCP"         , SOCK_DCCP        },  // 6  Datagram Congestion Control Protocol.
#endif
#ifdef SOCK_PACKET
	{ "SOCK_PACKET"       , SOCK_PACKET      },  // 10 Linux specific way of getting packets at the dev level.  For writing rarp and other similar things on the user level.
#endif

 // Flags to be ORed into the type parameter o}, socket and socketpair and
 // used for the flags parameter of paccept.

#ifdef SOCK_CLOEXEC
	{ "SOCK_CLOEXEC"      , SOCK_CLOEXEC     },  // 02000000  Atomically set close-on-exec flag for the new descriptor(s).
#endif
#ifdef SOCK_NONBLOCK
	{ "SOCK_NONBLOCK"     , SOCK_NONBLOCK    },  // 02000000  Atomically mark descriptor(s) as non-blocking.
#endif
	{ NULL                , 0                }   // Sentinel
};


// SO_* defined in /usr/lib/asm-generic/socket.h --> use value out of range
#define T_NET_SO_FAMILY 100

static const struct t_typ t_net_optionList[ ] = {
	// fcntl based options
	{ "nonblock"          , O_NONBLOCK       },

	// getsockopt/setsockopt integers
	{ "recvlow"           , SO_RCVLOWAT      },
	{ "recvtimeout"       , SO_RCVTIMEO      },
	{ "sendbuffer"        , SO_SNDBUF        },
	{ "sendlow"           , SO_SNDLOWAT      },
	{ "sendtimeout"       , SO_SNDTIMEO      },
	{ "error"             , SO_ERROR         },
	{ "recvbuffer"        , SO_RCVBUF        },

	// getsockopt/setsockopt booleans
	{ "broadcast"         , SO_BROADCAST     },
	{ "debug"             , SO_DEBUG         },
	{ "dontroute"         , SO_DONTROUTE     },
	{ "keepalive"         , SO_KEEPALIVE     },
	{ "oobinline"         , SO_OOBINLINE     },
	{ "reuseaddr"         , SO_REUSEADDR     },
#ifdef SO_USELOOPBACK
	{ "useloopback"       , SO_USELOOPBACK   },
#endif
#ifdef SO_REUSEPORT
	{ "reuseport"         , SO_REUSEPORT     },
#endif

	// getsockopt/setsockopt integer
	// translated to strings
	{ "family"            , T_NET_SO_FAMILY  },   // this is a fake SO_*
	{ "type"              , SO_TYPE          },
#ifdef SO_PROTOCOL
	{ "protocol"          , SO_PROTOCOL      },
#endif
	{ NULL                , 0                }
};

static const struct t_typ t_net_shutList[ ] = {
#ifdef SHUT_RD
	{ "SHUT_RD"            , SHUT_RD         }, // No more receptions.
	{ "read"               , SHUT_RD         },
	{ "rd"                 , SHUT_RD         },
	{ "r"                  , SHUT_RD         },
#endif
#ifdef SHUT_WR
	{ "SHUT_WR"            , SHUT_WR         }, // No more transmissions.
	{ "write"              , SHUT_WR         },
	{ "wr"                 , SHUT_WR         },
	{ "w"                  , SHUT_WR         },
#endif
#ifdef SHUT_RDWR
	{ "SHUT_RDWR"          , SHUT_RDWR       }, // No more receptions or transmissions.
	{ "readwrite"          , SHUT_RDWR       },
	{ "rdwr"               , SHUT_RDWR       },
	{ "rw"                 , SHUT_RDWR       },
	{ "both"               , SHUT_RDWR       },
	{ "either"             , SHUT_RDWR       },
#endif
	{ NULL                , 0                }
};
