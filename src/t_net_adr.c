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
#include <sys/un.h>
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
static void
t_net_adr_setAddr( lua_State *L, struct sockaddr_storage *adr, const int pos )
{
	const char   *ipstr;
	size_t        iplen;

	if (0 == pos)
	{
		if (AF_INET6 == SOCK_ADDR_SS_FAMILY( adr ))
			SOCK_ADDR_IN6_ADDR( adr )        = in6addr_any;
		if (AF_INET  == SOCK_ADDR_SS_FAMILY( adr ))
			SOCK_ADDR_IN4_ADDR( adr ).s_addr = htonl( INADDR_ANY );
	}
	else
	{
		ipstr = luaL_checklstring( L, pos, &iplen );
		adr->ss_family = AF_UNSPEC;
		if (SOCK_ADDR_IN6_PTON( adr, ipstr ))
			adr->ss_family = AF_INET6;
		else if (SOCK_ADDR_IN4_PTON( adr, ipstr ))
			adr->ss_family = AF_INET;
		else
		{
			memcpy( SOCK_ADDR_UNX_PTR( adr )->sun_path, ipstr, iplen+1 );
			adr->ss_family = AF_UNIX;
		}
	}
}


/**--------------------------------------------------------------------------
 * IP4/IP6 setting of Port.
 * \param   L      Lua state.
 * \param   adr    struct sockaddr_storage*; address to set the Port to.
 * \param   pos    position of input on stack.
 * --------------------------------------------------------------------------*/
void
t_net_adr_setPort( lua_State *L, struct sockaddr_storage *adr, const int pos )
{
	int prt  = luaL_checkinteger( L, pos );
	luaL_argcheck( L, 0 <= prt && prt <= 65536, pos, "port number out of range" );
	if (AF_INET6 == SOCK_ADDR_SS_FAMILY( adr ))
		SOCK_ADDR_IN6_PTR( adr )->sin6_port = htons( prt );
	if (AF_INET  == SOCK_ADDR_SS_FAMILY( adr ))
		SOCK_ADDR_IN4_PTR( adr )->sin_port  = htons( prt );
}


/**--------------------------------------------------------------------------
 * Evaluate stack parameters to set address criteria.
 * \param   L      Lua state.
 * \param   int    offset  on stack to start reading values.
 * \lparam  string IPv4/IPv6 address for the socket.
 * \lparam  int    port for the socket.
 * --------------------------------------------------------------------------*/
static struct sockaddr_storage
*t_net_adr_getFromStack( lua_State *L, int pos )
{
	struct sockaddr_storage *adr    = t_net_adr_check_ud( L, pos, 0 );

	if (NULL == adr)
		adr = t_net_adr_create_ud( L );
	else
		return adr;   // return address as is

	// DETERMINE IF ANY ADRRESS STRING WAS GIVEN
	// if no addr is given assume INADDR_ANY
	if (LUA_TSTRING != lua_type( L, pos ))
	{
		adr->ss_family = _t_net_default_family;
		t_net_adr_setAddr( L, adr, 0 );
	}
	else
	{
		t_net_adr_setAddr( L, adr, pos );
		lua_remove( L, pos );     // remove IP string
	}
	// DETERMINE IF ANY PORT WAS GIVEN
	if (lua_isnumber( L, pos ))
	{
		t_net_adr_setPort( L, adr, pos );
		lua_remove( L, pos );
	}
	// put the address into the stack were the parameters used to be
	lua_insert( L, pos );
	return adr;
}


