/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_ip4.c
 * \brief     OOP wrapper for network addresses
 *            This is a thin wrapper around struct sockaddr_in
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#endif

#include "t.h"
#include "t_net.h"



/**--------------------------------------------------------------------------
 * Create an IP endpoint and return it.
 * \param   L      Lua state.
 * \lparam  port   Port for the socket.
 * \lparam  string IP address for the socket.
 * \lreturn ud     sockkaddr_in* userdata instance.
 * \return  int    # of values pushed onto the stack.
 * TODO:  allow for empty endpoints if it makes sense
 * --------------------------------------------------------------------------*/
static int
lt_net_ip4_New( lua_State *L )
{
	struct sockaddr_in  *ip4;
	ip4 = t_net_ip4_create_ud( L );
	t_net_ip4_set( L, 1, ip4 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Construct a NetAddress and return it.
 * \param   L     Lua state.
 * \lparam  CLASS Ip4
 * \lreturn ud    sockkaddr_in* userdata instance.
 * \return  int   # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_net_ip4__Call( lua_State *L )
{
	lua_remove( L, 1 );
	return lt_net_ip4_New( L );
}


/**--------------------------------------------------------------------------
 * Evaluate stack parameters to set endpoint criteria.
 * \param   L      Lua state.
 * \param   int    offset  on stack to start reading values
 * \param   struct sockaddr_in*  pointer to ip where values will be set
 * \lparam  string IP address for the socket.
 * \lparam  int    port for the socket.
 * TODO:  allow for empty endpoints if it makes sense
 * --------------------------------------------------------------------------*/
void
t_net_ip4_set( lua_State *L, int pos, struct sockaddr_in *ip )
{
	int           port;
	const char   *ips;      /// IP String aaa.bbb.ccc.ddd

	memset( (char *) &(*ip), 0, sizeof( ip ) );
	ip->sin_family = AF_INET;

   // First element is nil assign 0.0.0.0 and no port
	if (lua_isnoneornil( L, pos+0 ))
	{
		ip->sin_addr.s_addr = htonl( INADDR_ANY );
		return;
	}
	// First element is string -> Assume this is an IP address
	if (LUA_TSTRING == lua_type( L, pos+0 ))
	{
		ips = luaL_checkstring( L, pos+0 );
#ifdef _WIN32
		if ( InetPton( AF_INET, ips, &(ip->sin_addr) ) == 0)
			t_push_error( L, "InetPton() of %s failed", ips );
#else
		if ( inet_pton( AF_INET, ips, &(ip->sin_addr) ) == 0)
			t_push_error( L, "inet_aton() of %s failed", ips );
#endif
		lua_remove( L, pos+0 );
	}
	else
		ip->sin_addr.s_addr = htonl( INADDR_ANY );

	if (lua_isnumber( L, pos+0 ))   // pos+0 because previous string was removed if there
	{
		port = luaL_checkinteger( L, pos+0 );
		luaL_argcheck( L, 1 <= port && port <= 65536, pos+1,  // +1 because first was removed
		               "port number out of range" );
		ip->sin_port   = htons( port );
		lua_remove( L, pos+0 );
	}
}


/**--------------------------------------------------------------------------
 * Create an IP endpoint userdata and push to LuaStack.
 * \param   L      Lua state.
 * \return  struct sockaddr_in*  pointer to the sockaddr
 * --------------------------------------------------------------------------*/
struct sockaddr_in
*t_net_ip4_create_ud( lua_State *L )
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
*t_net_ip4_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_NET_IP4_TYPE );
	luaL_argcheck( L, (ud != NULL  || !check), pos, "`"T_NET_IP4_TYPE"` expected" );
	return (NULL==ud) ? NULL : (struct sockaddr_in *) ud;
}


