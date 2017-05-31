/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net_adr.c
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
#include <arpa/inet.h>   // htons
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#endif

#include "t_net_l.h"

#ifdef DEBUG
#include "t_dbg.h"
#endif


/**--------------------------------------------------------------------------
 * IP4/IP6 setting of Address.
 * \param   L      Lua state.
 * \param   adr    struct sockaddr_storage*; address to set the IP to.
 * \param   ips    IP string.
 * --------------------------------------------------------------------------*/
static inline void
t_net_adr_setAddr( lua_State *L, struct sockaddr_storage *adr, const char* ips )
{
	if (NULL == ips)
	{
		if (AF_INET6 == SOCK_ADDR_SS_FAMILY( adr ))
			SOCK_ADDR_IN6_ADDR( adr )        = in6addr_any;
		if (AF_INET  == SOCK_ADDR_SS_FAMILY( adr ))
			SOCK_ADDR_IN4_ADDR( adr ).s_addr = htonl( INADDR_ANY );
	}
	else
		if (0 == SOCK_ADDR_SET_INET_PTON( adr, ips ) )
			t_push_error( L, "inet_pton() of %s failed", ips );
}


/**--------------------------------------------------------------------------
 * IP4/IP6 setting of Port.
 * \param   adr    struct sockaddr_storage*; address to set the IP to.
 * \param   port   int.
 * --------------------------------------------------------------------------*/
static inline void
t_net_adr_setPort( struct sockaddr_storage *adr, int port )
{
	if (AF_INET6 == SOCK_ADDR_SS_FAMILY( adr ))
		SOCK_ADDR_IN6_PTR( adr )->sin6_port = htons( port );
	if (AF_INET  == SOCK_ADDR_SS_FAMILY( adr ))
		SOCK_ADDR_IN4_PTR( adr )->sin_port  = htons( port );
}


/**--------------------------------------------------------------------------
 * Construct a Net.Address and return it.
 * \param   L      Lua state.
 * \lparam  CLASS  table T.Net.Address
 * \lparam  port   Port for the Address.
 * \lparam  string IP address in xxx.xxx.xxx.xxx format.
 * \lreturn ud     sockaddr_storage* userdata instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_net_adr__Call( lua_State *L )
{
	struct sockaddr_storage  *adr;

	lua_remove( L, 1 );
	if (t_net_adr_is( L, 1 ))
	{
		lt_net_adr_getIpAndPort( L );   //S: adr ip prt
		lua_remove( L, 1 );             //S: ip prt
	}
	adr = t_net_adr_create_ud( L );
	t_net_adr_set( L, 1, adr );
	return 1;
}


/**--------------------------------------------------------------------------
 * Evaluate stack parameters to set endpoint criteria.
 * \param   L      Lua state.
 * \param   int    offset  on stack to start reading values
 * \param   struct sockaddr_storage*  pointer to ip where values will be set
 * \lparam  string IP address for the socket.
 * \lparam  int    port for the socket.
 * TODO:  allow for empty endpoints if it makes sense
 * --------------------------------------------------------------------------*/
void
t_net_adr_set( lua_State *L, int pos, struct sockaddr_storage *adr )
{
	int           port;

	memset( (void *) &(*adr), 0, sizeof( struct sockaddr_storage ) );
	SOCK_ADDR_SS_FAMILY( adr ) = AF_INET;

	// No first element -> assign 0.0.0.0 and no port
	if (lua_isnone( L, pos+0 ))
	{
		t_net_adr_setAddr( L, adr, NULL );
		return;
	}
	// First element is string -> Assume this is an IP address
	if (LUA_TSTRING == lua_type( L, pos+0 ))
	{
		t_net_adr_setAddr( L, adr, luaL_checkstring( L, pos+0 ) );
		lua_remove( L, pos+0 );
	}
	else
		t_net_adr_setAddr( L, adr, NULL );

	if (lua_isnumber( L, pos+0 ))   // pos+0 because previous string was removed if there
	{
		port = luaL_checkinteger( L, pos+0 );
		luaL_argcheck( L, 0 <= port && port <= 65536, pos+1,  // +1 because first was removed
		               "port number out of range" );
		t_net_adr_setPort( adr, port );
		lua_remove( L, pos+0 );
	}
	// no need to else set .port = 0 because memset( *, 0) at beginning of function
}


/**--------------------------------------------------------------------------
 * Create an IP endpoint userdata and push to LuaStack.
 * \param   L      Lua state.
 * \return  struct sockaddr_in*  pointer to the sockaddr
 * --------------------------------------------------------------------------*/
struct sockaddr_storage
*t_net_adr_create_ud( lua_State *L )
{
	struct sockaddr_storage  *adr;

	adr = (struct sockaddr_storage *) lua_newuserdata( L, sizeof(struct sockaddr_storage) );

	luaL_getmetatable( L, T_NET_ADR_TYPE );
	lua_setmetatable( L , -2 );
	return adr;
}


/**--------------------------------------------------------------------------
 * Check a value on the stack for being a struct sockaddr_in
 * \param   L      Lua state.
 * \param   int    position on the stack.
 * \return  struct sockaddr_in*  pointer to the sockaddr
 * --------------------------------------------------------------------------*/
struct sockaddr_storage
*t_net_adr_check_ud( lua_State *L, int pos, int check )
{
	void *ud = luaL_testudata( L, pos, T_NET_ADR_TYPE );
	if (NULL == ud && check) t_typeerror( L , pos, T_NET_ADR_TYPE );
	return (NULL==ud) ? NULL : (struct sockaddr_storage *) ud;
}


