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
	int   family;
	int   protocol;
	int   type;
	int   fd;    ///< socket handle
};



// ----------------------------- INTERFACES -----------------------------------
// t_net.c
int               t_net_getProtocolByName ( lua_State *L, const int pos );
char             *t_net_getProtocolByValue( lua_State *L, const int prot );
int               t_net_getdef       ( lua_State *L, const int pos, struct t_net_sck **sock, struct sockaddr_in **addr );
int               t_net_testOption   ( lua_State *L, int pos, const char *const lst[] );

// t_net_ifc.c
int               luaopen_t_net_ifc   ( lua_State *L );
void              t_net_ifc_check_ud  ( lua_State *L, int pos );
int               t_net_ifc_create_ud ( lua_State *L, const char *name );


// t_net_sck.c
int               luaopen_t_net_sck  ( lua_State *L );
struct t_net_sck *t_net_sck_create_ud( lua_State *L, int family, int protocol, int raw, int create );
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
int               t_net_sck_getsockname( struct t_net_sck *sck, struct sockaddr_in *adr );


// ----------------------------- CONSTANT VALUES -----------------------------------
// only have common "family names" for commmon
static const char *const t_net_familyName[ ] = { "ip4"   , "ip6"    , "unix" , NULL };
static const int         t_net_familyType[ ] = { AF_INET , AF_INET6 , AF_UNIX };
static const char *const t_net_typeName[ ]   = { "stream"    , "datagram" , "raw" };
static const int         t_net_typeType[ ]   = { SOCK_STREAM , SOCK_DGRAM , SOCK_RAW };

// SO_* defined in /usr/lib/asm-generic/socket.h --> use value out of range
#define T_NET_SO_FAMILY 100

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
#ifdef SO_PROTOCOL
	, SO_PROTOCOL
#endif
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
#ifdef SO_PROTOCOL
	, "protocol"
#endif
	, "reuseaddr"
#ifdef SO_USELOOPBACK
	, "useloopback"
#endif
#ifdef SO_REUSEPORT
	, "reuseport"
#endif
	, NULL
};


