/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net.c
 * \brief     OOP wrapper around network sockets.
 *            TCP/UDP, read write connect bind etc
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */


#include "t.h"
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
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#endif
#include "t_net.h"


// ---------------------- HELPERS ---------------------------------------------
int
t_net_getProtocol( lua_State *L, const int pos )
{
	const char      *pName = luaL_optstring( L, pos, "TCP" );
	struct protoent  result_buf;
	struct protoent *result;
	char             buf[ BUFSIZ ];

#if defined(__linux__)
	luaL_argcheck( L,
		getprotobyname_r( pName, &result_buf, buf, sizeof( buf ), &result) == 0,
		pos,
		"unknown protocol name" );
#else
	result = getprotobyname( pName );
	luaL_argcheck( L, result != NULL, pos, "unknown protocol name" );
	result_buf = *result;
#endif
	return result_buf.p_proto;
}

/**--------------------------------------------------------------------------
 * Evaluate elements on stack to be definitions or instances of sock and ip.
 * This is a helper function to handle possible input scenarios for many
 * functions such as bind, listen, connect etc.
 * possible combinations:
 *  - ...( sck, ip )
 *  - ...( ip)     TODO: FIX THIS
 *  - ...( sck,'ipString', port )
 *  - ...( sck, port )           -- IP will be 0.0.0.0
 *  - ...( sck, 'ipstring' )     -- Port unassigned
 *  - ...( sck )                 -- Port unassigned, IP 0.0.0.0
 *
 * \param   L      Lua state.
 * \param   int    Position on the stack.
 * \return  struct t_net* pointer to the struct t_net.
 * --------------------------------------------------------------------------*/
void
t_net_getdef( lua_State *L, const int pos, struct t_net_sck **sock, struct sockaddr_in **addr )
{
	*sock = t_net_sck_check_ud( L, pos+0, 0 );
	*addr = t_net_ip4_check_ud( L, pos+1, 0 );

	if (NULL == *sock)     // handle T.Net.whatever( )
	{
		// in all shortcuts of listen(ip,port), bind(ip,port), connect(ip,port)
		// It defaults to IPv4 and TCP, for others Sockets and Addresses must be
		// created explicitely
		*sock = t_net_sck_create_ud( L, AF_INET, SOCK_STREAM, IPPROTO_TCP, 1  );
		lua_insert( L, pos+0 );
	}

	if (NULL == *addr)
	{
		*addr = t_net_ip4_create_ud( L );
		t_net_ip4_set( L, pos+1, *addr );
		lua_insert( L, pos+1 );
	}
}

/** -------------------------------------------------------------------------
 * get socket otions from an array like luaL_checkoption but no error
 * \param   L      Lua state.
 * \param   pos    stack position of string to test.
 * \param   lst    NULL Terminated array of strings.
 * \return  idx    index in array where string occurs, -1 if not found.
 *-------------------------------------------------------------------------*/
int
t_net_testOption( lua_State *L, int pos, const char *const lst[] )
{
	const char *nme = luaL_checkstring( L, pos );
	int i;
	for (i=0; lst[i]; i++)
		if (strcmp( lst[i], nme ) == 0)
			return i;
	return -1;
}


/**--------------------------------------------------------------------------
 * Class functions library definition
 * --------------------------------------------------------------------------*/
static const luaL_Reg t_net_cf [] =
{
	  { NULL,    NULL }
};


/**--------------------------------------------------------------------------
 * Pushes the Socket library onto the stack
 *          - creates Metatable with functions
 *          - creates metatable with methods
 * \param   L     The lua state.
 * \lreturn table  the library
 * \return  int    # of values pushed onto the stack.
 * --------------------------------------------------------------------------*/
LUA_API int
luaopen_t_net( lua_State *L )
{
	luaL_newlib( L, t_net_cf );
	luaopen_t_net_ip4( L );
	lua_setfield( L, -2, T_NET_IP4_NAME );
	luaopen_t_net_ifc( L );
	lua_setfield( L, -2, T_NET_IFC_NAME );
	luaopen_t_net_sck( L );
	lua_setfield( L, -2, T_NET_SCK_NAME );
	return 1;
}