/**--------------------------------------------------------------------------
 * Construct a Net.Address and return it.
 * \param   L      Lua state.
 * \lparam  CLASS  table t.Net.Address
 * \lparam  port   Port for the Address.
 * \lparam  string IP address in xxx.xxx.xxx.xxx format.
 * \lreturn ud     sockaddr_storage* userdata instance.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
static int
lt_net_adr__Call( lua_State *L )
{
	char                     ip[ INET6_ADDRSTRLEN ];
	struct sockaddr_storage *adr = t_net_adr_check_ud( L, 2, 0 );
	lua_remove( L, 1 );
	if (NULL != adr )
	{
		lua_remove( L, 1 );
		SOCK_ADDR_INET_NTOP( adr, ip );
		lua_pushstring( L, ip );
		lua_pushinteger( L, ntohs( SOCK_ADDR_SS_PORT( adr ) ) );
	}
	t_net_adr_getFromStack( L, 1 );
	return 1;
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
	memset( (void *) &(*adr), 0, sizeof( struct sockaddr_storage ) );

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
 * Get properties of t.Net.Address.
 * \param   L      Lua state.
 * \lparam  adr    sockaddr_storage*; t.Net.Address userdata instance.
 * \lparam  key    string; key of property.
 * \lretrun value  Property value.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_net_adr__index( lua_State *L )
{
	struct sockaddr_storage *adr    = t_net_adr_check_ud( L, 1, 1 );
	char                     ip[ INET6_ADDRSTRLEN ];
	const char              *key;
	size_t                   keyLen;

	if (LUA_TSTRING != lua_type( L, 2 ))
	{
		lua_pushnil( L );
		return 1;
	}
	else
		key = luaL_checklstring( L, 2, &keyLen );

	if (0 == strncmp( key, "ip", keyLen ))
	{
		SOCK_ADDR_INET_NTOP( adr, ip );
		lua_pushstring( L, ip );
	}
	else if (0 == strncmp( key, "port", keyLen ))
		lua_pushinteger( L, ntohs( SOCK_ADDR_SS_PORT( adr ) ) );
	else if (0 == strncmp( key, "family", keyLen ))
	{
		lua_pushinteger( L, SOCK_ADDR_SS_FAMILY( adr ) );
		t_net_getFamilyValue( L, -1 );
	}
	else
		lua_pushnil( L );
	return 1;
}


/**--------------------------------------------------------------------------
 * Set properties of t.Net.Address.
 * \param   L      Lua state.
 * \lparam  adr    sockaddr_storage*; t.Net.Address userdata instance.
 * \lparam  key    string; key of property.
 * \lparam  value  value; value of property.
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
lt_net_adr__newindex( lua_State *L )
{
	struct sockaddr_storage *adr    = t_net_adr_check_ud( L, 1, 1 );
	const char              *key;
	size_t                   keyLen;

	if (LUA_TSTRING != lua_type( L, 2 ))
		return 0;
	else
		key = luaL_checklstring( L, 2, &keyLen );

	if (0 == strncmp( key, "ip", keyLen ))
		t_net_adr_setAddr( L, adr, 3 );
	else if (0 == strncmp( key, "port", keyLen ))
		t_net_adr_setPort( L, adr, 3 );
	else if (0 == strncmp( key, "family", keyLen ) )
	{
		if (lua_isnumber( L, 3 ) || t_net_getFamilyValue( L, 3 ))
			adr->ss_family = luaL_checkinteger( L, 3 );
	}
	return 0;
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
	char                     ip[ INET6_ADDRSTRLEN ];
	SOCK_ADDR_INET_NTOP( adr, ip );
	if (AF_INET6 == SOCK_ADDR_SS_FAMILY( adr ))
		lua_pushfstring( L, T_NET_ADR_TYPE"{[%s]:%d}: %p", ip, ntohs( SOCK_ADDR_SS_PORT( adr ) ), adr );
	else
		lua_pushfstring( L, T_NET_ADR_TYPE"{%s:%d}: %p",   ip, ntohs( SOCK_ADDR_SS_PORT( adr ) ), adr );
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
	struct sockaddr_storage *adr2 = t_net_adr_check_ud( L, 2, 1 );   //S: adr1 adr2

	if ( SOCK_ADDR_EQ_FAMILY( adr1, adr2 ) &&
	     SOCK_ADDR_EQ_ADDR( adr1, adr2 )   &&
	     SOCK_ADDR_EQ_PORT( adr1, adr2 ) )
		lua_pushboolean( L, 1 );
	else
		lua_pushboolean( L, 0 );
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
	, { "__index"    , lt_net_adr__index }
	, { "__newindex" , lt_net_adr__newindex }
	, { NULL         , NULL}
};


/**--------------------------------------------------------------------------
 * Pushes the t.Ip library onto the stack
 * \param   L     The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
int
luaopen_t_net_adr( lua_State *L )
{
	// just make metatable known to be able to register and check userdata
	// this is only avalable a <instance>:func()
	luaL_newmetatable( L, T_NET_ADR_TYPE );   // stack: functions meta
	luaL_setfuncs( L, t_net_adr_m, 0 );
	//lua_setfield( L, -1, "__index" );
	lua_pop( L, 1 );

	// Push the class onto the stack
	// this is avalable as t.Net.Address.<member>
	luaL_newlib( L, t_net_adr_cf );

	// set the methods as metatable
	// this is only avalable a <instance>.<member>
	luaL_newlib( L, t_net_adr_fm );
	lua_setmetatable( L, -2 );
	return 1;
}

