/* vim: ts=3 sw=3 sts=3 tw=80 sta noet list
*/
/**
 * \file      t_net.c
 * \brief     OOP wrapper around network sockets.
 *            TCP/UDP, read write connect bind etc.
 * \author    tkieslich
 * \copyright See Copyright notice at the end of t.h
 */

#include "t_net_l.h"


#ifdef _WIN32
#include <WinSock2.h>
#include <winsock.h>
#include <time.h>
#include <stdint.h>
#include <WS2tcpip.h>
#include <Windows.h>
#else
#include <ctype.h>                // toupper()
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#endif

#ifdef DEBUG
#include "t_dbg.h"
#endif

int _t_net_default_family = AF_INET;

// ---------------------- HELPERS ---------------------------------------------

/**--------------------------------------------------------------------------
 * Read protocol name from stack and overwrite it with protocol number.
 * \param   L        Lua state.
 * \param   pos      Position of name on Lua stack.
 * \param   dft      char*; Default name if pos is empty.
 * \lparam  string   Name of protocol.
 * \lreturn integer  Value of protocol.
 * --------------------------------------------------------------------------*/
void
t_net_getProtocolByName( lua_State *L, int pos, const char *dft )
{
	const char      *pName = (NULL == dft)
	                          ? luaL_checkstring( L, pos )
	                          : luaL_optstring( L, pos, dft );
	struct protoent  result_buf;
	struct protoent *result;
	//TODO: define _REENTRANT properly
#if defined(__linux__) && defined(_REENTRANT)
	char             buf[ BUFSIZ ];
#endif
	// get absolute stack position
	pos = (pos < 0) ? lua_gettop( L ) + pos + 1 : pos;
	luaL_argcheck( L, pos-1 <= lua_gettop( L ), pos, "can't convert protocol name, stack too short" );

	//TODO: define _REENTRANT properly
#if defined(__linux__) && defined(_REENTRANT)
	luaL_argcheck( L,
		0 == getprotobyname_r( pName, &result_buf, buf, sizeof( buf ), &result ),
		pos,
		"unknown protocol name" );
#else
	result = getprotobyname( pName );
	luaL_argcheck( L, result != NULL, pos, "unknown protocol name" );
	result_buf = *result;
#endif
	lua_pushinteger( L, result_buf.p_proto );
	if (lua_gettop( L ) > pos)
		lua_replace( L, pos );
}


/**--------------------------------------------------------------------------
 * Read protocol value from stack and overwrite it with protocol name.
 * \param   L        Lua state.
 * \param   pos      Position of name on Lua stack.
 * \param   dft      int*; Default protocol number if pos is empty.
 * \lparam  string   Value of protocol.
 * \lreturn integer  Name of protocol.
 * --------------------------------------------------------------------------*/
void
t_net_getProtocolByValue( lua_State *L, int pos, const int dft )
{
	const int        val   = (dft < 1)
	                          ? luaL_checkinteger( L, pos )
	                          : luaL_optinteger( L, pos, dft );
	struct protoent  result_buf;
	struct protoent *result;
	//TODO: define _REENTRANT properly
#if defined(__linux__) && defined(_REENTRANT)
	char             buf[ BUFSIZ ];
#endif
	// get absolute stack position
	pos = (pos < 0) ? lua_gettop( L ) + pos + 1 : pos;
	luaL_argcheck( L, pos-1 <= lua_gettop( L ), pos, "can't convert protocol, stack too short" );

	//TODO: define _REENTRANT properly
#if defined(__linux__) && defined(_REENTRANT)
	luaL_argcheck( L,
		0 == getprotobynumber_r( val, &result_buf, buf, sizeof( buf ), &result),
		0,
		"unknown protocol value" );
#else
	result = getprotobynumber( val );
	luaL_argcheck( L, result != NULL, 0, "unknown protocol value" );
	result_buf = *result;
#endif
	lua_pushstring( L, result_buf.p_name );
	if (lua_gettop( L ) > pos)
		lua_replace( L, pos );
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
	luaopen_t_net_adr( L );
	lua_setfield( L, -2, T_NET_ADR_IDNT );
	luaopen_t_net_ifc( L );
	lua_setfield( L, -2, T_NET_IFC_IDNT );
	luaopen_t_net_sck( L );
	lua_setfield( L, -2, T_NET_SCK_IDNT );
	return 1;
}
