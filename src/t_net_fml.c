// vim: ts=3 sw=3 sts=3 tw=80 sta noet list
/*
 * \file      t_net_fml.c
 * \brief     OOP wrapper for network family definition
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_net_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif


/* --------------------------------------------------------------------------
 * Class library function definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_fml_cf [] =
{
	  { NULL,    NULL }
};


/* --------------------------------------------------------------------------
 * Pushes the Socket Protocols library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L     The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
luaopen_t_net_fml( lua_State *L )
{
	luaL_newlib( L, t_net_fml_cf );
// reverse setting of fml.AF_INET=2 AND fml[2]='AF_INET' allows basic reverse
// lookup. Needed by t.Net.Interface without running `require't.Net.Family'`
#define DF_AF( fml )                \
   lua_pushinteger( L, fml );       \
   lua_setfield( L, -2, #fml "" );  \
   lua_pushstring( L, #fml "" );    \
   lua_rawseti( L, -2, fml );

	// populating the family table based on features that actually exist at compile time
#ifdef AF_UNSPEC
	DF_AF( AF_UNSPEC    )  //  0 Unspecified.
#endif
#ifdef AF_LOCAL
	DF_AF( AF_LOCAL     )  //  1 Local to host (pipes and file-domain).
#endif
#ifdef AF_UNIX
	DF_AF( AF_UNIX      )  //  AF_LOCAL POSIX name for AF_LOCAL.
#endif
#ifdef AF_FILE
	DF_AF( AF_FILE      )  //  AF_LOCAL Another non-standard name for AF_LOCAL.
#endif
#ifdef AF_INET
	DF_AF( AF_INET      )  //  2 IP protocol family.
#endif
#ifdef AF_AX25
	DF_AF( AF_AX25      )  //  3 Amateur Radio AX.25.
#endif
#ifdef AF_IPX
	DF_AF( AF_IPX       )  //  4 Novell Internet Protocol.
#endif
#ifdef AF_APPLETALK
	DF_AF( AF_APPLETALK )  //  5 Appletalk DDP.
#endif
#ifdef AF_NETROM
	DF_AF( AF_NETROM    )  //  6 Amateur radio NetROM.
#endif
#ifdef AF_BRIDGE
	DF_AF( AF_BRIDGE    )  //  7 Multiprotocol bridge.
#endif
#ifdef AF_ATMPVC
	DF_AF( AF_ATMPVC    )  //  8 ATM PVCs.
#endif
#ifdef AF_X25
	DF_AF( AF_X25       )  //  9 Reserved for X.25 project.
#endif
#ifdef AF_INET6
	DF_AF( AF_INET6     )  // 10 IP version 6.
#endif
#ifdef AF_ROSE
	DF_AF( AF_ROSE      )  // 11 Amateur Radio X.25 PLP.
#endif
#ifdef AF_DECnet
	DF_AF( AF_DECnet    )  // 12 Reserved for DECnet project.
#endif
#ifdef AF_NETBEUI
	DF_AF( AF_NETBEUI   )  // 13 Reserved for 802.2LLC project.
#endif
#ifdef AF_SECURITY
	DF_AF( AF_SECURITY  )  // 14 Security callback pseudo AF.
#endif
#ifdef AF_KEY
	DF_AF( AF_KEY       )  // 15 PF_KEY key management API.
#endif
#ifdef AF_NETLINK
	DF_AF( AF_NETLINK   )  // 16
#endif
#ifdef AF_ROUTE
	DF_AF( AF_ROUTE     )  // 1F_NETLINK Alias to emulate 4.4BSD.
#endif
#ifdef AF_PACKET
	DF_AF( AF_PACKET    )  // 17 Packet family.
#endif
#ifdef AF_ASH
	DF_AF( AF_ASH       )  // 18 Ash.
#endif
#ifdef AF_ECONET
	DF_AF( AF_ECONET    )  // 19 Acorn Econet.
#endif
#ifdef AF_ATMSVC
	DF_AF( AF_ATMSVC    )  // 20 ATM SVCs.
#endif
#ifdef AF_RDS
	DF_AF( AF_RDS       )  // 21 RDS sockets.
#endif
#ifdef AF_SNA
	DF_AF( AF_SNA       )  // 22 Linux SNA Project
#endif
#ifdef AF_IRDA
	DF_AF( AF_IRDA      )  // 23 IRDA sockets.
#endif
#ifdef AF_PPPOX
	DF_AF( AF_PPPOX     )  // 24 PPPoX sockets.
#endif
#ifdef AF_WANPIPE
	DF_AF( AF_WANPIPE   )  // 25 Wanpipe API sockets.
#endif
#ifdef AF_LLC
	DF_AF( AF_LLC       )  // 26 Linux LLC.
#endif
#ifdef AF_IB
	DF_AF( AF_IB        )  // 27 Native InfiniBand address.
#endif
#ifdef AF_MPLS
	DF_AF( AF_MPLS      )  // 28 MPLS.
#endif
#ifdef AF_CAN
	DF_AF( AF_CAN       )  // 29 Controller Area Network.
#endif
#ifdef AF_TIPC
	DF_AF( AF_TIPC      )  // 30 TIPC sockets.
#endif
#ifdef AF_BLUETOOTH
	DF_AF( AF_BLUETOOTH )  // 31 Bluetooth sockets.
#endif
#ifdef AF_IUCV
	DF_AF( AF_IUCV      )  // 32 IUCV sockets.
#endif
#ifdef AF_RXRPC
	DF_AF( AF_RXRPC     )  // 33 RxRPC sockets.
#endif
#ifdef AF_ISDN
	DF_AF( AF_ISDN      )  // 34 mISDN sockets.
#endif
#ifdef AF_PHONET
	DF_AF( AF_PHONET    )  // 35 Phonet sockets.
#endif
#ifdef AF_IEEE802154
	DF_AF( AF_IEEE802154)  // 36 IEEE 802.15.4 sockets.
#endif
#ifdef AF_CAIF
	DF_AF( AF_CAIF      )  // 37 CAIF sockets.
#endif
#ifdef AF_ALG
	DF_AF( AF_ALG       )  // 38 Algorithm sockets.
#endif
#ifdef AF_NFC
	DF_AF( AF_NFC       )  // 39 NFC sockets.
#endif
#ifdef AF_VSOCK
	DF_AF( AF_VSOCK     )  // 40 vSockets.
#endif
#ifdef AF_KCM
	DF_AF( AF_KCM       )  // 41 Kernel Connection Multiplexor.
#endif
#ifdef AF_QIPCRTR
	DF_AF( AF_QIPCRTR   )  // 42 Qualcomm IPC Router.
#endif
#ifdef AF_SMC
	DF_AF( AF_SMC       )  // 43 SMC Sockets.
#endif
#ifdef AF_XDP
	DF_AF( AF_XDP       )  // 44 XDP Sockets.
#endif
#ifdef AF_MAX
	DF_AF( AF_MAX       )  // 45 For now..
#endif

#undef DF_AF
	return 1;
}

/*
 * Berkely Socket Specs lifted of Linux header file

#define PF_UNSPEC	0	// Unspecified.
#define PF_LOCAL	1	// Local to host (pipes and file-domain).
#define PF_UNIX		PF_LOCAL // POSIX name for PF_LOCAL.
#define PF_FILE		PF_LOCAL // Another non-standard name for PF_LOCAL.
#define PF_INET		2	// IP protocol family.
#define PF_AX25		3	// Amateur Radio AX.25.
#define PF_IPX		4	// Novell Internet Protocol.
#define PF_APPLETALK	5	// Appletalk DDP.
#define PF_NETROM	6	// Amateur radio NetROM.
#define PF_BRIDGE	7	// Multiprotocol bridge.
#define PF_ATMPVC	8	// ATM PVCs.
#define PF_X25		9	// Reserved for X.25 project.
#define PF_INET6	10	// IP version 6.
#define PF_ROSE		11	// Amateur Radio X.25 PLP.
#define PF_DECnet	12	// Reserved for DECnet project.
#define PF_NETBEUI	13	// Reserved for 802.2LLC project.
#define PF_SECURITY	14	// Security callback pseudo AF.
#define PF_KEY		15	// PF_KEY key management API.
#define PF_NETLINK	16
#define PF_ROUTE	PF_NETLINK // Alias to emulate 4.4BSD.
#define PF_PACKET	17	// Packet family.
#define PF_ASH		18	// Ash.
#define PF_ECONET	19	// Acorn Econet.
#define PF_ATMSVC	20	// ATM SVCs.
#define PF_RDS		21	// RDS sockets.
#define PF_SNA		22	// Linux SNA Project
#define PF_IRDA		23	// IRDA sockets.
#define PF_PPPOX	24	// PPPoX sockets.
#define PF_WANPIPE	25	// Wanpipe API sockets.
#define PF_LLC		26	// Linux LLC.
#define PF_IB		27	// Native InfiniBand address.
#define PF_MPLS		28	// MPLS.
#define PF_CAN		29	// Controller Area Network.
#define PF_TIPC		30	// TIPC sockets.
#define PF_BLUETOOTH	31	// Bluetooth sockets.
#define PF_IUCV		32	// IUCV sockets.
#define PF_RXRPC	33	// RxRPC sockets.
#define PF_ISDN		34	// mISDN sockets.
#define PF_PHONET	35	// Phonet sockets.
#define PF_IEEE802154	36	// IEEE 802.15.4 sockets.
#define PF_CAIF		37	// CAIF sockets.
#define PF_ALG		38	// Algorithm sockets.
#define PF_NFC		39	// NFC sockets.
#define PF_VSOCK	40	// vSockets.
#define PF_KCM		41	// Kernel Connection Multiplexor.
#define PF_QIPCRTR	42	// Qualcomm IPC Router.
#define PF_SMC		43	// SMC sockets.
#define PF_MAX		44	// For now..
*/
