/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_ifs.c
 * \brief     OOP wrapper for network interfaces
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#ifdef _WIN32
#include <WinSock2.h>
#include <winsock.h>
#include <time.h>
#include <stdint.h>
#include <WS2tcpip.h>
#include <Windows.h>
#else
#include <stdio.h>
#include <string.h>               // strcpy
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif

#include "t.h"
#include "t_net.h"


/**--------------------------------------------------------------------------
 * Parse interface address into a T.Net.Ipv4 struct.
 * \param   L        Lua state.
 * \param   int      sd Socket Descriptor.
 * \param   ifreq    struct ifreq pointer.
 * \param   int      Type for ioctl() request (SIOCGIFADDR,SIOCGIFBRDADDR,SIOCGIFNETMASK).
 * \param   int*     int pointer to address.
 * \lreturn ud       T.Net.Ipv4 userdata instance.
 * \return  int/bool 1 if succesful, else 0.
 * --------------------------------------------------------------------------*/
static int
t_net_ifc_parseAddr( lua_State *L, int sd, struct ifreq *ifr, int ioctltype, int *ip_int )
{
	struct sockaddr_in *addr;

	if (ioctl( sd, ioctltype, ifr ) == 0)
	{
		addr    = t_net_ip4_create_ud( L );
		*ip_int = ((struct sockaddr_in *)(&(ifr)->ifr_addr))->sin_addr.s_addr;
		lua_pushfstring( L, "%d.%d.%d.%d", INT_TO_ADDR( *ip_int ) );
		t_net_ip4_set( L, -1, addr );
		return 1;
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Parse flags on interface.
 * If broadcast and running flags are set consider it the default Interface.
 * \param   L        Lua state.
 * \param   int      sd Socket Descriptor.
 * \param   char*    name of interface.
 * \lparam  table    Table representing the interface.
 * \return  int/bool 1 if is default, else 0.
 * --------------------------------------------------------------------------*/
static int
t_net_ifc_parseFlag( lua_State *L, int sd, const char *name )
{
	struct ifreq ifr;
	strcpy( ifr.ifr_name, name );
	int is_broadcast = 0;
	int is_running   = 0;

	if (0 == ioctl( sd, SIOCGIFFLAGS, (char *)&ifr ))
	{
#define IF_FLAG( b ) \
	if (ifr.ifr_flags & b) { \
		is_broadcast = (is_broadcast || "IFF_BROADCAST" == #b ""); \
		is_running   = (is_running   || "IFF_RUNNING"   == #b ""); \
		lua_pushboolean( L, 1 ); \
		lua_setfield( L, -2, #b "" ); }

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
	}
	return is_broadcast && is_running;
}


/**--------------------------------------------------------------------------
 * Parse interface interface information into a table.
 * \param   L        Lua state.
 * \param   int      sd Socket Descriptor.
 * \param   ifreq    struct ifreq pointer.
 * \lreturn table    T.Net.Interface instance.
 * \return  void
 * --------------------------------------------------------------------------*/
static void
t_net_ifc_parseIfreq( lua_State *L, int sd, struct ifreq *ifr )
{
	struct sockaddr_in *nw_addr;
	int                 addr, bcast, nmask;

	if (AF_INET != ifr->ifr_addr.sa_family)
		return;

	lua_newtable( L );
	if (t_net_ifc_parseAddr( L, sd, ifr, SIOCGIFADDR, &addr ))
		lua_setfield( L, -2, "address" );
	if (t_net_ifc_parseAddr( L, sd, ifr, SIOCGIFBRDADDR, &bcast ))
		lua_setfield( L, -2, "broadcast" );
	if (t_net_ifc_parseAddr( L, sd, ifr, SIOCGIFNETMASK, &nmask))
		lua_setfield( L, -2, "netmask" );

	nw_addr = t_net_ip4_create_ud( L );
	lua_pushfstring( L, "%d.%d.%d.%d", INT_TO_ADDR( addr & nmask ) );
	t_net_ip4_set( L, -1, nw_addr );
	lua_setfield( L, -2, "network" );

	if (t_net_ifc_parseFlag( L, sd, ifr->ifr_name ))
	{
		lua_pushvalue( L, -1 );
		lua_setfield( L, -3, "default" ); // ifcs.default = ifc
	}

	lua_pushstring( L, ifr->ifr_name );
	lua_setfield( L, -2, "name" );

	lua_setfield( L, -2, ifr->ifr_name ); // ifcs.name = ifc
}


/**--------------------------------------------------------------------------
 * Construct a T.Net.Interface and return it.
 * \param   L      Lua state.
 * \lparam  CLASS  table T.Net.Interface
 * \lreturn ud     sockkaddr_in* userdata instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_net_ifc__Call( lua_State *L )
{
	struct sockaddr_in  *ifc;

	lua_remove( L, 1 );
	ifc = t_net_ifc_create_ud( L );
	return 1;
}


/**--------------------------------------------------------------------------
 * List all available interfaces.
 * \param   L      Lua state.
 * \lparam  table  List with all interfaces.
 * \lparam  int    port for the socket.
 * --------------------------------------------------------------------------*/
static int
lt_net_ifc_List( lua_State *L )
{
	struct ifconf       ifc;
	struct ifreq        ifr[ 255 ];
	int                 sd;
	size_t              i;

	// Create a socket so we can use ioctl on the file
	// descriptor to retrieve the interface info.
	sd = socket( PF_INET, SOCK_DGRAM, 0 );

	if (sd > 0)
	{
		ifc.ifc_len           = sizeof( ifr );
		ifc.ifc_ifcu.ifcu_buf = (caddr_t) ifr;

		if (ioctl( sd, SIOCGIFCONF, &ifc ) == 0)
		{
			lua_newtable( L );

			for (i = 0; i < ifc.ifc_len / sizeof( struct ifreq ); ++i)
				t_net_ifc_parseIfreq( L, sd, &(ifr[ i ]) );
		}

		close( sd );
		return 1;
	}
	return 0;
}


/**--------------------------------------------------------------------------
 * Create an IP endpoint userdata and push to LuaStack.
 * \param   L      Lua state.
 * \return  struct sockaddr_in*  pointer to the sockaddr
 * --------------------------------------------------------------------------*/
struct sockaddr_in
*t_net_ifc_create_ud( lua_State *L )
{
	struct sockaddr_in  *ip4;

	ip4 = (struct sockaddr_in *) lua_newuserdata( L, sizeof(struct sockaddr_in) );

	luaL_getmetatable( L, T_NET_IP4_TYPE );
	lua_setmetatable( L , -2 );
	return ip4;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct sockaddr_in
 * \param   L      Lua state.
 * \param   int    position on the stack
 * \return  struct sockaddr_in*  pointer to the sockaddr
 * --------------------------------------------------------------------------*/
struct sockaddr_in
*t_net_ifc_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_NET_IFC_TYPE );
	luaL_argcheck( L, (ud != NULL  || !check), pos, "`"T_NET_IFC_TYPE"` expected" );
	return (NULL==ud) ? NULL : (struct sockaddr_in *) ud;
}


/**--------------------------------------------------------------------------
 * Prints out the ip endpoint.
 * \param   L      Lua state.
 * \lparam  ud     sockkaddr_in* userdata instance.
 * \lreturn string formatted string representing sockkaddr (IP:Port).
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_net_ifc__tostring( lua_State *L )
{
	struct sockaddr_in *ifc = t_net_ip4_check_ud( L, 1, 1 );
	lua_pushfstring( L, T_NET_IFC_TYPE"{%s}: %p",
			INT_TO_ADDR( ifc->sin_addr.s_addr ),
			ifc );
	return 1;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_net_ifc_fm [] = {
	  { "__call"     , lt_net_ifc__Call }
	, { NULL         , NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_ifc_cf [] =
{
	  { "list"       , lt_net_ifc_List }
	, { NULL         , NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_net_ifc_m [] = {
	// metamethods
	  { "__tostring" , lt_net_ifc__tostring }
	, { NULL         ,  NULL}
};


/**--------------------------------------------------------------------------
 * Pushes the T.Net.Interface library onto the stack
 * \param   L     The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_net_ifc( lua_State *L )
{
	// T.Net.Interface instance metatable
	luaL_newmetatable( L, T_NET_IFC_TYPE );
	luaL_setfuncs( L, t_net_ifc_m, 0 );
	lua_setfield( L, -1, "__index" );

	// T.Buffer class
	luaL_newlib( L, t_net_ifc_cf );
	luaL_newlib( L, t_net_ifc_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