static const struct t_typ t_net_familyList[ ] = {
	  { "ip4"      	, AF_INET       }
	, { "ip6"      	, AF_INET6      }
	, { "unix"     	, AF_UNIX       }

#ifdef AF_UNSPEC
	, { "AF_UNSPEC"	, AF_UNSPEC     } //    0 Unspecified.
#endif
#ifdef AF_LOCAL
	, { "AF_LOCAL" 	, AF_LOCAL      }     //  1 Local to host (pipes and file-domain).
#endif
#ifdef AF_UNIX
	, { "AF_UNIX"  	, AF_UNIX       }     //  AF_LOCAL POSIX name for AF_LOCAL.
#endif
#ifdef AF_FILE
	, { "AF_FILE"  	, AF_FILE       }     //  AF_LOCAL Another non-standard name for AF_LOCAL.
#endif
#ifdef AF_INET
	, { "AF_INET"  	, AF_INET       }     //  2 IP protocol family.
#endif
#ifdef AF_AX25
	, { "AF_AX25"  	, AF_AX25       }     //  3 Amateur Radio AX.25.
#endif
#ifdef AF_IPX
	, { "AF_IPX"   	, AF_IPX        }     //  4 Novell Internet Protocol.
#endif
#ifdef AF_APPLETALK
	, { "AF_APPLETALK"   , AF_APPLETALK  }     //  5 Appletalk DDP.
#endif
#ifdef AF_NETROM
	, { "AF_NETROM"	, AF_NETROM     }     //  6 Amateur radio NetROM.
#endif
#ifdef AF_BRIDGE
	, { "AF_BRIDGE"	, AF_BRIDGE     }     //  7 Multiprotocol bridge.
#endif
#ifdef AF_ATMPVC
	, { "AF_ATMPVC"	, AF_ATMPVC     }     //  8 ATM PVCs.
#endif
#ifdef AF_X25
	, { "AF_X25"   	, AF_X25        }     //  9 Reserved for X.25 project.
#endif
#ifdef AF_INET6
	, { "AF_INET6" 	, AF_INET6      }     // 10 IP version 6.
#endif
#ifdef AF_ROSE
	, { "AF_ROSE"  	, AF_ROSE       }     // 11 Amateur Radio X.25 PLP.
#endif
#ifdef AF_DECnet
	, { "AF_DECnet"	, AF_DECnet     }     // 12 Reserved for DECnet project.
#endif
#ifdef AF_NETBEUI
	, { "AF_NETBEUI"     , AF_NETBEUI    }     // 13 Reserved for 802.2LLC project.
#endif
#ifdef AF_SECURITY
	, { "AF_SECURITY"    , AF_SECURITY   }     // 14 Security callback pseudo AF.
#endif
#ifdef AF_KEY
	, { "AF_KEY"   	, AF_KEY        }     // 15 PF_KEY key management API.
#endif
#ifdef AF_NETLINK
	, { "AF_NETLINK"     , AF_NETLINK    }     // 16
#endif
#ifdef AF_ROUTE
	, { "AF_ROUTE" 	, AF_ROUTE      }     // 1F_NETLINK Alias to emulate 4.4BSD.
#endif
#ifdef AF_PACKET
	, { "AF_PACKET"	, AF_PACKET     }     // 17 Packet family.
#endif
#ifdef AF_ASH
	, { "AF_ASH"   	, AF_ASH        }     // 18 Ash.
#endif
#ifdef AF_ECONET
	, { "AF_ECONET"	, AF_ECONET     }     // 19 Acorn Econet.
#endif
#ifdef AF_ATMSVC
	, { "AF_ATMSVC"	, AF_ATMSVC     }     // 20 ATM SVCs.
#endif
#ifdef AF_RDS
	, { "AF_RDS"   	, AF_RDS        }     // 21 RDS sockets.
#endif
#ifdef AF_SNA
	, { "AF_SNA"   	, AF_SNA        }     // 22 Linux SNA Project
#endif
#ifdef AF_IRDA
	, { "AF_IRDA"  	, AF_IRDA       }     // 23 IRDA sockets.
#endif
#ifdef AF_PPPOX
	, { "AF_PPPOX" 	, AF_PPPOX      }     // 24 PPPoX sockets.
#endif
#ifdef AF_WANPIPE
	, { "AF_WANPIPE"     , AF_WANPIPE    }     // 25 Wanpipe API sockets.
#endif
#ifdef AF_LLC
	, { "AF_LLC"   	, AF_LLC        }     // 26 Linux LLC.
#endif
#ifdef AF_IB
	, { "AF_IB"    	, AF_IB         }     // 27 Native InfiniBand address.
#endif
#ifdef AF_MPLS
	, { "AF_MPLS"  	, AF_MPLS       }     // 28 MPLS.
#endif
#ifdef AF_CAN
	, { "AF_CAN"   	, AF_CAN        }     // 29 Controller Area Network.
#endif
#ifdef AF_TIPC
	, { "AF_TIPC"  	, AF_TIPC       }     // 30 TIPC sockets.
#endif
#ifdef AF_BLUETOOTH
	, { "AF_BLUETOOTH"   , AF_BLUETOOTH  }     // 31 Bluetooth sockets.
#endif
#ifdef AF_IUCV
	, { "AF_IUCV"  	, AF_IUCV       }     // 32 IUCV sockets.
#endif
#ifdef AF_RXRPC
	, { "AF_RXRPC" 	, AF_RXRPC      }     // 33 RxRPC sockets.
#endif
#ifdef AF_ISDN
	, { "AF_ISDN"  	, AF_ISDN       }     // 34 mISDN sockets.
#endif
#ifdef AF_PHONET
	, { "AF_PHONET"	, AF_PHONET     }     // 35 Phonet sockets.
#endif
#ifdef AF_IEEE802154
	, { "AF_IEEE802154"  , AF_IEEE802154 }     // 36 IEEE 802.15.4 sockets.
#endif
#ifdef AF_CAIF
	, { "AF_CAIF"  	, AF_CAIF       }     // 37 CAIF sockets.
#endif
#ifdef AF_ALG
	, { "AF_ALG"   	, AF_ALG        }     // 38 Algorithm sockets.
#endif
#ifdef AF_NFC
	, { "AF_NFC"   	, AF_NFC        }     // 39 NFC sockets.
#endif
#ifdef AF_VSOCK
	, { "AF_VSOCK" 	, AF_VSOCK      }     // 40 vSockets.
#endif
#ifdef AF_KCM
	, { "AF_KCM"   	, AF_KCM        }     // 41 Kernel Connection Multiplexor.
#endif
#ifdef AF_QIPCRTR
	, { "AF_QIPCRTR"     , AF_QIPCRTR    }     // 42 Qualcomm IPC Router.
#endif
#ifdef AF_MAX
	, { "AF_MAX"   	, AF_MAX        }     // 43 For now..
	, { NULL       	, 0          }     // Sentinel
#endif
};


