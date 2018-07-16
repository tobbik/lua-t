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
#include "t_net_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <net/if.h>
#include <linux/if_link.h>


/**--------------------------------------------------------------------------
 * Parse flags on interface.
 * \param   L        Lua state.
 * \param   flags    flags according to SIOCGIFFLAGS.
 * \lreturn table    Table representing with all flags.
 * --------------------------------------------------------------------------*/
static void
p_net_ifc_parseFlags( lua_State *L, unsigned int flags )
{
	lua_newtable( L );
#define IF_FLAG( b ) \
	if (flags & b) \
		lua_pushboolean( L, 1 ); \
	else \
		lua_pushboolean( L, 0 ); \
	lua_setfield( L, -2, #b "" );
	IF_FLAG( IFF_UP );
	IF_FLAG( IFF_BROADCAST );
	IF_FLAG( IFF_DEBUG );
	IF_FLAG( IFF_LOOPBACK );
	IF_FLAG( IFF_POINTOPOINT );
	IF_FLAG( IFF_RUNNING );
	IF_FLAG( IFF_NOARP );
	IF_FLAG( IFF_PROMISC );
	IF_FLAG( IFF_NOTRAILERS );
	IF_FLAG( IFF_ALLMULTI );
	IF_FLAG( IFF_MASTER );
	IF_FLAG( IFF_SLAVE );
	IF_FLAG( IFF_MULTICAST );
	IF_FLAG( IFF_PORTSEL );
	IF_FLAG( IFF_AUTOMEDIA );
	IF_FLAG( IFF_DYNAMIC );
#undef IF_FLAG
	lua_setfield( L, -2, "flags" );
}


/**--------------------------------------------------------------------------
 * Extract statistic data from interface if available
 * \param   L        Lua state.
 * \param   ifa      struct ifaddrs instance.
 * \lreturn table    aggregated statistic data.
 * \return  int/bool 1 if succesful, else 0.
 * --------------------------------------------------------------------------*/
static void
p_net_ifs_getAddrStats( lua_State *L, struct ifaddrs *ifa )
{
	struct rtnl_link_stats *stats = ifa->ifa_data;

	if (AF_PACKET != ifa->ifa_addr->sa_family && NULL == ifa->ifa_data)
		return;
	// TODO: find a common ground with windows if possible/necessary
	lua_createtable( L, 21, 0 );
	// literally translate to Lua table
	lua_pushinteger( L, stats->rx_packets );             // total packets received
	lua_setfield( L, -2, "rx_packets" );
	lua_pushinteger( L, stats->tx_packets );             // total packets transmitted
	lua_setfield( L, -2, "tx_packets" );
	lua_pushinteger( L, stats->rx_bytes );               // total bytes received
	lua_setfield( L, -2, "rx_bytes" );
	lua_pushinteger( L, stats->tx_bytes );               // total bytes transmitted
	lua_setfield( L, -2, "tx_bytes" );
	lua_pushinteger( L, stats->rx_errors );              // bad packets received
	lua_setfield( L, -2, "rx_errors" );
	lua_pushinteger( L, stats->tx_errors );              // packet transmit problems
	lua_setfield( L, -2, "tx_errors" );
	lua_pushinteger( L, stats->rx_dropped );             // no space in linux buffers
	lua_setfield( L, -2, "rx_dropped" );
	lua_pushinteger( L, stats->tx_dropped );             // no space available in linux
	lua_setfield( L, -2, "tx_dropped" );
	lua_pushinteger( L, stats->multicast );              // multicast packets received
	lua_setfield( L, -2, "multicast" );
	lua_pushinteger( L, stats->collisions );
	lua_setfield( L, -2, "collisions" );
	// detailed rx_errors:
	lua_pushinteger( L, stats->rx_length_errors );
	lua_setfield( L, -2, "rx_length_errors" );
	lua_pushinteger( L, stats->rx_over_errors );         // receiver ring buff overflow
	lua_setfield( L, -2, "rx_over_errors" );
	lua_pushinteger( L, stats->rx_crc_errors );          // recved pkt with crc error
	lua_setfield( L, -2, "rx_crc_errors" );
	lua_pushinteger( L, stats->rx_frame_errors );        // recv'd frame alignment error
	lua_setfield( L, -2, "rx_frame_errors" );
	lua_pushinteger( L, stats->rx_fifo_errors );         // recv'r fifo overrun
	lua_setfield( L, -2, "rx_fifo_errors" );
	lua_pushinteger( L, stats->rx_missed_errors );       // receiver missed packet
	lua_setfield( L, -2, "rx_missed_errors" );
	// detailed tx_errors
	lua_pushinteger( L, stats->tx_aborted_errors );
	lua_setfield( L, -2, "tx_aborted_errors" );
	lua_pushinteger( L, stats->tx_carrier_errors );
	lua_setfield( L, -2, "tx_carrier_errors" );
	lua_pushinteger( L, stats->tx_fifo_errors );
	lua_setfield( L, -2, "tx_fifo_errors" );
	lua_pushinteger( L, stats->tx_heartbeat_errors );
	lua_setfield( L, -2, "tx_heartbeat_errors" );
	lua_pushinteger( L, stats->tx_window_errors );
	lua_setfield( L, -2, "tx_window_errors" );
	lua_setfield( L, -2, "stats" );
}


/**--------------------------------------------------------------------------
 * Parse interface address into a t.Net.Address (sockaddr_storage).
 * take a sockaddr and turn it into a Lua controlled sockaddr_storage
 * this way it can get returned as a t.Net.Address instance
 * \param   L        Lua state.
 * \param   addr     struct sockaddr instance.
 * \param   found    int*, if ANY address was found set to 1.
 * \lreturn ud       t.Net.Address userdata instance.
 * \return  int/bool 1 if succesful, else 0.
 * --------------------------------------------------------------------------*/
static int
p_net_ifs_makeAddress( lua_State *L, struct sockaddr* addr, int *found )
{
	struct sockaddr_storage   *adr;

	if (NULL == addr || (AF_INET6 != addr->sa_family && AF_INET != addr->sa_family))
		return 0;
	else
	{
		adr = t_net_adr_create_ud( L );
		memcpy( adr, addr, (AF_INET6 == addr->sa_family)
				? sizeof( struct sockaddr_in6 )
				: sizeof( struct sockaddr_in ) );
		*found = 1;
		return 1;
	}
}


/**--------------------------------------------------------------------------
 * Extract addresses of a family type.
 * \param   L        Lua state.
 * \param   ifa      struct ifaddrs instance.
 * \lreturn table    table of t.Net.Address userdata instances.
 * \return  int/bool 1 if succesful, else 0.
 * --------------------------------------------------------------------------*/
static void
p_net_ifs_getAddrPayload( lua_State *L, struct ifaddrs *ifa )
{
	int found = 0;
	lua_pushinteger( L, ifa->ifa_addr->sa_family );
	t_net_getFamilyValue( L, -1 );

	lua_newtable( L );
	if (p_net_ifs_makeAddress( L, ifa->ifa_addr     , &found ))
		lua_setfield( L, -2, "address" );
	if (p_net_ifs_makeAddress( L, ifa->ifa_netmask  , &found ))
		lua_setfield( L, -2, "netmask" );
	if (p_net_ifs_makeAddress( L, ifa->ifa_broadaddr, &found ))
		lua_setfield( L, -2, "broadcast" );
	if (p_net_ifs_makeAddress( L, ifa->ifa_dstaddr  , &found ))
		lua_setfield( L, -2, "peeraddress" );
	if (found)
		lua_rawset( L, -3 );
	else
		lua_pop( L, 2 );
}


/**--------------------------------------------------------------------------
 * Create an t.Net.Interface Lua table and push to LuaStack.
 * \param   L      Lua state.
 * \lreturn table  t.Net.Interface Lua table instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
p_net_ifc_get( lua_State *L, const char *if_name )
{
	struct ifaddrs   *all_ifas, *ifa;

	if (0 != getifaddrs( &all_ifas ))
		return t_push_error( L, "couldn't retrieve interface information" );

	lua_newtable( L );            // create table that lists ALL interfaces
	for (ifa = all_ifas; ifa ; ifa = ifa->ifa_next)
	{
		lua_getfield( L, -1,  ifa->ifa_name );
		//printf("N: %s(%s)\n", ifa->ifa_name, (ifa->ifa_addr) ?"True":"False" );
		if (lua_isnil( L, -1 ))
		{
			lua_pop( L, 1 );        // pop nil
			lua_newtable( L );      // create table for specific named interface
			luaL_getmetatable( L, T_NET_IFC_TYPE );
			lua_setmetatable( L , -2 );
			lua_pushvalue( L, -1 ); // Repush table
			lua_setfield( L, -3, ifa->ifa_name );
			lua_pushstring( L, ifa->ifa_name );
			lua_setfield( L, -2, "name" );
		}                          //S: lst ifc
		if (ifa->ifa_addr && AF_INET   == ifa->ifa_addr->sa_family)
		{
			p_net_ifc_parseFlags( L, ifa->ifa_flags );
			p_net_ifs_getAddrPayload( L, ifa );
		}
		if (ifa->ifa_addr && AF_INET6  == ifa->ifa_addr->sa_family)
			p_net_ifs_getAddrPayload( L, ifa );
		if (ifa->ifa_addr && AF_PACKET == ifa->ifa_addr->sa_family)
			p_net_ifs_getAddrStats( L, ifa );
		lua_pop( L, 1 );
	}

	freeifaddrs( all_ifas );
	if (NULL != if_name)
	{
		lua_getfield( L, -1, if_name );
		lua_remove( L, -2 );
	}
	return 1;
}