/**--------------------------------------------------------------------------
 * Set Ip and Port of the IP endpoint.
 * \param   L      Lua state.
 * \lparam  ud     sockkaddr_in* userdata instance.
 * \lparam  string IPv4 address.
 * \lparam  int    Port number.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_net_ip4_setIpAndPort( lua_State *L )
{
	struct sockaddr_in *ip4 = t_net_ip4_check_ud( L, 1, 1 );

	t_net_ip4_set( L, 1, ip4 );
	return 0;
}


/**--------------------------------------------------------------------------
 * Get IP and port from the IP endpoint.
 * \param   L      Lua state.
 * \lparam  ud     sockkaddr_in* userdata instance.
 * \lretrun string IPv4 address.
 * \lretrun int    Port number.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_net_ip4_getIpAndPort( lua_State *L )
{
	struct sockaddr_in *ip4 = t_net_ip4_check_ud( L, 1, 1 );
	lua_pushstring( L, inet_ntoa( ip4->sin_addr ) );
	lua_pushinteger( L, ntohs( ip4->sin_port ) );
	return 2;
}


/**--------------------------------------------------------------------------
 * Prints out the ip endpoint.
 * \param   L      Lua state.
 * \lparam  ud     sockkaddr_in* userdata instance.
 * \lreturn string formatted string representing sockkaddr (IP:Port).
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_net_ip4__tostring( lua_State *L )
{
	struct sockaddr_in *ip4 = t_net_ip4_check_ud( L, 1, 1 );
	lua_pushfstring( L, T_NET_IP4_TYPE"{%s:%d}: %p",
			inet_ntoa( ip4->sin_addr ),
			ntohs( ip4->sin_port ),
			ip4 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Compares values of two IP Endpoints.
 * \param   L      Lua state.
 * \lparam  ud     sockkaddr_in* userdata instance.
 * \lparam  ud     sockkaddr_in* userdata instance.
 * \lreturn bool   1 if both IP and Port are equal, else 0.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_net_ip4__eq( lua_State *L )
{
	struct sockaddr_in *ip4    = t_net_ip4_check_ud( L, 1, 1 );
	struct sockaddr_in *ip4Cmp = t_net_ip4_check_ud( L, 2, 1 );

	if (ip4->sin_family != ip4Cmp->sin_family)
	{
		lua_pushboolean( L, 0 );
		return 1;
	}

	if (ip4->sin_family == AF_INET)
	{
		if (ip4->sin_addr.s_addr != ip4Cmp->sin_addr.s_addr ||
		    ip4->sin_port != ip4Cmp->sin_port)
		{
			lua_pushboolean( L, 0 );
			return 1;
		}
	}
	else
	{
		// TODO: Deal with IPv6 at some point!
			lua_pushboolean( L, 0 );
			return 1;
	}
	lua_pushboolean( L, 1 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Converts an integer into an IP address xxx.xxx.xxx.xxx representation
 * \param   L      Lua state.
 * \lparam  int    the IPv4 Address as 32 bit integer.
 * \lreturn string IP address in xxx.xxx.xxx.xxx
 * \return  int    # of values pushed onto the stack.
 * TODO: check compile time endianess and reorder the uint8_t array
 * --------------------------------------------------------------------------*/
static int
lt_net_ip4_Int2ip( lua_State *L )
{
	uint32_t  ip_int  = luaL_checkinteger( L, 1 );
	uint8_t  *ip      = (uint8_t *) &ip_int;

	lua_pushfstring( L, "%d.%d.%d.%d",
		ip[3], ip[2], ip[1], ip[0] );
	return 1;
}


/**--------------------------------------------------------------------------
 * Converts an IPv4 Address into an unisgned integer
 * \param   L      Lua state.
 * \lparam  string the IPv4 Address as xxx.xxx.xxx.xxx string
 * \lreturn int    representing the IpAddress
 * \return  int    # of values pushed onto the stack.
 * TODO: check compile time endianess and reorder the uint8_t array
 * --------------------------------------------------------------------------*/
static int
lt_net_ip4_Ip2int (lua_State *L)
{
	uint32_t       ip[4];
	const char   *ip_str  = luaL_checkstring( L, 1 );
	sscanf( ip_str, "%d.%d.%d.%d", &ip[3], &ip[2], &ip[1], &ip[0] );

	//printf ("strined IP[%d.%d.%d.%d]\n", ip[3], ip[2], ip[1], ip[0]);
	lua_pushinteger( L, ip[0] | ip[1] << 8 | ip[2] << 16 | ip[3] << 24 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_net_ip4_fm [] = {
	{ "__call",      lt_net_ip4__Call },
	{ NULL,   NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_ip4_cf [] =
{
	{ "new",       lt_net_ip4_New },
	{ "ip2int",    lt_net_ip4_Ip2int },
	{ "int2ip",    lt_net_ip4_Int2ip },
	{ NULL,        NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_net_ip4_m [] = {
	// metamethods
	{ "__tostring",  lt_net_ip4__tostring },
	{ "__eq",        lt_net_ip4__eq },
	// object methods
	{ "get",         lt_net_ip4_getIpAndPort },
	{ "set",         lt_net_ip4_setIpAndPort },
	{ NULL,   NULL}
};


/**--------------------------------------------------------------------------
 * Pushes the t.Ip library onto the stack
 * \param   L     The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_net_ip4( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	// this is only avalable a <instance>:func()
	luaL_newmetatable( L, T_NET_IP4_TYPE );   // stack: functions meta
	luaL_setfuncs( L, t_net_ip4_m, 0 );
	lua_setfield( L, -1, "__index" );

	// Push the class onto the stack
	// this is avalable as T.ip.<member>
	luaL_newlib( L, t_net_ip4_cf );
	lua_pushstring( L, "127.0.0.1" );
	lua_setfield( L, -2, "localhost" );

	// set the methods as metatable
	// this is only avalable a <instance>.<member>
	luaL_newlib( L, t_net_ip4_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