static const struct t_typ t_net_typeList[ ] = {
	  { "stream"   	, SOCK_STREAM    }
	, { "datagram" 	, SOCK_DGRAM     }
	, { "raw"      	, SOCK_RAW       }

#ifdef SOCK_STREAM
	, { "SOCK_STREAM"    , SOCK_STREAM    }  // 1         Sequenced, reliable, connection-based byte streams.
#endif
#ifdef SOCK_DGRAM
	, { "SOCK_DGRAM"     , SOCK_DGRAM     }  // 2         Connectionless, unreliable datagrams of fixed maximum length.
#endif
#ifdef SOCK_RAW
	, { "SOCK_RAW" 	, SOCK_RAW       }  // 3         Raw protocol interface.
#endif
#ifdef SOCK_RDM
	, { "SOCK_RDM" 	, SOCK_RDM       }  // 4         Reliably-delivered messages
#endif
#ifdef SOCK_SEQPACKET
	, { "SOCK_SEQPACKET" , SOCK_SEQPACKET }  // 5         Sequenced, reliable, connection-based, datagrams of fixed maximum length.
#endif
#ifdef SOCK_DCCP
	, { "SOCK_DCCP"	, SOCK_DCCP      }  // 6         Datagram Congestion Control Protocol.
#endif
#ifdef SOCK_PACKET
	, { "SOCK_PACKET"    , SOCK_PACKET    }  // 10        Linux specific way of getting packets at the dev level.  For writing rarp and other similar things on the user level.
#endif

 // Flags to be ORed into the type parameter of socket and socketpair and
 // used for the flags parameter of paccept.

#ifdef SOCK_CLOEXEC
	, { "SOCK_CLOEXEC"   , SOCK_CLOEXEC   }  // 02000000  Atomically set close-on-exec flag for the new descriptor(s).
#endif
#ifdef SOCK_NONBLOCK
	, { "SOCK_NONBLOCK"  , SOCK_NONBLOCK  }  // 02000000  Atomically mark descriptor(s) as non-blocking.
#endif
	, { NULL       	, 0          }     // Sentinel
};


