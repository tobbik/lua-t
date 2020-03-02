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
p_net_ifc_parseFlags( lua_State *L, struct ifaddrs *ifa )
{
	if (ifa->ifa_addr && ifa->ifa_flags && AF_INET == ifa->ifa_addr->sa_family)
	{
#define IF_FLAG( b ) \
	lua_pushboolean( L, (ifa->ifa_flags & b) ? 1 : 0 ); \
	lua_setfield( L, -2, #b "" );
		lua_createtable( L, 17, 0 );

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
		lua_setfield( L, -2, "flags" );
#undef IF_FLAG
	}
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

	if (ifa->ifa_addr && ifa->ifa_data && AF_PACKET == ifa->ifa_addr->sa_family)
	{
#define DF_FL( FLD )                       \
   lua_pushinteger( L, stats->FLD );       \
   lua_setfield( L, -2, #FLD "" );

		lua_createtable( L, 21, 0 );

		DF_FL( rx_packets );             // total packets received
		DF_FL( tx_packets );             // total packets transmitted
		DF_FL( rx_bytes );               // total bytes received
		DF_FL( tx_bytes );               // total bytes transmitted
		DF_FL( rx_errors );              // bad packets received
		DF_FL( tx_errors );              // packet transmit problems
		DF_FL( rx_dropped );             // no space in linux buffers
		DF_FL( tx_dropped );             // no space available in linux
		DF_FL( multicast );              // multicast packets received
		DF_FL( collisions );
		// detailed rx_errors:
		DF_FL( rx_length_errors );
		DF_FL( rx_over_errors );         // receiver ring buff overflow
		DF_FL( rx_crc_errors );          // recved pkt with crc error
		DF_FL( rx_frame_errors );        // recv'd frame alignment error
		DF_FL( rx_fifo_errors );         // recv'r fifo overrun
		DF_FL( rx_missed_errors );       // receiver missed packet
		// detailed tx_errors
		DF_FL( tx_aborted_errors );
		DF_FL( tx_carrier_errors );
		DF_FL( tx_fifo_errors );
		DF_FL( tx_heartbeat_errors );
		DF_FL( tx_window_errors );
		lua_setfield( L, -2, "stats" );
#undef DF_PT
	}
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
p_net_ifs_makeAddress( lua_State *L, struct sockaddr* addr, const char * adr_name )
{
	struct sockaddr_storage   *adr;

	if (NULL == addr)
		return 0;
	else
	{
		adr = t_net_adr_create_ud( L );
		memcpy( adr, addr, (AF_INET6 == addr->sa_family)
				? sizeof( struct sockaddr_in6 )
				: sizeof( struct sockaddr_in ) );
		lua_setfield( L, -2, adr_name );
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
	if (ifa->ifa_addr && (AF_INET == ifa->ifa_addr->sa_family || AF_INET6 == ifa->ifa_addr->sa_family))
	{
		lua_pushinteger( L, ifa->ifa_addr->sa_family );
		t_net_getFamilyValue( L, -1 );

		lua_newtable( L );
		p_net_ifs_makeAddress( L, ifa->ifa_addr     , "address" );
		p_net_ifs_makeAddress( L, ifa->ifa_netmask  , "netmask" );
		if (ifa->ifa_flags & IFF_BROADCAST)
			p_net_ifs_makeAddress( L, ifa->ifa_broadaddr, "broadcast" );
		else
			p_net_ifs_makeAddress( L, ifa->ifa_dstaddr  , "peeraddress" );
		lua_rawset( L, -3 );
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
		return t_push_error( L, "couldn't retrieve interface information" );

	if (NULL == if_name)
		lua_newtable( L );            // create table that lists ALL interfaces
	else
		lua_pushnil( L );             // "fake" empty lua_getfield() result
	for (ifa = all_ifas; ifa; ifa = ifa->ifa_next)
	{
		if (NULL != if_name && 0 != strcmp( ifa->ifa_name, if_name ))
			continue;
		if (NULL == if_name)
			lua_getfield( L, -1,  ifa->ifa_name );
		//printf("N: %s(%s)\n", ifa->ifa_name, (ifa->ifa_addr) ? "True":"False" );
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
		p_net_ifc_parseFlags( L, ifa );      // only executes for sa_family = AF_INET
		p_net_ifs_getAddrPayload( L, ifa );  // only executes for sa_family = AF_INET || AF_INET6
		p_net_ifs_getAddrStats( L, ifa );   // only executes for sa_family = AF_PACKET

		if (NULL == if_name)
			lua_pop( L, 1 );
	}

	freeifaddrs( all_ifas );
	return 1;
}


