/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      p_net_ifs_unx.c
 * \brief     Linux specific implementation to get interface data.  This is a
 *            wrapper around getifaddrs() returning a table of information.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

// this header order makes __USE_MISC visible and hence all the POSIX stuff
#define _DEFAULT_SOURCE 1
#include "t_net_l.h"

#if defined(DEBUG)
#include "t_dbg.h"
#endif

#include <string.h>   // strcmp,memcpy

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <net/if.h>

#if defined(LUAT_USE_LINUX)
#include <linux/if_link.h>
#include <linux/if_packet.h>
#else
#include <net/if_dl.h>
#endif

/**--------------------------------------------------------------------------
 * Parse flags on interface.
 * \param   L        Lua state.
 * \param   flags    flags according to SIOCGIFFLAGS.
 * \lreturn table    Table representing with all flags.
 * --------------------------------------------------------------------------*/
static void
p_net_ifc_parseFlags( lua_State *L, struct ifaddrs *ifa )
{
	lua_pushstring( L, "flags" );
	lua_rawget( L, -2 );
	if (lua_isnil( L, -1 )) // only do it if it was'n done before
	{
		lua_pop( L, 1 );        // pop nil
		//if (ifa->ifa_addr && ifa->ifa_flags && ( AF_INET == ifa->ifa_addr->sa_family ||
		//	 AF_INET6 == ifa->ifa_addr->sa_family || AF_PACKET == ifa->ifa_addr->sa_family ))
		if (ifa->ifa_flags)
		{
			lua_createtable( L, 17, 0 );
#define IF_FLAG( FLG )                                   \
   lua_pushboolean( L, (ifa->ifa_flags & FLG) ? 1 : 0 ); \
   lua_setfield( L, -2, #FLG "" );

#ifdef  IFF_UP
			IF_FLAG( IFF_UP            );
#endif
#ifdef  IFF_BROADCAST
			IF_FLAG( IFF_BROADCAST     );
#endif
#ifdef  IFF_DEBUG
			IF_FLAG( IFF_DEBUG         );
#endif
#ifdef  IFF_LOOPBACK
			IF_FLAG( IFF_LOOPBACK      );
#endif
#ifdef  IFF_POINTOPOINT
			IF_FLAG( IFF_POINTOPOINT   );
#endif
#ifdef  IFF_RUNNING
			IF_FLAG( IFF_RUNNING       );
#endif
#ifdef  IFF_NOARP
			IF_FLAG( IFF_NOARP         );
#endif
#ifdef  IFF_PROMISC
			IF_FLAG( IFF_PROMISC       );
#endif
#ifdef  IFF_ALLMULTI
			IF_FLAG( IFF_ALLMULTI      );
#endif
#ifdef  IFF_MULTICAST
			IF_FLAG( IFF_MULTICAST     );
#endif
#ifdef  IFF_NOTRAILERS
			IF_FLAG( IFF_NOTRAILERS    );
#endif
#ifdef  IFF_MASTER
			IF_FLAG( IFF_MASTER        );
#endif
#ifdef  IFF_SLAVE
			IF_FLAG( IFF_SLAVE         );
#endif
#ifdef  IFF_PORTSEL
			IF_FLAG( IFF_PORTSEL       );
#endif
#ifdef  IFF_AUTOMEDIA
			IF_FLAG( IFF_AUTOMEDIA     );
#endif
#ifdef  IFF_DYNAMIC
			IF_FLAG( IFF_DYNAMIC       );
#endif
#ifdef  IFF_KNOWSEPOCH
			IF_FLAG( IFF_KNOWSEPOCH    );
#endif
#ifdef  IFF_DRV_RUNNING
			IF_FLAG( IFF_DRV_RUNNING   );
#endif
#ifdef  IFF_DRV_OACTIVE
			IF_FLAG( IFF_DRV_OACTIVE   );
#endif
#ifdef  IFF_SIMPLEX
			IF_FLAG( IFF_SIMPLEX       );
#endif
#ifdef  IFF_LINK0
			IF_FLAG( IFF_LINK0         );
#endif
#ifdef  IFF_LINK1
			IF_FLAG( IFF_LINK1         );
#endif
#ifdef  IFF_LINK2
			IF_FLAG( IFF_LINK2         );
#endif
#ifdef  IFF_ALTPHYS
			IF_FLAG( IFF_ALTPHYS       );
#endif
#ifdef  IFF_CANTCONFIG
			IF_FLAG( IFF_CANTCONFIG    );
#endif
#ifdef  IFF_PPROMISC
			IF_FLAG( IFF_PPROMISC      );
#endif
#ifdef  IFF_MONITOR
			IF_FLAG( IFF_MONITOR       );
#endif
#ifdef  IFF_STATICARP
			IF_FLAG( IFF_STATICARP     );
#endif
#ifdef  IFF_DYING
			IF_FLAG( IFF_DYING         );
#endif
#ifdef  IFF_RENAMING
			IF_FLAG( IFF_RENAMING      );
#endif
#ifdef  IFF_NOGROUP
			IF_FLAG( IFF_NOGROUP       );
#endif

			lua_setfield( L, -2, "flags" );
#undef IF_FLAG
		}
	}
	else
		lua_pop(L, 1 );
}


/**--------------------------------------------------------------------------
 * Extract statistic data from interface if available
 * \param   L        Lua state.
 * \param   ifa      struct ifaddrs instance.
 * \lreturn table    aggregated statistic data.
 * \return  int/bool 1 if succesful, else 0.
 * --------------------------------------------------------------------------*/
#if defined(LUAT_USE_LINUX)
static void
p_net_ifs_getStats( lua_State *L, struct ifaddrs *ifa )
{
	struct rtnl_link_stats *stats;
	struct sockaddr_ll     *ll_addr;
	// TODO: This can hold a MAC Address but not a FireWire address -> needs
	// hardware to do more dev work.  Consider using sockaddr_ll->sll_halen
	// properly.
	char                    hw_buffer[ 19 ]; // 18+1 for NUL termination

	if (ifa->ifa_addr && ifa->ifa_data && AF_PACKET == ifa->ifa_addr->sa_family)
	{
		stats    = ifa->ifa_data;
#define IF_STAT( FLD )                     \
   lua_pushinteger( L, stats->FLD );       \
   lua_setfield( L, -2, #FLD "" );

		lua_createtable( L, 21, 0 );

		IF_STAT( rx_packets );             // total packets received
		IF_STAT( tx_packets );             // total packets transmitted
		IF_STAT( rx_bytes );               // total bytes received
		IF_STAT( tx_bytes );               // total bytes transmitted
		IF_STAT( rx_errors );              // bad packets received
		IF_STAT( tx_errors );              // packet transmit problems
		IF_STAT( rx_dropped );             // no space in linux buffers
		IF_STAT( tx_dropped );             // no space available in linux
		IF_STAT( multicast );              // multicast packets received
		IF_STAT( collisions );
		// detailed rx_errors:
		IF_STAT( rx_length_errors );
		IF_STAT( rx_over_errors );         // receiver ring buff overflow
		IF_STAT( rx_crc_errors );          // recved pkt with crc error
		IF_STAT( rx_frame_errors );        // recv'd frame alignment error
		IF_STAT( rx_fifo_errors );         // recv'r fifo overrun
		IF_STAT( rx_missed_errors );       // receiver missed packet
		// detailed tx_errors
		IF_STAT( tx_aborted_errors );
		IF_STAT( tx_carrier_errors );
		IF_STAT( tx_fifo_errors );
		IF_STAT( tx_heartbeat_errors );
		IF_STAT( tx_window_errors );
		lua_setfield( L, -2, "stats" );
#undef IF_STAT
		ll_addr = (struct sockaddr_ll*) ifa->ifa_addr;
		sprintf( hw_buffer, " %02x:%02x:%02x:%02x:%02x:%02x",
		    ll_addr->sll_addr[0],
		    ll_addr->sll_addr[1],
		    ll_addr->sll_addr[2],
		    ll_addr->sll_addr[3],
		    ll_addr->sll_addr[4],
		    ll_addr->sll_addr[5] );
		lua_pushlstring( L, hw_buffer, 18 );
		lua_setfield( L, -2, "hw_address" );
		lua_pushinteger( L, ll_addr->sll_ifindex);
		lua_setfield( L, -2, "index" );
	}
}
#else
// BSD compatible implementation
static void
p_net_ifs_getStats( lua_State *L, struct ifaddrs *ifa )
{
	struct if_data         *stats;
	struct sockaddr_dl     *ll_addr;
	// TODO: This can hold a MAC Address but not a FireWire address -> needs
	// hardware to do more dev work.  Consider using sockaddr_ll->sll_halen
	// properly.
	char                    hw_buffer[ 19 ]; // 18+1 for NUL termination
	unsigned char          *mac;

	if (ifa->ifa_addr && ifa->ifa_data && AF_LINK == ifa->ifa_addr->sa_family)
	{
		stats    = ifa->ifa_data;
#define IF_STAT( FLD )                     \
   lua_pushinteger( L, stats->FLD );       \
   lua_setfield( L, -2, #FLD "" );

		lua_createtable( L, 12, 0 );

		IF_STAT( ifi_ipackets );           // total packets received
		IF_STAT( ifi_opackets );           // total packets transmitted
		IF_STAT( ifi_ibytes );             // total bytes received
		IF_STAT( ifi_obytes );             // total bytes transmitted
		IF_STAT( ifi_ierrors );            // bad packets received
		IF_STAT( ifi_oerrors );            // packet transmit problems
		IF_STAT( ifi_iqdrops );            // no space in kernel buffers
		IF_STAT( ifi_oqdrops );            // no space in kernel buffers
		IF_STAT( ifi_imcasts );            // multicast packets received
		IF_STAT( ifi_omcasts );            // multicast packets transmitted
		IF_STAT( ifi_collisions );
		IF_STAT( ifi_noproto );            // number of package with unknown protocol
		lua_setfield( L, -2, "stats" );
#undef IF_STAT
		mac     = (unsigned char *) LLADDR ((struct sockaddr_dl *)(ifa)->ifa_addr);
		ll_addr = (struct sockaddr_dl*) ifa->ifa_addr;
		sprintf( hw_buffer, " %02x:%02x:%02x:%02x:%02x:%02x",
		    mac[0],
		    mac[1],
		    mac[2],
		    mac[3],
		    mac[4],
		    mac[5] );
		lua_pushlstring( L, hw_buffer, 18 );
		lua_setfield( L, -2, "hw_address" );
		lua_pushinteger( L, ll_addr->sdl_index);
		lua_setfield( L, -2, "index" );
	}
}
#endif


/**--------------------------------------------------------------------------
 * Parse interface address into a t.Net.Address (sockaddr_storage).
 * take a sockaddr and turn it into a Lua controlled sockaddr_storage
 * this way it can be returned as a t.Net.Address instance
 * \param   L        Lua state.
 * \param   addr     struct sockaddr instance.
 * \param   adr_name Name/Function of address.
 * \lreturn void     Push value to table
 * \return  void
 * --------------------------------------------------------------------------*/
static inline void
p_net_ifs_makeAddress( lua_State *L, struct sockaddr* iadr, const char * adr_name )
{
	struct sockaddr_storage   *adr;

	if (iadr)
	{
		adr = t_net_adr_create_ud( L );
		memcpy( adr, iadr, (AF_INET6 == iadr->sa_family)
				? sizeof( struct sockaddr_in6 )
				: sizeof( struct sockaddr_in ) );
		lua_setfield( L, -2, adr_name );
	}
}


/**--------------------------------------------------------------------------
 * Extract addresses of a family type.
 * \param   L        Lua state.
 * \param   ifa      struct ifaddrs instance.
 * \lreturn table    table of t.Net.Address userdata instances.
 * --------------------------------------------------------------------------*/
static void
p_net_ifs_getAddresses( lua_State *L, struct ifaddrs *ifa )
{
	int created = 0;
	if (ifa->ifa_addr && (AF_INET == ifa->ifa_addr->sa_family || AF_INET6 == ifa->ifa_addr->sa_family))
	{
		lua_pushinteger( L, ifa->ifa_addr->sa_family );
		t_getLoadedValue( L, 2, -1,  "t."T_NET_IDNT, T_NET_FML_IDNT );
		lua_rawget( L, -2 );
		if (lua_isnil( L, -1 ))
		{
			lua_pop( L, 1 );              // pop nil
			lua_pushinteger( L, ifa->ifa_addr->sa_family );
			t_getLoadedValue( L, 2, -1,  "t."T_NET_IDNT, T_NET_FML_IDNT );
			lua_newtable( L );            // create table that lists ALL addresses for family
			created = 1;
		}
		lua_newtable( L );               // create table that lists ALL addresses for this interface address
		p_net_ifs_makeAddress( L, ifa->ifa_addr     , "address" );
		p_net_ifs_makeAddress( L, ifa->ifa_netmask  , "netmask" );
		if (ifa->ifa_flags & IFF_BROADCAST)
			p_net_ifs_makeAddress( L, ifa->ifa_broadaddr, "broadcast" );
		else
			p_net_ifs_makeAddress( L, ifa->ifa_dstaddr  , "peer" );
		lua_rawseti( L, -2, lua_rawlen( L, -2 ) + 1 );
		if (created)
			lua_rawset( L, -3 );
		else
			lua_pop( L, 1 );
	}
}


/**--------------------------------------------------------------------------
 * Create an t.Net.Interface Lua table and push to LuaStack.
 * \param   L      Lua state.
 * \lreturn table  t.Net.Interface instance or Lua table with all available
 *                 adapters instances.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
p_net_ifc_get( lua_State *L, const char *if_name )
{
	struct ifaddrs   *all_ifas, *ifa;

	if (0 != getifaddrs( &all_ifas ))
		return t_push_error( L, 0, 0, "couldn't retrieve interface information" );

	if (NULL == if_name)
		lua_newtable( L );            // create table that lists ALL interfaces
	else
		lua_pushnil( L );             // "fake" empty lua_getfield() result

	for (ifa = all_ifas; ifa; ifa = ifa->ifa_next)
	{
		//printf("N: %s(%d)\n", ifa->ifa_name, (ifa->ifa_addr) ? ifa->ifa_addr->sa_family : -1 );
		if (NULL != if_name && 0 != strcmp( ifa->ifa_name, if_name ))
			continue;
		if (NULL == if_name)
			lua_getfield( L, -1,  ifa->ifa_name );
		if (lua_isnil( L, -1 ))
		{
			lua_pop( L, 1 );        // pop nil
			lua_newtable( L );      // create table for specific named interface
			lua_pushstring( L, ifa->ifa_name );
			lua_setfield( L, -2, "name" );
			luaL_getmetatable( L, T_NET_IFC_TYPE );
			lua_setmetatable( L , -2 );
		}
		if (NULL == if_name)
		{
			lua_pushvalue( L, -1 ); // Repush table
			lua_setfield( L, -3, ifa->ifa_name );
		}
		p_net_ifc_parseFlags( L, ifa );      // only executes once per ifa_name
		p_net_ifs_getAddresses( L, ifa );    // only executes for sa_family = AF_INET || AF_INET6
		p_net_ifs_getStats( L, ifa );        // only executes for sa_family = AF_PACKET

		if (NULL == if_name)
			lua_pop( L, 1 );
	}

	freeifaddrs( all_ifas );
	return 1;
}