static const struct t_typ t_net_protocolList[ ] = {
#ifdef IPPROTO_IP
	  { "IPPROTO_IP"        , IPPROTO_IP       }  //0,	   /* Dummy protocol for TCP.  */
#endif
#ifdef IPPROTO_ICMP
	, { "IPPROTO_ICMP"      , IPPROTO_ICMP     }  //1,	   /* Internet Control Message Protocol.  */
#endif
#ifdef IPPROTO_IGMP
	, { "IPPROTO_IGMP"      , IPPROTO_IGMP     }  //2,	   /* Internet Group Management Protocol. */
#endif
#ifdef IPPROTO_IPIP
	, { "IPPROTO_IPIP"      , IPPROTO_IPIP     }  //4,	   /* IPIP tunnels (older KA9Q tunnels use 94).  */
#endif
#ifdef IPPROTO_TCP
	, { "IPPROTO_TCP"       , IPPROTO_TCP      }  //6,	   /* Transmission Control Protocol.  */
#endif
#ifdef IPPROTO_EGP
	, { "IPPROTO_EGP"       , IPPROTO_EGP      }  //8,	   /* Exterior Gateway Protocol.  */
#endif
#ifdef IPPROTO_PUP
	, { "IPPROTO_PUP"       , IPPROTO_PUP      }  //12,	   /* PUP protocol.  */
#endif
#ifdef IPPROTO_UDP
	, { "IPPROTO_UDP"       , IPPROTO_UDP      }  //17,	   /* User Datagram Protocol.  */
#endif
#ifdef IPPROTO_IDP
	, { "IPPROTO_IDP"       , IPPROTO_IDP      }  //22,	   /* XNS IDP protocol.  */
#endif
#ifdef IPPROTO_TP
	, { "IPPROTO_TP"        , IPPROTO_TP       }  //29,	   /* SO Transport Protocol Class 4.  */
#endif
#ifdef IPPROTO_DCCP
	, { "IPPROTO_DCCP"      , IPPROTO_DCCP     }  //33,	   /* Datagram Congestion Control Protocol.  */
#endif
#ifdef IPPROTO_IPV6
	, { "IPPROTO_IPV6"      , IPPROTO_IPV6     }  //41,     /* IPv6 header.  */
#endif
#ifdef IPPROTO_RSVP
	, { "IPPROTO_RSVP"      , IPPROTO_RSVP     }  //46,	   /* Reservation Protocol.  */
#endif
#ifdef IPPROTO_GRE
	, { "IPPROTO_GRE"       , IPPROTO_GRE      }  //47,	   /* General Routing Encapsulation.  */
#endif
#ifdef IPPROTO_ESP
	, { "IPPROTO_ESP"       , IPPROTO_ESP      }  //50,      /* encapsulating security payload.  */
#endif
#ifdef IPPROTO_AH
	, { "IPPROTO_AH"        , IPPROTO_AH       }  //51,       /* authentication header.  */
#endif
#ifdef IPPROTO_MTP
	, { "IPPROTO_MTP"       , IPPROTO_MTP      }  //92,	   /* Multicast Transport Protocol.  */
#endif
#ifdef IPPROTO_BEETPH
	, { "IPPROTO_BEETPH"    , IPPROTO_BEETPH   }  //94,   /* IP option pseudo header for BEET.  */
#endif
#ifdef IPPROTO_ENCAP
	, { "IPPROTO_ENCAP"     , IPPROTO_ENCAP    }  //98,	   /* Encapsulation Header.  */
#endif
#ifdef IPPROTO_PIM
	, { "IPPROTO_PIM"       , IPPROTO_PIM      }  //103,	   /* Protocol Independent Multicast.  */
#endif
#ifdef IPPROTO_COMP
	, { "IPPROTO_COMP"      , IPPROTO_COMP     }  //108,	   /* Compression Header Protocol.  */
#endif
#ifdef IPPROTO_SCTP
	, { "IPPROTO_SCTP"      , IPPROTO_SCTP     }  //132,	   /* Stream Control Transmission Protocol.  */
#endif
#ifdef IPPROTO_UDPLITE
	, { "IPPROTO_UDPLITE"   , IPPROTO_UDPLITE  }  //136, /* UDP-Lite protocol.  */
#endif
#ifdef IPPROTO_MPLS
	, { "IPPROTO_MPLS"      , IPPROTO_MPLS     }  //137,    /* MPLS in IP.  */
#endif
#ifdef IPPROTO_RAW
	, { "IPPROTO_RAW"       , IPPROTO_RAW      }  //255,	   /* Raw IP packets.  */
#endif
#ifdef IPPROTO_MAX
	, { "IPPROTO_MAX"       , IPPROTO_MAX      }
#endif
	, { NULL                , 0                }

};