/**--------------------------------------------------------------------------
 * Set Ip and Port of the IP endpoint.
 * \param   L      Lua state.
 * \lparam  ud     sockkaddr_in* userdata instance.
 * \lparam  string IP address.
 * \lparam  int    Port number.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_net_adr_setIpAndPort( lua_State *L )
{
	struct sockaddr_storage *adr = t_net_adr_check_ud( L, 1, 1 );

	t_net_adr_set( L, 1, adr );
	return 0;
}


/**--------------------------------------------------------------------------
 * Get IP and port from the IP endpoint.
 * \param   L      Lua state.
 * \lparam  ud     sockkaddr_in* userdata instance.
 * \lretrun string IP address.
 * \lretrun int    Port number.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_net_adr_getIpAndPort( lua_State *L )
{
	struct sockaddr_storage *adr = t_net_adr_check_ud( L, 1, 1 );
	char             dst[ INET6_ADDRSTRLEN ];

	SOCK_ADDR_GET_INET_NTOP( adr, dst );
	lua_pushstring( L, dst );
	lua_pushinteger( L, ntohs( SOCK_ADDR_SS_PORT( adr ) ) );
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
lt_net_adr__tostring( lua_State *L )
{
	struct sockaddr_storage *adr = t_net_adr_check_ud( L, 1, 1 );   //S: adr
	lt_net_adr_getIpAndPort( L );                                   //S: adr ip prt
	if (AF_INET6 == SOCK_ADDR_SS_FAMILY( adr ))
		lua_pushfstring( L, T_NET_ADR_TYPE"{[%s]:%d}: %p", lua_tostring( L, -2 ), lua_tointeger( L, -1 ), adr );
	else
		lua_pushfstring( L, T_NET_ADR_TYPE"{%s:%d}: %p",   lua_tostring( L, -2 ), lua_tointeger( L, -1 ), adr );
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
lt_net_adr__eq( lua_State *L )
{
	struct sockaddr_storage *adr1 = t_net_adr_check_ud( L, 1, 1 );   //S: adr
	struct sockaddr_storage *adr2 = t_net_adr_check_ud( L, 1, 1 );   //S: adr1 adr2

	if (! SOCK_ADDR_EQ_FAMILY( adr1, adr2 ) ||
	    ! SOCK_ADDR_EQ_ADDR( adr1, adr2 )   ||
	    ! SOCK_ADDR_EQ_PORT( adr1, adr2 )  )
		lua_pushboolean( L, 0 );
	else
		lua_pushboolean( L, 1 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Converts an integer into an IP address xxx.xxx.xxx.xxx representation.
 * \param   L      Lua state.
 * \lparam  int    the IPv4 Address as 32 bit integer.
 * \lreturn string IP address in xxx.xxx.xxx.xxx.
 * \return  int    # of values pushed onto the stack.
 * TODO: check compile time endianess and reorder the uint8_t array
 * --------------------------------------------------------------------------*/
static int
lt_net_adr_Int2ip( lua_State *L )
{
	lua_pushfstring( L, "%d.%d.%d.%d", INT_TO_ADDR( luaL_checkinteger( L, -1 ) ) );
	return 1;
}


/**--------------------------------------------------------------------------
 * Converts an IPv4 Address into an unisgned integer.
 * \param   L      Lua state.
 * \lparam  string the IPv4 Address as xxx.xxx.xxx.xxx string.
 * \lreturn int    representing the IpAddress.
 * \return  int    # of values pushed onto the stack.
 * TODO: check compile time endianess and reorder the uint8_t array
 * --------------------------------------------------------------------------*/
static int
lt_net_adr_Ip2int( lua_State *L )
{
	lua_Integer      ip[ 4 ];
	sscanf( luaL_checkstring( L, 1 ), "%lld.%lld.%lld.%lld",
		&ip[0], &ip[1], &ip[2], &ip[3] );

	lua_pushinteger( L, ip[0] | ip[1] << 8 | ip[2] << 16 | ip[3] << 24 );
	return 1;
}


/**--------------------------------------------------------------------------
 * Class metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_net_adr_fm [] = {
	  { "__call"     , lt_net_adr__Call }
	, { NULL         , NULL }
};

/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_adr_cf [] =
{
	  { "ip2int"     , lt_net_adr_Ip2int }
	, { "int2ip"     , lt_net_adr_Int2ip }
	, { NULL         , NULL }
};

/**--------------------------------------------------------------------------
 * Objects metamethods library definition
 * --------------------------------------------------------------------------*/
static const struct luaL_Reg t_net_adr_m [] = {
	// metamethods
	  { "__tostring" , lt_net_adr__tostring }
	, { "__eq"       , lt_net_adr__eq }
	// object methods
	, { "get"        , lt_net_adr_getIpAndPort }
	, { "set"        , lt_net_adr_setIpAndPort }
	, { NULL         , NULL}
};


/**--------------------------------------------------------------------------
 * Pushes the t.Ip library onto the stack
 * \param   L     The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUAMOD_API int
luaopen_t_net_adr( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	// this is only avalable a <instance>:func()
	luaL_newmetatable( L, T_NET_ADR_TYPE );   // stack: functions meta
	luaL_setfuncs( L, t_net_adr_m, 0 );
	lua_setfield( L, -1, "__index" );

	// Push the class onto the stack
	// this is avalable as T.ip.<member>
	luaL_newlib( L, t_net_adr_cf );
	lua_pushstring( L, "127.0.0.1" );
	lua_setfield( L, -2, "localhost" );

	// set the methods as metatable
	// this is only avalable a <instance>.<member>
	luaL_newlib( L, t_net_adr_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

