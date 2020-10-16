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
int
luaopen_t_net( lua_State *L )
{
	luaL_newlib( L, t_net_cf );

	luaopen_t_net_adr( L );
	lua_setfield( L, -2, T_NET_ADR_IDNT );
	luaopen_t_net_ifc( L );
	lua_setfield( L, -2, T_NET_IFC_IDNT );
	luaopen_t_net_sck( L );
	lua_setfield( L, -2, T_NET_SCK_IDNT );
	luaopen_t_net_fml( L );
	lua_setfield( L, -2, T_NET_FML_IDNT );
	return 1;
}
